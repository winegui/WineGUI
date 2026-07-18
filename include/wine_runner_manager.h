/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_manager.h
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
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "wine_runner_types.h"

/**
 * \class WineRunnerManager
 * \brief Download & manage precompiled Wine runner builds.
 * All methods are blocking and thread-safe; call the network/install methods from a worker thread.
 * Errors are reported by throwing std::runtime_error (same style as the Helper class).
 */
class WineRunnerManager
{
public:
  // -- Sources (static curated data, no I/O)
  static const std::vector<WineRunner::Source>& get_sources();
  static const WineRunner::Source& get_source(WineRunner::SourceId source_id);

  // -- Release listing (network; cached per session; throws std::runtime_error)
  static std::vector<WineRunner::Release> get_releases(WineRunner::SourceId source_id);
  static void invalidate_release_cache();

  // -- Install (network + tar subprocess; throws std::runtime_error)
  static bool download_and_install(const WineRunner::Release& release,
                                   const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb,
                                   const std::function<void(WineRunner::InstallPhase)>& phase_cb,
                                   const std::atomic<bool>& cancel);

  // -- Local enumeration (filesystem only; never throws, returns what it finds)
  static std::vector<WineRunner::InstalledRunner> get_installed_runners();
  static std::vector<WineRunner::InstalledRunner> get_installed_runners(const std::string& runners_base_dir);
  static std::optional<WineRunner::InstalledRunner> find_runner_by_bin_dir(const std::string& wine_bin_path);
  static bool is_installed(const WineRunner::Release& release);
  static std::string get_runners_dir();

  // -- Removal (throws std::runtime_error on failure or on a path-safety violation)
  static void remove_runner(const WineRunner::InstalledRunner& runner);
  static bool is_runner_used_by_bottle(const WineRunner::InstalledRunner& runner, const std::vector<std::string>& bottle_wine_bin_paths);

  // -- Pure helpers, public for unit testing
  static std::vector<WineRunner::Release> parse_github_releases_json(WineRunner::SourceId source_id, const std::string& json_body);
  static std::optional<WineRunner::Release> classify_kron4ek_asset(const std::string& asset_name);
  static std::optional<WineRunner::Release> classify_geproton_asset(const std::string& asset_name);
  static std::string expected_install_dir_name(const WineRunner::Release& release);
  static std::string derive_display_name(const std::string& runner_dir_name);
  static std::optional<std::string> find_wine_bin_dir(const std::string& runner_dir);
  static std::optional<std::string> parse_checksum_file(const std::string& file_content, const std::string& asset_name);

private:
  WineRunnerManager() = delete;

  static std::string fetch_url(const std::string& url);
  static bool download_file(const std::string& url,
                            const std::string& dest_path,
                            std::uint64_t expected_size,
                            const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb,
                            const std::atomic<bool>& cancel);
  static void extract_archive(const std::string& archive_path, const std::string& staging_dir);
  static std::string compute_checksum(const std::string& file_path, WineRunner::ChecksumType checksum_type);
  static void verify_archive_checksum(const WineRunner::Release& release, const std::string& archive_path);
  static void sweep_leftover_temp_dirs(const std::string& runners_dir);
  static bool is_safe_file_name(const std::string& name);
};
