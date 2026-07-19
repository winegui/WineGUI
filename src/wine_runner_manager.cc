/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_manager.cc
 * \brief   Download & manage precompiled Wine runner builds
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
#include "wine_runner_manager.h"

#include "helper.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glibmm/checksum.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>
#include <glibmm/timer.h>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

/// Curated list of supported runner providers
static const std::vector<WineRunner::Source> RunnerSources = {
    {WineRunner::SourceId::Kron4ekWineBuilds, "Wine builds by Kron4ek", "Vanilla, Staging, Staging-TkG and Proton Wine builds", "Kron4ek",
     "Wine-Builds"},
    {WineRunner::SourceId::GEProton, "GE-Proton", "Proton build by GloriousEggroll with extra patches (successor of Wine-GE)", "GloriousEggroll",
     "proton-ge-custom"},
};

/// Per-session cache of the fetched GitHub release lists (protects against the GitHub API rate limit)
static std::map<WineRunner::SourceId, std::vector<WineRunner::Release>> release_cache;
static std::mutex release_cache_mutex;

/**
 * \brief Split a string into parts by delimiter
 */
static std::vector<std::string> split_string(const std::string& input, const char delimiter)
{
  std::vector<std::string> result;
  std::string::size_type start = 0;
  std::string::size_type end = 0;
  while ((end = input.find(delimiter, start)) != std::string::npos)
  {
    result.emplace_back(input.substr(start, end - start));
    start = end + 1;
  }
  result.emplace_back(input.substr(start));
  return result;
}

/**
 * \brief Join string parts with delimiter
 */
static std::string join_string(const std::vector<std::string>& parts, std::size_t first_index, std::size_t last_index, const char delimiter)
{
  std::string result;
  for (std::size_t i = first_index; i < last_index; ++i)
  {
    if (!result.empty())
      result += delimiter;
    result += parts.at(i);
  }
  return result;
}

/**
 * \brief Human readable name of a build variant (eg. "staging-tkg" becomes "Staging-TkG")
 */
static std::string variant_display_name(const std::string& variant)
{
  static const std::map<std::string, std::string> KnownVariantNames = {
      {"vanilla", "Vanilla"}, {"staging", "Staging"}, {"tkg", "TkG"}, {"proton", "Proton"}, {"ntsync", "NTSync"}};
  std::vector<std::string> tokens = split_string(variant, '-');
  std::vector<std::string> display_tokens;
  display_tokens.reserve(tokens.size());
  for (std::string token : tokens)
  {
    if (auto it = KnownVariantNames.find(token); it != KnownVariantNames.end())
    {
      display_tokens.emplace_back(it->second);
    }
    else
    {
      if (!token.empty())
        token[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));
      display_tokens.emplace_back(token);
    }
  }
  return join_string(display_tokens, 0, display_tokens.size(), '-');
}

/**
 * \brief Get the curated list of supported runner providers
 * \return List of runner sources
 */
// Public API (also used by the unit tests)
// cppcheck-suppress unusedFunction
const std::vector<WineRunner::Source>& WineRunnerManager::get_sources()
{
  return RunnerSources;
}

/**
 * \brief Get a single runner provider by ID
 * \param[in] source_id Source ID
 * \return Runner source
 */
const WineRunner::Source& WineRunnerManager::get_source(WineRunner::SourceId source_id)
{
  auto it =
      std::find_if(RunnerSources.begin(), RunnerSources.end(), [source_id](const WineRunner::Source& source) { return source.id == source_id; });
  if (it == RunnerSources.end())
    throw std::runtime_error("Unknown Wine runner source.");
  return *it;
}

/**
 * \brief Fetch the list of downloadable releases of a runner provider from the GitHub API.
 * The result is cached for the rest of the session (see also invalidate_release_cache()).
 * \param[in] source_id Source ID
 * \throws std::runtime_error when the list could not be fetched or parsed (eg. offline or GitHub rate limit)
 * \return List of releases, newest first
 */
std::vector<WineRunner::Release> WineRunnerManager::get_releases(WineRunner::SourceId source_id)
{
  {
    std::lock_guard<std::mutex> lock(release_cache_mutex);
    if (auto it = release_cache.find(source_id); it != release_cache.end())
      return it->second;
  }

  const WineRunner::Source& source = get_source(source_id);
  std::string url = "https://api.github.com/repos/" + source.github_owner + "/" + source.github_repo + "/releases?per_page=100";
  std::string json_body;
  try
  {
    json_body = fetch_url(url);
  }
  catch (const std::runtime_error& error)
  {
    throw std::runtime_error("Could not fetch the release list of " + source.display_name +
                             " from GitHub.\n\nEither you are offline or the GitHub API rate limit was reached (max 60 requests per hour).\nAlready "
                             "installed runners keep working. Please, try again later.");
  }
  std::vector<WineRunner::Release> releases = parse_github_releases_json(source_id, json_body);

  std::lock_guard<std::mutex> lock(release_cache_mutex);
  release_cache[source_id] = releases;
  return releases;
}

