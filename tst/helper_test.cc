#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <glibmm/miscutils.h>
#include <giomm/init.h>
#include "helper.h"

namespace fs = std::filesystem;

class HelperTest : public ::testing::Test {
protected:
  std::string test_dir;

  static void SetUpTestSuite() {
    // Initialize Gio to prevent GLib warnings
    Gio::init();
  }

  void SetUp() override {
    test_dir = fs::temp_directory_path() / "winegui_helper_test";
    fs::create_directories(test_dir);
  }

  void TearDown() override {
    if (fs::exists(test_dir)) {
      fs::remove_all(test_dir);
    }
  }
};

// Test dir_exists function
TEST_F(HelperTest, DirExistsTrue) {
  EXPECT_TRUE(Helper::dir_exists(test_dir));
}

TEST_F(HelperTest, DirExistsFalse) {
  std::string non_existent = test_dir + "/non_existent_dir";
  EXPECT_FALSE(Helper::dir_exists(non_existent));
}

TEST_F(HelperTest, DirExistsFileNotDirectory) {
  std::string file_path = test_dir + "/test_file.txt";
  std::ofstream file(file_path);
  file << "test";
  file.close();
  
  EXPECT_FALSE(Helper::dir_exists(file_path));
}

// Test file_exists function
TEST_F(HelperTest, FileExistsTrue) {
  std::string file_path = test_dir + "/test_file.txt";
  std::ofstream file(file_path);
  file << "test content";
  file.close();
  
  EXPECT_TRUE(Helper::file_exists(file_path));
}

TEST_F(HelperTest, FileExistsFalse) {
  std::string non_existent = test_dir + "/non_existent_file.txt";
  EXPECT_FALSE(Helper::file_exists(non_existent));
}

TEST_F(HelperTest, FileExistsDirectoryNotFile) {
  EXPECT_FALSE(Helper::file_exists(test_dir));
}

// Test create_dir function
TEST_F(HelperTest, CreateDirSuccess) {
  std::string new_dir = test_dir + "/new_directory";
  EXPECT_FALSE(Helper::dir_exists(new_dir));
  
  bool result = Helper::create_dir(new_dir);
  EXPECT_TRUE(result);
  EXPECT_TRUE(Helper::dir_exists(new_dir));
}

TEST_F(HelperTest, CreateDirAlreadyExists) {
  // create_dir throws exception when directory already exists
  EXPECT_THROW(Helper::create_dir(test_dir), Glib::Error);
  EXPECT_TRUE(Helper::dir_exists(test_dir));
}

TEST_F(HelperTest, CreateDirNestedPath) {
  std::string nested_dir = test_dir + "/level1/level2/level3";
  bool result = Helper::create_dir(nested_dir);
  EXPECT_TRUE(result);
  EXPECT_TRUE(Helper::dir_exists(nested_dir));
}

// Test encode_text function
TEST_F(HelperTest, EncodeTextBasic) {
  std::string input = "Hello World";
  std::string result = Helper::encode_text(input);
  EXPECT_EQ(result, "Hello World");
}

TEST_F(HelperTest, EncodeTextWithAmpersand) {
  std::string input = "Tom & Jerry";
  std::string result = Helper::encode_text(input);
  EXPECT_EQ(result, "Tom &amp; Jerry");
}

TEST_F(HelperTest, EncodeTextWithLessThan) {
  std::string input = "5 < 10";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "5 < 10");
}

TEST_F(HelperTest, EncodeTextWithGreaterThan) {
  std::string input = "10 > 5";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "10 > 5");
}

TEST_F(HelperTest, EncodeTextWithQuotes) {
  std::string input = "He said \"Hello\"";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "He said \"Hello\"");
}

TEST_F(HelperTest, EncodeTextWithApostrophe) {
  std::string input = "It's working";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "It's working");
}

TEST_F(HelperTest, EncodeTextMultipleSpecialChars) {
  std::string input = "<tag attr=\"value\"> & 'text'";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "<tag attr=\"value\"> &amp; 'text'");
}

TEST_F(HelperTest, EncodeTextEmptyString) {
  std::string input = "";
  std::string result = Helper::encode_text(input);
  EXPECT_EQ(result, "");
}

// Test string_to_icon function
TEST_F(HelperTest, StringToIconExeFile) {
  std::string result = Helper::string_to_icon("program.exe");
  EXPECT_EQ(result, "default_app_file");
}

TEST_F(HelperTest, StringToIconMsiFile) {
  std::string result = Helper::string_to_icon("installer.msi");
  EXPECT_EQ(result, "installer_file");
}

