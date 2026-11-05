#include "helper.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gio/gdesktopappinfo.h>
#include <giomm/init.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <gtest/gtest.h>
#include <signal.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

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
  std::unique_ptr<ScopedPath> test_path;

  static void write_executable(const std::string& path, const std::string& script = "#!/bin/sh\nexit 0\n")
  {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream(path) << script;
    fs::permissions(path, fs::perms::owner_all);
  }

  static void make_runner(const std::string& bin_dir)
  {
    write_executable(bin_dir + "/wine");
    write_executable(bin_dir + "/wineserver");
  }

  static std::vector<std::string> desktop_arguments(const std::string& line)
  {
    return Glib::shell_parse_argv(line);
  }

  static void SetUpTestSuite()
  {
    // Initialize Gio to prevent GLib warnings
    Gio::init();
  }

  void SetUp() override
  {
    test_dir = fs::temp_directory_path() / "winegui_helper_test";
    fs::create_directories(test_dir);
    make_runner(test_dir + "/system/bin");
    test_path = std::make_unique<ScopedPath>(test_dir + "/system/bin:" + (std::getenv("PATH") ? std::getenv("PATH") : ""));
    write_executable(Glib::build_filename(Glib::get_user_data_dir(), "winegui", "winetricks"));
    fs::remove_all(fs::path(Helper::get_umu_executable_location()).parent_path());
  }

  void TearDown() override
  {
    test_path.reset();
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
  fs::permissions(bin_dir + "/wineserver", fs::perms::owner_all);

  EXPECT_EQ(Helper::get_wineserver_executable_location(bin_dir), bin_dir + "/wineserver");
}

