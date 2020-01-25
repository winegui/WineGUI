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
#include <memory>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <array>
#include <time.h>
#include <glibmm.h>
#include <giomm/file.h>
#include <glibmm/timeval.h>
#include <glibmm/fileutils.h>

std::vector<std::string> dirs{Glib::get_home_dir(), ".winegui"}; /*!< WineGui config/storage directory path */
static string WINEGUI_DIR = Glib::build_path(G_DIR_SEPARATOR_S, dirs);

// Wine & Winetricks exec
static const string WINE_EXECUTABLE = "wine"; /*!< Currently expect to be installed globally */
static const string WINETRICKS_EXECUTABLE = Glib::build_filename(WINEGUI_DIR, "winetricks"); /*!< winetricks shall be located within the .winegui folder */

// Reg files
static const string SYSTEM_REG = "system.reg";
static const string USER_REG = "user.reg";
static const string USERDEF_REG = "userdef.reg";

// Reg keys
static const string keyName9x = "Software\\\\Microsoft\\\\Windows\\\\CurrentVersion";
static const string keyNameNT = "Software\\\\Microsoft\\\\Windows NT\\\\CurrentVersion";
static const string keyType   = "System\\\\CurrentControlSet\\\\Control\\\\ProductOptions";

// Reg names
static const string nameNTVersion   = "CurrentVersion";
static const string nameNTBuild     = "CurrentBuildNumber";
static const string name9xVersion   = "VersionNumber";
static const string nameProductType = "ProductType";

// Other files
static const string WINEGUI_CONF = ".winegui.conf";
static const string UPDATE_TIMESTAMP = ".update-timestamp";

// Windows version table to convert Windows version in registery to BottleType Windows enum value
// Source: https://github.com/wine-mirror/wine/blob/master/programs/winecfg/appdefaults.c#L49
// Note: Build number is tranformed to decimal number
static const struct
{
  const BottleTypes::Windows windows;
  const string versionNumber;
  const string buildNumber;
  const string productType;
} win_versions[] =
{
  {BottleTypes::Windows::Windows10,     "10.0", "17134", "WinNT"},
  {BottleTypes::Windows::Windows81,     "6.3",  "9600",  "WinNT"},
  {BottleTypes::Windows::Windows8,      "6.2",  "9200",  "WinNT"},
  {BottleTypes::Windows::Windows2008R2, "6.1",  "7601",  "ServerNT"},
  {BottleTypes::Windows::Windows7,      "6.1",  "7601",  "WinNT"},
  {BottleTypes::Windows::Windows2008,   "6.0",  "6002",  "ServerNT"},
  {BottleTypes::Windows::WindowsVista,  "6.0",  "6002",  "WinNT"},
  {BottleTypes::Windows::Windows2003,   "5.2",  "3790",  "ServerNT"},
  {BottleTypes::Windows::WindowsXP,     "5.2",  "3790",  "WinNT"}, // 64-bit
  {BottleTypes::Windows::WindowsXP,     "5.1",  "2600",  "WinNT"}, // 32-bit
  {BottleTypes::Windows::Windows2000,   "5.0",  "2195",  "WinNT"},
  {BottleTypes::Windows::WindowsME,     "4.90", "3000",  ""},
  {BottleTypes::Windows::Windows98,     "4.10", "2222",  ""},
  {BottleTypes::Windows::Windows95,     "4.0",  "950",   ""},
  {BottleTypes::Windows::WindowsNT40,   "4.0",  "1381",  "WinNT"},
  {BottleTypes::Windows::WindowsNT351,  "3.51", "1057",  "WinNT"},
  {BottleTypes::Windows::Windows31,     "3.10", "0",     ""},
  {BottleTypes::Windows::Windows30,     "3.0",  "0",     ""},
  {BottleTypes::Windows::Windows20,     "2.0",  "0",     ""}
};

// Meyers Singleton
Helper::Helper()= default;
Helper::~Helper()= default;

Helper& Helper::getInstance() {
  static Helper instance;
  return instance;
}

