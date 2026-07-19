/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_types.h
 * \brief   Data types for downloadable, precompiled Wine runner builds
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

#include <cstdint>
#include <string>

namespace WineRunner
{
  /**
   * \enum SourceId
   * \brief Supported providers of precompiled Wine runner builds
   */
  enum class SourceId
  {
    Kron4ekWineBuilds, /*!< github.com/Kron4ek/Wine-Builds (Vanilla, Staging, Staging-TkG & Proton builds) */
    GEProton,          /*!< github.com/GloriousEggroll/proton-ge-custom (GE-Proton builds) */
  };

  /**
   * \enum ChecksumType
   * \brief Type of checksum published by the runner source (used to verify the downloaded archive)
   */
  enum class ChecksumType
  {
    None,   /*!< Release without a published checksum */
    Sha256, /*!< Kron4ek: per-release "sha256sums.txt" asset */
    Sha512, /*!< GE-Proton: per-asset "<name>.sha512sum" asset */
  };

  /**
   * \struct Source
   * \brief Curated, compile-time information of a supported runner provider
   */
  struct Source
  {
    SourceId id = SourceId::Kron4ekWineBuilds; /*!< Source ID */
    std::string display_name;                  /*!< Human readable source name */
    std::string description;                   /*!< One-line description for the UI */
    std::string github_owner;                  /*!< GitHub repository owner */
    std::string github_repo;                   /*!< GitHub repository name */
  };

  /**
   * \struct Release
   * \brief One downloadable Wine build archive (a single release tag can yield multiple entries, one per variant)
   */
  struct Release
  {
    SourceId source = SourceId::Kron4ekWineBuilds;   /*!< Source this release belongs to */
    std::string tag_name;                            /*!< GitHub release tag, eg. "11.13" or "GE-Proton11-1" */
    std::string version;                             /*!< Parsed version for display, eg. "11.13" or "11-1" */
    std::string variant;                             /*!< Build variant: "vanilla", "staging", "staging-tkg", "proton" or "GE-Proton" */
    bool wow64 = false;                              /*!< True for WoW64 builds (no separate wine64 binary present) */
    std::string asset_name;                          /*!< Archive file name, eg. "wine-11.13-staging-amd64.tar.xz" */
    std::string download_url;                        /*!< Direct download URL of the archive */
    std::uint64_t size_bytes = 0;                    /*!< Archive size in bytes (for download progress) */
    std::string published_at;                        /*!< ISO-8601 publish date from the GitHub API */
    ChecksumType checksum_type = ChecksumType::None; /*!< Type of published checksum */
    std::string checksum_url;                        /*!< Download URL of the checksum file (empty if none) */
  };

  /**
   * \struct InstalledRunner
   * \brief A Wine runner build present in the WineGUI runners directory
   */
  struct InstalledRunner
  {
    std::string name;         /*!< Directory name (unique key), eg. "wine-11.13-staging-amd64" */
    std::string display_name; /*!< Human readable name, eg. "Wine 11.13 Staging" */
    std::string runner_dir;   /*!< Absolute path of the runner directory (deleted on remove) */
    std::string bin_dir;      /*!< Absolute directory containing the wine binary (value for the bottle wine_bin_path) */
    std::string wine_version; /*!< Output of "wine --version" (empty when it failed) */
    bool has_wine64 = false;  /*!< True when a separate wine64 binary exists (used by the binary resolver, not a 32/64-bit capability signal) */
    bool wow64 = false;       /*!< True for WoW64 builds (64-bit-only prefixes; derived from the "-wow64" asset token in the runner directory name) */
  };

  /**
   * \enum InstallPhase
   * \brief Current phase of a runner install operation
   */
  enum class InstallPhase
  {
    Idle,        /*!< No install running */
    Downloading, /*!< Downloading the archive */
    Verifying,   /*!< Verifying the archive checksum */
    Extracting,  /*!< Extracting the archive */
  };

  /**
   * \enum InstallStatus
   * \brief Final status of a runner install operation
   */
  enum class InstallStatus
  {
    Success,   /*!< Runner installed successfully */
    Cancelled, /*!< Install was cancelled by the user */
    Error,     /*!< Install failed (see error message) */
  };
} // namespace WineRunner