TEST_F(HelperTest, GetWineserverExecutableLocationCustomPathWithoutWineserver)
{
  // A broken custom installation must never select the system wineserver
  std::string bin_dir = test_dir + "/runner-no-server/bin";
  fs::create_directories(bin_dir);
  EXPECT_THROW(Helper::get_wineserver_executable_location(bin_dir), std::runtime_error);
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

TEST_F(HelperTest, DesktopLaunchPreservesSelectedInstallationAndArguments)
{
  const std::string bin = test_dir + "/runner with spaces/bin";
  make_runner(bin);
  write_executable(bin + "/wine64");
  for (bool prefer64 : {false, true})
  {
    for (const std::string command : {"notepad", "/some path/game.exe", "C:\\Games\\game.exe"})
    {
      auto args = desktop_arguments(Helper::build_desktop_exec_line(prefer64, "/a prefix", bin, command, {{"TEST", "value with spaces"}}));
      EXPECT_NE(std::find(args.begin(), args.end(), "WINE=" + bin + (prefer64 ? "/wine64" : "/wine")), args.end());
      EXPECT_NE(std::find(args.begin(), args.end(), "WINESERVER=" + bin + "/wineserver"), args.end());
      EXPECT_NE(std::find(args.begin(), args.end(), "WINEPREFIX=/a prefix"), args.end());
      EXPECT_NE(std::find(args.begin(), args.end(), "TEST=value with spaces"), args.end());
      EXPECT_EQ(args.back(), command);
    }
  }
}

TEST_F(HelperTest, BuildWineLaunchCommandUsesStartForApplications)
{
  EXPECT_EQ(Glib::shell_parse_argv(Helper::build_wine_launch_command("/a path/game.exe")),
            (std::vector<std::string>{"start", "/unix", "/a path/game.exe"}));
  EXPECT_EQ(Glib::shell_parse_argv(Helper::build_wine_launch_command("notepad")), (std::vector<std::string>{"start", "notepad"}));
}

TEST_F(HelperTest, BuildWineLaunchCommandAlwaysUsesMsiExecForInstallers)
{
  EXPECT_EQ(Glib::shell_parse_argv(Helper::build_wine_launch_command("/a path/setup.msi", true)),
            (std::vector<std::string>{"msiexec", "/i", "/a path/setup.msi"}));
}

TEST_F(HelperTest, BuildDesktopExecLineWinetricks)
{
  auto args = desktop_arguments(Helper::build_desktop_exec_line(false, "/a prefix", "", Helper::get_winetricks_location() + " --gui -q"));
  EXPECT_EQ(args.back(), "-q");
  EXPECT_NE(std::find(args.begin(), args.end(), Helper::get_winetricks_location()), args.end());
  EXPECT_NE(std::find(args.begin(), args.end(), "WINE=" + test_dir + "/system/bin/wine"), args.end());
  EXPECT_NE(std::find(args.begin(), args.end(), "WINESERVER=" + test_dir + "/system/bin/wineserver"), args.end());
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
  EXPECT_EQ(Helper::build_runner_command(false, bin_dir, "start /unix \"game.exe\""), "env -u WINE -u WINESERVER -u WINELOADER PROTONPATH='" +
                                                                                          runner_dir + "' PROTON_VERB=run '" + managed_launcher +
                                                                                          "' start /unix \"game.exe\"");
  EXPECT_EQ(Helper::build_winetricks_command(bin_dir, "corefonts"),
            "env -u WINE -u WINESERVER -u WINELOADER PROTONPATH='" + runner_dir + "' '" + managed_launcher + "' winetricks corefonts");
  EXPECT_THROW(Helper::get_wine_executable_location(false, bin_dir), std::runtime_error);
  EXPECT_THROW(Helper::get_wineserver_executable_location(bin_dir), std::runtime_error);
}

TEST_F(HelperTest, FormatCpuListUsesFirstPermittedLogicalCpus)
{
  EXPECT_EQ(Helper::format_cpu_list({2, 4, 5, 6, 9}, 4), "2,4-6");
  EXPECT_EQ(Helper::format_cpu_list({3, 7}, 99), "3,7");
  EXPECT_EQ(Helper::format_cpu_list({3, 7}, 0), "");
  EXPECT_THROW(Helper::format_cpu_list({}, 1), std::runtime_error);
}

TEST_F(HelperTest, ApplyCpuCoreLimitWrapsTheRunner)
{
  const std::string command = "env PROTONPATH='/runner' umu-run game.exe";
  EXPECT_EQ(Helper::apply_cpu_core_limit(command, 0), command);

  const std::string limited = Helper::apply_cpu_core_limit(command, 1);
  EXPECT_NE(limited.find("taskset' --cpu-list '"), std::string::npos);
  EXPECT_LT(limited.find("taskset' --cpu-list"), limited.find("env PROTONPATH"));
  EXPECT_TRUE(limited.ends_with(command));
}

TEST_F(HelperTest, FullCpuCountIsNotAnEffectiveLimit)
{
  const int allowed_cpu_count = static_cast<int>(Helper::get_allowed_cpu_ids().size());
  ASSERT_GT(allowed_cpu_count, 0);
  EXPECT_EQ(Helper::get_effective_cpu_core_limit(0), 0);
  if (allowed_cpu_count > 1)
  {
    EXPECT_EQ(Helper::get_effective_cpu_core_limit(allowed_cpu_count - 1), allowed_cpu_count - 1);
    EXPECT_NE(Helper::apply_cpu_core_limit("wine game.exe", allowed_cpu_count - 1).find("taskset"), std::string::npos);
  }
  EXPECT_EQ(Helper::get_effective_cpu_core_limit(allowed_cpu_count), 0);
  EXPECT_EQ(Helper::get_effective_cpu_core_limit(allowed_cpu_count + 1), 0);
  EXPECT_EQ(Helper::apply_cpu_core_limit("wine game.exe", allowed_cpu_count), "wine game.exe");
}

TEST_F(HelperTest, DesktopLaunchUsesStartWhenCpuLimitIncludesEveryPermittedCpu)
{
  const int allowed_cpu_count = static_cast<int>(Helper::get_allowed_cpu_ids().size());
  ASSERT_GT(allowed_cpu_count, 0);
  const std::string result = Helper::build_desktop_exec_line(false, "/home/user/.wine", "", "notepad", {}, allowed_cpu_count);
  EXPECT_EQ(desktop_arguments(result).back(), "notepad");
  EXPECT_EQ(result.find("taskset"), std::string::npos);
}

TEST_F(HelperTest, ApplyCpuCoreLimitFailsClosedWithoutTaskset)
{
  ScopedPath path_without_taskset(test_dir);
  EXPECT_THROW(Helper::apply_cpu_core_limit("wine notepad", 1), std::runtime_error);
  EXPECT_EQ(Helper::apply_cpu_core_limit("wine notepad", 0), "wine notepad");
}

TEST_F(HelperTest, BuildDesktopExecLineAppliesCpuLimitBeforeRegularWine)
{
  const std::string result = Helper::build_desktop_exec_line(false, "/home/user/.wine", "", "notepad", {}, 1);
  EXPECT_NE(result.find("taskset\" \"--cpu-list\" \""), std::string::npos);
  EXPECT_LT(result.find("taskset\" \"--cpu-list"), result.find("\"start\" \"notepad\""));

  const std::string unix_result = Helper::build_desktop_exec_line(false, "/home/user/.wine", "", "/home/user/Games/game.exe", {}, 1);
  EXPECT_LT(unix_result.find("taskset\" \"--cpu-list"), unix_result.find("\"start\" \"/unix\" \"/home/user/Games/game.exe\""));
}

TEST_F(HelperTest, RunProgramUnderWineAppliesExplicitCpuLimit)
{
  const std::string prefix = test_dir + "/limited-prefix";
  const std::string bin_dir = test_dir + "/limited-runner/bin";
  fs::create_directories(prefix);
  make_runner(bin_dir);
  std::ofstream(bin_dir + "/wine") << "#!/bin/sh\ntaskset -pc $$\n";
  fs::permissions(bin_dir + "/wine", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  int exit_code = -1;
  const std::string output = Helper::run_program_under_wine(false, prefix, 1, "notepad", "", {}, false, true, bin_dir, &exit_code, 1);
  EXPECT_EQ(exit_code, 0);
  EXPECT_NE(output.find("current affinity list:"), std::string::npos);
  EXPECT_EQ(output.find(','), std::string::npos);
  EXPECT_EQ(output.find('-'), std::string::npos);
}

TEST_F(HelperTest, DetachedWineLaunchDoesNotWaitForDescendantPipes)
{
  const std::string prefix = test_dir + "/detached-prefix";
  const std::string bin_dir = test_dir + "/detached-runner/bin";
  fs::create_directories(prefix);
  make_runner(bin_dir);
  std::ofstream(bin_dir + "/wine") << "#!/bin/sh\nsleep 2 &\nexit 0\n";
  fs::permissions(bin_dir + "/wine", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  const auto started = std::chrono::steady_clock::now();
  Helper::launch_program_under_wine(false, prefix, 1, "start notepad", "", {}, false, true, bin_dir);
  const auto elapsed = std::chrono::steady_clock::now() - started;
  EXPECT_LT(elapsed, std::chrono::milliseconds(500));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::ifstream children("/proc/self/task/" + std::to_string(getpid()) + "/children");
  std::string child_pids;
  std::getline(children, child_pids);
  EXPECT_TRUE(child_pids.empty());
}

TEST_F(HelperTest, DetachedWineLaunchWritesDirectlyToBottleLogAndAppliesCpuLimit)
{
  const std::string prefix = test_dir + "/logging-prefix";
  const std::string bin_dir = test_dir + "/logging-runner/bin";
  fs::create_directories(prefix);
  make_runner(bin_dir);
  std::ofstream(bin_dir + "/wine") << "#!/bin/sh\ntaskset -pc $$\necho standard-output\necho standard-error >&2\n";
  fs::permissions(bin_dir + "/wine", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  Helper::launch_program_under_wine(false, prefix, 1, "start notepad", "", {}, true, true, bin_dir, 1);
  const std::string log_path = Helper::get_log_file_path(prefix);
  std::string output;
  for (int attempt = 0; attempt < 20; ++attempt)
  {
    if (fs::exists(log_path))
      output = Glib::file_get_contents(log_path);
    if (output.contains("standard-error"))
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  ASSERT_TRUE(fs::exists(log_path));
  EXPECT_NE(output.find("current affinity list:"), std::string::npos);
  EXPECT_TRUE(output.contains("standard-output"));
  EXPECT_TRUE(output.contains("standard-error"));
}

TEST_F(HelperTest, WineServerWaitTimesOutAndReapsItsChild)
{
  const std::string prefix = test_dir + "/wait-prefix";
  const std::string bin_dir = test_dir + "/wait-runner/bin";
  fs::create_directories(prefix);
  fs::create_directories(bin_dir);
  std::ofstream(bin_dir + "/wineserver") << "#!/bin/sh\nsleep 5\n";
  fs::permissions(bin_dir + "/wineserver", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

  const auto started = std::chrono::steady_clock::now();
  EXPECT_EQ(Helper::wait_until_wineserver_is_terminated(prefix, bin_dir, std::chrono::milliseconds(50)), WineServerWaitResult::TimedOut);
  EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(1));
  EXPECT_EQ(waitpid(-1, nullptr, WNOHANG), -1);
}

TEST_F(HelperTest, DetectsRunningWineApplicationButIgnoresBackgroundServices)
{
  const std::string prefix = test_dir + "/active-prefix";
  const pid_t service_pid = fork();
  ASSERT_GE(service_pid, 0);
  if (service_pid == 0)
  {
    setenv("WINEPREFIX", prefix.c_str(), 1);
    execl("/bin/sleep", "services.exe", "5", static_cast<char*>(nullptr));
    _exit(127);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(Helper::has_running_wine_application(prefix));

  const pid_t child_pid = fork();
  ASSERT_GE(child_pid, 0);
  if (child_pid == 0)
  {
    setenv("WINEPREFIX", prefix.c_str(), 1);
    execl("/bin/sleep", "game.exe", "5", static_cast<char*>(nullptr));
    _exit(127);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(Helper::has_running_wine_application(prefix));
  EXPECT_FALSE(Helper::has_running_wine_application(test_dir + "/inactive-prefix"));
  kill(child_pid, SIGTERM);
  ASSERT_EQ(waitpid(child_pid, nullptr, 0), child_pid);
  kill(service_pid, SIGTERM);
  ASSERT_EQ(waitpid(service_pid, nullptr, 0), service_pid);
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
  auto args = desktop_arguments(result);
  EXPECT_NE(std::find(args.begin(), args.end(), "PROTONPATH=" + runner_dir), args.end());
  EXPECT_NE(std::find(args.begin(), args.end(), Helper::get_umu_executable_location()), args.end());
  EXPECT_EQ(args.back(), prefix + "/game.exe");
  result = Helper::build_desktop_exec_line(false, prefix, bin_dir, prefix + "/game.exe", {}, 1);
  EXPECT_NE(result.find("taskset"), std::string::npos);
  EXPECT_EQ(desktop_arguments(result).back(), prefix + "/game.exe");
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
  const std::string game_command = Helper::build_wine_launch_command("C:\\Games\\game.exe");
  std::string output = Helper::run_program_under_wine(false, prefix, 1, game_command, "", {{"GAMEID", "umu-example"}, {"STORE", "gog"}}, false, true,
                                                      bin_dir, &exit_code);
  EXPECT_EQ(exit_code, 0);
  EXPECT_TRUE(output.contains("WINEPREFIX=" + prefix));
  EXPECT_TRUE(output.contains("PROTONPATH=" + runner_dir));
  EXPECT_TRUE(output.contains("GAMEID=umu-example"));
  EXPECT_TRUE(output.contains("STORE=gog"));
  EXPECT_TRUE(output.contains("PROTON_VERB=run"));
  EXPECT_TRUE(output.contains("ARGS=start C:\\Games\\game.exe"));

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
                                             "env WINEPREFIX=\"/home/user/.wine\" wine64 start \"notepad\"", "logo_big.png", "TestBottle", false,
                                             "/home/user/.wine", "notepad");
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
  EXPECT_TRUE(contents.find("X-WineGUI-Managed=true") != std::string::npos);
  EXPECT_TRUE(contents.find("X-WineGUI-BottlePath=/home/user/.wine") != std::string::npos);
  EXPECT_TRUE(contents.find("X-WineGUI-ApplicationCommand=bm90ZXBhZA==") != std::string::npos);
}

TEST_F(HelperTest, RefreshManagedShortcutUsesCurrentBottleLaunchSettings)
{
  const std::string target_dir = Glib::build_filename(Glib::get_user_data_dir(), "applications");
  const std::string old_path = test_dir + "/old-prefix";
  const std::string new_path = test_dir + "/new-prefix";
  const std::string old_file = target_dir + "/winegui-affinity-old-test-app.desktop";
  const std::string new_file = target_dir + "/winegui-affinity-new-affinity-test-app.desktop";
  fs::remove(old_file);
  fs::remove(new_file);

  ASSERT_TRUE(Helper::create_desktop_file(target_dir, "winegui-affinity-old-test-app.desktop", "Affinity Test App", "",
                                          "env WINEPREFIX='old' wine old.exe", "logo.png", "Affinity Old", false, old_path, "test.exe"));

  BottleConfigData config;
  config.name = "Affinity New";
  config.wine_bin_path = "";
  config.cpu_core_limit = 1;
  std::map<int, ApplicationData> applications{{0, {"Affinity Test App", "", "test.exe"}}};
  EXPECT_TRUE(Helper::refresh_managed_shortcuts("Affinity Old", old_path, config, applications, new_path).empty());
  EXPECT_FALSE(fs::exists(old_file));
  ASSERT_TRUE(fs::exists(new_file));

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(new_file);
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "X-WineGUI-Bottle"), "Affinity New");
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "X-WineGUI-BottlePath").raw(), new_path);
  EXPECT_NE(keyfile->get_string("Desktop Entry", "Exec").find("taskset"), std::string::npos);
  EXPECT_NE(keyfile->get_string("Desktop Entry", "Exec").find(new_path), std::string::npos);
  fs::remove(new_file);
}

TEST_F(HelperTest, RefreshManagedShortcutMigratesCommandFromLegacyExec)
{
  const std::string target_dir = Glib::build_filename(Glib::get_user_data_dir(), "applications");
  const std::string old_path = test_dir + "/legacy-prefix";
  const std::string shortcut = target_dir + "/winegui-legacy-bottle-legacy-game.desktop";
  fs::remove(shortcut);
  fs::create_directories(target_dir);
  std::ofstream(shortcut) << "[Desktop Entry]\n"
                             "Type=Application\n"
                             "Name=Legacy Game\n"
                             "Exec=env WINEPREFIX=\""
                          << old_path
                          << "\" wine start /unix \"/legacy/menu/Game Launcher.exe\"\n"
                             "X-WineGUI-Bottle=Legacy Bottle\n";

  BottleConfigData config;
  config.name = "Legacy Bottle";
  std::map<int, ApplicationData> applications;
  EXPECT_TRUE(Helper::refresh_managed_shortcuts("Legacy Bottle", old_path, config, applications, old_path).empty());

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(shortcut);
  EXPECT_TRUE(keyfile->get_boolean("Desktop Entry", "X-WineGUI-Managed"));
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "X-WineGUI-ApplicationCommand").raw(), "L2xlZ2FjeS9tZW51L0dhbWUgTGF1bmNoZXIuZXhl");
  EXPECT_NE(keyfile->get_string("Desktop Entry", "Exec").find("/legacy/menu/Game Launcher.exe"), std::string::npos);
  fs::remove(shortcut);
}

