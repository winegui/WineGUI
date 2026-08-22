#include "helper.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <giomm/init.h>
#include <glibmm/miscutils.h>
#include <gtest/gtest.h>
#include <thread>

namespace fs = std::filesystem;

class ScopedPath
{
public:
  explicit ScopedPath(const std::string& value)
  {
    const char* current = std::getenv("PATH");
    had_original_ = current != nullptr;
    if (had_original_)
      original_ = current;
    if (setenv("PATH", value.c_str(), 1) != 0)
      throw std::runtime_error("Could not set PATH for test");
  }

  ~ScopedPath()
  {
    if (had_original_)
      setenv("PATH", original_.c_str(), 1);
    else
      unsetenv("PATH");
  }

private:
  bool had_original_ = false;
  std::string original_;
};

class HelperTest : public ::testing::Test
{
protected:
  std::string test_dir;

  static void SetUpTestSuite()
  {
    // Initialize Gio to prevent GLib warnings
    Gio::init();
  }

  void SetUp() override
  {
    test_dir = fs::temp_directory_path() / "winegui_helper_test";
    fs::create_directories(test_dir);
    fs::remove_all(fs::path(Helper::get_umu_executable_location()).parent_path());
  }

  void TearDown() override
  {
    if (fs::exists(test_dir))
    {
      fs::remove_all(test_dir);
    }
    fs::remove_all(fs::path(Helper::get_umu_executable_location()).parent_path());
  }
};

// Test dir_exists function
TEST_F(HelperTest, DirExistsTrue)
{
  EXPECT_TRUE(Helper::dir_exists(test_dir));
}

TEST_F(HelperTest, DirExistsFalse)
{
  std::string non_existent = test_dir + "/non_existent_dir";
  EXPECT_FALSE(Helper::dir_exists(non_existent));
}

TEST_F(HelperTest, DirExistsFileNotDirectory)
{
  std::string file_path = test_dir + "/test_file.txt";
  std::ofstream file(file_path);
  file << "test";
  file.close();

  EXPECT_FALSE(Helper::dir_exists(file_path));
}

// Test file_exists function
TEST_F(HelperTest, FileExistsTrue)
{
  std::string file_path = test_dir + "/test_file.txt";
  std::ofstream file(file_path);
  file << "test content";
  file.close();

  EXPECT_TRUE(Helper::file_exists(file_path));
}

TEST_F(HelperTest, FileExistsFalse)
{
  std::string non_existent = test_dir + "/non_existent_file.txt";
  EXPECT_FALSE(Helper::file_exists(non_existent));
}

TEST_F(HelperTest, FileExistsDirectoryNotFile)
{
  EXPECT_FALSE(Helper::file_exists(test_dir));
}

// Test create_dir function
TEST_F(HelperTest, CreateDirSuccess)
{
  std::string new_dir = test_dir + "/new_directory";
  EXPECT_FALSE(Helper::dir_exists(new_dir));

  bool result = Helper::create_dir(new_dir);
  EXPECT_TRUE(result);
  EXPECT_TRUE(Helper::dir_exists(new_dir));
}

TEST_F(HelperTest, CreateDirAlreadyExists)
{
  // create_dir throws exception when directory already exists
  EXPECT_THROW(Helper::create_dir(test_dir), Glib::Error);
  EXPECT_TRUE(Helper::dir_exists(test_dir));
}

TEST_F(HelperTest, CreateDirNestedPath)
{
  std::string nested_dir = test_dir + "/level1/level2/level3";
  bool result = Helper::create_dir(nested_dir);
  EXPECT_TRUE(result);
  EXPECT_TRUE(Helper::dir_exists(nested_dir));
}

// Test encode_text function
TEST_F(HelperTest, EncodeTextBasic)
{
  std::string input = "Hello World";
  std::string result = Helper::encode_text(input);
  EXPECT_EQ(result, "Hello World");
}

TEST_F(HelperTest, EncodeTextWithAmpersand)
{
  std::string input = "Tom & Jerry";
  std::string result = Helper::encode_text(input);
  EXPECT_EQ(result, "Tom &amp; Jerry");
}

TEST_F(HelperTest, EncodeTextWithLessThan)
{
  std::string input = "5 < 10";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "5 < 10");
}

TEST_F(HelperTest, EncodeTextWithGreaterThan)
{
  std::string input = "10 > 5";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "10 > 5");
}

TEST_F(HelperTest, EncodeTextWithQuotes)
{
  std::string input = "He said \"Hello\"";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "He said \"Hello\"");
}

