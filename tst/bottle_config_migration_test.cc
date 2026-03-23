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
};

TEST_F(BottleConfigMigrationTest, MigrateLegacyConfigToVersion2) {
  CreateLegacyConfigFile();

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_EQ(config.name, "Test Bottle");
  EXPECT_EQ(config.description, "Legacy config without version");
  EXPECT_EQ(config.wine_bin_path, "");
  EXPECT_FALSE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 1);
  EXPECT_EQ(config.config_version, 2);
  EXPECT_EQ(config.env_vars.size(), 2);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);

  EXPECT_TRUE(keyfile->has_key("General", "ConfigVersion"));
  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 2);
  EXPECT_TRUE(keyfile->has_key("Wine", "BinaryPath"));
  EXPECT_EQ(keyfile->get_string("Wine", "BinaryPath"), "");
}

TEST_F(BottleConfigMigrationTest, NoMigrationNeededForVersion2) {
  CreateVersion2ConfigFile();

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_EQ(config.name, "Test Bottle V2");
  EXPECT_EQ(config.description, "Config with version 2");
  EXPECT_EQ(config.wine_bin_path, "/opt/wine/bin/wine");
  EXPECT_TRUE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 2);
  EXPECT_EQ(config.config_version, 2);
}

TEST_F(BottleConfigMigrationTest, CreateNewConfigFileWhenMissing) {
  ASSERT_FALSE(fs::exists(config_file_path));

  BottleConfigData config;
  std::map<int, ApplicationData> app_list;
  std::tie(config, app_list) = BottleConfigFile::read_config_file(test_dir);

  EXPECT_TRUE(fs::exists(config_file_path));
  EXPECT_EQ(config.config_version, 2);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);

  EXPECT_TRUE(keyfile->has_key("General", "ConfigVersion"));
  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 2);
  EXPECT_TRUE(keyfile->has_key("Wine", "BinaryPath"));
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
  EXPECT_FALSE(config.logging_enabled);
  EXPECT_EQ(config.debug_log_level, 1);
  EXPECT_EQ(config.config_version, 2);
  EXPECT_TRUE(config.env_vars.empty());
}

TEST_F(BottleConfigMigrationTest, WriteConfigIncludesVersion) {
  BottleConfigData config;
  config.name = "Test Write";
  config.description = "Test Description";
  config.wine_bin_path = "/usr/bin/wine";
  config.logging_enabled = true;
  config.debug_log_level = 3;
  config.config_version = 2;

  std::map<int, ApplicationData> app_list;

  bool success = BottleConfigFile::write_config_file(test_dir, config, app_list);
  EXPECT_TRUE(success);

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(config_file_path);

  EXPECT_EQ(keyfile->get_integer("General", "ConfigVersion"), 2);
  EXPECT_EQ(keyfile->get_string("General", "Name"), "Test Write");
  EXPECT_EQ(keyfile->get_string("Wine", "BinaryPath"), "/usr/bin/wine");
  EXPECT_TRUE(keyfile->get_boolean("Logging", "Enabled"));
  EXPECT_EQ(keyfile->get_integer("Logging", "DebugLevel"), 3);
}
