/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    helper.h
 * \brief   Helper class for Bottle Manager and CLI
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
#pragma once

#include <string>
#include <vector>
#include "bottle_types.h"

using std::string;

/**
 * \class Helper
 * \brief Provide some helper methods for Bottle Manager and CLI
 */
class Helper
{
public:
  static string retrieveName(const string prefix_path);
  static string retrieveWindowsOSVersion(const string prefix_path);
  static BottleTypes::Bit retrieveSystemBit(const string prefix_path);
  static BottleTypes::AudioDriver retrieveAudioDriver(const string prefix_path);
  static string retrieveVirtualDesktop(const string prefix_path);
  static string retrieveLastWineUpdate(const string prefix_path);
  static bool retrieveWineStatus(const string prefix_path);
  static string retrieveCLetterDrive(const string prefix_path);
  static bool exists(const string& prefix_path);
  static std::vector<string> retrieveBottles(const string& prefix_path);
  static string retrieveWineVersion();
private:
  static string exec(const char* cmd);
  static void setWinePrefix(const string prefix_path);
  static void removeWinePrefix();
  static std::vector<string> readFile(const string file_path);
  static std::vector<string> split(const string& s, char delimiter);
};