TEST_F(HelperTest, EncodeTextWithApostrophe)
{
  std::string input = "It's working";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "It's working");
}

TEST_F(HelperTest, EncodeTextMultipleSpecialChars)
{
  std::string input = "<tag attr=\"value\"> & 'text'";
  std::string result = Helper::encode_text(input);
  // encode_text only encodes ampersand
  EXPECT_EQ(result, "<tag attr=\"value\"> &amp; 'text'");
}

TEST_F(HelperTest, EncodeTextEmptyString)
{
  std::string input = "";
  std::string result = Helper::encode_text(input);
  EXPECT_EQ(result, "");
}

// Test string_to_icon function
TEST_F(HelperTest, StringToIconExeFile)
{
  std::string result = Helper::string_to_icon("program.exe");
  EXPECT_EQ(result, "default_app_file");
}

TEST_F(HelperTest, StringToIconMsiFile)
{
  std::string result = Helper::string_to_icon("installer.msi");
  EXPECT_EQ(result, "installer_file");
}

TEST_F(HelperTest, StringToIconLnkFile)
{
  std::string result = Helper::string_to_icon("shortcut.lnk");
  EXPECT_EQ(result, "link_file");
}

TEST_F(HelperTest, StringToIconTxtFile)
{
  std::string result = Helper::string_to_icon("document.txt");
  EXPECT_EQ(result, "text_file");
}

TEST_F(HelperTest, StringToIconPdfFile)
{
  std::string result = Helper::string_to_icon("document.pdf");
  EXPECT_EQ(result, "pdf_file");
}

TEST_F(HelperTest, StringToIconDocFile)
{
  std::string result = Helper::string_to_icon("document.doc");
  EXPECT_EQ(result, "word_document");
}

TEST_F(HelperTest, StringToIconDocxFile)
{
  std::string result = Helper::string_to_icon("document.docx");
  EXPECT_EQ(result, "word_document");
}

TEST_F(HelperTest, StringToIconXlsFile)
{
  std::string result = Helper::string_to_icon("spreadsheet.xls");
  EXPECT_EQ(result, "excel_document");
}

TEST_F(HelperTest, StringToIconXlsxFile)
{
  std::string result = Helper::string_to_icon("spreadsheet.xlsx");
  EXPECT_EQ(result, "excel_document");
}

TEST_F(HelperTest, StringToIconUnknownExtension)
{
  std::string result = Helper::string_to_icon("file.unknown");
  EXPECT_EQ(result, "unknown_file");
}

TEST_F(HelperTest, StringToIconNoExtension)
{
  std::string result = Helper::string_to_icon("filename");
  EXPECT_EQ(result, "unknown_file");
}

TEST_F(HelperTest, StringToIconFullPath)
{
  std::string result = Helper::string_to_icon("/path/to/program.exe");
  EXPECT_EQ(result, "default_app_file");
}

TEST_F(HelperTest, StringToIconCaseInsensitive)
{
  std::string result = Helper::string_to_icon("PROGRAM.EXE");
  EXPECT_EQ(result, "default_app_file");
}

// Test get_folder_name function
TEST_F(HelperTest, GetFolderNameBasic)
{
  std::string prefix = "/home/user/.local/share/winegui/Prefixes/MyBottle";
  std::string result = Helper::get_folder_name(prefix);
  EXPECT_EQ(result, "MyBottle");
}

TEST_F(HelperTest, GetFolderNameWithTrailingSlash)
{
  std::string prefix = "/home/user/.local/share/winegui/Prefixes/MyBottle/";
  std::string result = Helper::get_folder_name(prefix);
  // Trailing slash should be handled gracefully
  EXPECT_EQ(result, "MyBottle");
}

TEST_F(HelperTest, GetFolderNameWithMultipleTrailingSlashes)
{
  std::string prefix = "/home/user/.local/share/winegui/Prefixes/MyBottle///";
  std::string result = Helper::get_folder_name(prefix);
  // Multiple trailing slashes should be handled
  EXPECT_EQ(result, "MyBottle");
}

TEST_F(HelperTest, GetFolderNameSingleLevel)
{
  std::string prefix = "MyBottle";
  std::string result = Helper::get_folder_name(prefix);
  // Single level returns "- Unknown -"
  EXPECT_EQ(result, "- Unknown -");
}