/**
 * \brief Clear the cached release lists, so the next get_releases() call fetches a fresh list
 */
void WineRunnerManager::invalidate_release_cache()
{
  std::lock_guard<std::mutex> lock(release_cache_mutex);
  release_cache.clear();
}

/**
 * \brief Parse a GitHub releases API JSON response into a list of downloadable releases.
 * Draft & pre-releases are skipped, just like assets that are not usable Wine build archives
 * (wrong architecture, checksum files, etc).
 * \param[in] source_id Source ID the JSON belongs to
 * \param[in] json_body Raw JSON body of the GitHub releases API response
 * \throws std::runtime_error when the JSON could not be parsed
 * \return List of releases (in the same order as the API response, thus newest first)
 */
std::vector<WineRunner::Release> WineRunnerManager::parse_github_releases_json(WineRunner::SourceId source_id, const std::string& json_body)
{
  std::vector<WineRunner::Release> releases;
  nlohmann::json json;
  try
  {
    json = nlohmann::json::parse(json_body);
  }
  catch (const nlohmann::json::parse_error& error)
  {
    throw std::runtime_error("Could not parse the GitHub API response (invalid JSON).");
  }
  if (!json.is_array())
  {
    throw std::runtime_error("Unexpected GitHub API response (expected a list of releases).");
  }

  for (const auto& release_json : json)
  {
    if (!release_json.is_object())
      continue;
    if (release_json.value("draft", false) || release_json.value("prerelease", false))
      continue;
    std::string tag_name = release_json.value("tag_name", "");
    std::string published_at = release_json.value("published_at", "");
    if (!release_json.contains("assets") || !release_json["assets"].is_array())
      continue;

    // First pass over the assets: collect the checksum file URLs
    // Kron4ek publishes a single "sha256sums.txt" per release, GE-Proton a "<name>.sha512sum" per archive
    std::string sha256sums_url;
    std::map<std::string, std::string> sha512sum_urls;
    for (const auto& asset : release_json["assets"])
    {
      std::string asset_name = asset.value("name", "");
      if (source_id == WineRunner::SourceId::Kron4ekWineBuilds && asset_name == "sha256sums.txt")
      {
        sha256sums_url = asset.value("browser_download_url", "");
      }
      else if (source_id == WineRunner::SourceId::GEProton && asset_name.ends_with(".sha512sum"))
      {
        std::string base_name = asset_name.substr(0, asset_name.size() - std::string(".sha512sum").size());
        sha512sum_urls[base_name] = asset.value("browser_download_url", "");
      }
    }

    // Second pass: classify the Wine build archives
    for (const auto& asset : release_json["assets"])
    {
      std::string asset_name = asset.value("name", "");
      std::optional<WineRunner::Release> release =
          (source_id == WineRunner::SourceId::Kron4ekWineBuilds) ? classify_kron4ek_asset(asset_name) : classify_geproton_asset(asset_name);
      if (!release.has_value())
        continue;
      release->tag_name = tag_name;
      release->published_at = published_at;
      release->download_url = asset.value("browser_download_url", "");
      release->size_bytes = asset.value("size", static_cast<std::uint64_t>(0));
      if (source_id == WineRunner::SourceId::Kron4ekWineBuilds && !sha256sums_url.empty())
      {
        release->checksum_type = WineRunner::ChecksumType::Sha256;
        release->checksum_url = sha256sums_url;
      }
      else if (source_id == WineRunner::SourceId::GEProton)
      {
        std::string base_name = expected_install_dir_name(release.value());
        if (auto it = sha512sum_urls.find(base_name); it != sha512sum_urls.end())
        {
          release->checksum_type = WineRunner::ChecksumType::Sha512;
          release->checksum_url = it->second;
        }
      }
      if (release->download_url.empty())
        continue;
      releases.emplace_back(release.value());
    }
  }
  return releases;
}

