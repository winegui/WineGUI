#include "umu_launcher_manager.h"

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glibmm/checksum.h>
#include <glibmm/spawn.h>
#include <gtest/gtest.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

class UmuLauncherManagerTest : public ::testing::Test
{
protected:
  fs::path test_dir;

  void SetUp() override
  {
    test_dir = fs::temp_directory_path() / ("winegui_umu_manager_test_" + std::to_string(getpid()));
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
  }

  void TearDown() override
  {
    fs::remove_all(test_dir);
  }

  static std::string sha256(const fs::path& path)
  {
    Glib::Checksum checksum(Glib::Checksum::Type::SHA256);
    std::ifstream file(path, std::ios::binary);
    std::vector<char> buffer(64 * 1024);
    while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || file.gcount() > 0)
      checksum.update(reinterpret_cast<const guchar*>(buffer.data()), static_cast<gssize>(file.gcount()));
    return checksum.get_string();
  }

  fs::path create_archive(const std::string& name = "archive.tar", const std::string& alias_target = "umu-run", bool extra_file = false)
  {
    const fs::path source = test_dir / ("source-" + name);
    fs::create_directories(source / "umu");
    std::ofstream(source / "umu" / "umu-run") << "#!/bin/sh\nexit 0\n";
    fs::create_symlink(alias_target, source / "umu" / "umu_run.py");
    if (extra_file)
      std::ofstream(source / "umu" / "unexpected") << "unsafe";
    const fs::path archive = test_dir / name;
    std::vector<std::string> argv{"tar", "-cf", archive.string(), "--no-recursion", "-C", source.string(), "umu/", "umu/umu-run", "umu/umu_run.py"};
    if (extra_file)
      argv.emplace_back("umu/unexpected");
    std::string output;
    std::string error;
    int status = 0;
    Glib::spawn_sync("", argv, Glib::SpawnFlags::SEARCH_PATH, {}, &output, &error, &status);
    EXPECT_EQ(status, 0) << error;
    return archive;
  }

  UmuLauncherManager::Release release_for(const fs::path& archive, const std::string& version = "test-1")
  {
    return {version, archive.filename().string(), "file://" + archive.string(), fs::file_size(archive), sha256(archive)};
  }
};

TEST_F(UmuLauncherManagerTest, InstallsVerifiedArchiveAtStableManagedPath)
{
  const fs::path archive = create_archive();
  const auto release = release_for(archive);
  const fs::path base = test_dir / "managed";

  EXPECT_TRUE(UmuLauncherManager::ensure_installed(base, release));
  EXPECT_TRUE(UmuLauncherManager::is_ready(base, release));
  EXPECT_EQ(fs::read_symlink(base / "umu-run"), fs::path("versions") / release.version / "umu-run");
  EXPECT_EQ(access((base / "umu-run").c_str(), X_OK), 0);
}

TEST_F(UmuLauncherManagerTest, ExistingVerifiedInstallSkipsTheNetwork)
{
  const fs::path archive = create_archive();
  auto release = release_for(archive);
  const fs::path base = test_dir / "managed";
  ASSERT_TRUE(UmuLauncherManager::ensure_installed(base, release));
  release.download_url = "file:///does/not/exist";
  EXPECT_TRUE(UmuLauncherManager::ensure_installed(base, release));
}

TEST_F(UmuLauncherManagerTest, RejectsChecksumMismatch)
{
  const fs::path archive = create_archive();
  auto release = release_for(archive);
  release.sha256 = std::string(64, '0');
  EXPECT_THROW(UmuLauncherManager::ensure_installed(test_dir / "managed", release), std::runtime_error);
  EXPECT_FALSE(UmuLauncherManager::is_ready(test_dir / "managed", release));
}

TEST_F(UmuLauncherManagerTest, RejectsMalformedOrUnexpectedArchiveLayout)
{
  const fs::path archive = create_archive("extra.tar", "umu-run", true);
  const auto release = release_for(archive);
  EXPECT_THROW(UmuLauncherManager::ensure_installed(test_dir / "managed", release), std::runtime_error);
}

TEST_F(UmuLauncherManagerTest, RejectsUnsafeSymlink)
{
  const fs::path archive = create_archive("symlink.tar", "../../outside");
  const auto release = release_for(archive);
  EXPECT_THROW(UmuLauncherManager::ensure_installed(test_dir / "managed", release), std::runtime_error);
}

