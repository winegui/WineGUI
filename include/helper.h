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
  static std::vector<string> GetBottlesPaths(const string& dir_path);
  static string GetWineVersion();
  static string GetName(const string prefix_path);
  static BottleTypes::Windows GetWindowsOSVersion(const string prefix_path);
  static BottleTypes::Bit GetSystemBit(const string prefix_path);
  static BottleTypes::AudioDriver GetAudioDriver(const string prefix_path);
  static string GetVirtualDesktop(const string prefix_path);
  static string GetLastWineUpdated(const string prefix_path);
  static bool GetBottleStatus(const string prefix_path);
  static string GetCLetterDrive(const string prefix_path);
  static bool DirExists(const string& dir_path);
  static bool FileExists(const string& filer_path);
private:
  static string Exec(const char* cmd);
  static string GetValueByKey(const string& filename, const string& key);
  static std::vector<string> ReadFile(const string file_path);
  static std::vector<string> Split(const string& s, char delimiter);
};
