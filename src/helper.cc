/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    helper.cc
 * \brief   Provide some helper methods for Bottle Manager and CLI interaction
 * \author  Melroy van den Berg <webmaster1989@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "helper.h"

#include <fstream>
#include <ctime>
#include <iomanip>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <array>
#include <glibmm.h>
#include <giomm/file.h>
#include <glibmm/fileutils.h>

// Reg files
static const string SYSTEM_REG = "system.reg";
static const string USER_REG = "user.reg";
static const string USERDEF_REG = "userdef.reg";

// Reg keys
static const string keyName9x = "Software\\Microsoft\\Windows\\CurrentVersion";
static const string keyNameNT = "Software\\Microsoft\\Windows NT\\CurrentVersion";

// Reg names
static const string nameNTVersion = "CurrentVersion";
static const string nameNTBuild  = "CurrentBuildNumber";
static const string name9xVersion = "VersionNumber";

// Other files
static const string WINEGUI_CONF = ".winegui.conf";
static const string UPDATE_TIMESTAMP = ".update-timestamp";


// Source: https://github.com/wine-mirror/wine/blob/master/programs/winecfg/appdefaults.c#L49
// Build number is tranformed to decimal number
static const struct
{
    const string version;
    const string description;
    const string versionNumber;
    const string buildNumber;
    const string servicePack;
    const BottleTypes::Windows windows;
    const BottleTypes::Bit bitOnly;
} win_versions[] =
{
  {"win10",     "Windows 10",      "10.0", "17134", "",     BottleTypes::Windows::Windows10},
  {"win81",     "Windows 8.1",     "6.3",  "9600",  "",     BottleTypes::Windows::Windows81},
  {"win8",      "Windows 8",       "6.2",  "9200",  "",     BottleTypes::Windows::Windows8},
  {"win2008r2", "Windows 2008 R2", "6.1",  "7601",  "SP1",  BottleTypes::Windows::Windows2008R2},
  {"win7",      "Windows 7",       "6.1",  "7601",  "SP1",  BottleTypes::Windows::Windows7},
  {"win2008",   "Windows 2008",    "6.0",  "6002",  "SP2",  BottleTypes::Windows::Windows2008},
  {"vista",     "Windows Vista",   "6.0",  "6002",  "SP2",  BottleTypes::Windows::WindowsVista},
  {"win2003",   "Windows 2003",    "5.2",  "3790",  "SP2",  BottleTypes::Windows::Windows2003},
  {"winxp64",   "Windows XP",      "5.2",  "3790",  "SP2",  BottleTypes::Windows::WindowsXP,    BottleTypes::Bit::win64},
  {"winxp",     "Windows XP",      "5.1",  "2600",  "SP3",  BottleTypes::Windows::WindowsXP,    BottleTypes::Bit::win32},
  {"win2k",     "Windows 2000",    "5.0",  "2195",  "SP4",  BottleTypes::Windows::Windows2000,  BottleTypes::Bit::win32},
  {"winme",     "Windows ME",      "4.90", "3000",  "",     BottleTypes::Windows::WindowsME,    BottleTypes::Bit::win32},
  {"win98",     "Windows 98",      "4.10", "2222",  "",     BottleTypes::Windows::Windows98,    BottleTypes::Bit::win32},
  {"win95",     "Windows 95",      "4.0",  "950",   "",     BottleTypes::Windows::Windows95,    BottleTypes::Bit::win32},
  {"nt40",      "Windows NT 4.0",  "4.0",  "1381",  "SP6a", BottleTypes::Windows::WindowsNT40,  BottleTypes::Bit::win32},
  {"nt351",     "Windows NT 3.51", "3.51", "1057",  "SP5",  BottleTypes::Windows::WindowsNT351, BottleTypes::Bit::win32},
  {"win31",     "Windows 3.1",     "3.10", "0",     "",     BottleTypes::Windows::Windows31,    BottleTypes::Bit::win32},
  {"win30",     "Windows 3.0",     "3.0",  "0",     "",     BottleTypes::Windows::Windows30,    BottleTypes::Bit::win32},
  {"win20",     "Windows 2.0",     "2.0",  "0",     "",     BottleTypes::Windows::Windows20,    BottleTypes::Bit::win32}
};

/**
 * \brief Get the bottle directories within the given path
 * \param[in] Directory path to search in
 * \return vector of strings of found directories (*full paths*)
 */
std::vector<string> Helper::GetBottlesPaths(const string& dir_path)
{
  std::vector<std::string> r;
  Glib::Dir dir(dir_path);
  auto name = dir.read_name();
  while(!name.empty())
  {
    auto path = Glib::build_filename(dir_path, name);
    if(Glib::file_test(path, Glib::FileTest::FILE_TEST_IS_DIR)) {
      r.push_back(path);
    }
    name = dir.read_name();
  }
  return r;
}