TEST_F(HelperTest, RefreshManagedShortcutTranslatesCommandWhenPrefixMoves)
{
  const std::string target_dir = Glib::build_filename(Glib::get_user_data_dir(), "applications");
  const std::string old_path = test_dir + "/old-prefix";
  const std::string new_path = test_dir + "/renamed-prefix";
  const std::string old_file = target_dir + "/winegui-old-bottle-prefix-game.desktop";
  const std::string new_file = target_dir + "/winegui-renamed-bottle-prefix-game.desktop";
  const std::string old_command = old_path + "/drive_c/Games/Prefix Game.exe";
  const std::string new_command = new_path + "/drive_c/Games/Prefix Game.exe";
  fs::remove(old_file);
  fs::remove(new_file);

  ASSERT_TRUE(Helper::create_desktop_file(target_dir, "winegui-old-bottle-prefix-game.desktop", "Prefix Game", "",
                                          "env WINEPREFIX='old' wine start /unix 'old'", "logo.png", "Old Bottle", false, old_path, old_command));

  BottleConfigData config;
  config.name = "Renamed Bottle";
  std::map<int, ApplicationData> applications;
  EXPECT_TRUE(Helper::refresh_managed_shortcuts("Old Bottle", old_path, config, applications, new_path).empty());
  EXPECT_FALSE(fs::exists(old_file));
  ASSERT_TRUE(fs::exists(new_file));

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(new_file);
  gsize decoded_size = 0;
  guchar* decoded = g_base64_decode(keyfile->get_string("Desktop Entry", "X-WineGUI-ApplicationCommand").c_str(), &decoded_size);
  const std::string stored_command(reinterpret_cast<const char*>(decoded), decoded_size);
  g_free(decoded);
  EXPECT_EQ(stored_command, new_command);
  EXPECT_NE(keyfile->get_string("Desktop Entry", "Exec").find(new_command), std::string::npos);
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "Exec").find(old_command), std::string::npos);
  fs::remove(new_file);
}