/****************************************************************************
 *  Public methods                                                          *
 ****************************************************************************/

/**
 * \brief Get the bottle directories within the given path
 * \param[in] dir_path Path to search in
 * \return map of path names (strings) and modification time (in ms) of found directories (*full paths*)
 */
std::map<std::string, unsigned long> Helper::GetBottlesPaths(const string& dir_path) // , sort = DEFAULT, NAME, DATE>
{
  std::map<std::string, unsigned long> r;
  Glib::Dir dir(dir_path);
  auto name = dir.read_name();
  while (!name.empty())
  {    
    auto path = Glib::build_filename(dir_path, name);
    if (Glib::file_test(path, Glib::FileTest::FILE_TEST_IS_DIR)) {
      r.insert(std::pair<string, unsigned long>(path, GetModifiedTime(path)));
    }
    name = dir.read_name();
  }
  return r;
}

/**
 * \brief Run a Windows program under Wine (run in thread, and dettach it)
 * \param[in] prefix_path - The path to bottle wine
 * \param[in] program - Program/executable that will be executed
 * \param[in] enable_tracing - Enable debugging tracing to file
 * \param[in] is_msi_file - Is the program a MSI installer, let's instal it
 */
void Helper::RunProgramUnderWine(string prefix_path, string program, bool enable_tracing, bool is_msi_file)
{
  string msi = "";
  if (is_msi_file) {
    msi = " msiexec /i";
  }
  // Execute the command and show the user a message when exit code is non-zero
  ExecTracing(("WINEPREFIX=\"" + prefix_path + "\"" + msi + " " + WINE_EXECUTABLE + " " + program).c_str(), enable_tracing);
}

/**
 * \brief Run any program with only setting the WINEPREFIX env variable.
 * \param[in] prefix_path - The path to wine bottle
 * \param[in] program - Program that gets executed (ideally full path)
 * \param[in] enable_tracing - Enable debugging tracing to file (give_error should be true as well!)
 * \param[in] give_error - Inform user when application exit with non-zero exit code
 */
void Helper::RunProgramWithPrefix(string prefix_path, string program, bool enable_tracing, bool give_error)
{
  bool execTracing = false;
  if (enable_tracing) {
    execTracing = true;
  }
  if (!give_error) {
    execTracing = false;
  }
  if (execTracing) {
    // Execute the command and show the user a message when exit code is non-zero
    ExecTracing(("WINEPREFIX=\"" + prefix_path + "\" " + program).c_str(), enable_tracing);
  } else {
    // No tracing and no error message when exit code is non-zero
    Exec(("WINEPREFIX=\"" + prefix_path + "\" " + program).c_str());
  }
}

/**
 * \brief Get Wine version from CLI
 * \return Return the wine version
 */
string Helper::GetWineVersion()
{
  string result = Exec((WINE_EXECUTABLE + " --version").c_str());
  if (!result.empty()) {
    std::vector<string> results = Split(result, '-');
    if (results.size() >= 2) {
      string version = results.at(1);;
      // Remove new lines
      version.erase(std::remove(version.begin(), version.end(), '\n'), version.end());
      return version;
    } else {
      throw std::runtime_error("Could not determ wine version?\nSomething went wrong.");
    }
  } else {
    throw std::runtime_error("Could not receive Wine version!\n\nIs wine installed?");
  }
}

/**
 * \brief Create new Wine bottle from prefix
 * \throw Throw an error when something went wrong during the creation of the bottle
 * \param[in] prefix_path - The path to create a Wine bottle from
 * \param[in] bit - Create 32-bit Wine of 64-bit Wine bottle
 * \param[in] disable_gecko_mono - Do NOT install Mono & Gecko (by default should be false)
 */
