/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    umu_launcher_manager.cc
 * \brief   Install and maintain WineGUI's private Proton launcher
 * \author  Melroy van den Berg <webmaster1989@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */
#include "umu_launcher_manager.h"

#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <glibmm/checksum.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>
#include <iostream>
#include <limits>
#include <mutex>
#include <signal.h>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace
{
  const UmuLauncherManager::Release PinnedRelease{
      "1.4.4",
      "umu-launcher-1.4.4-zipapp.tar",
      "https://github.com/Open-Wine-Components/umu-launcher/releases/download/1.4.4/umu-launcher-1.4.4-zipapp.tar",
      430080,
      "eb590691841f7fad3fc3ad8fd5db4ccb87849fe7948e62b28ece7a4ee48cc851",
  };

  std::mutex install_mutex;
  std::atomic<bool> accepted_existing_version{false};

  bool cancelled(const std::atomic<bool>* cancel)
  {
    return cancel != nullptr && cancel->load();
  }

  bool process_is_alive(pid_t process_id)
  {
    if (process_id <= 0)
      return false;
    if (kill(process_id, 0) == 0)
      return true;
    return errno != ESRCH;
  }

  std::string compute_sha256(const fs::path& path)
  {
    Glib::Checksum checksum(Glib::Checksum::Type::SHA256);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
      throw std::runtime_error("Could not open the managed component download for verification.");
    std::vector<char> buffer(1024 * 1024);
    while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || file.gcount() > 0)
      checksum.update(reinterpret_cast<const guchar*>(buffer.data()), static_cast<gssize>(file.gcount()));
    return checksum.get_string();
  }

  bool spawn_wait_cancellable(const std::vector<std::string>& argv,
                              const std::atomic<bool>* cancel,
                              const std::function<void()>& poll_cb,
                              const std::string& failure_message)
  {
    Glib::Pid pid = 0;
    try
    {
      Glib::spawn_async("", argv, Glib::SpawnFlags::SEARCH_PATH | Glib::SpawnFlags::DO_NOT_REAP_CHILD, {}, &pid);
    }
    catch (const Glib::Error& error)
    {
      throw std::runtime_error(failure_message + " Technical error: " + error.what());
    }

    int wait_status = 0;
    while (true)
    {
      pid_t result = waitpid(pid, &wait_status, WNOHANG);
      if (result == pid)
        break;
      if (result < 0)
      {
        Glib::spawn_close_pid(pid);
        throw std::runtime_error(failure_message + " Technical error: " + std::strerror(errno));
      }
      if (cancelled(cancel))
      {
        kill(pid, SIGTERM);
        waitpid(pid, &wait_status, 0);
        Glib::spawn_close_pid(pid);
        return false;
      }
      if (poll_cb)
        poll_cb();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Glib::spawn_close_pid(pid);
    if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
      throw std::runtime_error(failure_message);
    return true;
  }

  void copy_local_file(const fs::path& source,
                       const fs::path& destination,
                       std::uint64_t expected_size,
                       const std::atomic<bool>* cancel,
                       const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb)
  {
    std::ifstream input(source, std::ios::binary);
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!input.is_open() || !output.is_open())
      throw std::runtime_error("Could not copy the managed component test archive.");
    std::vector<char> buffer(64 * 1024);
    std::uint64_t copied = 0;
    while (input.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || input.gcount() > 0)
    {
      if (cancelled(cancel))
        return;
      output.write(buffer.data(), input.gcount());
      copied += static_cast<std::uint64_t>(input.gcount());
      if (progress_cb)
        progress_cb(copied, expected_size);
    }
  }

  bool download_archive(const UmuLauncherManager::Release& release,
                        const fs::path& destination,
                        const std::atomic<bool>* cancel,
                        const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb)
  {
    if (release.download_url.starts_with("file://"))
    {
      copy_local_file(release.download_url.substr(7), destination, release.size_bytes, cancel, progress_cb);
      return !cancelled(cancel);
    }

    const auto poll_progress = [&]()
    {
      if (!progress_cb)
        return;
      std::error_code error_code;
      std::uint64_t bytes = fs::file_size(destination, error_code);
      if (!error_code)
        progress_cb(bytes, release.size_bytes);
    };
    bool completed =
        spawn_wait_cancellable({"wget", "--quiet", "--timeout=30", "--tries=2", "--output-document=" + destination.string(), release.download_url},
                               cancel, poll_progress, "Could not download the managed GE-Proton component.");
    if (completed && progress_cb)
      progress_cb(release.size_bytes, release.size_bytes);
    return completed;
  }

  std::vector<std::string> archive_members(const fs::path& archive_path)
  {
    std::string output;
    std::string error_output;
    int wait_status = 0;
    try
    {
      Glib::spawn_sync("", {"tar", "-tf", archive_path.string()}, Glib::SpawnFlags::SEARCH_PATH, {}, &output, &error_output, &wait_status);
    }
    catch (const Glib::Error& error)
    {
      throw std::runtime_error("Could not inspect the managed component archive: " + std::string(error.what()));
    }
    if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
      throw std::runtime_error("The managed component archive is malformed.");

    std::vector<std::string> members;
    std::string::size_type start = 0;
    while (start < output.size())
    {
      std::string::size_type end = output.find('\n', start);
      std::string member = output.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
      if (!member.empty())
        members.emplace_back(member);
      if (end == std::string::npos)
        break;
      start = end + 1;
    }
    return members;
  }

  void validate_archive(const fs::path& archive_path)
  {
    const std::vector<std::string> expected{"umu/", "umu/umu-run", "umu/umu_run.py"};
    if (archive_members(archive_path) != expected)
      throw std::runtime_error("The managed component archive has an unexpected or unsafe layout.");

    std::string verbose_output;
    std::string error_output;
    int wait_status = 0;
    try
    {
      Glib::spawn_sync("", {"tar", "-tvf", archive_path.string()}, Glib::SpawnFlags::SEARCH_PATH, {}, &verbose_output, &error_output, &wait_status);
    }
    catch (const Glib::Error& error)
    {
      throw std::runtime_error("Could not validate the managed component archive: " + std::string(error.what()));
    }
    if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
      throw std::runtime_error("The managed component archive is malformed.");

    std::vector<std::string> lines;
    std::string::size_type start = 0;
    while (start < verbose_output.size())
    {
      const std::string::size_type end = verbose_output.find('\n', start);
      const std::string line = verbose_output.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
      if (!line.empty())
        lines.emplace_back(line);
      if (end == std::string::npos)
        break;
      start = end + 1;
    }
    if (lines.size() != 3 || lines[0].empty() || lines[0][0] != 'd' || !lines[0].ends_with(" umu/") || lines[1].empty() || lines[1][0] != '-' ||
        !lines[1].ends_with(" umu/umu-run") || lines[2].empty() || lines[2][0] != 'l' || !lines[2].ends_with(" umu/umu_run.py -> umu-run"))
      throw std::runtime_error("The managed component archive contains unsafe file types or links.");
  }

  void run_smoke_test(const fs::path& executable)
  {
    std::string output;
    std::string error_output;
    int wait_status = 0;
    try
    {
      Glib::spawn_sync("", {executable.string(), "--help"}, Glib::SpawnFlags::DEFAULT, {}, &output, &error_output, &wait_status);
    }
    catch (const Glib::Error& error)
    {
      throw std::runtime_error("The managed GE-Proton component could not be started: " + std::string(error.what()));
    }
    if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0)
      throw std::runtime_error("The managed GE-Proton component failed its startup check.");
  }

  void cleanup_abandoned_paths(const fs::path& base_dir)
  {
    std::error_code error_code;
    if (!fs::is_directory(base_dir, error_code))
      return;
    for (const fs::directory_entry& entry : fs::directory_iterator(base_dir, error_code))
    {
      if (error_code)
        return;
      const std::string name = entry.path().filename().string();
      if (!name.starts_with(".staging-") && !name.starts_with(".link-"))
        continue;
      const std::string process_text = name.substr(name.find_last_of('-') + 1);
      std::int64_t process_id = 0;
      auto [end, parse_error] = std::from_chars(process_text.data(), process_text.data() + process_text.size(), process_id);
      if (parse_error != std::errc() || end != process_text.data() + process_text.size() || process_id <= 0 ||
          process_id > static_cast<std::int64_t>(std::numeric_limits<pid_t>::max()) || process_is_alive(static_cast<pid_t>(process_id)))
        continue;
      fs::remove_all(entry.path(), error_code);
      error_code.clear();
    }

    const fs::path downloads_dir = base_dir / ".downloads";
    if (!fs::is_directory(downloads_dir, error_code))
      return;
    for (const fs::directory_entry& entry : fs::directory_iterator(downloads_dir, error_code))
    {
      const std::string name = entry.path().filename().string();
      const std::string part_marker = ".part-";
      const std::string downloaded_marker = ".downloaded-";
      std::string marker = name.rfind(part_marker) == std::string::npos ? downloaded_marker : part_marker;
      const auto marker_position = name.rfind(marker);
      if (marker_position == std::string::npos)
        continue;
      const std::string process_text = name.substr(marker_position + marker.size());
      std::int64_t process_id = 0;
      auto [end, parse_error] = std::from_chars(process_text.data(), process_text.data() + process_text.size(), process_id);
      if (parse_error == std::errc() && end == process_text.data() + process_text.size() && process_id > 0 &&
          process_id <= static_cast<std::int64_t>(std::numeric_limits<pid_t>::max()) && !process_is_alive(static_cast<pid_t>(process_id)))
        fs::remove(entry.path(), error_code);
      error_code.clear();
    }
  }

  class FileLock
  {
  public:
    FileLock(const fs::path& path, const std::atomic<bool>* cancel)
    {
      descriptor_ = open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
      if (descriptor_ < 0)
        throw std::runtime_error("Could not open the managed component installation lock: " + std::string(std::strerror(errno)));
      while (flock(descriptor_, LOCK_EX | LOCK_NB) != 0)
      {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
          const std::string error_message = std::strerror(errno);
          close(descriptor_);
          descriptor_ = -1;
          throw std::runtime_error("Could not lock the managed component installation: " + error_message);
        }
        if (cancelled(cancel))
          return;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      locked_ = true;
    }

    ~FileLock()
    {
      if (descriptor_ >= 0)
      {
        if (locked_)
          flock(descriptor_, LOCK_UN);
        close(descriptor_);
      }
    }

    bool locked() const
    {
      return locked_;
    }

  private:
    int descriptor_ = -1;
    bool locked_ = false;
  };

  struct PathCleanup
  {
    std::vector<fs::path> paths;
    ~PathCleanup()
    {
      for (const fs::path& path : paths)
      {
        std::error_code error_code;
        fs::remove_all(path, error_code);
      }
    }
  };
} // namespace

