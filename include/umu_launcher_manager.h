/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    umu_launcher_manager.h
 * \brief   Install and maintain WineGUI's private Proton launcher
 * \author  Melroy van den Berg <webmaster1989@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

/**
 * \class UmuLauncherManager
 * \brief Owns the download, verification and atomic installation of WineGUI's private Proton launcher.
 *
 * The public user interface deliberately calls this a managed GE-Proton component. The upstream
 * component name and download details belong in logs, not in end-user setup instructions.
 */
class UmuLauncherManager
{
public:
  struct Release
  {
    std::string version;
    std::string asset_name;
    std::string download_url;
    std::uint64_t size_bytes = 0;
    std::string sha256;
  };

  static const Release& pinned_release();
  static bool is_ready();
  static bool is_ready(const std::string& base_dir, const Release& release);

  /** Install the pinned launcher if necessary. Returns false only when cancelled. */
  static bool ensure_installed(const std::atomic<bool>* cancel = nullptr, const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb = {});

  /** Testable/custom-root form. Production callers should use ensure_installed(). */
  static bool ensure_installed(const std::string& base_dir,
                               const Release& release,
                               const std::atomic<bool>* cancel = nullptr,
                               const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb = {},
                               bool smoke_test = true);

  static std::string user_error_message();

private:
  UmuLauncherManager() = delete;
};
