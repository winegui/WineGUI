/**
 * Copyright (c) 2022 WineGUI
 *
 * \file    config_file.cc
 * \brief   WineGUI Configuration file supporting methods
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
#include "config_file.h"
#include <iostream>

/// Meyers Singleton
ConfigFile::ConfigFile() = default;
/// Destructor
ConfigFile::~ConfigFile() = default;

/**
 * \brief Get singleton instance
 * \return ConfigFile reference (singleton)
 */
ConfigFile& ConfigFile::get_instance()
{
  static ConfigFile instance;
  return instance;
}

/**
 * \brief Write config file to disk
 * \param config_data Configuration data struct
 * \return true if succesfully written, otherwise false
 */
bool ConfigFile::write_config_file(const ConfigData& config_data)
{
  bool success = false;
  Glib::KeyFile keyfile;
  std::vector<std::string> config_dirs{Glib::get_home_dir(), ".winegui"};
  std::string config_location = Glib::build_path(G_DIR_SEPARATOR_S, config_dirs);
  std::string file_path = Glib::build_filename(config_location, "config.ini");
  try
  {
    keyfile.set_string("General", "DefaultFolder", config_data.default_folder);
    keyfile.set_boolean("General", "PreferWine64", config_data.prefer_wine64);
    keyfile.set_boolean("General", "EnableDebugLogging", config_data.enable_debug_logging);
    keyfile.set_boolean("General", "EnableLoggingStderr", config_data.enable_logging_stderr);
    success = keyfile.save_to_file(file_path);
  }
  catch (const Glib::Error& ex)
  {
    std::cerr << "Error: Exception while loading key file: " << ex.what() << std::endl;
    // will return default values
  }
  return success;
}

ConfigData ConfigFile::read_config_file()
{
  Glib::KeyFile keyfile;
  std::vector<std::string> config_dirs{Glib::get_home_dir(), ".winegui"};
  std::string config_location = Glib::build_path(G_DIR_SEPARATOR_S, config_dirs);
  std::string file_path = Glib::build_filename(config_location, "config.ini");

  std::vector<std::string> prefix_dirs{Glib::get_home_dir(), ".winegui", "prefixes"};
  std::string default_prefix_folder = Glib::build_path(G_DIR_SEPARATOR_S, prefix_dirs);

  struct ConfigData config;
  // Defaults config values
  config.default_folder = default_prefix_folder;
  config.prefer_wine64 = false;
  config.enable_debug_logging = false;
  config.enable_logging_stderr = true;

  // Check if config file exists
  if (!Glib::file_test(file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
  {
    // Config file doesn't exist, make a new file with default configs, retun default config data below
    ConfigFile::write_config_file(config);
  }
  else
  {
    // Config file exists
    try
    {
      keyfile.load_from_file(file_path);
      config.default_folder = keyfile.get_string("General", "DefaultFolder");
      config.prefer_wine64 = keyfile.get_boolean("General", "PreferWine64");
      config.enable_debug_logging = keyfile.get_boolean("General", "EnableDebugLogging");
      config.enable_logging_stderr = keyfile.get_boolean("General", "EnableLoggingStderr");
    }
    catch (const Glib::Error& ex)
    {
      std::cerr << "Error: Exception while loading config file: " << ex.what() << std::endl;
      // Lets write a new config file and return the default values below
      ConfigFile::write_config_file(config);
    }
  }
  return config;
}