/**
 * \brief Classify a Kron4ek Wine-Builds release asset file name.
 * Only 64-bit (amd64) tar.xz Wine build archives are accepted.
 * Examples: "wine-11.13-staging-amd64.tar.xz", "wine-11.13-staging-tkg-amd64-wow64.tar.xz", "wine-proton-9.0-4-amd64.tar.xz".
 * \param[in] asset_name Asset file name
 * \return Partially filled Release (variant/version/wow64/asset_name) or nullopt when the asset is not usable
 */
std::optional<WineRunner::Release> WineRunnerManager::classify_kron4ek_asset(const std::string& asset_name)
{
  static const std::string prefix = "wine-";
  static const std::string suffix = ".tar.xz";
  if (!asset_name.starts_with(prefix) || !asset_name.ends_with(suffix))
    return std::nullopt;
  if (!is_safe_file_name(asset_name))
    return std::nullopt;

  std::string base = asset_name.substr(prefix.size(), asset_name.size() - prefix.size() - suffix.size());
  std::vector<std::string> tokens = split_string(base, '-');

  // Only accept the amd64 builds, reject other architectures
  std::size_t amd64_index = tokens.size();
  for (std::size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokens.at(i) == "x86" || tokens.at(i) == "aarch64" || tokens.at(i) == "arm64ec")
      return std::nullopt;
    if (tokens.at(i) == "amd64" && amd64_index == tokens.size())
      amd64_index = i;
  }
  if (amd64_index == tokens.size() || amd64_index == 0)
    return std::nullopt;

  // WoW64 builds are marked with a "wow64" token after "amd64" (they ship no separate wine64 binary)
  bool wow64 = false;
  for (std::size_t i = amd64_index + 1; i < tokens.size(); ++i)
  {
    if (tokens.at(i) == "wow64")
      wow64 = true;
  }

  WineRunner::Release release;
  release.source = WineRunner::SourceId::Kron4ekWineBuilds;
  release.asset_name = asset_name;
  release.wow64 = wow64;
  if (tokens.at(0) == "proton")
  {
    // Proton builds have the variant token in front of the version (eg. "wine-proton-9.0-4-amd64.tar.xz")
    release.variant = "proton";
    release.version = join_string(tokens, 1, amd64_index, '-');
  }
  else
  {
    release.version = tokens.at(0);
    release.variant = (amd64_index > 1) ? join_string(tokens, 1, amd64_index, '-') : "vanilla";
  }
  // Version sanity check: must start with a digit
  if (release.version.empty() || std::isdigit(static_cast<unsigned char>(release.version[0])) == 0)
    return std::nullopt;
  return release;
}

/**
 * \brief Classify a GE-Proton release asset file name.
 * Only the x86_64 tar.gz archives are accepted (eg. "GE-Proton11-1.tar.gz", rejecting the "-aarch64" variants).
 * \param[in] asset_name Asset file name
 * \return Partially filled Release (variant/version/asset_name) or nullopt when the asset is not usable
 */
std::optional<WineRunner::Release> WineRunnerManager::classify_geproton_asset(const std::string& asset_name)
{
  static const std::string prefix = "GE-Proton";
  static const std::string suffix = ".tar.gz";
  if (!asset_name.starts_with(prefix) || !asset_name.ends_with(suffix))
    return std::nullopt;
  if (!is_safe_file_name(asset_name))
    return std::nullopt;

  std::string version = asset_name.substr(prefix.size(), asset_name.size() - prefix.size() - suffix.size());
  // Version must be digits/dots/dashes only (this rejects eg. "GE-Proton11-1-aarch64.tar.gz")
  if (version.empty() || std::isdigit(static_cast<unsigned char>(version[0])) == 0)
    return std::nullopt;
  if (!std::all_of(version.begin(), version.end(),
                   [](unsigned char character) { return std::isdigit(character) != 0 || character == '-' || character == '.'; }))
    return std::nullopt;

  WineRunner::Release release;
  release.source = WineRunner::SourceId::GEProton;
  release.asset_name = asset_name;
  release.variant = "GE-Proton";
  release.version = version;
  return release;
}

/**
 * \brief Directory name the release archive is expected to extract to (also used as install directory name)
 * \param[in] release Release
 * \return Directory name (asset file name without the .tar.xz / .tar.gz extension)
 */
std::string WineRunnerManager::expected_install_dir_name(const WineRunner::Release& release)
{
  std::string name = release.asset_name;
  for (const std::string& suffix : {std::string(".tar.xz"), std::string(".tar.gz")})
  {
    if (name.ends_with(suffix))
    {
      name.resize(name.size() - suffix.size());
      break;
    }
  }
  return name;
}

/**
 * \brief Derive a human readable display name from a runner directory name
 * (eg. "wine-11.13-staging-amd64" becomes "Wine 11.13 Staging")
 * \param[in] runner_dir_name Runner directory name
 * \return Human readable display name
 */