const UmuLauncherManager::Release& UmuLauncherManager::pinned_release()
{
  return PinnedRelease;
}

bool has_usable_existing_launcher(const fs::path& root)
{
  const fs::path launcher = root / "umu-run";
  std::error_code error_code;
  if (!fs::is_symlink(launcher, error_code) || !fs::is_regular_file(launcher, error_code) || access(launcher.c_str(), X_OK) != 0)
    return false;
  const fs::path relative_target = fs::read_symlink(launcher, error_code);
  if (error_code || relative_target.is_absolute() || relative_target.empty() || *relative_target.begin() != "versions")
    return false;
  const fs::path resolved = fs::weakly_canonical(root / relative_target, error_code);
  const fs::path versions = fs::weakly_canonical(root / "versions", error_code);
  if (error_code || resolved.parent_path().parent_path() != versions)
    return false;
  try
  {
    run_smoke_test(launcher);
    return true;
  }
  catch (const std::exception& error)
  {
    std::cerr << "WARN: Existing managed GE-Proton launcher failed its startup check: " << error.what() << std::endl;
    return false;
  }
}

bool UmuLauncherManager::is_ready()
{
  const std::string base_dir = Glib::build_filename(Glib::get_user_data_dir(), "winegui", "umu");
  return is_ready(base_dir, pinned_release()) || (accepted_existing_version.load() && has_usable_existing_launcher(base_dir));
}