TEST_F(UmuLauncherManagerTest, CancelledInstallLeavesNoActiveLauncher)
{
  const fs::path archive = create_archive();
  const auto release = release_for(archive);
  std::atomic<bool> cancel{true};
  EXPECT_FALSE(UmuLauncherManager::ensure_installed(test_dir / "managed", release, &cancel));
  EXPECT_FALSE(UmuLauncherManager::is_ready(test_dir / "managed", release));
}

TEST_F(UmuLauncherManagerTest, InterruptedDownloadIsCleanedAndCanBeRetried)
{
  const fs::path archive = create_archive();
  const auto release = release_for(archive);
  const fs::path base = test_dir / "managed";
  std::atomic<bool> cancel{false};

  EXPECT_FALSE(UmuLauncherManager::ensure_installed(base, release, &cancel, [&cancel](std::uint64_t, std::uint64_t) { cancel.store(true); }));
  EXPECT_FALSE(UmuLauncherManager::is_ready(base, release));
  for (const auto& entry : fs::directory_iterator(base / ".downloads"))
    EXPECT_FALSE(entry.path().filename().string().contains(".part-"));

  cancel.store(false);
  EXPECT_TRUE(UmuLauncherManager::ensure_installed(base, release, &cancel));
  EXPECT_TRUE(UmuLauncherManager::is_ready(base, release));
}

TEST_F(UmuLauncherManagerTest, RejectsTruncatedArchive)
{
  const fs::path archive = create_archive();
  const fs::path truncated = test_dir / "truncated.tar";
  std::ifstream input(archive, std::ios::binary);
  std::ofstream output(truncated, std::ios::binary);
  std::vector<char> bytes(256);
  input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  output.write(bytes.data(), input.gcount());
  output.close();

  const auto release = release_for(truncated);
  EXPECT_THROW(UmuLauncherManager::ensure_installed(test_dir / "managed", release), std::runtime_error);
  EXPECT_FALSE(UmuLauncherManager::is_ready(test_dir / "managed", release));
}

TEST_F(UmuLauncherManagerTest, OfflineInitialInstallReportsFailure)
{
  const fs::path archive = create_archive();
  auto release = release_for(archive);
  release.download_url = "file:///definitely/missing/umu-launcher.tar";
  EXPECT_THROW(UmuLauncherManager::ensure_installed(test_dir / "managed", release), std::runtime_error);
  EXPECT_FALSE(UmuLauncherManager::is_ready(test_dir / "managed", release));
}

TEST_F(UmuLauncherManagerTest, ConcurrentInstallersConvergeOnOneValidVersion)
{
  const fs::path archive = create_archive();
  const auto release = release_for(archive);
  const fs::path base = test_dir / "managed";
  bool first = false;
  bool second = false;
  std::thread first_thread([&]() { first = UmuLauncherManager::ensure_installed(base, release); });
  std::thread second_thread([&]() { second = UmuLauncherManager::ensure_installed(base, release); });
  first_thread.join();
  second_thread.join();
  EXPECT_TRUE(first);
  EXPECT_TRUE(second);
  EXPECT_TRUE(UmuLauncherManager::is_ready(base, release));
}

TEST_F(UmuLauncherManagerTest, FailedUpgradeKeepsPreviousLauncherActive)
{
  const fs::path first_archive = create_archive("first.tar");
  const auto first_release = release_for(first_archive, "one");
  const fs::path base = test_dir / "managed";
  ASSERT_TRUE(UmuLauncherManager::ensure_installed(base, first_release));

  const fs::path broken_archive = create_archive("broken.tar", "umu-run", true);
  const auto broken_release = release_for(broken_archive, "two");
  EXPECT_THROW(UmuLauncherManager::ensure_installed(base, broken_release), std::runtime_error);
  EXPECT_EQ(fs::read_symlink(base / "umu-run"), fs::path("versions") / "one" / "umu-run");
  EXPECT_TRUE(UmuLauncherManager::is_ready(base, first_release));
}

// Deliberately excluded from the normal offline-capable unit suite. Run before changing the
// pinned metadata to verify the official release URL, size, checksum, archive layout and zipapp.
TEST_F(UmuLauncherManagerTest, DISABLED_InstallsPinnedOfficialReleaseEndToEnd)
{
  const char* requested_dir = std::getenv("WINEGUI_UMU_E2E_DIR");
  const fs::path managed_dir = requested_dir == nullptr ? test_dir / "managed" : fs::path(requested_dir);
  EXPECT_TRUE(UmuLauncherManager::ensure_installed(managed_dir, UmuLauncherManager::pinned_release()));
  EXPECT_TRUE(UmuLauncherManager::is_ready(managed_dir, UmuLauncherManager::pinned_release()));
}
