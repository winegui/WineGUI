/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_install_task.cc
 * \brief   Asynchronous facade around WineRunnerManager (worker thread + Glib::Dispatcher signals)
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
#include "wine_runner_install_task.h"

#include "umu_launcher_manager.h"
#include "wine_runner_manager.h"

#include <iostream>
#include <stdexcept>

/**
 * \brief Constructor. Construct on the GUI thread (Glib::Dispatcher requirement).
 * The internal completion handlers are connected first, so they run before any UI handler
 * connected to the same dispatchers (which may start a new operation).
 */
WineRunnerInstallTask::WineRunnerInstallTask()
{
  releases_fetched.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::finish_operation));
  fetch_failed.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::finish_operation));
  install_finished.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::finish_operation));
  remove_finished.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::finish_operation));
}

/**
 * \brief Destructor. Cancels a running operation and waits for the worker thread.
 */
WineRunnerInstallTask::~WineRunnerInstallTask()
{
  cancel_requested_.store(true);
  if (thread_ && thread_->joinable())
  {
    thread_->join();
  }
  thread_.reset();
}

/**
 * \brief Whether an operation is currently running
 * \return True when busy (new operations are refused)
 */
bool WineRunnerInstallTask::is_busy() const
{
  return is_busy_.load();
}

/**
 * \brief Fetch the release list of a runner source (async).
 * Fires releases_fetched or fetch_failed when done. No-op when busy.
 * \param[in] source_id Source ID
 */
void WineRunnerInstallTask::fetch_releases_async(WineRunner::SourceId source_id)
{
  if (is_busy_.exchange(true))
    return;
  cleanup_thread();
  cancel_requested_.store(false);
  fetched_source_id_.store(source_id);
  thread_ = std::make_unique<std::thread>(
      [this, source_id]
      {
        try
        {
          std::vector<WineRunner::Release> releases = WineRunnerManager::get_releases(source_id, &cancel_requested_);
          {
            std::lock_guard<std::mutex> lock(data_mutex_);
            releases_ = std::move(releases);
          }
          fetched_source_id_.store(source_id);
          // is_busy_ is cleared on the GUI thread (see finish_operation), never here: the GUI thread
          // could otherwise start a new operation before the dispatcher below is handled, after which
          // the pending dispatcher would join that brand new worker thread and freeze the UI.
          releases_fetched.emit();
        }
        // Catch every standard exception (incl. Glib::Error), not just std::runtime_error: an
        // exception escaping this thread function would terminate the whole application, so any
        // unexpected one has to become a failed operation instead
        catch (const std::exception& error)
        {
          std::cerr << "ERROR: Wine runner installation failed: " << error.what() << std::endl;
          {
            std::lock_guard<std::mutex> lock(data_mutex_);
            error_message_ = error.what();
          }
          fetch_failed.emit();
        }
      });
}

/**
 * \brief Download & install a runner release (async).
 * Fires progress_changed during the install and install_finished when done. No-op when busy.
 * \param[in] release Release to install
 */
void WineRunnerInstallTask::install_async(const WineRunner::Release& release)
{
  if (is_busy_.exchange(true))
    return;
  cleanup_thread();
  cancel_requested_.store(false);
  bytes_done_.store(0);
  bytes_total_.store(release.size_bytes);
  phase_.store(WineRunner::InstallPhase::Idle);
  thread_ = std::make_unique<std::thread>(
      [this, release]
      {
        try
        {
          if (release.source == WineRunner::SourceId::GEProton && !UmuLauncherManager::is_ready())
          {
            phase_.store(WineRunner::InstallPhase::PreparingSupport);
            bytes_done_.store(0);
            bytes_total_.store(UmuLauncherManager::pinned_release().size_bytes);
            progress_changed.emit();
            if (!UmuLauncherManager::ensure_installed(&cancel_requested_,
                                                      [this](std::uint64_t bytes_done, std::uint64_t bytes_total)
                                                      {
                                                        bytes_done_.store(bytes_done);
                                                        bytes_total_.store(bytes_total);
                                                        progress_changed.emit();
                                                      }))
            {
              status_.store(WineRunner::InstallStatus::Cancelled);
              phase_.store(WineRunner::InstallPhase::Idle);
              install_finished.emit();
              return;
            }
          }
          bytes_done_.store(0);
          bytes_total_.store(release.size_bytes);
          bool checksum_verified = false;
          bool success = WineRunnerManager::download_and_install(
              release,
              [this](std::uint64_t bytes_done, std::uint64_t bytes_total)
              {
                bytes_done_.store(bytes_done);
                bytes_total_.store(bytes_total);
                progress_changed.emit();
              },
              [this](WineRunner::InstallPhase phase)
              {
                phase_.store(phase);
                progress_changed.emit();
              },
              cancel_requested_, &checksum_verified);
          checksum_verified_.store(checksum_verified);
          status_.store(success ? WineRunner::InstallStatus::Success : WineRunner::InstallStatus::Cancelled);
        }
        // See fetch_releases_async(): an escaping exception would terminate the application
        catch (const std::exception& error)
        {
          {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (release.source == WineRunner::SourceId::GEProton && !UmuLauncherManager::is_ready())
              error_message_ = UmuLauncherManager::user_error_message();
            else
              error_message_ = error.what();
          }
          status_.store(WineRunner::InstallStatus::Error);
        }
        phase_.store(WineRunner::InstallPhase::Idle);
        install_finished.emit();
      });
}