std::string WineRunnerManager::derive_display_name(const std::string& runner_dir_name)
{
  if (runner_dir_name.starts_with("GE-Proton"))
  {
    return "GE-Proton " + runner_dir_name.substr(std::string("GE-Proton").size());
  }
  if (runner_dir_name.starts_with("wine-"))
  {
    // Reuse the asset classifier by re-adding the archive extension
    std::optional<WineRunner::Release> release = classify_kron4ek_asset(runner_dir_name + ".tar.xz");
    if (release.has_value())
    {
      std::string display_name = "Wine " + release->version;
      if (release->variant != "vanilla")
        display_name += " " + variant_display_name(release->variant);
      if (release->wow64)
        display_name += " (WoW64)";
      return display_name;
    }
  }
  return runner_dir_name;
}

/**
 * \brief Locate the directory containing the wine binary within a runner directory.
 * Supports the regular layout (bin/wine) and the Proton layout (files/bin/wine).
 * \param[in] runner_dir Absolute path of the runner directory
 * \return Absolute path of the binary directory or nullopt when no wine binary was found
 */
std::optional<std::string> WineRunnerManager::find_wine_bin_dir(const std::string& runner_dir)
{
  const std::vector<fs::path> bin_dir_candidates = {fs::path(runner_dir) / "bin", fs::path(runner_dir) / "files" / "bin"};
  for (const fs::path& bin_dir : bin_dir_candidates)
  {
    std::error_code error_code;
    if (fs::is_regular_file(bin_dir / "wine", error_code) || fs::is_regular_file(bin_dir / "wine64", error_code))
      return bin_dir.string();
  }
  return std::nullopt;
}

/**
 * \brief The WineGUI Wine runners directory (~/.local/share/winegui/runners)
 * \return Absolute path of the runners directory
 */
std::string WineRunnerManager::get_runners_dir()
{
  return Glib::build_path(G_DIR_SEPARATOR_S, std::vector<std::string>{Glib::get_user_data_dir(), "winegui", "runners"});
}

/**
 * \brief List the installed Wine runners in the default runners directory
 * \return List of installed runners (never throws, returns what it finds)
 */
std::vector<WineRunner::InstalledRunner> WineRunnerManager::get_installed_runners()
{
  return get_installed_runners(get_runners_dir());
}

/**
 * \brief List the installed Wine runners in the given base directory
 * \param[in] runners_base_dir Base directory to scan (parameter mainly exists for unit testing)
 * \return List of installed runners, newest name first (never throws, returns what it finds)
 */
std::vector<WineRunner::InstalledRunner> WineRunnerManager::get_installed_runners(const std::string& runners_base_dir)
{
  std::vector<WineRunner::InstalledRunner> runners;
  std::error_code error_code;
  if (!fs::is_directory(runners_base_dir, error_code))
    return runners;
  try
  {
    Glib::Dir dir(runners_base_dir);
    for (const auto& entry_name : dir)
    {
      // Skip hidden entries (incl. the .tmp & .staging-* transient directories)
      if (entry_name.empty() || entry_name[0] == '.')
        continue;
      std::string runner_dir = Glib::build_filename(runners_base_dir, entry_name);
      if (!fs::is_directory(runner_dir, error_code))
        continue;
      std::optional<std::string> bin_dir = find_wine_bin_dir(runner_dir);
      if (!bin_dir.has_value())
        continue;
      WineRunner::InstalledRunner runner;
      runner.name = entry_name;
      runner.display_name = derive_display_name(entry_name);
      runner.runner_dir = runner_dir;
      runner.bin_dir = bin_dir.value();
      runner.has_wine64 = fs::is_regular_file(fs::path(runner.bin_dir) / "wine64", error_code);
      // WoW64 (64-bit-only) is reliably signalled only by the "-wow64" token in the archive/directory name,
      // which is preserved as the runner directory name. Neither a missing wine64 nor a missing i386-unix tree
      // is a reliable signal (eg. Proton WoW64 ships both yet still refuses a 32-bit prefix).
      std::optional<WineRunner::Release> classified = classify_kron4ek_asset(entry_name + ".tar.xz");
      runner.wow64 = classified.has_value() && classified->wow64;
      try
      {
        // Request the 64-bit binary, get_wine_executable_location() falls back to the unified wine binary for WoW64 builds
        runner.wine_version = Helper::get_wine_version(true, "", runner.bin_dir);
      }
      catch (const std::runtime_error& version_error)
      {
        runner.wine_version = "";
      }
      runners.emplace_back(runner);
    }
  }
  catch (const Glib::FileError& file_error)
  {
    std::cout << "Error: Could not read the Wine runners directory: " << file_error.what() << std::endl;
  }
  // Sort newest name first (reverse lexicographic is good enough for version-suffixed directory names)
  std::sort(runners.begin(), runners.end(),
            [](const WineRunner::InstalledRunner& a, const WineRunner::InstalledRunner& b) { return a.name > b.name; });
  return runners;
}

