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
    std::vector<std::string> config = ReadFile(prefix_path + "/.winegui.conf");
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
  // TODO: system.reg is not the last up to date info! And missing the ProductName for older verions!
  // However, "WINEPREFIX=~/.bla wine cmd /C ver" does show the actual current Windows version
  // The registery (incl. wine reg query) is not sufficient enough, since older NT versions doesn't list the ProductName
  // Internal translation is needed (eg. v6.1 = Windows 7), see: https://en.wikipedia.org/wiki/List_of_Microsoft_Windows_versions
  // Source code: https://github.com/wine-mirror/wine/blob/master/dlls/ntdll/version.c
  // Too bad running a wine command (even just 'ver') takes quite some time for some reason.
  string filename = prefix_path + "/system.reg";
  string key = "\"ProductName\"=\"";
  if(Helper::FileExists(filename)) {
    string value = Helper::GetValueByKey(filename, key);
    if(!value.empty()) {
      if(value.compare("Microsoft Windows 2003") == 0) {
        return BottleTypes::Windows::Windows2003;
      } else if(value.compare("Microsoft Windows 2008") == 0) {
        return BottleTypes::Windows::Windows2008;
      } else if(value.compare("Microsoft Windows XP") == 0) {
        return BottleTypes::Windows::WindowsXP;
      } else if(value.compare("Microsoft Windows Vista") == 0) {
        return BottleTypes::Windows::WindowsVista;
      } else if(value.compare("Microsoft Windows 7") == 0) {
        return BottleTypes::Windows::Windows7;
      } else if(value.compare("Microsoft Windows 8") == 0) {
        return BottleTypes::Windows::Windows8;
      } else if(value.compare("Microsoft Windows 8.1") == 0) {
        return BottleTypes::Windows::Windows81;
      } else if(value.compare("Microsoft Windows 10") == 0) {
        return BottleTypes::Windows::Windows10;        
      } else {
        throw std::runtime_error("Could not determ Windows OS version (Unknown version).");
      }
    } else {
      throw std::runtime_error("Could not determ Windows OS version.");    
    }
  } else {
    throw std::runtime_error("Could not determ Windows OS version.");    
  }
}

/**
 * \brief Get system processor bit (32/64). *Throw runtime_error* when not found.
 * \return 32-bit or 64-bit
 */
BottleTypes::Bit Helper::GetSystemBit(const string prefix_path)
{
  string filename = prefix_path + "/user.reg";
  string key = "#arch=";
  if(Helper::FileExists(filename)) {
    string value = Helper::GetValueByKey(filename, key);
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
  } else {
    throw std::runtime_error("Could not determ Windows system bit.\nDoes the Wine bottle exists?");
  }
}

/**
 * \brief Get Audio driver
 * \return Audio Driver (eg. alsa/coreaudio/oss/pulse)
 */
BottleTypes::AudioDriver Helper::GetAudioDriver(const string prefix_path)
{
  string filename = prefix_path + "/user.reg";
  string key = "\"Audio\"=";
  if(Helper::FileExists(filename)) {
    string value = Helper::GetValueByKey(filename, key);
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
  } else {
    throw std::runtime_error("Could not determ Audio driver");
  }
}

/**
 * \brief Get emulation resolution
 * \return Return the virtual desktop resolution or 'disabled' when disabled fully.
 */
string Helper::GetVirtualDesktop(const string prefix_path)
{
  string filename = prefix_path + "/user.reg";
  string key = "\"Default\"=";
  if(Helper::FileExists(filename)) {
    string value = Helper::GetValueByKey(filename, key);
    if(!value.empty()) {
      // Return the resolution
      return value;
    } else {
      // If not found, it's disabled
      return BottleTypes::VIRTUAL_DESKTOP_DISABLED;
    }
  } else {
    throw std::runtime_error("Could not determ Virtual Desktop");
  }
}

/**
 * \brief Get the date/time of the last time the Wine Inf file was updated
 * \return Date/time of last update
 */
string Helper::GetLastWineUpdated(const string prefix_path)
{
  string filename = prefix_path + "/.update-timestamp";
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
     Helper::FileExists(prefix_path + "/system.reg")) {
      // This takes too long! Think about a better alternative?
      //string result = Exec(("WINEPREFIX=" + prefix_path + " wine cmd /Q /C ver").c_str());
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
  string c_drive_location = prefix_path + "/dosdevices/c:/";
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
 * \brief Get the value by key from registery
 * \param[in] Filename location path (eg. reg file)
 * \param[in] Key pattern to search for
 * \return The value or empty if not found
 */
string Helper::GetValueByKey(const string& filename, const string& key)
{
  string matchStr = "";
  FILE *f;
  char buffer[100];
  const char *pattern = key.c_str();
  char* match_pch = NULL;
  if ((f = fopen(filename.c_str(), "r")) == NULL) {
    throw std::runtime_error("File could not be opened");
  }
  while (fgets(buffer, sizeof(buffer), f)) {
    // Put the strstr match char point in 'match_pch'
    // It returns the pointer to the first occurrence until the null character (end of line)
    if ((match_pch = strstr(buffer, pattern)) != NULL) {
      // Match!
      break;
    }
  }
  fclose(f);

  if(match_pch != NULL) {
    // Create string
    matchStr = string(match_pch);

    std::vector<string> results = Helper::Split(matchStr, '=');
    if(results.size() >= 2 ) {
      matchStr = results.at(1);
      // TODO: Combine the removals in a single iteration?
      // Remove double-quote chars
      matchStr.erase(std::remove(matchStr.begin(), matchStr.end(), '\"' ), matchStr.end());
      // Remove new lines
      matchStr.erase(std::remove(matchStr.begin(), matchStr.end(), '\n'), matchStr.end());
    }
  }
  return matchStr;
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