// Test log_level_to_winedebug_string function
TEST_F(HelperTest, LogLevelToWineDebugLevel0)
{
  std::string result = Helper::log_level_to_winedebug_string(0);
  EXPECT_EQ(result, "-all");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel1)
{
  std::string result = Helper::log_level_to_winedebug_string(1);
  // Level 1 is default, returns empty string
  EXPECT_EQ(result, "");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel2)
{
  std::string result = Helper::log_level_to_winedebug_string(2);
  EXPECT_EQ(result, "fixme-all");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel3)
{
  std::string result = Helper::log_level_to_winedebug_string(3);
  EXPECT_EQ(result, "warn+all");
}

TEST_F(HelperTest, LogLevelToWineDebugLevel4)
{
  std::string result = Helper::log_level_to_winedebug_string(4);
  EXPECT_EQ(result, "+fps");
}

TEST_F(HelperTest, LogLevelToWineDebugInvalidLevel)
{
  std::string result = Helper::log_level_to_winedebug_string(99);
  EXPECT_EQ(result, "- Unknown Log Level -");
}

// Test is_default_wine_bottle function
TEST_F(HelperTest, IsDefaultWineBottleTrue)
{
  std::string home = Glib::get_home_dir();
  std::string default_wine = home + "/.wine";
  bool result = Helper::is_default_wine_bottle(default_wine);
  EXPECT_TRUE(result);
}

TEST_F(HelperTest, IsDefaultWineBottleTrueWithTrailingSlash)
{
  std::string home = Glib::get_home_dir();
  std::string default_wine = home + "/.wine/";
  bool result = Helper::is_default_wine_bottle(default_wine);
  // Trailing slash causes mismatch
  EXPECT_FALSE(result);
}

TEST_F(HelperTest, IsDefaultWineBottleFalse)
{
  std::string custom_bottle = "/home/user/.local/share/winegui/Prefixes/MyBottle";
  bool result = Helper::is_default_wine_bottle(custom_bottle);
  EXPECT_FALSE(result);
}

TEST_F(HelperTest, IsDefaultWineBottleFalseEmpty)
{
  bool result = Helper::is_default_wine_bottle("");
  EXPECT_FALSE(result);
}

// Test get_log_file_path function
TEST_F(HelperTest, GetLogFilePathBasic)
{
  std::string prefix = "/home/user/.wine";
  std::string result = Helper::get_log_file_path(prefix);
  EXPECT_EQ(result, "/home/user/.wine/winegui.log");
}

TEST_F(HelperTest, GetLogFilePathWithTrailingSlash)
{
  std::string prefix = "/home/user/.wine/";
  std::string result = Helper::get_log_file_path(prefix);
  EXPECT_EQ(result, "/home/user/.wine/winegui.log");
}

TEST_F(HelperTest, GetLogFilePathEmpty)
{
  std::string prefix = "";
  std::string result = Helper::get_log_file_path(prefix);
  EXPECT_EQ(result, "winegui.log");
}

// Test get_wine_executable_location function
TEST_F(HelperTest, GetWineExecutableLocationSystemDefaultsToWine)
{
  // System Wine defaults to the plain "wine" binary (it runs both 32-bit and 64-bit prefixes)
  std::string result = Helper::get_wine_executable_location(false, "");
  EXPECT_EQ(result, "wine");
  // And with no arguments at all
  EXPECT_EQ(Helper::get_wine_executable_location(), "wine");
}

TEST_F(HelperTest, GetWineExecutableLocationSystemOptInWine64)
{
  // With the opt-in set, system Wine returns "wine64" when a wine64 binary is on PATH, otherwise it
  // gracefully falls back to "wine". Both are valid depending on the test environment.
  std::string result = Helper::get_wine_executable_location(true, "");
  EXPECT_TRUE(result == "wine64" || result == "wine") << "unexpected: " << result;
}

TEST_F(HelperTest, GetWineExecutableLocationRunnerDefaultsToWine)
{
  // A runner with only a unified "wine" binary uses it for both the default and the wine64 opt-in
  // (the opt-in gracefully falls back to wine when no wine64 binary is present)
  std::string bin_dir = test_dir + "/runner/bin";
  fs::create_directories(bin_dir);
  std::ofstream wine_file(bin_dir + "/wine");
  wine_file << "fake";
  wine_file.close();

  EXPECT_EQ(Helper::get_wine_executable_location(false, bin_dir), bin_dir + "/wine");
  EXPECT_EQ(Helper::get_wine_executable_location(true, bin_dir), bin_dir + "/wine");
}