/**
 * \brief Normalize a filesystem path for comparison (resolves symlinks & removes trailing slashes where possible)
 */
static std::string normalize_path(const std::string& path)
{
  std::error_code error_code;
  fs::path canonical = fs::weakly_canonical(fs::path(path), error_code);
  if (error_code)
    canonical = fs::path(path).lexically_normal();
  std::string result = canonical.string();
  while (result.size() > 1 && result.back() == G_DIR_SEPARATOR)
    result.pop_back();
  return result;
}

/**
 * \brief Find the installed runner a bottle wine binary path belongs to (if any)
 * \param[in] wine_bin_path Bottle wine binary directory path
 * \return The installed runner or nullopt when the path is empty or not an installed runner
 */
std::optional<WineRunner::InstalledRunner> WineRunnerManager::find_runner_by_bin_dir(const std::string& wine_bin_path)
{
  if (wine_bin_path.empty())
    return std::nullopt;
  std::string normalized_bin_path = normalize_path(wine_bin_path);
  std::vector<WineRunner::InstalledRunner> runners = get_installed_runners();
  auto it = std::find_if(runners.begin(), runners.end(), [&normalized_bin_path](const WineRunner::InstalledRunner& runner)
                         { return normalize_path(runner.bin_dir) == normalized_bin_path; });
  if (it == runners.end())
    return std::nullopt;
  return *it;
}

/**
 * \brief Check whether the release is already installed (its install directory exists)
 * \param[in] release Release
 * \return True when already installed
 */
bool WineRunnerManager::is_installed(const WineRunner::Release& release)
{
  std::error_code error_code;
  return fs::is_directory(fs::path(get_runners_dir()) / expected_install_dir_name(release), error_code);
}

/**
 * \brief Remove an installed Wine runner from disk.
 * Refuses to remove anything outside the WineGUI runners directory.
 * \param[in] runner Installed runner
 * \throws std::runtime_error on failure or on a path-safety violation
 */
void WineRunnerManager::remove_runner(const WineRunner::InstalledRunner& runner)
{
  if (runner.name.empty() || runner.name[0] == '.' || !is_safe_file_name(runner.name))
  {
    throw std::runtime_error("Refusing to remove the Wine runner: invalid runner name.");
  }
  fs::path runners_dir = normalize_path(get_runners_dir());
  fs::path target_dir = normalize_path(runner.runner_dir);
  if (target_dir.parent_path() != runners_dir || target_dir == runners_dir)
  {
    throw std::runtime_error("Refusing to remove the Wine runner: it's located outside the WineGUI runners directory.");
  }
  std::error_code error_code;
  fs::remove_all(target_dir, error_code);
  if (error_code)
  {
    throw std::runtime_error("Could not remove the Wine runner: " + error_code.message());
  }
}

/**
 * \brief Check whether any bottle still uses the given runner (via its stored wine binary path)
 * \param[in] runner Installed runner
 * \param[in] bottle_wine_bin_paths Wine binary paths of all bottles
 * \return True when at least one bottle points into the runner directory
 */
bool WineRunnerManager::is_runner_used_by_bottle(const WineRunner::InstalledRunner& runner, const std::vector<std::string>& bottle_wine_bin_paths)
{
  std::string runner_dir = normalize_path(runner.runner_dir);
  return std::any_of(bottle_wine_bin_paths.begin(), bottle_wine_bin_paths.end(),
                     [&runner_dir](const std::string& bottle_path)
                     {
                       if (bottle_path.empty())
                         return false;
                       std::string normalized_bottle_path = normalize_path(bottle_path);
                       return normalized_bottle_path == runner_dir || normalized_bottle_path.starts_with(runner_dir + G_DIR_SEPARATOR_S);
                     });
}

/**
 * \brief Parse a checksum file (sha256sums.txt / *.sha512sum format: "<hex digest>  <file name>" per line)
 * \param[in] file_content Content of the checksum file
 * \param[in] asset_name Archive file name to look up
 * \return Lowercase hex digest of the asset or nullopt when the asset was not found in the file
 */
