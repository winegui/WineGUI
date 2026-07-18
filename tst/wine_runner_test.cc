#include "wine_runner_manager.h"
#include <filesystem>
#include <fstream>
#include <giomm/init.h>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class WineRunnerTest : public ::testing::Test
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
    test_dir = fs::temp_directory_path() / "winegui_wine_runner_test";
    fs::create_directories(test_dir);
  }

  void TearDown() override
  {
    if (fs::exists(test_dir))
    {
      fs::remove_all(test_dir);
    }
  }

  void create_fake_file(const std::string& file_path)
  {
    fs::create_directories(fs::path(file_path).parent_path());
    std::ofstream file(file_path);
    file << "fake";
    file.close();
  }
};

// Test classify_kron4ek_asset function

TEST_F(WineRunnerTest, ClassifyKron4ekVanillaAsset)
{
  auto release = WineRunnerManager::classify_kron4ek_asset("wine-11.13-amd64.tar.xz");
  ASSERT_TRUE(release.has_value());
  EXPECT_EQ(release->source, WineRunner::SourceId::Kron4ekWineBuilds);
  EXPECT_EQ(release->version, "11.13");
  EXPECT_EQ(release->variant, "vanilla");
  EXPECT_FALSE(release->wow64);
}

TEST_F(WineRunnerTest, ClassifyKron4ekStagingAsset)
{
  auto release = WineRunnerManager::classify_kron4ek_asset("wine-11.13-staging-amd64.tar.xz");
  ASSERT_TRUE(release.has_value());
  EXPECT_EQ(release->version, "11.13");
  EXPECT_EQ(release->variant, "staging");
  EXPECT_FALSE(release->wow64);
}

TEST_F(WineRunnerTest, ClassifyKron4ekStagingTkgWow64Asset)
{
  auto release = WineRunnerManager::classify_kron4ek_asset("wine-11.13-staging-tkg-amd64-wow64.tar.xz");
  ASSERT_TRUE(release.has_value());
  EXPECT_EQ(release->version, "11.13");
  EXPECT_EQ(release->variant, "staging-tkg");
  EXPECT_TRUE(release->wow64);
}

TEST_F(WineRunnerTest, ClassifyKron4ekProtonAsset)
{
  // Proton builds have the variant token in front of the version
  auto release = WineRunnerManager::classify_kron4ek_asset("wine-proton-9.0-4-amd64.tar.xz");
  ASSERT_TRUE(release.has_value());
  EXPECT_EQ(release->variant, "proton");
  EXPECT_EQ(release->version, "9.0-4");
  EXPECT_FALSE(release->wow64);
}

TEST_F(WineRunnerTest, ClassifyKron4ekRejectsX86Asset)
{
  EXPECT_FALSE(WineRunnerManager::classify_kron4ek_asset("wine-11.13-x86.tar.xz").has_value());
  EXPECT_FALSE(WineRunnerManager::classify_kron4ek_asset("wine-11.13-staging-x86.tar.xz").has_value());
}

TEST_F(WineRunnerTest, ClassifyKron4ekRejectsOtherFiles)
{
  EXPECT_FALSE(WineRunnerManager::classify_kron4ek_asset("sha256sums.txt").has_value());
  EXPECT_FALSE(WineRunnerManager::classify_kron4ek_asset("wine-11.13-arm64ec-aarch64.tar.xz").has_value());
  EXPECT_FALSE(WineRunnerManager::classify_kron4ek_asset("wine-11.13-amd64.tar.gz").has_value());
  EXPECT_FALSE(WineRunnerManager::classify_kron4ek_asset("wine-amd64.tar.xz").has_value());
}

// Test classify_geproton_asset function

TEST_F(WineRunnerTest, ClassifyGEProtonAsset)
{
  auto release = WineRunnerManager::classify_geproton_asset("GE-Proton11-1.tar.gz");
  ASSERT_TRUE(release.has_value());
  EXPECT_EQ(release->source, WineRunner::SourceId::GEProton);
  EXPECT_EQ(release->variant, "GE-Proton");
  EXPECT_EQ(release->version, "11-1");
}