TEST_F(HelperTest, RefreshManagedShortcutUpdatesUnmappableLegacyExecWhenPrefixMoves)
{
  const std::string target_dir = Glib::build_filename(Glib::get_user_data_dir(), "applications");
  const std::string old_path = test_dir + "/old-legacy-prefix";
  const std::string new_path = test_dir + "/renamed-legacy-prefix";
  const std::string old_file = target_dir + "/winegui-old-legacy-unknown.desktop";
  const std::string new_file = target_dir + "/winegui-renamed-legacy-unknown.desktop";
  fs::remove(old_file);
  fs::remove(new_file);
  fs::create_directories(target_dir);
  std::ofstream(old_file) << "[Desktop Entry]\n"
                             "Type=Application\n"
                             "Name=Unknown\n"
                             "Exec=env WINEPREFIX=\""
                          << old_path << "\" custom-launcher --prefix \"" << old_path << "\" --cache \"" << old_path
                          << "-backup\"\n"
                             "X-WineGUI-Bottle=Old Legacy\n";

  BottleConfigData config;
  config.name = "Renamed Legacy";
  std::map<int, ApplicationData> applications;
  EXPECT_TRUE(Helper::refresh_managed_shortcuts("Old Legacy", old_path, config, applications, new_path).empty());
  EXPECT_FALSE(fs::exists(old_file));
  ASSERT_TRUE(fs::exists(new_file));

  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(new_file);
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "X-WineGUI-Bottle"), "Renamed Legacy");
  const std::string exec_line = keyfile->get_string("Desktop Entry", "Exec");
  EXPECT_NE(exec_line.find(new_path), std::string::npos);
  EXPECT_NE(exec_line.find(old_path + "-backup"), std::string::npos);
  EXPECT_FALSE(keyfile->has_key("Desktop Entry", "X-WineGUI-Managed"));
  fs::remove(new_file);
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