std::optional<std::string> WineRunnerManager::parse_checksum_file(const std::string& file_content, const std::string& asset_name)
{
  for (const std::string& line : split_string(file_content, '\n'))
  {
    std::istringstream line_stream(line);
    std::string digest;
    std::string file_name;
    if (!(line_stream >> digest >> file_name))
      continue;
    // Support the binary-mode marker (eg. "*wine-11.13-amd64.tar.xz") and paths (basename match)
    if (!file_name.empty() && file_name[0] == '*')
      file_name = file_name.substr(1);
    file_name = Glib::path_get_basename(file_name);
    if (file_name != asset_name)
      continue;
    std::transform(digest.begin(), digest.end(), digest.begin(), [](unsigned char character) { return std::tolower(character); });
    return digest;
  }
  return std::nullopt;
}

/**
 * \brief Download & install a Wine runner release into the WineGUI runners directory.
 * Downloads the archive, verifies the published checksum, extracts to a staging directory,
 * validates the archive layout and finally moves the runner into place (atomic rename).
 * \param[in] release Release to install
 * \param[in] progress_cb Progress callback (bytes done, bytes total), invoked from the calling thread; may be empty
 * \param[in] phase_cb Phase change callback, invoked from the calling thread; may be empty
 * \param[in] cancel Cancellation flag (polled during the download and between phases)
 * \throws std::runtime_error on failure
 * \return True on success, false when cancelled
 */
bool WineRunnerManager::download_and_install(const WineRunner::Release& release,
                                             const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb,
                                             const std::function<void(WineRunner::InstallPhase)>& phase_cb,
                                             const std::atomic<bool>& cancel)
{
  // Never use unvalidated names from the GitHub API in filesystem paths
  if (!is_safe_file_name(release.asset_name))
  {
    throw std::runtime_error("Refusing to install: unexpected archive file name.");
  }
  std::string runners_dir = get_runners_dir();
  std::error_code error_code;
  fs::create_directories(runners_dir, error_code);
  if (error_code)
  {
    throw std::runtime_error("Could not create the Wine runners directory: " + runners_dir);
  }
  sweep_leftover_temp_dirs(runners_dir);
  if (is_installed(release))
  {
    throw std::runtime_error("This Wine runner version is already installed.");
  }

  std::string tmp_dir = Glib::build_filename(runners_dir, ".tmp");
  fs::create_directories(tmp_dir, error_code);
  if (error_code)
  {
    throw std::runtime_error("Could not create the temporary download directory: " + tmp_dir);
  }
  std::string archive_path = Glib::build_filename(tmp_dir, release.asset_name);
  std::string staging_dir = Glib::build_filename(runners_dir, ".staging-" + std::to_string(getpid()));

  // Remove the transient download & staging files again in every exit path (success, cancel & error)
  struct TransientFilesCleanup
  {
    std::vector<std::string> paths;
    ~TransientFilesCleanup()
    {
      for (const auto& path : paths)
      {
        std::error_code cleanup_error;
        fs::remove_all(path, cleanup_error);
      }
    }
  } cleanup;
  cleanup.paths = {archive_path, archive_path + ".part", staging_dir};

  if (phase_cb)
    phase_cb(WineRunner::InstallPhase::Downloading);
  if (!download_file(release.download_url, archive_path + ".part", release.size_bytes, progress_cb, cancel))
  {
    return false; // Cancelled
  }
  fs::rename(archive_path + ".part", archive_path, error_code);
  if (error_code)
  {
    throw std::runtime_error("Could not move the downloaded archive: " + error_code.message());
  }
  if (cancel.load())
    return false;

  if (phase_cb)
    phase_cb(WineRunner::InstallPhase::Verifying);
  verify_archive_checksum(release, archive_path);
  if (cancel.load())
    return false;

  if (phase_cb)
    phase_cb(WineRunner::InstallPhase::Extracting);
  fs::create_directories(staging_dir, error_code);
  if (error_code)
  {
    throw std::runtime_error("Could not create the staging directory: " + staging_dir);
  }
  extract_archive(archive_path, staging_dir);
  if (cancel.load())
    return false;

  // Validate the archive layout: expect exactly one top-level directory with a safe name, containing a wine binary
  std::vector<std::string> top_level_entries;
  try
  {
    Glib::Dir staging(staging_dir);
    top_level_entries.assign(staging.begin(), staging.end());
  }
  catch (const Glib::FileError& file_error)
  {
    throw std::runtime_error("Could not read the staging directory: " + std::string(file_error.what()));
  }
  if (top_level_entries.size() != 1)
  {
    throw std::runtime_error("Unsupported archive layout (expected a single top-level directory).");
  }
  std::string top_dir_name = top_level_entries.at(0);
  if (!is_safe_file_name(top_dir_name))
  {
    throw std::runtime_error("Unsupported archive layout (unexpected top-level directory name).");
  }
  std::string extracted_dir = Glib::build_filename(staging_dir, top_dir_name);
  if (!fs::is_directory(extracted_dir, error_code))
  {
    throw std::runtime_error("Unsupported archive layout (expected a single top-level directory).");
  }
  if (!find_wine_bin_dir(extracted_dir).has_value())
  {
    throw std::runtime_error("Unsupported archive layout (no wine binary found within the archive).");
  }

  // Move the runner into place (atomic rename, staging is on the same filesystem)
  std::string target_dir = Glib::build_filename(runners_dir, top_dir_name);
  if (fs::exists(target_dir, error_code))
  {
    throw std::runtime_error("This Wine runner is already installed (directory already exists): " + target_dir);
  }
  fs::rename(extracted_dir, target_dir, error_code);
  if (error_code)
  {
    throw std::runtime_error("Could not move the Wine runner into place: " + error_code.message());
  }
  return true;
}