/**
 * \brief Get Wine version from CLI
 * \return Return the wine version
 */
string Helper::GetWineVersion()
{
  string result = Exec("wine --version");
  if(!result.empty()) {
    std::vector<string> results = Split(result, '-');
    if(results.size() >= 2) {
      string result = results.at(1);;
      // Remove new lines
      result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
      return result;
    } else {
      throw std::runtime_error("Could not determ wine version?\nSomething went wrong.");
    }
  } else {
    throw std::runtime_error("Could not receive Wine version!\n\nIs wine installed?");
  }
}

/**
 * \brief Get Wine Bottle Name from configuration file (if possible)
 * \return Bottle name
 */
string Helper::GetName(const string prefix_path)
{
  try
  {
    std::vector<std::string> config = ReadFile(Glib::build_filename(prefix_path, WINEGUI_CONF));
    for(std::vector<std::string>::iterator config_line = config.begin(); config_line != config.end(); ++config_line) {
      auto delimiterPos = (*config_line).find("=");
      auto name = (*config_line).substr(0, delimiterPos);
      auto value = (*config_line).substr(delimiterPos + 1);
      if (name.compare("name") == 0) {
        return value;
      }
    }    
  }
  catch(const std::exception& e)
  {
    // Do nothing, continue
  }

  // Fall-back: get last directory name of path string
  string name = "- Unknown -";
  std::size_t last_index = prefix_path.find_last_of("/\\");
  if (last_index != string::npos) {
    // Get only the last directory name from path (+ remove slash)
    name = prefix_path.substr(last_index+1);
    // Remove dot if present (=hidden dir)
    size_t dot_index = name.find_first_of('.');
    if(dot_index == 0) {
      // Remove dot at start
      name = name.substr(1);
    }
  }
  return name;
}

/**
 * \brief Get current Windows OS version
 * \return Return the Windows OS version
 */
BottleTypes::Windows Helper::GetWindowsOSVersion(const string prefix_path)
{
  // TODO: Try first reg keyNameNT (with nameNTVersion & nameNTBuild names) and otherwise reg keyName9x (with name9xVersion name)

  string filename = Glib::build_filename(prefix_path, SYSTEM_REG);
  string version = "";
  string version9x = "";
  if(!(version = Helper::GetRegValue(filename, keyNameNT, nameNTVersion)).empty())
  {
    string buildNumberNT = Helper::GetRegValue(filename, keyNameNT, nameNTBuild);
    // Find Windows version
    for (unsigned long i = 0; i < sizeof(win_versions); i++)
    {
      // Check if version + build number matches
      if(((win_versions[i].versionNumber).compare(version) == 0) && 
        ((win_versions[i].buildNumber).compare(buildNumberNT) == 0))
      {
        return win_versions[i].windows;
      }
    }
  }
  else if(!(version = Helper::GetRegValue(filename, keyName9x, name9xVersion)).empty())
  {
    string currentVersion = "";
    string currentBuildNumber = "";
    std::vector<string> versionList = Split(version, '.');
    // Only get minor & major
    if(sizeof(versionList) >= 2) {
      currentVersion = versionList.at(0) + '.' + versionList.at(1);
    }
    // Get build number
    if(sizeof(versionList) >= 3) {
      currentBuildNumber = versionList.at(2);
    }
     
    // Find Windows version
    for (unsigned long i = 0; i < sizeof(win_versions); i++)
    {
      // Check if version + build number matches
      if(((win_versions[i].versionNumber).compare(version) == 0) && 
        ((win_versions[i].buildNumber).compare(buildNumberNT) == 0))
      {
        return win_versions[i].windows;
      }
    }
  }
  else
  {
    throw std::runtime_error("Could not determ Windows OS version.");    
  }
  // Function didn't return before (meaning no match found)
  throw std::runtime_error("Could not determ Windows OS version.");
}

/**
 * \brief Get system processor bit (32/64). *Throw runtime_error* when not found.
 * \return 32-bit or 64-bit
 */
BottleTypes::Bit Helper::GetSystemBit(const string prefix_path)
{
  string filename = Glib::build_filename(prefix_path, USER_REG);

  string metaValueName = "arch";
  string value = Helper::Helper::GetRegMetaData(filename, metaValueName);
  if(!value.empty()) {
    if(value.compare("win32") == 0) {
      return BottleTypes::Bit::win32;
    } else if(value.compare("win64") == 0) {
      return BottleTypes::Bit::win64;
    } else {
      throw std::runtime_error("Could not determ Windows system bit (not win32 and not win64?).");
    }
  } else {
    throw std::runtime_error("Could not determ Windows system bit.");
  }
}

/**
 * \brief Get Audio driver
 * \return Audio Driver (eg. alsa/coreaudio/oss/pulse)
 */
