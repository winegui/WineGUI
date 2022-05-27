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
  // Defaults values
  config.default_folder = default_prefix_folder;
  config.prefer_wine64 = false;
  config.enable_debug_logging = false;

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
    }
    catch (const Glib::Error& ex)
    {
      std::cerr << "Error: Exception while loading config file: " << ex.what() << std::endl;
      // will return default values
    }
  }
  return config;
}
