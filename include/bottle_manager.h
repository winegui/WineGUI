/**
 * Copyright (c) 2019-2023 WineGUI
 *
 * \file    bottle_manager.h
 * \brief   Wine Bottle manager controller
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

#include <gtkmm.h>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "bottle_types.h"
#include "general_config_struct.h"

using std::string;

// Forward declaration
class MainWindow;
class SignalController;
class BottleItem;

/**
 * \class BottleManager
 * \brief Wine Bottle Controller that controls it all
 */
class BottleManager
{
public:
  // Signals
  sigc::signal<void> reset_active_bottle;               /*!< Send signal: Clear the current active bottle */
  sigc::signal<void> bottle_removed;                    /*!< Send signal: When the bottle is confirmed to be removed */
  Glib::Dispatcher finished_package_install_dispatcher; /*!< Signal that Wine package install is completed */

  explicit BottleManager(MainWindow& main_window);
  virtual ~BottleManager();

  void prepare();
  void update_config_and_bottles(const Glib::ustring& select_bottle_name, bool is_startup);
  void new_bottle(SignalController* caller,
                  const Glib::ustring& name,
                  BottleTypes::Windows windows_version,
                  BottleTypes::Bit bit,
                  const Glib::ustring& virtual_desktop_resolution,
                  bool disable_gecko_mono,
                  BottleTypes::AudioDriver audio);
  void update_bottle(SignalController* caller,
                     const Glib::ustring& name,
                     const Glib::ustring& folder_name,
                     const Glib::ustring& description,
                     BottleTypes::Windows windows_version,
                     const Glib::ustring& virtual_desktop_resolution,
                     BottleTypes::AudioDriver audio,
                     bool is_debug_logging,
                     int debug_log_level);
  void clone_bottle(SignalController* caller, const Glib::ustring& name, const Glib::ustring& folder_name, const Glib::ustring& description);
  void delete_bottle();
  void set_active_bottle(BottleItem* bottle);
  const Glib::ustring& get_error_message() const;

  // Signal handlers
  void run_executable(string program, bool is_msi_file);
  void run_program(string program);
  void open_c_drive();
  void reboot();
  void update();
  void open_log_file();
  void kill_processes();
  void install_d3dx9(Gtk::Window& parent, const string& version);
  void install_dxvk(Gtk::Window& parent, const string& version);
  void install_vkd3d(Gtk::Window& parent);
  void install_visual_cpp_package(Gtk::Window& parent, const string& version);
  void install_dot_net(Gtk::Window& parent, const string& version);
  void install_core_fonts(Gtk::Window& parent);
  void install_liberation(Gtk::Window& parent);

private:
  // Synchronizes access to data members using mutexes
  mutable std::mutex error_message_mutex_;
  mutable std::mutex output_loging_mutex_;
  Glib::Dispatcher update_bottles_dispatcher_; /*!< Dispatcher if the bottle list needs to be updated, from thread */
  Glib::Dispatcher write_log_dispatcher_;      /*!< Dispatcher if we can write the output logging to disk */

  MainWindow& main_window_;
  string bottle_location_;
  std::list<BottleItem> bottles_;
  BottleItem* active_bottle_;
  bool is_display_default_wine_machine_;
  bool is_wine64_bit_;
  bool is_logging_stderr_;
  int previous_active_bottle_index_;
  std::size_t previous_bottles_list_size_;

  //// error_message is used by both the GUI thread and NewBottle thread (used a 'temp' location)
  Glib::ustring error_message_;
  std::string logging_bottle_prefix_;
  std::string output_logging_;

  // Signal handlers
  virtual void write_log_to_file();

  GeneralConfigData load_and_save_general_config();
  bool is_bottle_not_null();
  string get_deinstall_mono_command();
  string get_wine_version();
  std::vector<string> get_bottle_paths();
  std::list<BottleItem> create_wine_bottles(std::vector<string> bottle_dirs);
};
