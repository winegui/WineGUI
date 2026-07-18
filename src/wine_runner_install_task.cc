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

#include "wine_runner_manager.h"

#include <stdexcept>

/**
 * \brief Constructor. Construct on the GUI thread (Glib::Dispatcher requirement).
 * The internal cleanup handlers are connected first, so they run before any UI handler
 * connected to the same dispatchers.
 */
WineRunnerInstallTask::WineRunnerInstallTask()
{
  releases_fetched.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::cleanup_thread));
  fetch_failed.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::cleanup_thread));
  install_finished.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::cleanup_thread));
  remove_finished.connect(sigc::mem_fun(*this, &WineRunnerInstallTask::cleanup_thread));
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
          std::vector<WineRunner::Release> releases = WineRunnerManager::get_releases(source_id);
          {
            std::lock_guard<std::mutex> lock(data_mutex_);
            releases_ = std::move(releases);
          }
          fetched_source_id_.store(source_id);
          is_busy_.store(false);
          releases_fetched.emit();
        }
        catch (const std::runtime_error& error)
        {
          {
            std::lock_guard<std::mutex> lock(data_mutex_);
            error_message_ = error.what();
          }
          is_busy_.store(false);
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
              cancel_requested_);
          status_.store(success ? WineRunner::InstallStatus::Success : WineRunner::InstallStatus::Cancelled);
        }
        catch (const std::runtime_error& error)
        {
          {
            std::lock_guard<std::mutex> lock(data_mutex_);
            error_message_ = error.what();
          }
          status_.store(WineRunner::InstallStatus::Error);
        }
        phase_.store(WineRunner::InstallPhase::Idle);
        is_busy_.store(false);
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
        catch (const std::runtime_error& error)
        {
          error_message = error.what();
        }
        {
          std::lock_guard<std::mutex> lock(data_mutex_);
          error_message_ = error_message;
        }
        is_busy_.store(false);
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
 * \brief Join & release a finished worker thread (called on the GUI thread via the dispatchers)
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
