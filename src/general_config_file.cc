/**
 * Copyright (c) 2022-2025 WineGUI
 *
 * \file    general_config_file.cc
 * \brief   WineGUI Configuration file supporting methods
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
#include "general_config_file.h"
#include <giomm.h>
#include <glibmm.h>
#include <iostream>

/// Meyers Singleton
GeneralConfigFile::GeneralConfigFile() = default;
/// Destructor
GeneralConfigFile::~GeneralConfigFile() = default;

/**
 * \brief Get singleton instance
 * \return GeneralConfigFile reference (singleton)
 */
GeneralConfigFile& GeneralConfigFile::get_instance()
{
  static GeneralConfigFile instance;
  return instance;
}

/**
 * \brief Write generic config file to disk
 * \param general_config Generic configuration data struct
 * \return true if successfully written, otherwise false
 */
bool GeneralConfigFile::write_config_file(const GeneralConfigData& general_config)
{
  bool success = false;
  Glib::KeyFile keyfile;
  // Verify: No need to check the old location, since read_config_file() should already
  // migrated the config file to the new location during start-up.
  std::vector<std::string> config_dirs{Glib::get_user_config_dir(), "winegui"};
  std::string config_location = Glib::build_path(G_DIR_SEPARATOR_S, config_dirs);
  std::string config_file_path = Glib::build_filename(config_location, "config.ini");
  try
  {
    keyfile.set_string("General", "DefaultFolder", general_config.default_folder);
    keyfile.set_boolean("General", "DisplayDefaultWineMachine", general_config.display_default_wine_machine);
    keyfile.set_boolean("General", "EnableLoggingStderr", general_config.enable_logging_stderr);
    success = keyfile.save_to_file(config_file_path);
  }
  catch (const Glib::Error& ex)
  {
    std::cerr << "Error: Exception while loading key file: " << ex.what() << std::endl;
    // will return default values
  }
  return success;
}

/**
 * \brief Read generic config file from disk
 * \return Generic Config data
 */
GeneralConfigData GeneralConfigFile::read_config_file()
{
  Glib::KeyFile keyfile;
  std::vector<std::string> config_dirs{Glib::get_user_config_dir(), "winegui"};
  std::string config_location = Glib::build_path(G_DIR_SEPARATOR_S, config_dirs);
  std::string config_file_path = Glib::build_filename(config_location, "config.ini");
  std::vector<std::string> prefix_dirs{Glib::get_user_data_dir(), "winegui", "prefixes"};
  std::string default_prefix_folder = Glib::build_path(G_DIR_SEPARATOR_S, prefix_dirs);

  // Check if config folder directory exists
  if (!Glib::file_test(config_location, Glib::FileTest::FILE_TEST_IS_DIR))
  {
    // Directory doesn't exists, make new config directory (incl. parents)
    Glib::RefPtr<Gio::File> directory = Gio::File::create_for_path(config_location);
    if (directory)
      directory->make_directory_with_parents();
  }

  // Execute migration (if required)
  // Returns the default prefix folder, depending if there are bottles found in the old prefix location
  std::string final_default_prefix_folder = config_and_folder_migration(config_file_path, default_prefix_folder);

  struct GeneralConfigData general_config;
  // Defaults config values
  general_config.default_folder = final_default_prefix_folder;
  general_config.display_default_wine_machine = true;
  general_config.enable_logging_stderr = true;

  // Check if config file exists
  if (!Glib::file_test(config_file_path, Glib::FileTest::FILE_TEST_IS_REGULAR))
  {
    // Config file doesn't exist, make a new file with default configs, return default config data below
    GeneralConfigFile::write_config_file(general_config);
  }
  else
  {
    // Config file exists
    try
    {
      keyfile.load_from_file(config_file_path);
      general_config.default_folder = keyfile.get_string("General", "DefaultFolder");
      general_config.display_default_wine_machine = keyfile.get_boolean("General", "DisplayDefaultWineMachine");
      general_config.enable_logging_stderr = keyfile.get_boolean("General", "EnableLoggingStderr");
    }
    catch (const Glib::Error& ex)
    {
      std::cerr << "Error: Exception while loading config file: " << ex.what() << std::endl;
      // Lets write a new config file and return the default values below
      GeneralConfigFile::write_config_file(general_config);
    }
  }
  return general_config;
}

/**
 * \brief Migration code from old to new location.
 * \param config_file_path_new The new config file path
 * \param default_prefix_folder_new The new prefix folder location
 * \return The final default bottle prefix folder location
 */