TEST_F(HelperTest, BottleConfigurationAlwaysUsesSelectedWineAndServer)
{
  const std::string bin = test_dir + "/runner's files/bin";
  const std::string prefix = test_dir + "/new bottle";
  fs::create_directories(prefix);
  make_runner(bin);
  write_executable(
      bin + "/wine",
      "#!/bin/sh\nprintf '%s|%s|%s|%s\\n' \"$WINE\" \"$WINESERVER\" \"$WINELOADER\" \"$*\" >> \"$WINEPREFIX/trace\"\n\"$WINESERVER\" --probe\n");
  write_executable(bin + "/wineserver", "#!/bin/sh\necho selected-server >> \"$WINEPREFIX/trace\"\n");
  write_executable(test_dir + "/system/bin/wine", "#!/bin/sh\nexit 91\n");
  write_executable(test_dir + "/system/bin/wineserver", "#!/bin/sh\nexit 92\n");
  write_executable(Helper::get_winetricks_location(), "#!/bin/sh\nexec \"$WINE\" \"$@\"\n");

  Helper::create_wine_bottle(false, prefix, BottleTypes::Bit::win32, true, bin);
  Helper::set_windows_version(prefix, BottleTypes::Windows::Windows95, false, bin);
  Helper::set_virtual_desktop(prefix, "1024x768", false, bin);
  Helper::disable_virtual_desktop(prefix, false, bin);
  Helper::set_audio_driver(prefix, BottleTypes::AudioDriver::pulseaudio, false, bin);
  const std::string trace = Glib::file_get_contents(prefix + "/trace");
  for (const std::string verb : {"wineboot", "win95", "vd=1024x768", "vd=off", "sound=pulse"})
    EXPECT_TRUE(trace.contains(verb)) << trace;
  EXPECT_TRUE(trace.contains(bin + "/wine|" + bin + "/wineserver|" + bin + "/wine"));
  EXPECT_EQ(std::count(trace.begin(), trace.end(), '\n'), 10);
}