TEST_F(HelperTest, GetWineExecutableLocationWithTrailingSlash)
{
  std::string bin_dir = test_dir + "/runner-slash/bin";
  fs::create_directories(bin_dir);
  std::ofstream wine_file(bin_dir + "/wine");
  wine_file << "fake";
  wine_file.close();

  std::string result = Helper::get_wine_executable_location(true, bin_dir + "/");
  EXPECT_EQ(result, bin_dir + "/wine");
}

TEST_F(HelperTest, GetWineExecutableLocationRunnerFallsBackToWine64WhenNoWine)
{
  // Only when a runner ships no unified "wine" binary do we fall back to "wine64"
  std::string bin_dir = test_dir + "/wine64-only/bin";
  fs::create_directories(bin_dir);
  std::ofstream wine64_file(bin_dir + "/wine64");
  wine64_file << "fake";
  wine64_file.close();

  EXPECT_EQ(Helper::get_wine_executable_location(true, bin_dir), bin_dir + "/wine64");
  EXPECT_EQ(Helper::get_wine_executable_location(false, bin_dir), bin_dir + "/wine64");
}

TEST_F(HelperTest, GetWineExecutableLocationRunnerHonorsWine64OptInWhenPresent)
{
  // When a runner ships both binaries, the default uses "wine" (32-bit safe) but the wine64 opt-in
  // honors the user's choice and returns "wine64"
  std::string bin_dir = test_dir + "/classic-build/bin";
  fs::create_directories(bin_dir);
  std::ofstream wine_file(bin_dir + "/wine");
  wine_file << "fake";
  wine_file.close();
  std::ofstream wine64_file(bin_dir + "/wine64");
  wine64_file << "fake";
  wine64_file.close();

  EXPECT_EQ(Helper::get_wine_executable_location(false, bin_dir), bin_dir + "/wine");
  EXPECT_EQ(Helper::get_wine_executable_location(true, bin_dir), bin_dir + "/wine64");
}

// Test get_wineserver_executable_location function
TEST_F(HelperTest, GetWineserverExecutableLocationDefault)
{
  EXPECT_EQ(Helper::get_wineserver_executable_location(""), "wineserver");
}

TEST_F(HelperTest, GetWineserverExecutableLocationCustomPath)
{
  std::string bin_dir = test_dir + "/runner/bin";
  fs::create_directories(bin_dir);
  std::ofstream wineserver_file(bin_dir + "/wineserver");
  wineserver_file << "fake";
  wineserver_file.close();

  EXPECT_EQ(Helper::get_wineserver_executable_location(bin_dir), bin_dir + "/wineserver");
}

TEST_F(HelperTest, GetWineserverExecutableLocationCustomPathWithoutWineserver)
{
  // Fall back to the global wineserver when the custom directory has none
  std::string bin_dir = test_dir + "/runner-no-server/bin";
  fs::create_directories(bin_dir);
  EXPECT_EQ(Helper::get_wineserver_executable_location(bin_dir), "wineserver");
}

// Test get_c_letter_drive function
TEST_F(HelperTest, GetCLetterDriveSuccess)
{
  // Create a mock Wine prefix structure
  std::string prefix = test_dir + "/test_prefix";
  std::string dosdevices = prefix + "/dosdevices";
  std::string c_drive = dosdevices + "/c:";

  fs::create_directories(c_drive);

  std::string result = Helper::get_c_letter_drive(prefix);
  EXPECT_EQ(result, c_drive);
}

TEST_F(HelperTest, GetCLetterDriveNonExistentPrefix)
{
  std::string prefix = test_dir + "/non_existent_prefix";
  EXPECT_THROW(Helper::get_c_letter_drive(prefix), std::runtime_error);
}

TEST_F(HelperTest, GetCLetterDriveMissingDosdevices)
{
  std::string prefix = test_dir + "/incomplete_prefix";
  fs::create_directories(prefix);

  // Don't create dosdevices/c: directory
  EXPECT_THROW(Helper::get_c_letter_drive(prefix), std::runtime_error);
}

// Test get_image_location function
TEST_F(HelperTest, GetImageLocationNotFound)
{
  // Test with a filename that doesn't exist
  std::string result = Helper::get_image_location("nonexistent_image.png");
  // Should return empty string when not found
  EXPECT_EQ(result, "");
}

TEST_F(HelperTest, GetImageLocationExistingFile)
{
  // Test with an actual existing image file from the project
  std::string result = Helper::get_image_location("ready.png");
  // Should find the file in ../images or ../../images relative paths
  EXPECT_FALSE(result.empty());
  EXPECT_TRUE(result.find("ready.png") != std::string::npos);
}