void Helper::CreateWineBottle(const string prefix_path, BottleTypes::Bit bit, const bool disable_gecko_mono)
{
  string wineArch = "";
  switch (bit) {
    case BottleTypes::Bit::win32:
      wineArch = " WINEARCH=win32";
      break;
    case BottleTypes::Bit::win64:
      wineArch = " WINEARCH=win64";
      break;
  }
  string wineDLLOverrides = (disable_gecko_mono) ? " WINEDLLOVERRIDES=\"mscoree=d;mshtml=d\"" : "";

  string result = Exec(("WINEPREFIX=\"" + prefix_path + "\"" + wineArch + wineDLLOverrides + " " +
    WINE_EXECUTABLE + " wineboot>/dev/null 2>&1; echo $?").c_str());
  if (!result.empty())
  {
    // Remove new lines
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    if (!(result.compare("0") == 0)) 
    {
      throw std::runtime_error("Something went wrong when creating a new Windows machine. Wine machine: " + 
        GetName(prefix_path) +
        "\n\nFull path location: " + prefix_path);
    }
  }
  else
  {
    throw std::runtime_error("Something went wrong when creating a new Windows machine. Wine machine:: " + 
      GetName(prefix_path) + "\n\nFull location: " + prefix_path);
  }  
}

/**
 * \brief Remove existing Wine bottle using prefix
 * \param[in] prefix_path - The wine bottle path which will be removed
 */
void Helper::RemoveWineBottle(const string prefix_path)
{
  if (Helper::DirExists(prefix_path))
  {
    string result = Exec(("rm -rf \"" + prefix_path + "\"; echo $?").c_str());
    if (!result.empty())
    {
      // Remove new lines
      result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
      if (!(result.compare("0") == 0)) 
      {
        throw std::runtime_error("Something went wrong when removing the Windows Machine. Wine machine: " + 
          GetName(prefix_path) + "\n\nFull path location: " + prefix_path);
      }
    }
    else
    {
      throw std::runtime_error("Could not remove Windows Machine, no result. Wine machine: " + 
        GetName(prefix_path) + "\n\nFull path location: " + prefix_path);
    }
  }
  else
  {
    throw std::runtime_error("Could not remove Windows Machine, prefix is not a directory. Wine machine: " + 
      GetName(prefix_path) + "\n\nFull path location: " + prefix_path);
  }  
}

/**
 * \brief Get Wine Bottle Name from configuration file (if possible)
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * \return Bottle name
 */
