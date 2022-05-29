/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    main_window.h
 * \brief   Main WineGUI window
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

#include "bottle_item.h"
#include "bottle_new_assistant.h"
#include "busy_dialog.h"
#include "menu.h"
#include <gtkmm.h>
#include <iostream>
#include <list>

using std::cout;
using std::endl;
using std::string;

/**
 * \class MainWindow
 * \brief Main GTK+ Window class
 */
class MainWindow : public Gtk::Window
{
public:
  // Signals
  sigc::signal<void> finished_new_bottle;        /*!< Finished signal */
  sigc::signal<void, BottleItem*> active_bottle; /*!< Set the active bottle in manager, based on the selected bottle */
  sigc::signal<void> show_edit_window;           /*!< show Edit window signal */
  sigc::signal<void> show_settings_window;       /*!< show Settings window signal */
  sigc::signal<void, Glib::ustring&, BottleTypes::Windows, BottleTypes::Bit, Glib::ustring&, bool&, BottleTypes::AudioDriver>
      new_bottle;                                       /*!< Create new Wine Bottle Signal */
  sigc::signal<void, string, bool> run_program;         /*!< Run an EXE or MSI application in Wine with provided filename */
  sigc::signal<void> open_c_drive;                      /*!< Open C: drive signal */
  sigc::signal<void> reboot_bottle;                     /*!< Emulate reboot signal */
  sigc::signal<void> update_bottle;                     /*!< Update Wine bottle signal */
  sigc::signal<void> open_log_file;                     /*!< Open log file signal */
  sigc::signal<void> kill_running_processes;            /*!< Kill all running processes signal */
  sigc::signal<bool, GdkEventButton*> right_click_menu; /*!< Right-mouse click in list box signal */

  explicit MainWindow(Menu& menu);
  virtual ~MainWindow();

  void set_wine_bottles(std::list<BottleItem>& bottles);
  void set_detailed_info(BottleItem& bottle);
  void reset_detailed_info();
  void show_info_message(const Glib::ustring& message, bool markup = false);
  void show_warning_message(const Glib::ustring& message, bool markup = false);
  void show_error_message(const Glib::ustring& message, bool markup = false);
  bool show_confirm_dialog(const Glib::ustring& message, bool markup = false);
  void show_busy_install_dialog(const Glib::ustring& message);
  void show_busy_install_dialog(Gtk::Window& parent, const Glib::ustring& message);
  void close_busy_dialog();

  // Signal handlers
  virtual void on_new_bottle_button_clicked();
  virtual void on_new_bottle_created();
  virtual void on_run_button_clicked();
  virtual void on_hide_window();
  virtual void on_give_feedback();
  virtual void on_check_version();
  virtual void on_exec_failure();

protected:
  // Signal handlers
  void on_startup_version_update();
  bool delete_window(GdkEventAny* any_event);
  Glib::RefPtr<Gio::Settings> window_settings; /*!< Window settings to store our window settings, even during restarts */

  // Child widgets
  Gtk::Box vbox;    /*!< The main vertical box */
  Gtk::Paned paned; /*!< The main paned panel (horizontal) */
  // Left widgets
  Gtk::ScrolledWindow scrolled_window; /*!< Scrolled Window container, which contains the listbox */
  Gtk::ListBox listbox;                /*!< Listbox in the left panel */
  // Right widgets
  Gtk::Box right_box;        /*!< Right panel horizontal box */
  Gtk::Toolbar toolbar;      /*!< Toolbar at top */
  Gtk::Separator separator1; /*!< Seperator */
  Gtk::Grid detail_grid;     /*!< Grid layout container to have multiple rows & columns below the toolbar */
  // Detailed info labels on the right panel
  Gtk::Label name;              /*!< Bottle name */
  Gtk::Label folder_name;       /*!< Folder name */
  Gtk::Label window_version;    /*!< Windows version text */
  Gtk::Label wine_version;      /*!< Wine version text */
  Gtk::Label wine_location;     /*!< Wine location text */
  Gtk::Label c_drive_location;  /*!< C:\ drive location text */
  Gtk::Label wine_last_changed; /*!< Last changed text */
  Gtk::Label audio_driver;      /*!< Audio driver text */
  Gtk::Label virtual_desktop;   /*!< Virtual desktop text */

  // Toolbar buttons
  Gtk::ToolButton new_button;            /*!< New toolbar button */
  Gtk::ToolButton edit_button;           /*!< Edit toolbar button */
  Gtk::ToolButton settings_button;       /*!< Settings toolbar button */
  Gtk::ToolButton run_button;            /*!< Run... toolbar button */
  Gtk::ToolButton open_c_driver_button;  /*!< Open C:\ drive toolbar button */
  Gtk::ToolButton reboot_button;         /*!< Reboot toolbar button */
  Gtk::ToolButton update_button;         /*!< Update toolbar button */
  Gtk::ToolButton open_log_file_button;  /*!< Open log file toolbar button */
  Gtk::ToolButton kill_processes_button; /*!< Kill processes toolbar button */

  // Busy dialog
  BusyDialog busy_dialog_; /*!< Busy dialog, when the user should wait until install is finished */
private:
  BottleNewAssistant new_bottle_assistant_; /*!< New bottle wizard (behind: new button toolbar) */

  // Signal handlers
  virtual void on_row_clicked(Gtk::ListBoxRow* row);
  virtual void on_new_bottle_apply();

  // Private methods
  void check_version_update(bool show_equal = false);
  void load_stored_window_settings();
  void create_left_panel();
  void create_right_panel();
  void set_sensitive_toolbar_buttons(bool sensitive);
  static void cc_list_box_update_header_func(Gtk::ListBoxRow* row, Gtk::ListBoxRow* before);
};