bool UmuLauncherManager::is_ready(const std::string& base_dir, const Release& release)
{
  const fs::path root(base_dir);
  const fs::path version_launcher = root / "versions" / release.version / "umu-run";
  const fs::path marker = root / "versions" / release.version / ".verified";
  const fs::path stable_launcher = root / "umu-run";
  std::error_code error_code;
  if (!fs::is_regular_file(version_launcher, error_code) || access(version_launcher.c_str(), X_OK) != 0 ||
      !fs::is_symlink(stable_launcher, error_code))
    return false;
  if (fs::read_symlink(stable_launcher, error_code) != fs::path("versions") / release.version / "umu-run" || error_code)
    return false;
  std::ifstream marker_file(marker);
  if (!marker_file.is_open())
    return false;
  std::string marker_value;
  std::getline(marker_file, marker_value);
  return !marker_file.bad() && marker_value == release.sha256;
}

bool UmuLauncherManager::ensure_installed(const std::atomic<bool>* cancel, const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb)
{
  const std::string base_dir = Glib::build_filename(Glib::get_user_data_dir(), "winegui", "umu");
  const bool can_fall_back = has_usable_existing_launcher(base_dir);
  try
  {
    const bool installed = ensure_installed(base_dir, pinned_release(), cancel, progress_cb, true);
    if (installed)
      accepted_existing_version.store(false);
    return installed;
  }
  catch (const std::exception& error)
  {
    if (can_fall_back && has_usable_existing_launcher(base_dir))
    {
      std::cerr << "WARN: Managed GE-Proton launcher update failed; continuing with the existing compatible version: " << error.what() << std::endl;
      accepted_existing_version.store(true);
      return true;
    }
    throw;
  }
}