string Helper::GetName(const string prefix_path)
{
  try
  {
    std::vector<std::string> config = ReadFile(Glib::build_filename(prefix_path, WINEGUI_CONF));
    for (std::vector<std::string>::iterator config_line = config.begin(); config_line != config.end(); ++config_line) {
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

  // Fall-back: get last directory name of path string as 'Bottle name'
  return getBottleDirFromPrefix(prefix_path);
}

/**
 * \brief Get current Windows OS version
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * \return Return the Windows OS version
 */
BottleTypes::Windows Helper::GetWindowsOSVersion(const string prefix_path)
{  
  string filename = Glib::build_filename(prefix_path, SYSTEM_REG);
  string version = "";
  string version9x = "";

  if (!(version = Helper::GetRegValue(filename, keyNameNT, nameNTVersion)).empty())
  {
    string buildNumberNT = Helper::GetRegValue(filename, keyNameNT, nameNTBuild);
    string typeNT = Helper::GetRegValue(filename, keyType, nameProductType);
    // Find Windows version
    for (unsigned int i = 0; i < BottleTypes::WINDOWS_ENUM_SIZE; i++)
    {      
      // Check if version + build number matches
      if ( ((win_versions[i].versionNumber).compare(version) == 0) && 
          ((win_versions[i].buildNumber).compare(buildNumberNT) == 0)
        )
      {
        if (!typeNT.empty())
        {
          if ((win_versions[i].productType).compare(typeNT) == 0)
          {
            return win_versions[i].windows;
          }
        } else {
          return win_versions[i].windows;
        }        
      }
    }
  }
  else if (!(version = Helper::GetRegValue(filename, keyName9x, name9xVersion)).empty())
  {
    string currentVersion = "";
    string currentBuildNumber = "";
    std::vector<string> versionList = Split(version, '.');
    // Only get minor & major
    if (sizeof(versionList) >= 2) {
      currentVersion = versionList.at(0) + '.' + versionList.at(1);
    }
    // Get build number
    if (sizeof(versionList) >= 3) {
      currentBuildNumber = versionList.at(2);
    }
     
    // Find Windows version
    for (unsigned int i = 0; i < BottleTypes::WINDOWS_ENUM_SIZE; i++)
    {
      // Check if version + build number matches
      if (((win_versions[i].versionNumber).compare(currentVersion) == 0) && 
        ((win_versions[i].buildNumber).compare(currentBuildNumber) == 0))
      {
        return win_versions[i].windows;
      }
    }
  }
  else
  {
    throw std::runtime_error("Could not determ Windows OS version, for Wine machine: " + 
      GetName(prefix_path) +
      "\n\nFull location: " + prefix_path);
  }
  // Function didn't return before (meaning no match found)
  throw std::runtime_error("Could not determ Windows OS version, for Wine machine: " + 
    GetName(prefix_path) +
    "\n\nFull location: " + prefix_path);
}

/**
 * \brief Get system processor bit (32/64). *Throw runtime_error* when not found.
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * \return 32-bit or 64-bit
 */
BottleTypes::Bit Helper::GetSystemBit(const string prefix_path)
{
  string filename = Glib::build_filename(prefix_path, USER_REG);

  string metaValueName = "arch";
  string value = Helper::Helper::GetRegMetaData(filename, metaValueName);
  if (!value.empty()) {
    if (value.compare("win32") == 0) {
      return BottleTypes::Bit::win32;
    } else if (value.compare("win64") == 0) {
      return BottleTypes::Bit::win64;
    } else {
      throw std::runtime_error("Could not determ Windows system bit (not win32 and not win64, value: " + value + "), for Wine machine: " + 
        GetName(prefix_path) +
        "\n\nFull location: " + prefix_path);
    }
  } else {
    throw std::runtime_error("Could not determ Windows system bit, for Wine machine: " + 
      GetName(prefix_path) +
      "\n\nFull location: " + prefix_path);
  }
}

/**
 * \brief Get Audio driver
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * \return Audio Driver (eg. alsa/coreaudio/oss/pulse)
 */
BottleTypes::AudioDriver Helper::GetAudioDriver(const string prefix_path)
{
  string filename = Glib::build_filename(prefix_path, USER_REG);
  string keyName = "Software\\\\Wine\\\\Drivers";
  string valueName = "Audio";
  string value = Helper::GetRegValue(filename, keyName, valueName);
  if (!value.empty()) {
    if (value.compare("pulse") == 0) {
      return BottleTypes::AudioDriver::pulseaudio;
    } else if (value.compare("alsa") == 0) {
      return BottleTypes::AudioDriver::alsa;
    } else if (value.compare("oss") == 0) {
      return BottleTypes::AudioDriver::oss;
    } else if (value.compare("coreaudio") == 0) {
      return BottleTypes::AudioDriver::coreaudio;
    } else if (value.compare("disabled") == 0) {
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
 * \param[in] prefix_path The prefix directory path to the bottle prefix
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
  string keyName = "Software\\\\Wine\\\\Explorer\\\\Desktops";
  string valueName = "Default";
  // TODO: first check of the Desktop value name in Software\\Wine\\Explorer
  string value = Helper::GetRegValue(filename, keyName, valueName);
  if (!value.empty()) {
    // Return the resolution
    return value;
  } else {
    return BottleTypes::VIRTUAL_DESKTOP_DISABLED;
  }
}

/**
 * \brief Get the date/time of the last time the Wine Inf file was updated
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * \return Date/time of last update
 */
string Helper::GetLastWineUpdated(const string prefix_path)
{
  string filename = Glib::build_filename(prefix_path, UPDATE_TIMESTAMP);
  if (Helper::FileExists(filename)) {
    std::vector<string> epoch_time = ReadFile(filename);
    if (epoch_time.size() >= 1) {
      string time = epoch_time.at(0);
      time_t secsSinceEpoch = strtoul(time.c_str(), NULL, 0);
      std::stringstream stringStream;
      stringStream << std::put_time(localtime(&secsSinceEpoch), "%c");
      return stringStream.str();
    } else {
      throw std::runtime_error("Could not determ last time wine update timestamp, for Wine machine: " + 
      GetName(prefix_path) +
      "\n\nFull location: " + prefix_path);
    }
  } else {
    throw std::runtime_error("Could not determ last time wine update timestamp, for Wine machine: " + 
      GetName(prefix_path) +
      "\n\nFull location: " + prefix_path);
  }
}

/**
 * \brief Get Bottle Status (is Bottle ready or not)
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * TODO: Maybe do not make this call blocking but async
 * \return True if everything is OK, otherwise false
 */
bool Helper::GetBottleStatus(const string prefix_path)
{
  // First check if directory exists at all (otherwise any wine command will create a new bottle)
  // And check if system.reg is present (important Wine file)
  // And finally if we can read-out the Windows OS version
  if (Helper::DirExists(prefix_path) &&
     Helper::FileExists(Glib::build_filename(prefix_path, SYSTEM_REG))) {
    try
    {
      Helper::GetWindowsOSVersion(prefix_path);
      return true;
    } catch (const std::runtime_error& error){
      // Not good!
      return false;
    }
    // TODO: Wine exec takes quite long, execute that in a seperate thread (don't block UI).
    // TODO: test the explorer /desktop=root part of the command
    //string result = Exec(("WINEPREFIX=\"" + prefix_path + "\" " + WINE_EXECUTABLE + " explorer /desktop=root cmd /Q /C ver").c_str());
  } else {
    return false;
  }
}

/**
 * \brief Get C:\ Drive location
 * \param[in] prefix_path The prefix directory path to the bottle prefix
 * \return Location of C:\ location under unix
 */
string Helper::GetCLetterDrive(const string prefix_path)
{
  // Determ C location
  string c_drive_location = Glib::build_filename(prefix_path, "/dosdevices/c:/");
  if (Helper::DirExists(prefix_path) &&
     Helper::DirExists(c_drive_location)) {
       return c_drive_location;
  } else {
    throw std::runtime_error("Could not determ C:\\ drive location, for Wine machine: " + 
      GetName(prefix_path) +
      "\n\nFull location: " + prefix_path);
  }
}

/**
 * \brief Check if *directory* exists or not
 * \param[in] dir_path The directory to be checked for existence
 * \return true if exists, otherwise false
 */
bool Helper::DirExists(const string& dir_path)
{    
  return Glib::file_test(dir_path, Glib::FileTest::FILE_TEST_IS_DIR);
}

/**
 * \brief Create directory (and intermediate parent directories if needed)
 * \param[in] dir_path The directory to be created
 * \return true if successfully created, otherwise false
 */
bool Helper::CreateDir(const string& dir_path)
{
  return (g_mkdir_with_parents(dir_path.c_str(), 0775) == 0);
}

/**
 * \brief Check if *file* exists or not
 * \param[in] file_path The file to be checked for existence
 * \return true if exists, otherwise false
 */
bool Helper::FileExists(const string& file_path)
{    
  return Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR);
}

/**
 * \brief Install or update Winetricks (eg. when not found locally yet)
 * Throws an error if the download and/or install was not successful.
 */
void Helper::InstallOrUpdateWinetricks()
{
  // Check if ~/.winegui directory is created
  if (!DirExists(WINEGUI_DIR)) {
    bool created = CreateDir(WINEGUI_DIR);
    if (!created)
    {
      throw std::runtime_error("Incorrect permissions to create a .winegui configuration folder! Abort.");
    }
  }

  Exec(("cd \"$(mktemp -d)\" && wget -q https://raw.githubusercontent.com/Winetricks/winetricks/master/src/winetricks && chmod +x winetricks && mv winetricks " + WINETRICKS_EXECUTABLE).c_str());
  // Winetricks script should exists now...
  if (!FileExists(WINETRICKS_EXECUTABLE))
  {
    throw std::runtime_error("Winetrick helper script can not be found / installed. This could/will result into issues with WineGUI!");
  }
}

/**
 * \brief Update an existing local Winetricks, only useful if winetricks is already deployed.
 */
void Helper::SelfUpdateWinetricks()
{
  if (FileExists(WINETRICKS_EXECUTABLE))
  {
    string result = Exec((WINETRICKS_EXECUTABLE + " --self-update >/dev/null 2>&1; echo $?").c_str());
    if (!result.empty())
    {
      result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
      if (result.compare("0") != 0)
      {
        throw std::invalid_argument("Could not update Winetricks, keep using the v" + Helper::GetWinetricksVersion());
      }
    } else {
      throw std::invalid_argument("Could not update Winetricks, keep using the v" + Helper::GetWinetricksVersion());
    }    
  }
  else
  {
    throw std::runtime_error("Try to update the Winetricks script, while there is no winetricks installed/not found!");
  }
}

/**
 * \brief Get the Winetrick version
 * \return The version of Winetricks
 */
string Helper::GetWinetricksVersion()
{
  string version = "";
  if (FileExists(WINETRICKS_EXECUTABLE)) {
    string result = Exec((WINETRICKS_EXECUTABLE + " --version").c_str());
    if (!result.empty())
    {
      if (result.length() >= 8) {
        version = result.substr(0, 8); // Retrieve YYYYMMDD
      }
    }
  }
  return version;
}

/**
 * \brief Set Windows OS version by using Winetricks
 */
void Helper::SetWindowsVersion(const string prefix_path, BottleTypes::Windows windows)
{
  if (FileExists(WINETRICKS_EXECUTABLE))
  {
    string win = BottleTypes::getWinetricksString(windows);
    string result = Exec(("WINEPREFIX=\"" + prefix_path + "\" " + WINETRICKS_EXECUTABLE + " " + win + ">/dev/null 2>&1; echo $?").c_str());
    if (!result.empty()) {
      result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
      if (result.compare("0") != 0) {
        throw std::runtime_error("Could not set Windows OS version");
      }
    } else {
      throw std::runtime_error("Could not set Windows OS version");
    }    
  }
}

/**
 * \brief Set custom virtual desktop resolution by using Winetricks
 */
void Helper::SetVirtualDesktop(const string prefix_path, string resolution)
{
  if (FileExists(WINETRICKS_EXECUTABLE))
  {
    std::vector<string> res = Split(resolution, 'x');
    if (res.size() >= 2)
    {
      int x = 0, y = 0;
      try 
      {
        x = std::atoi(res.at(0).c_str());
        y = std::atoi(res.at(1).c_str());
      }
      catch(std::exception const & e)
      {
        throw std::runtime_error("Could not set virtual desktop resolution (invalid input)");
      }

      if (x < 640 || y < 480)
      {
        // Set to minimum resolution
        resolution = "640x480";
      }

      string result = Exec(("WINEPREFIX=\"" + prefix_path + "\" " + WINETRICKS_EXECUTABLE + " vd=" + resolution + ">/dev/null 2>&1; echo $?").c_str());
      // Something returns non-zero... winetricks on the command line, does return zero ..
      /*
      if (!result.empty()) {
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        if (result.compare("0") != 0) {
          throw std::runtime_error("Could not set virtual desktop resolution");
        }
      } else {
        throw std::runtime_error("Could not set virtual desktop resolution");
      }*/
    }
    else
    {
      throw std::runtime_error("Could not set virtual desktop resolution (invalid input)");
    }
  }
}

/**
 * \brief Disable Virtual Desktop fully by using Winetricks
 */
void Helper::DisableVirtualDesktop(const string prefix_path)
{
  if (FileExists(WINETRICKS_EXECUTABLE))
  {
    string result = Exec(("WINEPREFIX=\"" + prefix_path + "\" " + WINETRICKS_EXECUTABLE + " vd=off>/dev/null 2>&1; echo $?").c_str());
    // Something returns non-zero... winetricks on the command line, does return zero ..
    /*if (!result.empty()) {
      result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
      if (result.compare("0") != 0) {
        throw std::runtime_error("Could not Disable Virtual Desktop");
      }
    } else {
      throw std::runtime_error("Could not Disable Virtual Desktop");
    }*/
  }
}

/**
 * \brief Set Audio Driver by using Winetricks
 * \param[in] audio_driver - Audio driver to be set
 */
void Helper::SetAudioDriver(const string prefix_path, BottleTypes::AudioDriver audio_driver)
{
  if (FileExists(WINETRICKS_EXECUTABLE))
  {
    string audio = BottleTypes::getWinetricksString(audio_driver);
    // 
    string result = Exec(("WINEPREFIX=\"" + prefix_path + "\" " + WINETRICKS_EXECUTABLE + " sound=" + audio + ">/dev/null 2>&1; echo $?").c_str());
    if (!result.empty()) {
      result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
      if (result.compare("0") != 0) {
        throw std::runtime_error("Could not set Audio driver");
      }
    } else {
      throw std::runtime_error("Could not set Audio driver");
    }  
  }
}

/**
 * \brief Get the Winetricks binary location
 * \return the full path to Winetricks
 */
string Helper::GetWinetricksLocation()
{
  string path = "";
  if (FileExists(WINETRICKS_EXECUTABLE))
  {
    path = WINETRICKS_EXECUTABLE;
  } else {
    g_warning("Could not find winetricks executable!");
  }
  return path;
}

/**
 * \brief Get the Wine Mono GUID
 * \param[in] application_name - Application name to search for
 * \return GUID
 */
string Helper::GetWineGUID(const string prefix_path, const string application_name)
{
  string result = Exec(("WINEPREFIX=\"" + prefix_path + "\" " + WINETRICKS_EXECUTABLE + " uninstaller --list | grep \"" + application_name + "\" | cut -d \"{\" -f2 | cut -d \"}\" -f1").c_str());
  if (!result.empty()) {
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
  }
  return result;
}

/****************************************************************************
 *  Private methods                                                         *
 ****************************************************************************/

/**
 * \brief Execute command on terminal. Return output.
 * \param[in] cmd The command to be executed
 * \return Terminal stdout
 */
string Helper::Exec(const char* cmd) {
  // Max 128 characters
  std::array<char, 128> buffer;
  string result = "";
  
  // Execute command using popen  
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), &pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

/**
 * \brief Execute command on terminal, give user an error went something went wrong.
 * Also write output to log (if debugging is enabled).
 * \param[in] cmd The command to be executed
 * \param[in] enableTracing Enable debugging tracing to log file (default false)
 * \return Terminal stdout
 */
void Helper::ExecTracing(const char* cmd, bool enableTracing) {
  // Max 128 characters
  std::array<char, 128> buffer;
  string result = "";

  // Execute command using popen
  // Use a custom delete method (CloseFile)
  std::unique_ptr<FILE, decltype(&CloseFile)> pipe(popen(cmd, "r"), &CloseFile);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  
  if (enableTracing) {
    // TODO: Dump result to log file.
  }
}

int Helper::CloseFile(std::FILE* file) {
  if (file) {
    if (std::fclose(file) != 0) {
      // Dispatcher will run the connected slot in the main loop,
      // instead of the same context/thread in case of a signal.emit() call.
      // This is needed because the CloseFile is called in a different context then usual!
      // Signal error message to the user:
      Helper::getInstance().failureOnExec.emit();
    }
  }
  return 0; // Just always return OK
}

/**
 * \brief Get a value from the registery from disk
 * \param[in] filename  File of registery
 * \param[in] keyName   Full path of the subkey (eg. Software\\\\Wine\\\\Explorer)
 * \param[in] valueName Specifies the registery value name (eg. Desktop)
 * \return Data of value name
 */
string Helper::GetRegValue(const string& filename, const string& keyName, const string& valueName)
{
  // We add '[' & ']' around the key name
  string keyPattern = '[' + keyName + ']';
  // We add double quotes around plus equal sign to the value name
  string valuePattern = '"' + valueName + "\"=";
  char* match_pch = NULL;
  if (Helper::FileExists(filename)) 
  {
    FILE *f;
    char buffer[100];
    if ((f = fopen(filename.c_str(), "r")) == NULL)
    {
      throw std::runtime_error("File could not be opened");
    }
    bool match = false;
    while (fgets(buffer, sizeof(buffer), f)) {
      // It returns the pointer to the first occurrence until the null character (end of line)
      if (!match) {
        // Search first for the applicable subkey
        if ((strstr(buffer, keyPattern.c_str())) != NULL) {
          match = true;
          // Continue to search for the key now
        }
      }
      else
      {
        // As long as there is no empty line (meaning end of the subkey section),
        // continue to search for the key
        if (strlen(buffer) == 0) {
           // Too late, nothing found within this subkey
          break;
        }
        else
        {
          // Search for the first occurence of the value name,
          // and put the strstr match char point in 'match_pch'
          match_pch = strstr(buffer, valuePattern.c_str());
          if (match_pch != NULL) {
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
 * \param[in] filename      File of registery
 * \param[in] metaValueName Specifies the registery value name (eg. arch)
 * \return Data of value name
 */
string Helper::GetRegMetaData(const string& filename, const string& metaValueName)
{
  string metaPattern = "#" + metaValueName + "=";
  char* match_pch = NULL;
  if (Helper::FileExists(filename)) 
  {
    FILE *f;
    char buffer[100];
    if ((f = fopen(filename.c_str(), "r")) == NULL)
    {
      throw std::runtime_error("File could not be opened");
    }
    while (fgets(buffer, sizeof(buffer), f)) {
      // Put the strstr match char point in 'match_pch'
      // It returns the pointer to the first occurrence until the null character (end of line)
      match_pch = strstr(buffer, metaPattern.c_str());
      if (match_pch != NULL) {
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
 * \brief Get the 'Bottle Name' (directory) from the full prefix path
 *  Can be used as fall-back.
 * \param[in] prefix_path   Full bottle prefix path
 * \return Bottle directory name
 */
string Helper::getBottleDirFromPrefix(const string& prefix_path)
{
  string name = "- Unknown -";
  std::size_t last_index = prefix_path.find_last_of("/\\");
  if (last_index != string::npos) {
    // Get only the last directory name from path (+ remove slash)
    name = prefix_path.substr(last_index+1);
    // Remove dot if present (=hidden dir)
    size_t dot_index = name.find_first_of('.');
    if (dot_index == 0) {
      // Remove dot at start
      name = name.substr(1);
    }
  }
  return name;
}

/**
 * \brief Create a string from a value name char pointer
 * \param charp Character pointer registery raw data value
 * \return string with the data
 */
string Helper::CharPointerValueToString(char* charp)
{
  if (charp != NULL)
  {
    string ret = string(charp);

    std::vector<string> results = Helper::Split(ret, '=');
    if (results.size() >= 2 ) {
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
 * \param[in] file_path File location to be read
 * \return Data from file
 */
std::vector<string> Helper::ReadFile(const string file_path)
{
  std::vector<string> output;
  std::ifstream myfile(file_path);
  if (myfile.is_open())
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
 * \brief Get the last modified date of a file and/or folder
 * \param[in] file_path File/Folder location
 * \return Last modifiction time in milliseconds (ms)
 */
unsigned long Helper::GetModifiedTime(const string file_path)
{
    auto time_info = Gio::File::create_for_path(file_path)->query_info("time");
    auto time = time_info->modification_time();
    return (time.tv_sec*1000)+(time.tv_usec/1000);
}

/**
 * \brief Split string by delimiter
 * \param[in] s         String to be splitted
 * \param[in] delimiter Delimiter character
 * \return Array of strings
 */
std::vector<string> Helper::Split(const string& s, char delimiter)
{
   std::vector<string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}
