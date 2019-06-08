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
#include <array>
#include <glibmm.h>
#include <giomm/file.h>
#include <glibmm/fileutils.h>

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
      if (name == "name") {
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
string Helper::GetWindowsOSVersion(const string prefix_path)
{
  string command = "cat " + prefix_path + "/system.reg | grep -m 1 '\"ProductName\"=' | cut -d '=' -f2 | sed 's/\"//g'";
  string result = Exec(command.c_str());
  if(result != "") {
    return result;
  } else {
    return " - Unknown Windows OS - ";
  }
}

/**
 * \brief Get system processor bit (32/64). Throw error when not found.
 * \return 32-bit or 64-bit
 */
BottleTypes::Bit Helper::GetSystemBit(const string prefix_path)
{
  string command = "cat " + prefix_path + "/system.reg | grep -m 1 '#arch' | cut -d '=' -f2";
  string result = Exec(command.c_str());
  if(result == "win32") {
    return BottleTypes::Bit::win32;
  } else if(result == "win64") {
    return BottleTypes::Bit::win64;
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
  string command = "cat " + prefix_path + "/user.reg | grep -m 1 '\"Audio\"=' | cut -d '=' -f2 | sed 's/\"//g'";
  string result = Exec(command.c_str());
  if(result == "pulse") {
    return BottleTypes::AudioDriver::pulseaudio;
  } else if(result == "alsa") {
    return BottleTypes::AudioDriver::alsa;
  } else if(result == "oss") {
    return BottleTypes::AudioDriver::oss;
  } else if(result == "coreaudio") {
    return BottleTypes::AudioDriver::coreaudio;
  } else if(result == "disabled") {
    return BottleTypes::AudioDriver::disabled;
  } else {
    return BottleTypes::AudioDriver::pulseaudio;
  }
}

/**
 * \brief Get emulation resolution
 * \return Return the virtual desktop resolution or 'disabled' when disabled fully.
 */
string Helper::GetVirtualDesktop(const string prefix_path)
{
  string command = "cat " + prefix_path + "/user.reg | grep -m 1 '\"Default\"=' | cut -d '=' -f2 | sed 's/\"//g'";
  string result = Exec(command.c_str());
  if(result != "") {
    return result;
  } else {
    return "disabled";
  }
}

/**
 * \brief Get the date/time of the last time the Wine Inf file was updated
 * \return Date/time of last update
 */
string Helper::GetLastWineUpdate(const string prefix_path)
{
  std::vector<string> epoch_time = ReadFile(prefix_path + "/.update-timestamp");
  if(epoch_time.size() > 1) {
    string time = epoch_time.at(0);
    time_t secsSinceEpoch = strtoul(time.c_str(), NULL, 0);
    std::stringstream stringStream;
    stringStream << std::put_time(localtime(&secsSinceEpoch), "%c") << std::endl;
    return stringStream.str();
  } else {
    return "- Unknown -";
  }
   
}

/**
 * \brief Get Wine Status (is Bottle ready or not)
 * \return True if everything is OK, otherwise false
 */
bool Helper::GetWineStatus(const string prefix_path)
{
  SetWinePrefix(prefix_path);
  string result = Exec("wine cmd /Q /C ver");
  // TODO: check exit code
  // Currenty only check on non-zero string
  if(result !="") {
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
  SetWinePrefix(prefix_path);
  string result = Exec("wine winepath C:");
  // TODO: check exit code
  if(result !="") {
    return result;
  } else {
    throw std::runtime_error("Could not find C:\\ drive location");
  }
}

/**
 * \brief Check if *directory* exists or not
 * \return true if exists, otherwise false
 */
bool Helper::Exists(const string& prefix_path)
{    
  return Glib::file_test(prefix_path, Glib::FileTest::FILE_TEST_IS_DIR);
}

std::vector<string> Helper::GetBottles(const string& prefix_path)
{
  std::vector<std::string> r;
  Glib::Dir dir(prefix_path);
  auto name = dir.read_name();
  while(!name.empty())
  {
    auto path = Glib::build_filename(prefix_path, name);
    if(Glib::file_test(path, Glib::FileTest::FILE_TEST_IS_DIR)) {
      r.push_back(path);
    }
    name = dir.read_name();
  }
  return r;
}


/**
 * \brief Get Wine version
 * \return Return the wine version
 */
string Helper::GetWineVersion()
{
  string result = Exec("wine --version");
  if(result != "") {
    std::vector<string> results = Split(result, '-');
    if(results.size() >= 2) {
      return results.at(1);
    } else {
      throw std::runtime_error("Could not determ wine version?\nSomething went wrong.");
    }
  } else {
    throw std::runtime_error("Could not receive Wine version!\n\nIs wine installed?");
  }
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

void Helper::SetWinePrefix(const string prefix_path) {
  char environ_variable[] = "WINEPREFIX=";
  strcat(environ_variable, prefix_path.c_str());
  int res = putenv(environ_variable);
  if(res != 0) {
    throw std::runtime_error("Can't set wine prefix environment variable");
  }
}

void Helper::RemoveWinePrefix() {
  int res = unsetenv("WINEPREFIX");
  if(res != 0) {
    throw std::runtime_error("Can't unset wine prefix environment variable");
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