/**
 * \brief Remove an installed runner (async).
 * Fires remove_finished when done (empty error message = success). No-op when busy.
 * \param[in] runner Installed runner to remove
 */
void WineRunnerInstallTask::remove_async(const WineRunner::InstalledRunner& runner)
{
  if (is_busy_.exchange(true))
    return;
  cleanup_thread();
  cancel_requested_.store(false);
  thread_ = std::make_unique<std::thread>(
      [this, runner]
      {
        Glib::ustring error_message = "";
        try
        {
          WineRunnerManager::remove_runner(runner);
        }
        // See fetch_releases_async(): an escaping exception would terminate the application
        catch (const std::exception& error)
        {
          error_message = error.what();
        }
        {
          std::lock_guard<std::mutex> lock(data_mutex_);
          error_message_ = error_message;
        }
        remove_finished.emit();
      });
}

/**
 * \brief Request cancellation of the running install.
 * The install_finished dispatcher will fire with InstallStatus::Cancelled.
 */
void WineRunnerInstallTask::cancel()
{
  cancel_requested_.store(true);
}

/**
 * \brief Get the fetched releases (after releases_fetched fired)
 * \return List of releases, newest first
 */
std::vector<WineRunner::Release> WineRunnerInstallTask::get_fetched_releases() const
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  return releases_;
}

/**
 * \brief Get the source the fetched releases belong to
 * \return Source ID
 */
WineRunner::SourceId WineRunnerInstallTask::get_fetched_source_id() const
{
  return fetched_source_id_.load();
}

/**
 * \brief Get the current download progress
 * \return Pair of bytes done & bytes total (total 0 = unknown, show a pulsing progress bar)
 */
std::pair<std::uint64_t, std::uint64_t> WineRunnerInstallTask::get_progress() const
{
  return {bytes_done_.load(), bytes_total_.load()};
}

/**
 * \brief Get the current install phase
 * \return Install phase
 */
WineRunner::InstallPhase WineRunnerInstallTask::get_phase() const
{
  return phase_.load();
}

/**
 * \brief Get the final status of the last install (after install_finished fired)
 * \return Install status
 */
WineRunner::InstallStatus WineRunnerInstallTask::get_install_status() const
{
  return status_.load();
}

/**
 * \brief Whether the last install was verified against a published checksum
 * \return True when verified, false when the source published no checksum for that release
 */
bool WineRunnerInstallTask::was_checksum_verified() const
{
  return checksum_verified_.load();
}

/**
 * \brief Get the error message of the last operation
 * \return Error message (empty string when there was no error)
 */
Glib::ustring WineRunnerInstallTask::get_error_message() const
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  return error_message_;
}

/*************************************************************
 * Private member functions                                  *
 *************************************************************/

/**
 * \brief Complete a finished operation on the GUI thread: join the worker thread and release the busy flag.
 * Connected to every completion dispatcher before the UI handlers, so a UI handler that starts a
 * follow-up operation always sees a joined thread and a cleared busy flag.
 */
void WineRunnerInstallTask::finish_operation()
{
  cleanup_thread();
  is_busy_.store(false);
}

/**
 * \brief Join & release a finished worker thread (called on the GUI thread)
 */
void WineRunnerInstallTask::cleanup_thread()
{
  if (thread_)
  {
    if (thread_->joinable())
      thread_->join();
    thread_.reset();
  }
}
