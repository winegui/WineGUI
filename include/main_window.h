/**
 * Copyright (c) 2019-2025 WineGUI
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

#include "app_list_model_column.h"
#include "app_list_struct.h"
#include "bottle_item.h"
#include "bottle_new_assistant.h"
#include "busy_dialog.h"
#include "dialog_window.h"
#include "general_config_struct.h"
#include <gtkmm.h>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <vector>

using std::cout;
using std::endl;
using std::string;

// Toolbar overflow management
struct ToolbarButtonData
{
  Gtk::Button* button;
  Glib::ustring label;
  Glib::ustring icon_name;
  Glib::ustring tooltip_text;
  Glib::ustring action_name;
};

/**
 * \class MainWindow
 * \brief Main GTK Application Window class
 */
class MainWindow : public Gtk::ApplicationWindow
{
public:
  // Signals
  sigc::signal<void(const Glib::ustring&)> finished_new_bottle; /*!< Finished signal after the bottle is created, with the new bottle name */
  sigc::signal<void(BottleItem*)> active_bottle;                /*!< Set the active bottle in manager, based on the selected bottle */
  sigc::signal<void()> show_edit_window;                        /*!< show Edit window signal */
  sigc::signal<void()> show_clone_window;                       /*!< show Clone window signal */
  sigc::signal<void()> show_configure_window;                   /*!< show Settings window signal */
  sigc::signal<void()> show_add_app_window;                     /*!< show add application window signal */
  sigc::signal<void()> show_remove_app_window;                  /*!< show remove application window signal */
  sigc::signal<void(Glib::ustring&, BottleTypes::Windows, BottleTypes::Bit, Glib::ustring&, bool&, BottleTypes::AudioDriver)>
      new_bottle;                                  /*!< Create new Wine Bottle Signal */
  sigc::signal<void(string, bool)> run_executable; /*!< Run an EXE or MSI application in Wine with provided filename */
  sigc::signal<void(string)> run_program;          /*!< Run program in Wine */
  sigc::signal<void()> open_c_drive;               /*!< Open C: drive signal */
  sigc::signal<void()> reboot_bottle;              /*!< Emulate reboot signal */
  sigc::signal<void()> update_bottle;              /*!< Update Wine bottle signal */
  sigc::signal<void()> open_log_file;              /*!< Open log file signal */
  sigc::signal<void()> kill_running_processes;     /*!< Kill all running processes signal */
  // TODO: Right menu click won't work anymore in GTK4
  // sigc::signal<bool(GdkEventButton*)> right_click_menu; /*!< Right-mouse click in list box signal */

  explicit MainWindow();
  virtual ~MainWindow();

  void set_wine_bottles(std::list<BottleItem>& bottles);
  void select_row_bottle(BottleItem& bottle);
  void reset_detailed_info();
  void reset_application_list();
  void set_general_config(const GeneralConfigData& config_data);
  void show_info_message(const Glib::ustring& message, bool markup = false);
  void show_warning_message(const Glib::ustring& message, bool markup = false);
  void show_error_message(const Glib::ustring& message, bool markup = false);
  DialogWindow* show_question_dialog(Gtk::Window* parent, const Glib::ustring& message, bool markup = false);
  void show_busy_install_dialog(Gtk::Window& parent, const Glib::ustring& message);
  void hide_busy_dialog();

  // Signal handlers
  virtual void on_new_bottle_button_clicked();
  virtual void on_new_bottle_created();
  virtual void on_run_button_clicked();
  virtual void on_refresh_app_list_button_clicked();
  virtual void on_hide_window();
  virtual void on_report_issue();
  virtual void on_issue_tickets();
  virtual void on_check_version();
  virtual void on_exec_failure();

protected:
  // Signal handlers
  void on_setup_label(const Glib::RefPtr<Gtk::ListItem>& list_item);
  void on_bind_icon_and_name(const Glib::RefPtr<Gtk::ListItem>& list_item);
  void on_error_message_check_version();
  void on_info_message_check_version();
  void on_new_version_available();
  bool on_delete_window();

  // Child widgets
  Gtk::Paned main_paned;                       /*!< The main paned panel */
  Glib::RefPtr<Gio::Settings> window_settings; /*!< Window settings to store our window settings, even during restarts */
  // Left widgets
  Gtk::ScrolledWindow scrolled_window_bottles_listbox; /*!< Scrolled Window container, which contains the listbox of bottles */
  Gtk::ListBox bottles_listbox;                        /*!< Listbox of Wine bottles in the left panel */
  // Right widgets
  Gtk::ScrolledWindow detail_grid_scrolled_window_detail;           /*!< Scrolled Window container for the detail grid */
  Gtk::Box right_vbox;                                              /*!< Right panel vertical box */
  Gtk::Box app_list_vbox;                                           /*!< Application list vertical box */
  Gtk::Box app_list_top_hbox;                                       /*!< Horizontal box above the app list */
  Gtk::SearchEntry app_list_search_entry;                           /*!< Application list search entry */
  Gtk::Paned container_paned;                                       /*!< Main container horizontal paned panel */
  Glib::RefPtr<Gio::ListStore<AppListModelColumns>> app_list_store; /*!< Application list store */
  Glib::RefPtr<Gtk::SingleSelection> app_list_selection_model;      /*!< Application list selection model */
  Glib::RefPtr<Gtk::SignalListItemFactory> app_list_factory;        /*!< Application list factory */
  Glib::RefPtr<Gtk::FilterListModel> app_list_filter_list_model;    /*!< Application list filter list model */
  Glib::RefPtr<Gtk::StringFilter> app_list_filter;                  /*!< Application list filter */
  Gtk::Box toolbar;                                                 /*!< Toolbar at top */
  Gtk::MenuButton menu_button_toolbar;                              /*!< The more menu button in the toolbar */
  Glib::RefPtr<Gio::Menu> toolbar_menu;                             /*!< The toolbar menu */
  Gtk::Separator separator1;                                        /*!< Separator */
  Gtk::Grid detail_grid;                                            /*!< Grid layout container to have multiple rows & columns below the toolbar */
  Gtk::ScrolledWindow app_list_scrolled_window;                     /*!< Scrolled Window container for app list */
  Gtk::ListView app_list_list_view;                                 /*!< List of applications put inside a list view */