bool UmuLauncherManager::ensure_installed(const std::string& base_dir,
                                          const Release& release,
                                          const std::atomic<bool>* cancel,
                                          const std::function<void(std::uint64_t, std::uint64_t)>& progress_cb,
                                          bool smoke_test)
{
  if (is_ready(base_dir, release))
    return true;
  std::unique_lock<std::mutex> process_lock(install_mutex);
  if (cancelled(cancel))
    return false;

  const fs::path root(base_dir);
  std::error_code error_code;
  fs::create_directories(root / "versions", error_code);
  fs::create_directories(root / ".downloads", error_code);
  if (error_code)
    throw std::runtime_error("Could not create WineGUI's managed component directory: " + root.string());

  FileLock file_lock(root / "install.lock", cancel);
  if (!file_lock.locked())
    return false;
  if (is_ready(base_dir, release))
    return true;
  cleanup_abandoned_paths(root);

  const std::string process_id = std::to_string(getpid());
  const fs::path partial_archive = root / ".downloads" / (release.asset_name + ".part-" + process_id);
  const fs::path complete_archive = root / ".downloads" / (release.asset_name + ".downloaded-" + process_id);
  const fs::path staging_dir = root / (".staging-" + process_id);
  const fs::path temporary_link = root / (".link-" + process_id);
  PathCleanup cleanup{{partial_archive, complete_archive, staging_dir, temporary_link}};
  for (const fs::path& transient_path : cleanup.paths)
  {
    fs::remove_all(transient_path, error_code);
    error_code.clear();
  }

  if (!download_archive(release, partial_archive, cancel, progress_cb))
    return false;
  if (cancelled(cancel))
    return false;
  const std::uint64_t actual_size = fs::file_size(partial_archive, error_code);
  if (error_code || actual_size != release.size_bytes)
    throw std::runtime_error("The managed GE-Proton component download was incomplete.");
  if (compute_sha256(partial_archive) != release.sha256)
    throw std::runtime_error("The managed GE-Proton component failed its integrity check.");
  fs::rename(partial_archive, complete_archive, error_code);
  if (error_code)
    throw std::runtime_error("Could not finalize the managed component download: " + error_code.message());

  validate_archive(complete_archive);
  fs::create_directories(staging_dir, error_code);
  if (error_code)
    throw std::runtime_error("Could not create the managed component staging directory.");
  if (!spawn_wait_cancellable({"tar", "-xf", complete_archive.string(), "-C", staging_dir.string(), "--no-same-owner", "--no-same-permissions"},
                              cancel, {}, "Could not extract the managed GE-Proton component."))
    return false;

  const fs::path extracted_launcher = staging_dir / "umu" / "umu-run";
  const fs::path extracted_alias = staging_dir / "umu" / "umu_run.py";
  if (!fs::is_regular_file(extracted_launcher, error_code) || !fs::is_symlink(extracted_alias, error_code) ||
      fs::read_symlink(extracted_alias, error_code) != "umu-run" || error_code)
    throw std::runtime_error("The managed component archive did not contain the expected safe files.");
  if (chmod(extracted_launcher.c_str(), 0755) != 0)
    throw std::runtime_error("Could not make the managed GE-Proton component executable.");
  if (smoke_test)
    run_smoke_test(extracted_launcher);

  const fs::path staged_version = staging_dir / "version";
  fs::rename(staging_dir / "umu", staged_version, error_code);
  if (error_code)
    throw std::runtime_error("Could not prepare the managed component version directory.");
  {
    std::ofstream marker(staged_version / ".verified", std::ios::trunc);
    marker << release.sha256 << '\n';
    if (!marker.good())
      throw std::runtime_error("Could not record the managed component verification state.");
  }

  const fs::path target_version = root / "versions" / release.version;
  const fs::path replaced_version = root / (".replaced-" + process_id);
  if (fs::exists(target_version, error_code))
  {
    fs::rename(target_version, replaced_version, error_code);
    if (error_code)
      throw std::runtime_error("Could not replace an incomplete managed component installation.");
    cleanup.paths.emplace_back(replaced_version);
  }
  fs::rename(staged_version, target_version, error_code);
  if (error_code)
  {
    if (fs::exists(replaced_version))
      fs::rename(replaced_version, target_version, error_code);
    throw std::runtime_error("Could not install the managed GE-Proton component.");
  }

  fs::create_symlink(fs::path("versions") / release.version / "umu-run", temporary_link, error_code);
  if (error_code)
    throw std::runtime_error("Could not create the managed component launcher link.");
  fs::rename(temporary_link, root / "umu-run", error_code);
  if (error_code)
    throw std::runtime_error("Could not activate the managed GE-Proton component.");

  std::cout << "INFO: Installed managed GE-Proton launcher component version " << release.version << " in " << root << std::endl;
  return true;
}

std::string UmuLauncherManager::user_error_message()
{
  return "WineGUI could not prepare GE-Proton support.\n\nCheck your internet connection and try again.";
}
