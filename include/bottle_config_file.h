/**
 * Copyright (c) 2022 WineGUI
 *
 * \file    bottle_config_file.h
 * \brief   Wine bottle config file helper class
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

struct BottleConfigData
{
  std::string name;
  std::string description;
  int debug_log_level;
};

/**
 * \class BottleConfigFile
 * \brief Bottle Config file helper methods
 */
class BottleConfigFile
{
public:
  // Singleton
  static BottleConfigFile& get_instance();

  static bool write_config_file(const std::string& prefix_path, const BottleConfigData& bottle_config);
  static BottleConfigData read_config_file(const std::string& prefix_path);

private:
  BottleConfigFile();
  ~BottleConfigFile();
  BottleConfigFile(const BottleConfigFile&) = delete;
  BottleConfigFile& operator=(const BottleConfigFile&) = delete;
};