  // Detailed info labels on the right panel
  Gtk::Label name_label;              /*!< Bottle name */
  Gtk::Label folder_name_label;       /*!< Folder name */
  Gtk::Label window_version_label;    /*!< Windows version text */
  Gtk::Label c_drive_location_label;  /*!< C:\ drive location text */
  Gtk::Label wine_version_label;      /*!< Wine version text */
  Gtk::Label wine_location_label;     /*!< Wine location text */
  Gtk::Label debug_log_level_label;   /*!< Debug log level text */
  Gtk::Label wine_last_changed_label; /*!< Last changed text */
  Gtk::Label audio_driver_label;      /*!< Audio driver text */
  Gtk::Label virtual_desktop_label;   /*!< Virtual desktop text */
  Gtk::Label description_label;       /*!< description text */

  // Toolbar buttons
  Gtk::Button new_button;            /*!< New toolbar button */
  Gtk::Button edit_button;           /*!< Edit toolbar button */
  Gtk::Button clone_button;          /*!< Clone toolbar button */
  Gtk::Button configure_button;      /*!< Configure toolbar button */
  Gtk::Button run_button;            /*!< Run... toolbar button */
  Gtk::Button open_c_driver_button;  /*!< Open C:\ drive toolbar button */
  Gtk::Button reboot_button;         /*!< Reboot toolbar button */
  Gtk::Button update_button;         /*!< Update toolbar button */
  Gtk::Button open_log_file_button;  /*!< Open log file toolbar button */
  Gtk::Button kill_processes_button; /*!< Kill processes toolbar button */

  // Other various buttons
  Gtk::Button add_app_list_button;     /*!< Button that add shortcut item to application list */
  Gtk::Button remove_app_list_button;  /*!< Button that remove shortcut item to application list */
  Gtk::Button refresh_app_list_button; /*!< Button that refreshes the application list */

private:
  // Dialogs
  BusyDialog busy_dialog_;                 /*!< Busy dialog, when the user should wait until install is finished */
  DialogWindow info_dialog_;               /*!< Info dialog, showing an info message */
  DialogWindow warning_dialog_;            /*!< Warning dialog, showing a warning message */
  DialogWindow error_dialog_;              /*!< Error dialog, showing an error message */
  DialogWindow question_dialog_;           /*!< Question dialog, showing a question message (yes/no) */
  mutable std::mutex info_message_mutex_;  /*!< Synchronizes access to info message using mutex */
  mutable std::mutex error_message_mutex_; /*!< Synchronizes access to error message using mutex */
  mutable std::mutex new_version_mutex_;   /*!< Synchronizes access to new version using mutex */
  Glib::ustring info_message_;
  Glib::ustring error_message_;
  string new_version_;
  string unknown_menu_item_name_;
  string unknown_desktop_item_name_;
  BottleNewAssistant new_bottle_assistant_; /*!< New bottle wizard (behind the "new" toolbar button) */
  GeneralConfigData general_config_data_;   /*!< General config data */
  std::thread* thread_check_version_;       /*!< Thread for checking version */
  // Dispatchers for handling signals from the thread towards a GUI thread
  Glib::Dispatcher error_message_check_version_dispatcher_;
  Glib::Dispatcher info_message_check_version_dispatcher_;
  Glib::Dispatcher new_version_available_dispatcher_;
  Glib::Dispatcher check_version_finished_dispatcher_;
  std::vector<ToolbarButtonData> toolbar_buttons_; /*!< Toolbar buttons for menu management */

  // Signal handlers
  void on_bottle_row_clicked(Gtk::ListBoxRow* row);
  void on_app_list_search();
  void on_application_row_activated(unsigned int position);
  void on_new_bottle_apply();
  void on_update_toolbar_overflow();

  // Private methods
  void set_detailed_info(const BottleItem& bottle);
  void set_application_list(const string& prefix_path, const std::map<int, ApplicationData>& app_List);
  void add_application(const string& name, const string& description, const string& command, const string& icon_name, bool is_icon_full_path = false);
  void cleanup_check_version_thread();
  void check_wine_binary();
  void check_version_update(bool show_equal_or_error = false);
  void check_version(bool show_equal_or_error);
  void load_stored_window_settings();
  void create_left_panel();
  void create_right_panel();
  void set_sensitive_toolbar_buttons(bool sensitive);
  static void cc_list_box_update_header_func(Gtk::ListBoxRow* list_box_row, Gtk::ListBoxRow* before);
};
