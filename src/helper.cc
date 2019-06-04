/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    helper.cc
 * \brief   Provide some helper methods for CLI interaction
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

/**
 * \brief Retrieve Wine Bottle Name from config
 * \return Name
 */
string Helper::retrieveName(string prefix_path)
{
  try
  {
    vector<string> config = readFile(prefix_path + "/.winegui.conf");
    for(vector<string>::iterator config_line = config.begin(); config_line != config.end(); ++config_line) {
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
  // Fall-back, get last directory name of path
  filesystem::path path = filesystem::u8path(prefix_path);
  return (*path.end()).u8string();
}

/**
 * \brief Retrieve current Windows OS version
 * \return Return the Windows OS version
 */
string Helper::retrieveWindowsOSVersion(string prefix_path)
{
  string command = "cat " + prefix_path + "/system.reg | grep -m 1 '\"ProductName\"=' | cut -d '=' -f2 | sed 's/\"//g'";
  string result = exec(command.c_str());
  if(result != "") {
    return result;
  } else {
    return " - Unknown Windows OS - ";
  }
}

/**
 * \brief Retrieve system processor bit (32/64). Throw error when not found.
 * \return 32-bit or 64-bit
 */
Bit Helper::retrieveSystemBit(string prefix_path)
{
  string command = "cat " + prefix_path + "/system.reg | grep -m 1 '#arch' | cut -d '=' -f2";
  string result = exec(command.c_str());
  if(result == "win32") {
    return Bit::win32;
  } else if(result == "win64") {
    return Bit::win64;
  } else {
    throw runtime_error("Could not determ Windows system bit.");
  }
}

/**
 * \brief Retrieve Audio driver
 * \return Audio Driver (eg. alsa/coreaudio/oss/pulse)
 */
AudioDriver Helper::retrieveAudioDriver(string prefix_path)
{
  string command = "cat " + prefix_path + "/user.reg | grep -m 1 '\"Audio\"=' | cut -d '=' -f2 | sed 's/\"//g'";
  string result = exec(command.c_str());
  if(result == "pulse") {
    return AudioDriver::pulseaudio;
  } else if(result == "alsa") {
    return AudioDriver::alsa;
  } else if(result == "oss") {
    return AudioDriver::oss;
  } else if(result == "coreaudio") {
    return AudioDriver::coreaudio;
  } else if(result == "disabled") {
    return AudioDriver::disabled;
  } else {
    return AudioDriver::pulseaudio;
  }
}

/**
 * \brief Retrieve emulation resolution
 * \return Return the virtual desktop resolution or 'disabled' when disabled fully.
 */
string Helper::retrieveVirtualDesktop(string prefix_path)
{
  string command = "cat " + prefix_path + "/user.reg | grep -m 1 '\"Default\"=' | cut -d '=' -f2 | sed 's/\"//g'";
  string result = exec(command.c_str());
  if(result != "") {
    return result;
  } else {
    return "disabled";
  }
}

/**
 * \brief Retrieve the date/time of the last time the Wine Inf file was updated
 * \return Date/time of last update
 */
string Helper::retrieveLastWineUpdate(string prefix_path)
{
  vector<string> epoch_time = readFile(prefix_path + "/.update-timestamp");
  if(epoch_time.size() > 1) {
    string time = epoch_time.at(0);
    time_t secsSinceEpoch = strtoul(time.c_str(), NULL, 0);
    stringstream stringStream;
    stringStream << put_time(localtime(&secsSinceEpoch), "%c") << endl;
    return stringStream.str();
  } else {
    return "- Unknown -";
  }
   
}

/**
 * \brief Retrieve Wine Status (is Bottle ready or not)
 * \return True if everything is OK, otherwise false
 */
bool Helper::retrieveWineStatus(string prefix_path)
{
  setWinePrefix(prefix_path);
  string result = exec("wine cmd /Q /C ver");
  // TODO: check exit code
  // Currenty only check on non-zero string
  if(result !="") {
    return true;
  } else {
    return false;
  }
}

/**
 * \brief Retrieve C:\ Drive location
 * \return Location of C:\ location under unix
 */
string Helper::retrieveCLetterDrive(string prefix_path)
{
  setWinePrefix(prefix_path);
  string result = exec("wine winepath C:");
  // TODO: check exit code
  if(result !="") {
    return result;
  } else {
    throw runtime_error("Could not find C:\\ drive location");
  }
}

/**
 * \brief Retrieve Wine version
 * \return Return the wine version
 */
string Helper::retrieveWineVersion()
{
  string result = exec("wine --version");
  if(result != "") {
    vector<string> results = split(result, '-');
    if(results.size() > 2) {
      return results.at(1);
    } else {
      throw runtime_error("Could not receive Wine version");
    }
  } else {
    return "Unknown Windows OS";
  }
}

/**
 * \brief Execute command on terminal. Return output.
 * \return Terminal stdout
 */
string Helper::exec(const char* cmd) {
  // Max 128 characters
  array<char, 128> buffer;
  string result;
  // Execute command using popen
  unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

void Helper::setWinePrefix(string prefix_path) {
  char environ_variable[] = "WINEPREFIX=";
  strcat(environ_variable, prefix_path.c_str());
  int res = putenv(environ_variable);
  if(res != 0) {
    throw runtime_error("Can't set wine prefix environment variable");
  }
}

void Helper::removeWinePrefix() {
  int res = unsetenv("WINEPREFIX");
  if(res != 0) {
    throw runtime_error("Can't unset wine prefix environment variable");
  }
}

/**
 * \brief Read data from file and returns it.
 * \return Data from file
 */
vector<string> Helper::readFile(string file_path)
{
  vector<string> output;
  ifstream myfile(file_path);
  if(myfile.is_open())
  {
    string line;
    while(getline(myfile, line))
    {
      output.push_back(line);
    }
    myfile.close();
  } else {
    throw runtime_error("Could not open file!");
  }
  return output;
}

/**
 * \brief Split string by delimiter
 * \return Array of strings
 */
vector<string> Helper::split(const string& s, char delimiter)
{
   vector<string> tokens;
   string token;
   istringstream tokenStream(s);
   while(getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}