// Test build_desktop_exec_line function

TEST_F(HelperTest, BuildDesktopExecLineUnixPath)
{
  // A Unix-style path should be wrapped with 'start /unix' and include the WINEPREFIX
  std::string result = Helper::build_desktop_exec_line(false, "/home/user/.wine", "", "/home/user/.wine/drive_c/game.exe");
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" wine start /unix \"/home/user/.wine/drive_c/game.exe\"");
}

TEST_F(HelperTest, BuildDesktopExecLineWindowsCommand)
{
  // A Windows-style command (like 'notepad') should be wrapped with 'start'
  std::string result = Helper::build_desktop_exec_line(false, "/home/user/.wine", "", "notepad");
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" wine start \"notepad\"");
}

TEST_F(HelperTest, BuildDesktopExecLineWithEnvVars)
{
  // Environment variables should be added to the env prefix
  std::vector<std::pair<std::string, std::string>> env_vars = {{"DXVK_HUD", "fps"}};
  std::string result = Helper::build_desktop_exec_line(false, "/home/user/.wine", "", "notepad", env_vars);
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" DXVK_HUD=\"fps\" wine start \"notepad\"");
}

TEST_F(HelperTest, BuildDesktopExecLineCustomWineBinPath)
{
  // A custom Wine binary path should be used instead of the global 'wine64'
  std::string result = Helper::build_desktop_exec_line(true, "/home/user/.wine", "/opt/wine/bin", "notepad");
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" /opt/wine/bin/wine64 start \"notepad\"");
}

TEST_F(HelperTest, BuildDesktopExecLineWinetricks)
{
  // Winetricks is a special case and does not run through the Wine binary
  std::string result = Helper::build_desktop_exec_line(true, "/home/user/.wine", "", "/some/path/winetricks --gui -q");
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" /some/path/winetricks --gui -q");
}

TEST_F(HelperTest, BuildDesktopExecLineRunnerHonorsWine64OptIn)
{
  // With the wine64 opt-in and a runner that actually ships a wine64 binary, the exec line uses wine64
  std::string bin_dir = test_dir + "/runner/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(bin_dir + "/wine64") << "fake";

  std::string result = Helper::build_desktop_exec_line(true, "/home/user/.wine", bin_dir, "notepad");
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" " + bin_dir + "/wine64 start \"notepad\"");
}

TEST_F(HelperTest, BuildDesktopExecLineRunnerWine64OptInFallsBackToWine)
{
  // With the wine64 opt-in but a runner that ships no wine64 binary, the exec line falls back to wine
  std::string bin_dir = test_dir + "/runner-nowine64/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";

  std::string result = Helper::build_desktop_exec_line(true, "/home/user/.wine", bin_dir, "notepad");
  EXPECT_EQ(result, "env WINEPREFIX=\"/home/user/.wine\" " + bin_dir + "/wine start \"notepad\"");
}

TEST_F(HelperTest, BuildRunnerCommandUsesUmuForGEProton)
{
  std::string runner_dir = test_dir + "/GE Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";

  std::string managed_launcher = Helper::get_umu_executable_location();
  EXPECT_TRUE(Helper::is_geproton_runner(bin_dir));
  EXPECT_EQ(Helper::build_runner_command(false, bin_dir, "start /unix \"game.exe\""),
            "env PROTONPATH='" + runner_dir + "' PROTON_VERB=run '" + managed_launcher + "' start /unix \"game.exe\"");
  EXPECT_EQ(Helper::build_winetricks_command(bin_dir, "corefonts"),
            "env PROTONPATH='" + runner_dir + "' '" + managed_launcher + "' winetricks corefonts");
  EXPECT_THROW(Helper::get_wine_executable_location(false, bin_dir), std::runtime_error);
  EXPECT_THROW(Helper::get_wineserver_executable_location(bin_dir), std::runtime_error);
}

TEST_F(HelperTest, ManagedComponentPathUsesXdgDataDirectory)
{
  EXPECT_EQ(Helper::get_umu_executable_location(), fs::path(Glib::get_user_data_dir()).append("winegui/umu/umu-run").string());
}