TEST_F(HelperTest, RunnerEnvironmentOverridesConflictingOperationVariables)
{
  const std::string bin = test_dir + "/selected/bin";
  make_runner(bin);
  const std::string prefix = test_dir + "/prefix";
  fs::create_directories(prefix);
  write_executable(bin + "/wine", "#!/bin/sh\nprintf '%s\\n' \"$WINE\" \"$WINESERVER\" \"$WINELOADER\" \"$KEEP_ME\"\n");
  const auto output = Helper::run_program_under_wine(
      false, prefix, 1, "cmd.exe", "",
      {{"WINE", "/wrong/wine"}, {"WINESERVER", "/wrong/server"}, {"WINELOADER", "/wrong/loader"}, {"KEEP_ME", "value's $literal"}}, true, true, bin);
  EXPECT_EQ(output, bin + "/wine\n" + bin + "/wineserver\n" + bin + "/wine\nvalue's $literal\n");
}

TEST_F(HelperTest, MissingSelectedExecutablesNeverFallBackToSystemWine)
{
  const std::string bin = test_dir + "/broken/bin";
  make_runner(bin);
  fs::remove(bin + "/wineserver");
  EXPECT_THROW(Helper::build_runner_command(false, bin, "wineboot"), std::runtime_error);
  EXPECT_THROW(Helper::build_winetricks_command(bin, "win95"), std::runtime_error);
  EXPECT_EQ(Helper::wait_until_wineserver_is_terminated(test_dir, bin), WineServerWaitResult::LaunchFailed);
  make_runner(bin);
  fs::remove(bin + "/wine");
  EXPECT_THROW(Helper::build_runner_command(false, bin, "wineboot"), std::runtime_error);
  make_runner(bin);
  fs::permissions(bin + "/wine", fs::perms::owner_read);
  EXPECT_THROW(Helper::build_runner_command(false, bin, "wineboot"), std::runtime_error);
}

