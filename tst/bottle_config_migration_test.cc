#include <gtest/gtest.h>
#include <glibmm.h>
#include <filesystem>
#include <fstream>
#include "bottle_config_file.h"

namespace fs = std::filesystem;

class BottleConfigMigrationTest : public ::testing::Test {
protected:
  std::string test_dir;
  std::string config_file_path;

  void SetUp() override {
    test_dir = fs::temp_directory_path() / "winegui_test_migration";
    fs::create_directories(test_dir);
    config_file_path = test_dir + "/winegui.ini";
  }

  void TearDown() override {
    if (fs::exists(test_dir)) {
      fs::remove_all(test_dir);
    }
  }

  void CreateLegacyConfigFile() {
    std::ofstream config_file(config_file_path);
    config_file << "[General]\n";
    config_file << "Name=Test Bottle\n";
    config_file << "Description=Legacy config without version\n";
    config_file << "\n";
    config_file << "[Logging]\n";
    config_file << "Enabled=false\n";
    config_file << "DebugLevel=1\n";
    config_file << "\n";
    config_file << "[EnvironmentVariables]\n";
    config_file << "WINEPREFIX=/tmp/test\n";
    config_file << "WINEDEBUG=-all\n";
    config_file.close();
  }

  void CreateVersion2ConfigFile() {
    std::ofstream config_file(config_file_path);
    config_file << "[General]\n";
    config_file << "ConfigVersion=2\n";
    config_file << "Name=Test Bottle V2\n";
    config_file << "Description=Config with version 2\n";
    config_file << "\n";
    config_file << "[Wine]\n";
    config_file << "BinaryPath=/opt/wine/bin/wine\n";
    config_file << "\n";
    config_file << "[Logging]\n";
    config_file << "Enabled=true\n";
    config_file << "DebugLevel=2\n";
    config_file.close();
  }

  void CreateVersion3ConfigFile() {
    std::ofstream config_file(config_file_path);
    config_file << "[General]\n";
    config_file << "ConfigVersion=3\n";
    config_file << "Name=Test Bottle V3\n";
    config_file << "Description=Config with version 3\n";
    config_file << "\n";
    config_file << "[Wine]\n";
    config_file << "BinaryPath=/opt/wine/bin\n";
    config_file << "UseWine64=true\n";
    config_file << "\n";
    config_file << "[Logging]\n";
    config_file << "Enabled=false\n";
    config_file << "DebugLevel=1\n";
    config_file.close();
  }
};

TEST_F(BottleConfigMigrationTest, MigrateLegacyConfigToCurrentVersion) {
  CreateLegacyConfigFile();

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_EQ(config.name, "Test Bottle");
  EXPECT_EQ(config.description, "Legacy config without version");
  EXPECT_EQ(config.wine_bin_path, "");
  EXPECT_FALSE(config.use_wine64);
  EXPECT_FALSE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 1);
  EXPECT_EQ(config.config_version, 3);
  EXPECT_EQ(config.env_vars.size(), 2);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);

  EXPECT_TRUE(keyfile->has_key("General", "ConfigVersion"));
  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 3);
  EXPECT_TRUE(keyfile->has_key("Wine", "BinaryPath"));
  EXPECT_EQ(keyfile->get_string("Wine", "BinaryPath"), "");
  EXPECT_TRUE(keyfile->has_key("Wine", "UseWine64"));
  EXPECT_FALSE(keyfile->get_boolean("Wine", "UseWine64"));
}

TEST_F(BottleConfigMigrationTest, MigrateVersion2ToVersion3) {
  // A version 2 config (no UseWine64 key) is migrated to version 3 with UseWine64 defaulted to false
  CreateVersion2ConfigFile();

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_EQ(config.name, "Test Bottle V2");
  EXPECT_EQ(config.description, "Config with version 2");
  EXPECT_EQ(config.wine_bin_path, "/opt/wine/bin/wine");
  EXPECT_FALSE(config.use_wine64);
  EXPECT_TRUE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 2);
  EXPECT_EQ(config.config_version, 3);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);
  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 3);
  EXPECT_TRUE(keyfile->has_key("Wine", "UseWine64"));
  EXPECT_FALSE(keyfile->get_boolean("Wine", "UseWine64"));
}

TEST_F(BottleConfigMigrationTest, NoMigrationNeededForVersion3) {
  // A current-version (v3) config is read back unchanged, preserving use_wine64
  CreateVersion3ConfigFile();

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_EQ(config.name, "Test Bottle V3");
  EXPECT_EQ(config.wine_bin_path, "/opt/wine/bin");
  EXPECT_TRUE(config.use_wine64);
  EXPECT_FALSE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 1);
  EXPECT_EQ(config.config_version, 3);
}

TEST_F(BottleConfigMigrationTest, CreateNewConfigFileWhenMissing) {
  ASSERT_FALSE(fs::exists(config_file_path));

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_TRUE(fs::exists(config_file_path));
  EXPECT_EQ(config.config_version, 3);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);

  EXPECT_TRUE(keyfile->has_key("General", "ConfigVersion"));
  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 3);
  EXPECT_TRUE(keyfile->has_key("Wine", "BinaryPath"));
  EXPECT_TRUE(keyfile->has_key("Wine", "UseWine64"));
}

TEST_F(BottleConfigMigrationTest, PreserveEnvironmentVariablesDuringMigration) {
  CreateLegacyConfigFile();

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  ASSERT_EQ(config.env_vars.size(), 2);
  
  bool found_wineprefix = false;
  bool found_winedebug = false;
  
  for (const auto& [key, value] : config.env_vars) {
    if (key == "WINEPREFIX" && value == "/tmp/test") {
      found_wineprefix = true;
    }
    if (key == "WINEDEBUG" && value == "-all") {
      found_winedebug = true;
    }
  }
  
  EXPECT_TRUE(found_wineprefix);
  EXPECT_TRUE(found_winedebug);
}

TEST_F(BottleConfigMigrationTest, DefaultConfigValues) {
  BottleConfigData config = BottleConfigFile::get_default_config(test_dir);

  EXPECT_FALSE(config.name.empty());
  EXPECT_EQ(config.description, "");
  EXPECT_EQ(config.wine_bin_path, "");
  EXPECT_FALSE(config.use_wine64);
  EXPECT_FALSE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 1);
  EXPECT_EQ(config.config_version, 3);
  EXPECT_TRUE(config.env_vars.empty());
}

TEST_F(BottleConfigMigrationTest, WriteConfigIncludesVersion) {
  BottleConfigData config;
  config.name = "Test Write";
  config.description = "Test Description";
  config.wine_bin_path = "/usr/bin/wine";
  config.use_wine64 = true;
  config.logging_enabled = true;
  config.debug_log_level = 3;
  config.config_version = 3;

  std::map<int, ApplicationData> app_list;

  bool success = BottleConfigFile::write_config_file(test_dir, config, app_list);
  EXPECT_TRUE(success);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);

  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 3);
  EXPECT_EQ(keyfile->get_string("General", "Name"), "Test Write");
  EXPECT_EQ(keyfile->get_string("Wine", "BinaryPath"), "/usr/bin/wine");
  EXPECT_TRUE(keyfile->get_boolean("Wine", "UseWine64"));
  EXPECT_TRUE(keyfile->get_boolean("Logging", "Enabled"));
  EXPECT_EQ(keyfile->get_integer("Logging", "DebugLevel"), 3);
}