BottleTypes::AudioDriver Helper::GetAudioDriver(const string prefix_path)
{
  string filename = Glib::build_filename(prefix_path, USER_REG);
  string keyName = "Software\\Wine\\Drivers";
  string valueName = "Audio";
  string value = Helper::GetRegValue(filename, keyName, valueName);
  if(!value.empty()) {
    if(value.compare("pulse") == 0) {
      return BottleTypes::AudioDriver::pulseaudio;
    } else if(value.compare("alsa") == 0) {
      return BottleTypes::AudioDriver::alsa;
    } else if(value.compare("oss") == 0) {
      return BottleTypes::AudioDriver::oss;
    } else if(value.compare("coreaudio") == 0) {
      return BottleTypes::AudioDriver::coreaudio;
    } else if(value.compare("disabled") == 0) {
      return BottleTypes::AudioDriver::disabled;
    } else {
      // Otherwise just return PulseAudio
      return BottleTypes::AudioDriver::pulseaudio;
    }
  } else {
    // If not found, it is set to PulseAudio
    return BottleTypes::AudioDriver::pulseaudio;
  }
}

/**
 * \brief Get emulation resolution
 * \return Return the virtual desktop resolution or 'disabled' when disabled fully.
 */
string Helper::GetVirtualDesktop(const string prefix_path)
{
  // TODO: Check if virtual desktop is enabled or disabled first! By looking if this value name is set:
  // If the user.reg key: "Software\\Wine\\Explorer" Value name: "Desktop" is NOT set, its disabled.
  // If this value name is set (store the value of "Desktop"...), virtual desktop is enabled.
  //
  // The resolution can be found in Key: Software\\Wine\\Explorer\\Desktops with the Value name set as value 
  // (see above, "Default" is the default value). eg. "Default"="1920x1080"

  string filename = Glib::build_filename(prefix_path, USER_REG);
  string keyName = "Software\\Wine\\Explorer\\Desktops";
  string valueName = "Default";
  // TODO: first check of the Desktop value name in Software\\Wine\\Explorer
  string value = Helper::GetRegValue(filename, keyName, valueName);
  if(!value.empty()) {
    // Return the resolution
    return value;
  } else {
    return BottleTypes::VIRTUAL_DESKTOP_DISABLED;
  }
}

/**
 * \brief Get the date/time of the last time the Wine Inf file was updated
 * \return Date/time of last update
 */
string Helper::GetLastWineUpdated(const string prefix_path)
{
  string filename = Glib::build_filename(prefix_path, UPDATE_TIMESTAMP);
  if(Helper::FileExists(filename)) {
    std::vector<string> epoch_time = ReadFile(filename);
    if(epoch_time.size() >= 1) {
      string time = epoch_time.at(0);
      time_t secsSinceEpoch = strtoul(time.c_str(), NULL, 0);
      std::stringstream stringStream;
      stringStream << std::put_time(localtime(&secsSinceEpoch), "%c");
      return stringStream.str();
    } else {
      throw std::runtime_error("Could not determ last time wine update timestamp");
    }
  } else {
    throw std::runtime_error("Could not determ last time wine update timestamp");
  }
}

/**
 * \brief Get Bottle Status (is Bottle ready or not)
 * \param[in] prefix path
 * TODO: Maybe do not make this call blocking but async
 * \return True if everything is OK, otherwise false
 */
bool Helper::GetBottleStatus(const string prefix_path)
{
  // First check if directory exists at all (otherwise any wine command will create a new bottle)
  // And check if system.reg is present (important Wine file)
  if(Helper::DirExists(prefix_path) &&
     Helper::FileExists(Glib::build_filename(prefix_path, SYSTEM_REG))) {
      // TODO: Wine exec takes quite long, execute that in a seperate thread (don't block UI).
      // TODO: test the explorer /desktop=root part of the command
      //string result = Exec(("WINEPREFIX=" + prefix_path + " wine explorer /desktop=root cmd /Q /C ver").c_str());
      // Check for 'Microsoft Windows' string present
      //if(result.find("Microsoft Windows") != string::npos) {
    return true;
  } else {
    return false;
  }
}

/**
 * \brief Get C:\ Drive location
 * \return Location of C:\ location under unix
 */
string Helper::GetCLetterDrive(const string prefix_path)
{
  // Determ C location
  string c_drive_location = Glib::build_filename(prefix_path, "/dosdevices/c:/");
  if(Helper::DirExists(prefix_path) &&
     Helper::DirExists(c_drive_location)) {
       return c_drive_location;
  } else {
    return "- Unknown C:\\ drive location -";
  }
}

/**
 * \brief Check if *directory* exists or not
 * \return true if exists, otherwise false
 */