TEST_F(WineRunnerTest, ClassifyGEProtonRejectsAarch64AndChecksumFiles)
{
  EXPECT_FALSE(WineRunnerManager::classify_geproton_asset("GE-Proton11-1-aarch64.tar.gz").has_value());
  EXPECT_FALSE(WineRunnerManager::classify_geproton_asset("GE-Proton11-1.sha512sum").has_value());
  EXPECT_FALSE(WineRunnerManager::classify_geproton_asset("GE-Proton11-1.tar.xz").has_value());
}

// Test parse_github_releases_json function

TEST_F(WineRunnerTest, ParseGithubReleasesJson)
{
  // Modeled on the live Kron4ek release payload
  std::string json_body = R"([
    {
      "tag_name": "11.13",
      "published_at": "2026-07-11T17:34:26Z",
      "draft": false,
      "prerelease": false,
      "assets": [
        {"name": "sha256sums.txt", "browser_download_url": "https://example.com/sha256sums.txt", "size": 882},
        {"name": "wine-11.13-amd64.tar.xz", "browser_download_url": "https://example.com/wine-11.13-amd64.tar.xz", "size": 105181000},
        {"name": "wine-11.13-x86.tar.xz", "browser_download_url": "https://example.com/wine-11.13-x86.tar.xz", "size": 60900000},
        {"name": "wine-11.13-staging-amd64-wow64.tar.xz", "browser_download_url": "https://example.com/wine-11.13-staging-amd64-wow64.tar.xz", "size": 105800000}
      ]
    },
    {
      "tag_name": "11.12",
      "published_at": "2026-06-27T00:00:00Z",
      "draft": false,
      "prerelease": true,
      "assets": [
        {"name": "wine-11.12-amd64.tar.xz", "browser_download_url": "https://example.com/wine-11.12-amd64.tar.xz", "size": 1}
      ]
    }
  ])";
  auto releases = WineRunnerManager::parse_github_releases_json(WineRunner::SourceId::Kron4ekWineBuilds, json_body);
  ASSERT_EQ(releases.size(), 2u); // x86 rejected, prerelease skipped
  EXPECT_EQ(releases.at(0).tag_name, "11.13");
  EXPECT_EQ(releases.at(0).variant, "vanilla");
  EXPECT_EQ(releases.at(0).size_bytes, 105181000u);
  EXPECT_EQ(releases.at(0).checksum_type, WineRunner::ChecksumType::Sha256);
  EXPECT_EQ(releases.at(0).checksum_url, "https://example.com/sha256sums.txt");
  EXPECT_EQ(releases.at(1).variant, "staging");
  EXPECT_TRUE(releases.at(1).wow64);
}

TEST_F(WineRunnerTest, ParseGithubReleasesJsonGEProton)
{
  std::string json_body = R"([
    {
      "tag_name": "GE-Proton11-1",
      "published_at": "2026-06-24T02:06:49Z",
      "draft": false,
      "prerelease": false,
      "assets": [
        {"name": "GE-Proton11-1.sha512sum", "browser_download_url": "https://example.com/GE-Proton11-1.sha512sum", "size": 151},
        {"name": "GE-Proton11-1.tar.gz", "browser_download_url": "https://example.com/GE-Proton11-1.tar.gz", "size": 554560000},
        {"name": "GE-Proton11-1-aarch64.sha512sum", "browser_download_url": "https://example.com/GE-Proton11-1-aarch64.sha512sum", "size": 159},
        {"name": "GE-Proton11-1-aarch64.tar.gz", "browser_download_url": "https://example.com/GE-Proton11-1-aarch64.tar.gz", "size": 635100000}
      ]
    }
  ])";
  auto releases = WineRunnerManager::parse_github_releases_json(WineRunner::SourceId::GEProton, json_body);
  ASSERT_EQ(releases.size(), 1u); // aarch64 & checksum assets rejected
  EXPECT_EQ(releases.at(0).version, "11-1");
  EXPECT_EQ(releases.at(0).checksum_type, WineRunner::ChecksumType::Sha512);
  EXPECT_EQ(releases.at(0).checksum_url, "https://example.com/GE-Proton11-1.sha512sum");
}