/*************************************************************
 * Private member functions                                  *
 *************************************************************/

/**
 * \brief Fetch a (small) file over HTTPS into memory, using a wget subprocess (no shell involved)
 * \param[in] url URL to fetch
 * \throws std::runtime_error on failure
 * \return Response body
 */
std::string WineRunnerManager::fetch_url(const std::string& url)
{
  std::string standard_output;
  std::string standard_error;
  int wait_status = 0;
  try
  {
    const std::vector<std::string> argv{"wget", "--quiet", "--timeout=30", "--tries=2", "--output-document=-", url};
    Glib::spawn_sync("", argv, Glib::SpawnFlags::SEARCH_PATH, {}, &standard_output, &standard_error, &wait_status);
  }
  catch (const Glib::Error& error)
  {
    throw std::runtime_error("Could not start wget: " + std::string(error.what()));
  }
  if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
  {
    throw std::runtime_error("Could not fetch: " + url);
  }
  return standard_output;
}

/**
 * \brief Download a file to disk using a wget subprocess (no shell involved), with progress & cancellation support.
 * Progress is reported by polling the growing destination file size (roughly 4 times per second).
 * \param[in] url URL to download
 * \param[in] dest_path Destination file path
 * \param[in] expected_size Expected file size in bytes (from the GitHub API, for the progress callback)
 * \param[in] progress_cb Progress callback (bytes done, bytes total); may be empty
 * \param[in] cancel Cancellation flag (polled); on cancel the wget subprocess is terminated
 * \throws std::runtime_error on failure
 * \return True on success, false when cancelled
 */
bool WineRunnerManager::download_file(const std::string& url,
                                      const std::string& dest_path,
                                      std::uint64_t expected_size,
                                      const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb,
                                      const std::atomic<bool>& cancel)
{
  Glib::Pid pid = 0;
  try
  {
    const std::vector<std::string> argv{"wget", "--quiet", "--timeout=30", "--output-document=" + dest_path, url};
    Glib::spawn_async("", argv, Glib::SpawnFlags::SEARCH_PATH | Glib::SpawnFlags::DO_NOT_REAP_CHILD, {}, &pid);
  }
  catch (const Glib::Error& error)
  {
    throw std::runtime_error("Could not start wget: " + std::string(error.what()));
  }

  bool cancelled = false;
  int wait_status = 0;
  while (true)
  {
    pid_t wait_result = waitpid(pid, &wait_status, WNOHANG);
    if (wait_result != 0)
      break; // Process exited (or waitpid failed)
    if (cancel.load())
    {
      kill(pid, SIGTERM);
      waitpid(pid, &wait_status, 0);
      cancelled = true;
      break;
    }
    if (progress_cb)
    {
      std::error_code error_code;
      std::uint64_t downloaded_size = fs::file_size(dest_path, error_code);
      if (!error_code)
        progress_cb(downloaded_size, expected_size);
    }
    Glib::usleep(250000); // 250 ms
  }
  Glib::spawn_close_pid(pid);

  if (cancelled)
  {
    std::error_code error_code;
    fs::remove(dest_path, error_code);
    return false;
  }
  if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
  {
    std::error_code error_code;
    fs::remove(dest_path, error_code);
    throw std::runtime_error("Download failed. Are you still online?\n\nURL: " + url);
  }
  if (progress_cb)
    progress_cb(expected_size, expected_size);
  return true;
}

