/**
 * Copyright (c) 2022-2025 WineGUI
 *
 * \file    bottle_config_file.cc
 * \brief   Wine bottle config file helper class
 * \author  Melroy van den Berg <melroy@melroy.org>
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
#include <string>

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
 * \param app_list Custom list of applications for this bottle
 * \return true if successfully written, otherwise false
 */
bool BottleConfigFile::write_config_file(const std::string& prefix_path,
                                         const BottleConfigData& bottle_config,
                                         const std::map<int, ApplicationData>& app_list)
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
    // Iterate over the key/value environment variable pairs (if present)
    for (const auto& [key, value] : bottle_config.env_vars)
    {
      keyfile.set_value("EnvironmentVariables", key, value);
    }
    // Save custom application list (if present)
    for (int i = 0; const auto& [_, app_data] : app_list)
    {
      // Instead of reusing the key, we will reindex if needed (starting from 0)
      std::string group_name = "Application." + std::to_string(i);
      keyfile.set_string(group_name, "Name", app_data.name);
      keyfile.set_string(group_name, "Description", app_data.description);
      keyfile.set_string(group_name, "Command", app_data.command);
      i++;
    }

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
 * \return Tuple of: 1. Wine Bottle Config data 2. Application list
 */
std::tuple<BottleConfigData, std::map<int, ApplicationData>> BottleConfigFile::read_config_file(const std::string& prefix_path)
{
  Glib::KeyFile keyfile;
  std::string file_path = Glib::build_filename(prefix_path, "winegui.ini");

  struct BottleConfigData bottle_config;
  std::map<int, ApplicationData> app_list; // Empty array
  /// Defaults config values ///
  // Name from wine prefix
  if (Helper::is_default_wine_bottle(prefix_path))
  {
    bottle_config.name = "Default Wine machine"; // In case of the default Wine bottle path (~/.wine)
  }
  else
  {
    bottle_config.name = Helper::get_folder_name(prefix_path); // Using the folder name as default name
  }
  bottle_config.description = "";        // Empty description
  bottle_config.logging_enabled = false; // Disable logging by default
  bottle_config.debug_log_level = 1;     // 1 (default)= Normal Wine debug logging: https://wiki.winehq.org/Debug_Channels

  // Check if config file exists
  if (!Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
  {
    // Config file doesn't exist, make a new file with default configs, return default config data below
    BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list);
  }
  else
  {
    // Config file exists
    try
    {
      keyfile.load_from_file(file_path);
      // Retrieve bottle config
      bottle_config.name = keyfile.get_string("General", "Name");
      bottle_config.description = keyfile.get_string("General", "Description");
      bottle_config.logging_enabled = keyfile.get_boolean("Logging", "Enabled");
      bottle_config.debug_log_level = keyfile.get_integer("Logging", "DebugLevel");

      // Retrieve environment variables (if present)
      if (keyfile.has_group("EnvironmentVariables"))
      {
        auto keys = keyfile.get_keys("EnvironmentVariables");
        for (Glib::ustring key : keys)
        {
          bottle_config.env_vars.emplace_back(std::pair<std::string, std::string>(key, keyfile.get_string("EnvironmentVariables", key)));
        }
      }

      // Retrieve custom application list (if present)
      auto groups = keyfile.get_groups();
      for (int i = 0; Glib::ustring group : groups)
      {
        if (std::string(group).starts_with("Application"))
        {
          app_list.insert(std::pair<int, ApplicationData>(
              i, {keyfile.get_string(group, "Name"), keyfile.get_string(group, "Description"), keyfile.get_string(group, "Command")}));
          i++;
        }
      }
    }
    catch (const Glib::Error& ex)
    {
      std::cerr << "Error: Exception while loading config file: " << ex.what() << std::endl;
      // Lets write a new config file and return the default values below
      BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list);
    }
  }

  return std::make_tuple(bottle_config, app_list);
}