TEST_F(WineRunnerTest, ParseGithubReleasesJsonInvalidJsonThrows)
{
  EXPECT_THROW(WineRunnerManager::parse_github_releases_json(WineRunner::SourceId::Kron4ekWineBuilds, "not json"), std::runtime_error);
  EXPECT_THROW(WineRunnerManager::parse_github_releases_json(WineRunner::SourceId::Kron4ekWineBuilds, "{\"message\": \"rate limited\"}"),
               std::runtime_error);
}

// Test expected_install_dir_name & derive_display_name functions

TEST_F(WineRunnerTest, ExpectedInstallDirName)
{
  auto release = WineRunnerManager::classify_kron4ek_asset("wine-11.13-staging-amd64.tar.xz");
  ASSERT_TRUE(release.has_value());
  EXPECT_EQ(WineRunnerManager::expected_install_dir_name(release.value()), "wine-11.13-staging-amd64");

  auto ge_release = WineRunnerManager::classify_geproton_asset("GE-Proton11-1.tar.gz");
  ASSERT_TRUE(ge_release.has_value());
  EXPECT_EQ(WineRunnerManager::expected_install_dir_name(ge_release.value()), "GE-Proton11-1");
}

TEST_F(WineRunnerTest, DeriveDisplayName)
{
  EXPECT_EQ(WineRunnerManager::derive_display_name("wine-11.13-amd64"), "Wine 11.13");
  EXPECT_EQ(WineRunnerManager::derive_display_name("wine-11.13-staging-amd64"), "Wine 11.13 Staging");
  EXPECT_EQ(WineRunnerManager::derive_display_name("wine-11.13-staging-tkg-amd64-wow64"), "Wine 11.13 Staging-TkG (WoW64)");
  EXPECT_EQ(WineRunnerManager::derive_display_name("wine-proton-9.0-4-amd64"), "Wine 9.0-4 Proton");
  EXPECT_EQ(WineRunnerManager::derive_display_name("GE-Proton11-1"), "GE-Proton 11-1");
  EXPECT_EQ(WineRunnerManager::derive_display_name("some-other-dir"), "some-other-dir");
}

// Test parse_checksum_file function

TEST_F(WineRunnerTest, ParseChecksumFileMultiLine)
{
  std::string content = "abc123DEF  wine-11.13-amd64.tar.xz\n"
                        "112233445566  wine-11.13-staging-amd64.tar.xz\n";
  auto digest = WineRunnerManager::parse_checksum_file(content, "wine-11.13-amd64.tar.xz");
  ASSERT_TRUE(digest.has_value());
  EXPECT_EQ(digest.value(), "abc123def"); // lowercased

  auto digest2 = WineRunnerManager::parse_checksum_file(content, "wine-11.13-staging-amd64.tar.xz");
  ASSERT_TRUE(digest2.has_value());
  EXPECT_EQ(digest2.value(), "112233445566");
}

TEST_F(WineRunnerTest, ParseChecksumFileSingleLineWithBinaryMarker)
{
  auto digest = WineRunnerManager::parse_checksum_file("deadbeef *GE-Proton11-1.tar.gz\n", "GE-Proton11-1.tar.gz");
  ASSERT_TRUE(digest.has_value());
  EXPECT_EQ(digest.value(), "deadbeef");
}

TEST_F(WineRunnerTest, ParseChecksumFileMissingAsset)
{
  EXPECT_FALSE(WineRunnerManager::parse_checksum_file("deadbeef  other-file.tar.gz\n", "GE-Proton11-1.tar.gz").has_value());
  EXPECT_FALSE(WineRunnerManager::parse_checksum_file("", "GE-Proton11-1.tar.gz").has_value());
}

// Test find_wine_bin_dir function

TEST_F(WineRunnerTest, FindWineBinDirRegularLayout)
{
  std::string runner_dir = test_dir + "/wine-11.13-amd64";
  create_fake_file(runner_dir + "/bin/wine");
  auto bin_dir = WineRunnerManager::find_wine_bin_dir(runner_dir);
  ASSERT_TRUE(bin_dir.has_value());
  EXPECT_EQ(bin_dir.value(), runner_dir + "/bin");
}