TEST_F(HelperTest, StringToIconLnkFile) {
  std::string result = Helper::string_to_icon("shortcut.lnk");
  EXPECT_EQ(result, "link_file");
}

TEST_F(HelperTest, StringToIconTxtFile) {
  std::string result = Helper::string_to_icon("document.txt");
  EXPECT_EQ(result, "text_file");
}

TEST_F(HelperTest, StringToIconPdfFile) {
  std::string result = Helper::string_to_icon("document.pdf");
  EXPECT_EQ(result, "pdf_file");
}

TEST_F(HelperTest, StringToIconDocFile) {
  std::string result = Helper::string_to_icon("document.doc");
  EXPECT_EQ(result, "word_document");
}

TEST_F(HelperTest, StringToIconDocxFile) {
  std::string result = Helper::string_to_icon("document.docx");
  EXPECT_EQ(result, "word_document");
}

TEST_F(HelperTest, StringToIconXlsFile) {
  std::string result = Helper::string_to_icon("spreadsheet.xls");
  EXPECT_EQ(result, "excel_document");
}

TEST_F(HelperTest, StringToIconXlsxFile) {
  std::string result = Helper::string_to_icon("spreadsheet.xlsx");
  EXPECT_EQ(result, "excel_document");
}

TEST_F(HelperTest, StringToIconUnknownExtension) {
  std::string result = Helper::string_to_icon("file.unknown");
  EXPECT_EQ(result, "unknown_file");
}

TEST_F(HelperTest, StringToIconNoExtension) {
  std::string result = Helper::string_to_icon("filename");
  EXPECT_EQ(result, "unknown_file");
}

TEST_F(HelperTest, StringToIconFullPath) {
  std::string result = Helper::string_to_icon("/path/to/program.exe");
  EXPECT_EQ(result, "default_app_file");
}

TEST_F(HelperTest, StringToIconCaseInsensitive) {
  std::string result = Helper::string_to_icon("PROGRAM.EXE");
  EXPECT_EQ(result, "default_app_file");
}

// Test get_folder_name function
TEST_F(HelperTest, GetFolderNameBasic) {
  std::string prefix = "/home/user/.local/share/winegui/Prefixes/MyBottle";
  std::string result = Helper::get_folder_name(prefix);
  EXPECT_EQ(result, "MyBottle");
}

TEST_F(HelperTest, GetFolderNameWithTrailingSlash) {
  std::string prefix = "/home/user/.local/share/winegui/Prefixes/MyBottle/";
  std::string result = Helper::get_folder_name(prefix);
  // Trailing slash should be handled gracefully
  EXPECT_EQ(result, "MyBottle");
}

TEST_F(HelperTest, GetFolderNameWithMultipleTrailingSlashes) {
  std::string prefix = "/home/user/.local/share/winegui/Prefixes/MyBottle///";
  std::string result = Helper::get_folder_name(prefix);
  // Multiple trailing slashes should be handled
  EXPECT_EQ(result, "MyBottle");
}

TEST_F(HelperTest, GetFolderNameSingleLevel) {
  std::string prefix = "MyBottle";
  std::string result = Helper::get_folder_name(prefix);
  // Single level returns "- Unknown -"
  EXPECT_EQ(result, "- Unknown -");
}

// Test log_level_to_winedebug_string function
TEST_F(HelperTest, LogLevelToWineDebugLevel0) {
  std::string result = Helper::log_level_to_winedebug_string(0);
  EXPECT_EQ(result, "-all");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel1) {
  std::string result = Helper::log_level_to_winedebug_string(1);
  // Level 1 is default, returns empty string
  EXPECT_EQ(result, "");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel2) {
  std::string result = Helper::log_level_to_winedebug_string(2);
  EXPECT_EQ(result, "fixme-all");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel3) {
  std::string result = Helper::log_level_to_winedebug_string(3);
  EXPECT_EQ(result, "warn+all");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel4) {
  std::string result = Helper::log_level_to_winedebug_string(4);
  EXPECT_EQ(result, "+fps");
}

TEST_F(HelperTest, LogLevelToWineDebugInvalidLevel) {
  std::string result = Helper::log_level_to_winedebug_string(99);
  EXPECT_EQ(result, "- Unknown Log Level -");
}

// Test is_default_wine_bottle function
TEST_F(HelperTest, IsDefaultWineBottleTrue) {
  std::string home = Glib::get_home_dir();
  std::string default_wine = home + "/.wine";
  bool result = Helper::is_default_wine_bottle(default_wine);
  EXPECT_TRUE(result);
}