TEST_F(HelperTest, SystemWineserverWaitSearchesPath)
{
  write_executable(test_dir + "/system/bin/wineserver", "#!/bin/sh\necho waited > \"$WINEPREFIX/waited\"\n");
  EXPECT_EQ(Helper::wait_until_wineserver_is_terminated(test_dir), WineServerWaitResult::Exited);
  EXPECT_EQ(Glib::file_get_contents(test_dir + "/waited"), "waited\n");
  write_executable(test_dir + "/system/bin/wineserver", "#!/bin/sh\nexit 9\n");
  EXPECT_EQ(Helper::wait_until_wineserver_is_terminated(test_dir), WineServerWaitResult::LaunchFailed);
}

TEST_F(HelperTest, WinetricksHonorsWine64PreferenceAndFallback)
{
  const std::string bin = test_dir + "/runner/bin";
  make_runner(bin);
  write_executable(bin + "/wine", "#!/bin/sh\necho wine\n");
  write_executable(bin + "/wine64", "#!/bin/sh\necho wine64\n");
  write_executable(Helper::get_winetricks_location(), "#!/bin/sh\nexec \"$WINE\" \"$@\"\n");
  EXPECT_EQ(Helper::run_program_under_wine(true, test_dir, 1, "winetricks --gui -q", "", {}, true, true, bin), "wine64\n");
  fs::remove(bin + "/wine64");
  EXPECT_EQ(Helper::run_program_under_wine(true, test_dir, 1, "winetricks --gui -q", "", {}, true, true, bin), "wine\n");
}