TEST_F(HelperTest, PrepareDxvkTestForGEProtonStagesCompletePayloadInUserDataDirectory)
{
  const fs::path source_dir = fs::path(test_dir) / "installed-apps";
  fs::create_directories(source_dir);
  const std::array<const char*, 4> payload{"d3d11-triangle.exe", "d3d11.dll", "d3dcompiler_47.dll", "dxgi.dll"};
  for (const char* filename : payload)
    std::ofstream(source_dir / filename) << "payload-" << filename;

  const fs::path staged_executable = Helper::prepare_dxvk_test_for_geproton((source_dir / "d3d11-triangle.exe").string());
  const fs::path expected_directory = fs::path(Glib::get_user_data_dir()) / "winegui" / "apps" / "dxvk-test";
  EXPECT_EQ(staged_executable, expected_directory / "d3d11-triangle.exe");
  for (const char* filename : payload)
  {
    std::ifstream staged_file(expected_directory / filename);
    const std::string contents((std::istreambuf_iterator<char>(staged_file)), std::istreambuf_iterator<char>());
    EXPECT_EQ(contents, std::string("payload-") + filename);
  }
}

TEST_F(HelperTest, PrepareDxvkTestForGEProtonRejectsIncompletePayload)
{
  const fs::path source_dir = fs::path(test_dir) / "incomplete-apps";
  fs::create_directories(source_dir);
  std::ofstream(source_dir / "d3d11-triangle.exe") << "test";

  EXPECT_THROW(Helper::prepare_dxvk_test_for_geproton((source_dir / "d3d11-triangle.exe").string()), std::runtime_error);
}

TEST_F(HelperTest, BuildDesktopExecLineUsesUmuForGEProton)
{
  std::string runner_dir = test_dir + "/GE-Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";
  std::string prefix = test_dir + "/My Prefix";
  fs::create_directories(prefix);
  std::ofstream(prefix + "/user.reg") << "WINE REGISTRY Version 2\n#arch=win64\n";

  std::string result = Helper::build_desktop_exec_line(false, prefix, bin_dir, prefix + "/game.exe");
  EXPECT_EQ(result, "env WINEPREFIX=\"" + prefix + "\" env PROTONPATH='" + runner_dir + "' PROTON_VERB=run '" +
                        Helper::get_umu_executable_location() + "' start /unix \"" + prefix + "/game.exe\"");
}

