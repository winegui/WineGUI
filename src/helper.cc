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
  string epoch_time = readFile(prefix_path + "/.update-timestamp");
  time_t secsSinceEpoch = strtoul(epoch_time.c_str(), NULL, 0);
  stringstream stringStream;
  stringStream << put_time(localtime(&secsSinceEpoch), "%c") << endl;
  return stringStream.str();
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

/**
 * \brief Read data from file and returns it.
 * \return Data from file
 */
string Helper::readFile(string file_path)
{
  string output = "";
  ifstream myfile(file_path);
  if(myfile.is_open())
  {
    string line;
    while(getline(myfile, line))
    {
      output += line + '\n';
    }
    myfile.close();
  } else {
    throw runtime_error("Could not open file!");
  }
  return output;
}