std::string GeneralConfigFile::config_and_folder_migration(const std::string& config_file_path_new, const std::string& default_prefix_folder_new)
{
  std::string final_default_prefix_folder = default_prefix_folder_new; // set default prefix folder first

  // For migration we search for the existing config file first,
  // if needed we move the config file to the new location (without any user impact)
  std::vector<std::string> config_dirs_old{Glib::get_home_dir(), ".winegui"};
  std::string config_location_old = Glib::build_path(G_DIR_SEPARATOR_S, config_dirs_old);
  std::string config_file_path_old = Glib::build_filename(config_location_old, "config.ini");
  // Also define old Winetricks binary file path
  std::string winetricks_file_path_old = Glib::build_filename(config_location_old, "winetricks");
  std::string winetricks_file_path_bak_old = Glib::build_filename(config_location_old, "winetricks.bak");

  // For the prefix location, we first check if the old location exists and contains any
  // bottles, if so, we keep using the old location.
  // Rationale: We can't just copy existing bottles to the new location, this might break shortcuts.
  // However, if the old location is un-used, we use the new location (which is the new default).
  std::vector<std::string> prefix_dirs_old{Glib::get_home_dir(), ".winegui", "prefixes"};
  std::string default_prefix_folder_old = Glib::build_path(G_DIR_SEPARATOR_S, prefix_dirs_old);

  // Check if old config directory (~/.winegui) still exists
  if (Glib::file_test(config_location_old, Glib::FileTest::FILE_TEST_IS_DIR))
  {
    // Check on old config.ini file
    if (Glib::file_test(config_file_path_old, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      bool has_error = false;
      try
      {
        // Move the existing config file to the new location
        Glib::RefPtr<Gio::File> file_to_move = Gio::File::create_for_path(config_file_path_old);
        if (!file_to_move)
          std::cerr << "Error: Gio::File::create_for_path() returned an empty pointer." << std::endl;
        else
          file_to_move->move(Gio::File::create_for_path(config_file_path_new));
        std::cout << "INFO: Config file is moved successfully!" << std::endl;
      }
      catch (const Gio::Error& ex)
      {
        has_error = true;
        std::cerr << "Error: Migration failed. Could not copy existing config file to new location, error: " << ex.what() << std::endl;
      }
      catch (const Glib::Error& ex)
      {
        has_error = true;
        std::cerr << "Error: Migration failed. Could not copy existing config file to new location, error: " << ex.what() << std::endl;
      }
      if (has_error)
      {
        // Maybe the config.ini file is already present in the new location?
        if (Glib::file_test(config_file_path_new, Glib::FileTest::FILE_TEST_IS_REGULAR))
        {
          // In that case, remove the old config file, without migration (maybe it was migrated in the past?)
          if (!Gio::File::create_for_path(config_file_path_old)->remove())
            std::cerr << "Warn: Could not remove the old config.ini file." << std::endl;
        }
      }
    }

    // Also remove old winetricks & winetricks.bak binaries location (if present)
    if (Glib::file_test(winetricks_file_path_old, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      // Remove winetricks binary
      if (!Gio::File::create_for_path(winetricks_file_path_old)->remove())
        std::cerr << "Warn: Could not remove old winetrick file." << std::endl;
    }
    if (Glib::file_test(winetricks_file_path_bak_old, Glib::FileTest::FILE_TEST_IS_REGULAR))
    {
      // Remove winetricks.bak binary
      if (!Gio::File::create_for_path(winetricks_file_path_bak_old)->remove())
        std::cerr << "Warn: Could not remove old winetrick.bak file." << std::endl;
    }

    // TODO: Eventually warn users about the obsolete prefixes folder. And remove/clean-up the migration code in the future.
    // For now give people some time to migrate.

    // Check if the old prefix directory is a directory / can be found & check if prefix folder is used.
    if (Glib::file_test(default_prefix_folder_old, Glib::FileTest::FILE_TEST_IS_DIR))
    {
      try
      {
        // Now check if there are any folders still present?
        Glib::RefPtr<Gio::File> old_folder = Gio::File::create_for_path(default_prefix_folder_old);
        if (!old_folder)
        {
          std::cerr << "Error: Gio::File::create_for_path() returned an empty pointer." << std::endl;
        }
        else
        {
          // Enumerate the children of the folder
          Glib::RefPtr<Gio::FileEnumerator> enumerator = old_folder->enumerate_children();
          if (!enumerator)
          {
            std::cerr << "Error: Gio::File::enumerate_children() returned an empty pointer." << std::endl;
          }
          else
          {
            // Count the folders
            int folder_count = 0;
            while (auto file_info = enumerator->next_file())
            {
              // Check if the file is a directory
              if (file_info->get_file_type() == Gio::FILE_TYPE_DIRECTORY)
                folder_count++;
            }
            if (folder_count > 0)
              final_default_prefix_folder = default_prefix_folder_old; // Keep using the old location, just to be safe
          }
        }
      }
      catch (const Gio::Error& ex)
      {
        std::cerr << "Error: Migration check failed. Unable to inspect old prefix folder, error: " << ex.what() << std::endl;
      }
      catch (const Glib::FileError& ex)
      {
        std::cerr << "Error: Migration check failed. Unable to inspect old prefix folder, error: " << ex.what() << std::endl;
      }
    }
  }
  return final_default_prefix_folder;
}