TEST_F(HelperTest, AutomaticShortcutRepairIsIdempotentAndLeavesUnmanagedFilesAlone)
{
  const std::string target = Glib::build_filename(Glib::get_user_data_dir(), "applications");
  const std::string filename = "winegui-repair-test-repair.desktop";
  const std::string bin = test_dir + "/selected/bin";
  make_runner(bin);
  const std::string command = Helper::get_winetricks_location() + " --gui -q";
  ASSERT_TRUE(
      Helper::create_desktop_file(target, filename, "Repair", "keep comment", "old-command", "keep-icon", "Repair Test", false, test_dir, command));
  const std::string unmanaged = target + "/winegui-repair-test-unmanaged.desktop";
  std::ofstream(unmanaged) << "[Desktop Entry]\nType=Application\nName=Unmanaged\nX-WineGUI-Bottle=Repair Test\nExec=leave-me\n";
  const auto original_unmanaged = Glib::file_get_contents(unmanaged);
  BottleConfigData config;
  config.name = "Repair Test";
  config.wine_bin_path = bin;
  EXPECT_TRUE(Helper::refresh_managed_shortcuts(config.name, test_dir, config, {}, test_dir, true).empty());
  auto keyfile = Glib::KeyFile::create();
  keyfile->load_from_file(target + "/" + filename);
  const auto args = desktop_arguments(keyfile->get_string("Desktop Entry", "Exec"));
  EXPECT_NE(std::find(args.begin(), args.end(), "WINE=" + bin + "/wine"), args.end());
  EXPECT_NE(std::find(args.begin(), args.end(), "WINESERVER=" + bin + "/wineserver"), args.end());
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "Comment"), "keep comment");
  EXPECT_EQ(keyfile->get_string("Desktop Entry", "Icon"), "keep-icon");
  const auto timestamp = fs::last_write_time(target + "/" + filename);
  EXPECT_TRUE(Helper::refresh_managed_shortcuts(config.name, test_dir, config, {}, test_dir, true).empty());
  EXPECT_EQ(timestamp, fs::last_write_time(target + "/" + filename));
  EXPECT_EQ(Glib::file_get_contents(unmanaged), original_unmanaged);
  fs::remove(target + "/" + filename);
  fs::remove(unmanaged);
}

TEST_F(HelperTest, DesktopEntryActuallyLaunchesSelectedWineWithLiteralArguments)
{
  const std::string bin = test_dir + "/runner's files/bin";
  make_runner(bin);
  write_executable(bin + "/wine", "#!/bin/sh\nprintf '%s\\n' \"$WINE\" \"$WINESERVER\" \"$@\" > \"$WINEPREFIX/desktop-trace\"\n");
  const std::string application = "game's $cash `literal` 100%.exe";
  const std::string line = Helper::build_desktop_exec_line(false, test_dir, bin, application, {{"WINESERVER", "/wrong/server"}});
  ASSERT_TRUE(Helper::create_desktop_file(test_dir, "launch.desktop", "Launch test", "", line, "", ""));
  GDesktopAppInfo* info = g_desktop_app_info_new_from_filename((test_dir + "/launch.desktop").c_str());
  ASSERT_NE(info, nullptr);
  GError* error = nullptr;
  EXPECT_TRUE(g_app_info_launch(G_APP_INFO(info), nullptr, nullptr, &error)) << (error ? error->message : "");
  g_clear_error(&error);
  g_object_unref(info);
  const std::string trace_path = test_dir + "/desktop-trace";
  for (int attempt = 0; attempt < 100 && !fs::exists(trace_path); ++attempt)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(fs::exists(trace_path));
  EXPECT_EQ(Glib::file_get_contents(trace_path), bin + "/wine\n" + bin + "/wineserver\nstart\n" + application + "\n");
}