TEST_F(HelperTest, RunProgramUnderWineUsesUmuEnvironmentForGEProton)
{
  std::string runner_dir = test_dir + "/GE Proton;11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";

  std::string prefix = test_dir + "/Prefix With Spaces";
  fs::create_directories(prefix);
  std::ofstream(prefix + "/user.reg") << "WINE REGISTRY Version 2\n#arch=win64\n";

  std::string fake_umu = Helper::get_umu_executable_location();
  fs::create_directories(fs::path(fake_umu).parent_path());
  std::ofstream(fake_umu) << "#!/bin/sh\n"
                             "printf "
                             "'WINEPREFIX=%s\\nPROTONPATH=%s\\nPROTON_VERB=%s\\nGAMEID=%s\\nSTORE=%s\\nARGS=%s\\n' "
                             "\"$WINEPREFIX\" \"$PROTONPATH\" \"$PROTON_VERB\" \"$GAMEID\" \"$STORE\" \"$*\"\n";
  fs::permissions(fake_umu, fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  int exit_code = -1;
  std::string output = Helper::run_program_under_wine(false, prefix, 1, "start /unix \"game.exe\"", "", {{"GAMEID", "umu-example"}, {"STORE", "gog"}},
                                                      false, true, bin_dir, &exit_code);
  EXPECT_EQ(exit_code, 0);
  EXPECT_TRUE(output.contains("WINEPREFIX=" + prefix));
  EXPECT_TRUE(output.contains("PROTONPATH=" + runner_dir));
  EXPECT_TRUE(output.contains("GAMEID=umu-example"));
  EXPECT_TRUE(output.contains("STORE=gog"));
  EXPECT_TRUE(output.contains("PROTON_VERB=run"));
  EXPECT_TRUE(output.contains("ARGS=start /unix game.exe"));

  output = Helper::run_program_under_wine(false, prefix, 1, "winetricks corefonts", "", {}, false, true, bin_dir, &exit_code);
  EXPECT_EQ(exit_code, 0);
  EXPECT_TRUE(output.contains("PROTON_VERB="));
  EXPECT_FALSE(output.contains("PROTON_VERB=runinprefix"));
  EXPECT_TRUE(output.contains("ARGS=winetricks corefonts"));
}

TEST_F(HelperTest, GEProtonSecondLaunchDoesNotWaitForActiveProcess)
{
  std::string runner_dir = test_dir + "/GE-Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";
  std::string prefix = test_dir + "/Win64 Prefix";
  fs::create_directories(prefix);
  std::ofstream(prefix + "/user.reg") << "WINE REGISTRY Version 2\n#arch=win64\n";

  std::string active_marker = test_dir + "/active";
  std::string fake_umu = Helper::get_umu_executable_location();
  fs::create_directories(fs::path(fake_umu).parent_path());
  std::ofstream(fake_umu) << "#!/bin/sh\n"
                             "if [ \"$1\" = hold.exe ]; then touch \""
                          << active_marker << "\"; sleep 1; rm -f \"" << active_marker
                          << "\"; fi\n"
                             "printf 'PROTON_VERB=%s ARGS=%s\\n' \"$PROTON_VERB\" \"$*\"\n";
  fs::permissions(fake_umu, fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  std::thread first([&]() { Helper::run_program_under_wine(false, prefix, 1, "hold.exe", "", {}, false, true, bin_dir); });
  for (int attempts = 0; attempts < 100 && !fs::exists(active_marker); ++attempts)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(fs::exists(active_marker));

  const auto started = std::chrono::steady_clock::now();
  int exit_code = -1;
  const std::string output = Helper::run_program_under_wine(false, prefix, 1, "second.exe", "", {}, false, true, bin_dir, &exit_code);
  const auto elapsed = std::chrono::steady_clock::now() - started;
  first.join();

  EXPECT_EQ(exit_code, 0);
  EXPECT_LT(elapsed, std::chrono::milliseconds(500));
  EXPECT_TRUE(output.contains("PROTON_VERB=run"));
  EXPECT_TRUE(output.contains("ARGS=second.exe"));
}

TEST_F(HelperTest, CreateWin64WineBottleInitializesGEProtonThroughUmu)
{
  std::string runner_dir = test_dir + "/GE-Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";

  std::string log_file = test_dir + "/umu-prefix.log";
  std::string fake_umu = Helper::get_umu_executable_location();
  fs::create_directories(fs::path(fake_umu).parent_path());
  std::ofstream(fake_umu) << "#!/bin/sh\n"
                             "printf 'WINEPREFIX=%s\\nWINEARCH=%s\\nPROTONPATH=%s\\nARGC=%s\\nARG1=%s\\n' "
                             "\"$WINEPREFIX\" \"$WINEARCH\" \"$PROTONPATH\" \"$#\" \"$1\" > \""
                          << log_file << "\"\n";
  fs::permissions(fake_umu, fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  std::string prefix = test_dir + "/New Prefix";
  EXPECT_NO_THROW(Helper::create_wine_bottle(false, prefix, BottleTypes::Bit::win64, false, bin_dir));

  std::ifstream log(log_file);
  std::string contents((std::istreambuf_iterator<char>(log)), std::istreambuf_iterator<char>());
  EXPECT_TRUE(contents.contains("WINEPREFIX=" + prefix));
  EXPECT_TRUE(contents.contains("WINEARCH=win64"));
  EXPECT_TRUE(contents.contains("PROTONPATH=" + runner_dir));
  EXPECT_TRUE(contents.contains("ARGC=1"));
  EXPECT_TRUE(contents.contains("ARG1=\n"));
}

TEST_F(HelperTest, CreateWin32WineBottleRejectsGEProtonWithoutRunningEmbeddedWine)
{
  std::string runner_dir = test_dir + "/GE-Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";

  std::string log_file = test_dir + "/wine-prefix.log";
  std::string fake_wine = bin_dir + "/wine";
  std::ofstream(fake_wine) << "#!/bin/sh\n"
                              "printf 'WINEPREFIX=%s\\nWINEARCH=%s\\nARGS=%s\\n' \"$WINEPREFIX\" \"$WINEARCH\" \"$*\" > \""
                           << log_file << "\"\n";
  fs::permissions(fake_wine, fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  std::string prefix = test_dir + "/New Win32 Prefix";
  std::string message;
  try
  {
    Helper::create_wine_bottle(false, prefix, BottleTypes::Bit::win32, false, bin_dir);
  }
  catch (const std::runtime_error& error)
  {
    message = error.what();
  }
  EXPECT_TRUE(message.contains("supports only 64-bit"));
  EXPECT_TRUE(message.contains("regular Wine runner"));
  EXPECT_FALSE(fs::exists(log_file));
  EXPECT_FALSE(fs::exists(Helper::get_umu_executable_location()));
}

TEST_F(HelperTest, RunProgramUnderWineRejectsWin32GEProtonBottle)
{
  std::string runner_dir = test_dir + "/GE-Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";
  std::ofstream(bin_dir + "/wine") << "#!/bin/sh\nprintf 'ARGS=%s\\n' \"$*\"\n";
  fs::permissions(bin_dir + "/wine", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  std::string prefix = test_dir + "/Win32 Prefix";
  fs::create_directories(prefix);
  std::ofstream(prefix + "/user.reg") << "WINE REGISTRY Version 2\n#arch=win32\n";

  std::string message;
  try
  {
    Helper::run_program_under_wine(false, prefix, 1, "start /unix \"game.exe\"", "", {}, false, true, bin_dir);
  }
  catch (const std::runtime_error& error)
  {
    message = error.what();
  }
  EXPECT_TRUE(message.contains("supports only 64-bit"));
  EXPECT_TRUE(message.contains("regular Wine runner"));
  EXPECT_FALSE(fs::exists(Helper::get_umu_executable_location()));
}

TEST_F(HelperTest, BuildDesktopExecLineRejectsWin32GEProtonBottle)
{
  std::string runner_dir = test_dir + "/GE-Proton11-3";
  std::string bin_dir = runner_dir + "/files/bin";
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wine") << "fake";
  std::ofstream(runner_dir + "/proton") << "fake";
  std::ofstream(runner_dir + "/toolmanifest.vdf") << "fake";
  std::string prefix = test_dir + "/Win32 Prefix";
  fs::create_directories(prefix);
  std::ofstream(prefix + "/user.reg") << "WINE REGISTRY Version 2\n#arch=win32\n";

  EXPECT_THROW(Helper::build_desktop_exec_line(false, prefix, bin_dir, "/games/game.exe"), std::runtime_error);
}

TEST_F(HelperTest, MissingManagedComponentProducesUserFacingError)
{
  fs::remove(Helper::get_umu_executable_location());

  std::string message;
  try
  {
    Helper::require_umu_available();
  }
  catch (const std::runtime_error& error)
  {
    message = error.what();
  }
  EXPECT_TRUE(message.contains("could not prepare GE-Proton support"));
  EXPECT_TRUE(message.contains("internet connection"));
  EXPECT_TRUE(message.contains("try again"));
  EXPECT_FALSE(message.contains("umu"));
  EXPECT_FALSE(message.contains("PATH"));
  EXPECT_FALSE(message.contains("Install"));
}

// Test create_desktop_file function

TEST_F(HelperTest, CreateDesktopFileWritesEntry)
{
  std::string target_dir = test_dir + "/applications";
  bool success = Helper::create_desktop_file(target_dir, "winegui-test-app.desktop", "Test App", "A test comment",
                                             "env WINEPREFIX=\"/home/user/.wine\" wine64 start \"notepad\"", "logo_big.png", "TestBottle", false);
  EXPECT_TRUE(success);

  std::string file_path = target_dir + "/winegui-test-app.desktop";
  EXPECT_TRUE(fs::exists(file_path));

  std::ifstream file(file_path);
  std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  EXPECT_TRUE(contents.find("[Desktop Entry]") != std::string::npos);
  EXPECT_TRUE(contents.find("Type=Application") != std::string::npos);
  EXPECT_TRUE(contents.find("Name=Test App") != std::string::npos);
  EXPECT_TRUE(contents.find("Comment=A test comment") != std::string::npos);
  EXPECT_TRUE(contents.find("Exec=env WINEPREFIX=\"/home/user/.wine\" wine64 start \"notepad\"") != std::string::npos);
  EXPECT_TRUE(contents.find("X-WineGUI-Bottle=TestBottle") != std::string::npos);
}

// Test to_filename_part function

TEST_F(HelperTest, ToFilenamePartLowercasesAndReplacesSpaces)
{
  EXPECT_EQ(Helper::to_filename_part("Wine Config"), "wine-config");
}

TEST_F(HelperTest, ToFilenamePartDropsSpecialCharacters)
{
  EXPECT_EQ(Helper::to_filename_part("My App (2.0)!"), "my-app-20");
}

TEST_F(HelperTest, ToFilenamePartKeepsDashAndUnderscore)
{
  EXPECT_EQ(Helper::to_filename_part("office_2019-pro"), "office_2019-pro");
}

TEST_F(HelperTest, ToFilenamePartEmptyFallback)
{
  EXPECT_EQ(Helper::to_filename_part("***"), "shortcut");
}