TEST_F(WineRunnerTest, FindWineBinDirProtonLayout)
{
  std::string runner_dir = test_dir + "/GE-Proton11-1";
  create_fake_file(runner_dir + "/files/bin/wine");
  auto bin_dir = WineRunnerManager::find_wine_bin_dir(runner_dir);
  ASSERT_TRUE(bin_dir.has_value());
  EXPECT_EQ(bin_dir.value(), runner_dir + "/files/bin");
}

TEST_F(WineRunnerTest, FindWineBinDirNoWineBinary)
{
  std::string runner_dir = test_dir + "/empty-dir";
  fs::create_directories(runner_dir + "/bin");
  EXPECT_FALSE(WineRunnerManager::find_wine_bin_dir(runner_dir).has_value());
}

// Test get_installed_runners function

TEST_F(WineRunnerTest, GetInstalledRunnersScansLayouts)
{
  std::string base_dir = test_dir + "/runners";
  create_fake_file(base_dir + "/wine-11.13-staging-amd64/bin/wine");
  create_fake_file(base_dir + "/wine-11.13-staging-amd64/bin/wine64");
  create_fake_file(base_dir + "/GE-Proton11-1/files/bin/wine");
  fs::create_directories(base_dir + "/broken-runner"); // No wine binary -> skipped
  create_fake_file(base_dir + "/.tmp/wine-download.tar.xz.part");
  fs::create_directories(base_dir + "/.staging-1234"); // Hidden dirs -> skipped

  auto runners = WineRunnerManager::get_installed_runners(base_dir);
  ASSERT_EQ(runners.size(), 2u);
  // Sorted reverse lexicographic: wine-* before GE-*
  EXPECT_EQ(runners.at(0).name, "wine-11.13-staging-amd64");
  EXPECT_EQ(runners.at(0).display_name, "Wine 11.13 Staging");
  EXPECT_EQ(runners.at(0).bin_dir, base_dir + "/wine-11.13-staging-amd64/bin");
  EXPECT_TRUE(runners.at(0).has_wine64);
  // Fake wine binary -> wine --version fails -> empty version (must not throw)
  EXPECT_EQ(runners.at(0).wine_version, "");
  EXPECT_EQ(runners.at(1).name, "GE-Proton11-1");
  EXPECT_EQ(runners.at(1).bin_dir, base_dir + "/GE-Proton11-1/files/bin");
  EXPECT_FALSE(runners.at(1).has_wine64);
}

TEST_F(WineRunnerTest, GetInstalledRunnersMissingDir)
{
  EXPECT_TRUE(WineRunnerManager::get_installed_runners(test_dir + "/does-not-exist").empty());
}

// Test is_runner_used_by_bottle function

TEST_F(WineRunnerTest, IsRunnerUsedByBottle)
{
  WineRunner::InstalledRunner runner;
  runner.name = "wine-11.13-amd64";
  runner.runner_dir = test_dir + "/runners/wine-11.13-amd64";
  runner.bin_dir = runner.runner_dir + "/bin";

  EXPECT_TRUE(WineRunnerManager::is_runner_used_by_bottle(runner, {runner.bin_dir}));
  EXPECT_TRUE(WineRunnerManager::is_runner_used_by_bottle(runner, {"", runner.bin_dir + "/"}));
  EXPECT_FALSE(WineRunnerManager::is_runner_used_by_bottle(runner, {"", "/usr/bin", test_dir + "/runners/wine-11.13-amd64-wow64/bin"}));
  EXPECT_FALSE(WineRunnerManager::is_runner_used_by_bottle(runner, {}));
}

// Test get_sources function

TEST_F(WineRunnerTest, GetSources)
{
  const auto& sources = WineRunnerManager::get_sources();
  ASSERT_EQ(sources.size(), 2u);
  EXPECT_EQ(sources.at(0).github_owner, "Kron4ek");
  EXPECT_EQ(sources.at(0).github_repo, "Wine-Builds");
  EXPECT_EQ(sources.at(1).github_owner, "GloriousEggroll");
  EXPECT_EQ(sources.at(1).github_repo, "proton-ge-custom");
}