/**
 * \brief Extract a tar archive (tar.xz / tar.gz) into the given directory, using a tar subprocess (no shell involved).
 * GNU tar refuses absolute paths & ".." members by default, which keeps the extraction within the staging directory.
 * \param[in] archive_path Archive file path
 * \param[in] staging_dir Directory to extract into
 * \throws std::runtime_error on failure
 */
void WineRunnerManager::extract_archive(const std::string& archive_path, const std::string& staging_dir)
{
  std::string standard_output;
  std::string standard_error;
  int wait_status = 0;
  try
  {
    const std::vector<std::string> argv{"tar", "-xf", archive_path, "-C", staging_dir, "--no-same-owner"};
    Glib::spawn_sync("", argv, Glib::SpawnFlags::SEARCH_PATH, {}, &standard_output, &standard_error, &wait_status);
  }
  catch (const Glib::Error& error)
  {
    throw std::runtime_error("Could not start tar: " + std::string(error.what()));
  }
  if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
  {
    throw std::runtime_error("Could not extract the archive.\n\n" + standard_error);
  }
}

/**
 * \brief Compute the checksum of a file (streamed in 1 MiB chunks)
 * \param[in] file_path File path
 * \param[in] checksum_type Checksum type (Sha256 or Sha512)
 * \throws std::runtime_error on failure
 * \return Lowercase hex digest
 */
std::string WineRunnerManager::compute_checksum(const std::string& file_path, WineRunner::ChecksumType checksum_type)
{
  Glib::Checksum checksum((checksum_type == WineRunner::ChecksumType::Sha512) ? Glib::Checksum::Type::SHA512 : Glib::Checksum::Type::SHA256);
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open())
  {
    throw std::runtime_error("Could not open the downloaded archive for checksum verification.");
  }
  std::vector<char> buffer(1024 * 1024);
  while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || file.gcount() > 0)
  {
    checksum.update(reinterpret_cast<const guchar*>(buffer.data()), static_cast<gssize>(file.gcount()));
  }
  return checksum.get_string();
}

/**
 * \brief Verify the downloaded archive against the checksum published by the runner source
 * \param[in] release Release the archive belongs to
 * \param[in] archive_path Downloaded archive file path
 * \throws std::runtime_error on a checksum mismatch (the archive is removed by the caller's cleanup)
 */
void WineRunnerManager::verify_archive_checksum(const WineRunner::Release& release, const std::string& archive_path)
{
  if (release.checksum_type == WineRunner::ChecksumType::None || release.checksum_url.empty())
  {
    // Both supported sources publish checksums for every release nowadays, only very old releases lack them
    std::cout << "WARN: No checksum published for " << release.asset_name << ", skipping the verification." << std::endl;
    return;
  }
  std::string checksum_file_content = fetch_url(release.checksum_url);
  std::optional<std::string> expected_digest = parse_checksum_file(checksum_file_content, release.asset_name);
  if (!expected_digest.has_value())
  {
    throw std::runtime_error("Could not verify the download: the published checksum file does not list the archive.");
  }
  std::string actual_digest = compute_checksum(archive_path, release.checksum_type);
  if (actual_digest != expected_digest.value())
  {
    throw std::runtime_error("Checksum verification of the downloaded archive failed!\n\nThe download is possibly corrupted (or tampered with). "
                             "Please, try again.");
  }
}

/**
 * \brief Remove leftover transient directories/files from a previously crashed or killed session
 * \param[in] runners_dir Runners directory
 */
void WineRunnerManager::sweep_leftover_temp_dirs(const std::string& runners_dir)
{
  try
  {
    Glib::Dir dir(runners_dir);
    for (const auto& entry_name : dir)
    {
      if (entry_name == ".tmp" || entry_name.starts_with(".staging-"))
      {
        std::error_code error_code;
        fs::remove_all(fs::path(runners_dir) / entry_name, error_code);
      }
    }
  }
  catch (const Glib::FileError& file_error)
  {
    // Non-critical
  }
}

/**
 * \brief Validate a file/directory name before using it in a filesystem path.
 * Only allows [A-Za-z0-9._+-] characters and rejects hidden names (leading dot).
 * \param[in] name File or directory name
 * \return True when the name is safe to use
 */
bool WineRunnerManager::is_safe_file_name(const std::string& name)
{
  if (name.empty() || name[0] == '.')
    return false;
  return std::all_of(name.begin(), name.end(), [](unsigned char character)
                     { return std::isalnum(character) != 0 || character == '.' || character == '_' || character == '+' || character == '-'; });
}