TEST_F(HelperTest, IsDefaultWineBottleTrueWithTrailingSlash) {
  std::string home = Glib::get_home_dir();
  std::string default_wine = home + "/.wine/";
  bool result = Helper::is_default_wine_bottle(default_wine);
  // Trailing slash causes mismatch
  EXPECT_FALSE(result);
}

TEST_F(HelperTest, IsDefaultWineBottleFalse) {
  std::string custom_bottle = "/home/user/.local/share/winegui/Prefixes/MyBottle";
  bool result = Helper::is_default_wine_bottle(custom_bottle);
  EXPECT_FALSE(result);
}

TEST_F(HelperTest, IsDefaultWineBottleFalseEmpty) {
  bool result = Helper::is_default_wine_bottle("");
  EXPECT_FALSE(result);
}

// Test get_log_file_path function
TEST_F(HelperTest, GetLogFilePathBasic) {
  std::string prefix = "/home/user/.wine";
  std::string result = Helper::get_log_file_path(prefix);
  EXPECT_EQ(result, "/home/user/.wine/winegui.log");
}

TEST_F(HelperTest, GetLogFilePathWithTrailingSlash) {
  std::string prefix = "/home/user/.wine/";
  std::string result = Helper::get_log_file_path(prefix);
  EXPECT_EQ(result, "/home/user/.wine/winegui.log");
}

TEST_F(HelperTest, GetLogFilePathEmpty) {
  std::string prefix = "";
  std::string result = Helper::get_log_file_path(prefix);
  EXPECT_EQ(result, "winegui.log");
}

// Test get_wine_executable_location function
TEST_F(HelperTest, GetWineExecutableLocation32Bit) {
  std::string result = Helper::get_wine_executable_location(false, "");
  EXPECT_EQ(result, "wine");
}

TEST_F(HelperTest, GetWineExecutableLocation64Bit) {
  std::string result = Helper::get_wine_executable_location(true, "");
  EXPECT_EQ(result, "wine64");
}

TEST_F(HelperTest, GetWineExecutableLocationWithCustomPath32Bit) {
  std::string result = Helper::get_wine_executable_location(false, "/opt/wine/bin");
  EXPECT_EQ(result, "/opt/wine/bin/wine");
}

TEST_F(HelperTest, GetWineExecutableLocationWithCustomPath64Bit) {
  std::string result = Helper::get_wine_executable_location(true, "/opt/wine/bin");
  EXPECT_EQ(result, "/opt/wine/bin/wine64");
}

TEST_F(HelperTest, GetWineExecutableLocationWithTrailingSlash) {
  std::string result = Helper::get_wine_executable_location(true, "/opt/wine/bin/");
  EXPECT_EQ(result, "/opt/wine/bin/wine64");
}

// Test get_c_letter_drive function
TEST_F(HelperTest, GetCLetterDriveSuccess) {
  // Create a mock Wine prefix structure
  std::string prefix = test_dir + "/test_prefix";
  std::string dosdevices = prefix + "/dosdevices";
  std::string c_drive = dosdevices + "/c:";
  
  fs::create_directories(c_drive);
  
  std::string result = Helper::get_c_letter_drive(prefix);
  EXPECT_EQ(result, c_drive);
}

TEST_F(HelperTest, GetCLetterDriveNonExistentPrefix) {
  std::string prefix = test_dir + "/non_existent_prefix";
  EXPECT_THROW(Helper::get_c_letter_drive(prefix), std::runtime_error);
}

TEST_F(HelperTest, GetCLetterDriveMissingDosdevices) {
  std::string prefix = test_dir + "/incomplete_prefix";
  fs::create_directories(prefix);
  
  // Don't create dosdevices/c: directory
  EXPECT_THROW(Helper::get_c_letter_drive(prefix), std::runtime_error);
}

// Test get_image_location function
TEST_F(HelperTest, GetImageLocationNotFound) {
  // Test with a filename that doesn't exist
  std::string result = Helper::get_image_location("nonexistent_image.png");
  // Should return empty string when not found
  EXPECT_EQ(result, "");
}

TEST_F(HelperTest, GetImageLocationExistingFile) {
  // Test with an actual existing image file from the project
  std::string result = Helper::get_image_location("ready.png");
  // Should find the file in ../images or ../../images relative paths
  EXPECT_FALSE(result.empty());
  EXPECT_TRUE(result.find("ready.png") != std::string::npos);
}
