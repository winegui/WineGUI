/**
 * Copyright (c) 2022 WineGUI
 *
 * \file    bottle_config_file.cc
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
#include "bottle_config_file.h"
#include "helper.h"
#include <glibmm.h>
#include <iostream>

/// Meyers BottleConfigFile
BottleConfigFile::BottleConfigFile() = default;
/// Destructor
BottleConfigFile::~BottleConfigFile() = default;

/**
 * \brief Get singleton instance
 * \return BottleConfigFile reference (singleton)
 */
BottleConfigFile& BottleConfigFile::get_instance()
{
  static BottleConfigFile instance;
  return instance;
}

/**
 * \brief Write config file to disk
 * \param prefix_path Wine prefix path
 * \param bottle_config Configuration data struct
 * \return true if succesfully written, otherwise false
 */
bool BottleConfigFile::write_config_file(const std::string& prefix_path, const BottleConfigData& bottle_config)
{
  bool success = false;
  Glib::KeyFile keyfile;
  std::string file_path = Glib::build_filename(prefix_path, "winegui.ini");
  try
  {
    keyfile.set_string("General", "Name", bottle_config.name);
    keyfile.set_string("General", "Description", bottle_config.description);
    keyfile.set_boolean("Logging", "Enabled", bottle_config.logging_enabled);
    keyfile.set_integer("Logging", "DebugLevel", bottle_config.debug_log_level);
    success = keyfile.save_to_file(file_path);
  }
  catch (const Glib::Error& ex)
  {
    std::cerr << "Error: Exception while loading key file: " << ex.what() << std::endl;
    // will return default values
  }
  return success;
}

/**
 * \brief Read wine bottle config file from disk
 * \param prefix_path Wine prefix path
 * \return Wine Bottle Config data
 */
BottleConfigData BottleConfigFile::read_config_file(const std::string& prefix_path)
{
  Glib::KeyFile keyfile;
  std::string file_path = Glib::build_filename(prefix_path, "winegui.ini");

  struct BottleConfigData bottle_config;
  // Defaults config values
  bottle_config.name = Helper::get_folder_name(prefix_path); // Name from wine prefix
  bottle_config.description = "";                            // Empty description
  bottle_config.logging_enabled = false;                     // Disable logging by default
  bottle_config.debug_log_level = 1;                         // 1 (default)= Normal Wine debug logging: https://wiki.winehq.org/Debug_Channels

  // Check if config file exists
  if (!Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
  {
    // Config file doesn't exist, make a new file with default configs, retun default config data below
    BottleConfigFile::write_config_file(prefix_path, bottle_config);
  }
  else
  {
    // Config file exists
    try
    {
      keyfile.load_from_file(file_path);
      bottle_config.name = keyfile.get_string("General", "Name");
      bottle_config.description = keyfile.get_string("General", "Description");
      bottle_config.logging_enabled = keyfile.get_boolean("Logging", "Enabled");
      bottle_config.debug_log_level = keyfile.get_integer("Logging", "DebugLevel");
    }
    catch (const Glib::Error& ex)
    {
      std::cerr << "Error: Exception while loading config file: " << ex.what() << std::endl;
      // Lets write a new config file and return the default values below
      BottleConfigFile::write_config_file(prefix_path, bottle_config);
    }
  }
  return bottle_config;
}
