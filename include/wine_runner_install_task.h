/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_install_task.h
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
#pragma once

#include <atomic>
#include <cstdint>
#include <glibmm/dispatcher.h>
#include <glibmm/ustring.h>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "wine_runner_types.h"

/**
 * \class WineRunnerInstallTask
 * \brief Runs the blocking WineRunnerManager operations in a worker thread.
 * All dispatchers fire on the GUI thread (construct this class on the GUI thread!).
 * Only one operation runs at a time (see is_busy()). After a dispatcher fired,
 * read the outcome via the thread-safe getters.
 */
class WineRunnerInstallTask
{
public:
  // Dispatchers (fired on the GUI thread)
  Glib::Dispatcher releases_fetched; /*!< Release list fetch finished successfully -> get_fetched_releases() */
  Glib::Dispatcher fetch_failed;     /*!< Release list fetch failed -> get_error_message() */
  Glib::Dispatcher progress_changed; /*!< Install progress/phase changed -> get_progress() / get_phase() */
  Glib::Dispatcher install_finished; /*!< Install finished -> get_install_status() + get_error_message() */
  Glib::Dispatcher remove_finished;  /*!< Removal finished -> get_error_message() (empty string = success) */

  WineRunnerInstallTask();
  virtual ~WineRunnerInstallTask();

  bool is_busy() const;
  void fetch_releases_async(WineRunner::SourceId source_id);
  void install_async(const WineRunner::Release& release);
  void remove_async(const WineRunner::InstalledRunner& runner);
  void cancel();

  // Thread-safe result accessors
  std::vector<WineRunner::Release> get_fetched_releases() const;
  WineRunner::SourceId get_fetched_source_id() const;
  std::pair<std::uint64_t, std::uint64_t> get_progress() const;
  WineRunner::InstallPhase get_phase() const;
  WineRunner::InstallStatus get_install_status() const;
  Glib::ustring get_error_message() const;

private:
  void cleanup_thread();

  std::unique_ptr<std::thread> thread_;                                                          /*!< Worker thread for all operations */
  std::atomic<bool> is_busy_{false};                                                             /*!< True while an operation is running */
  std::atomic<bool> cancel_requested_{false};                                                    /*!< Cancellation flag, polled by the install */
  std::atomic<std::uint64_t> bytes_done_{0};                                                     /*!< Download progress: bytes done */
  std::atomic<std::uint64_t> bytes_total_{0};                                                    /*!< Download progress: bytes total (0 = unknown) */
  std::atomic<WineRunner::InstallPhase> phase_{WineRunner::InstallPhase::Idle};                  /*!< Current install phase */
  std::atomic<WineRunner::InstallStatus> status_{WineRunner::InstallStatus::Success};            /*!< Final install status */
  std::atomic<WineRunner::SourceId> fetched_source_id_{WineRunner::SourceId::Kron4ekWineBuilds}; /*!< Source the fetched releases belong to */
  mutable std::mutex data_mutex_;                                                                /*!< Protects releases_ & error_message_ */
  std::vector<WineRunner::Release> releases_;                                                    /*!< Fetched releases (guarded by data_mutex_) */
  Glib::ustring error_message_; /*!< Error message of the last operation (guarded by data_mutex_) */
};