bool Helper::DirExists(const string& dir_path)
{    
  return Glib::file_test(dir_path, Glib::FileTest::FILE_TEST_IS_DIR);
}

/**
 * \brief Check if *file* exists or not
 * \return true if exists, otherwise false
 */
bool Helper::FileExists(const string& file_path)
{    
  return Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR);
}


/**
 * \brief Execute command on terminal. Return output.
 * \return Terminal stdout
 */
string Helper::Exec(const char* cmd) {
  // Max 128 characters
  std::array<char, 128> buffer;
  string result;
  // Execute command using popen
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

/**
 * \brief Get a value from the registery from disk
 * \param[in] filename  - File of registery
 * \param[in] keyName   - Full path of the subkey (eg. Software\\Wine\\Explorer)
 * \param[in] valueName - Specifies the registery value name (eg. Desktop)
 * \return Data of value name
 */
string Helper::GetRegValue(const string& filename, const string& keyName, const string& valueName)
{
  string matchStr = "";
  FILE *f;
  char buffer[100];
  // We add '[' & ']' around the key name
  const char *keyPattern = ('[' + keyName + ']').c_str();
  // We add double quotes around plus equal sign to the value name
  const char *valuePattern = ('"' + valueName + "\"=").c_str();
  char* match_pch = NULL;

  if(Helper::FileExists(filename)) 
  {
    if ((f = fopen(filename.c_str(), "r")) == NULL)
    {
      throw std::runtime_error("File could not be opened");
    }
    bool match = false;
    while (fgets(buffer, sizeof(buffer), f)) {
      // It returns the pointer to the first occurrence until the null character (end of line)
      if(!match) {
        // Search first for the applicable subkey
        if ((strstr(buffer, keyPattern)) != NULL) {
          match = true;
          // Continue to search for the key now
        }
      }
      else
      {
        // As long as there is no empty line (meaning end of the subkey section),
        // continue to search for the key
        if(strlen(buffer) == 0) {
           // Too late, nothing found within this subkey
          break;
        }
        else
        {
          // Search for the first occurence of the value name,
          // and put the strstr match char point in 'match_pch'
          if ((match_pch = strstr(buffer, valuePattern)) != NULL) {
            break;
          }
        }
      }
    }
    fclose(f);
    return CharPointerValueToString(match_pch);
  }
  else {
    throw std::runtime_error("Registery file does not exists. Can not determ Windows settings.");
  }
}

/**
 * \brief Get a meta value from the registery from disk
 * \param[in] filename  - File of registery
 * \param[in] metaValueName - Specifies the registery value name (eg. arch)
 * \return Data of value name
 */
string Helper::GetRegMetaData(const string& filename, const string& metaValueName)
{
  FILE *f;
  char buffer[100];
  const char *valuePattern = ('#' + metaValueName + '=').c_str();
  char* match_pch = NULL;

  if(Helper::FileExists(filename)) 
  {
    if ((f = fopen(filename.c_str(), "r")) == NULL)
    {
      throw std::runtime_error("File could not be opened");
    }
    while (fgets(buffer, sizeof(buffer), f)) {
      // Put the strstr match char point in 'match_pch'
      // It returns the pointer to the first occurrence until the null character (end of line)
      if ((match_pch = strstr(buffer, valuePattern)) != NULL) {
        // Match!
        break;
      }
    }
    fclose(f);
    return CharPointerValueToString(match_pch);
  }
  else {
    throw std::runtime_error("Registery file does not exists. Can not determ Windows settings.");
  }
}

/**
 * \brief Create a string from a value name char pointer
 * \param char pointer registery raw data value
 * \return string with the data
 */
string Helper::CharPointerValueToString(char* charp)
{
  if(charp != NULL)
  {
    string ret = string(charp);

    std::vector<string> results = Helper::Split(ret, '=');
    if(results.size() >= 2 ) {
      ret = results.at(1);
      // TODO: Combine the removals in a single iteration?
      // Remove double-quote chars
      ret.erase(std::remove(ret.begin(), ret.end(), '\"' ), ret.end());
      // Remove new lines
      ret.erase(std::remove(ret.begin(), ret.end(), '\n'), ret.end());
    }
    return ret;
  } 
  else
  {
    return "";
  }
}

/**
 * \brief Read data from file and returns it.
 * \return Data from file
 */
std::vector<string> Helper::ReadFile(const string file_path)
{
  std::vector<string> output;
  std::ifstream myfile(file_path);
  if(myfile.is_open())
  {
    std::string line;
    while(std::getline(myfile, line))
    {
      output.push_back(line);
    }
    myfile.close();
  } else {
    throw std::runtime_error("Could not open file!");
  }
  return output;
}

/**
 * \brief Split string by delimiter
 * \return Array of strings
 */
std::vector<string> Helper::Split(const string& s, char delimiter)
{
   std::vector<string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while(getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}