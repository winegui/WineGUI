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
 * \brief Create GUI in the activate signal trigger from the GTK app
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

string exec(const char* cmd) {
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