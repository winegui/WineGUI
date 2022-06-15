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

#include "app_list_model_column.h"
#include "bottle_item.h"
#include "bottle_new_assistant.h"
#include "busy_dialog.h"
#include "general_config_file.h"
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
  void select_row_bottle(BottleItem& bottle);
  void reset_detailed_info();
  void reset_application_list();
  void set_general_config(const GeneralConfigData& config_data);
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

  AppListModelColumns app_list_columns; /*!< Application list model columns for app tree view */

  // Child widgets
  Gtk::Box vbox;    /*!< The main vertical box */
  Gtk::Paned paned; /*!< The main paned panel (horizontal) */
  // Left widgets
  Gtk::ScrolledWindow scrolled_window_listbox; /*!< Scrolled Window container, which contains the listbox */
  Gtk::ListBox listbox;                        /*!< Listbox in the left panel */
  // Right widgets
  Gtk::ScrolledWindow detail_grid_scrolled_window_detail; /*!< Scrolled Window container for the detail grid */
  Gtk::ScrolledWindow app_list_scrolled_window;           /*!< Scrolled Window container for app list */
  Gtk::Box right_vbox;                                    /*!< Right panel vertical box */
  Gtk::Box app_list_vbox;                                 /*!< Application list vertical box */
  Gtk::SearchEntry app_list_search_entry;                 /*!< Application list search entry */
  Gtk::Paned container_paned;                             /*!< Main container horizontal paned panel */
  Glib::RefPtr<Gtk::ListStore> app_list_tree_model;       /*!< Application list tree model (using a liststore)  */
  Gtk::Toolbar toolbar;                                   /*!< Toolbar at top */
  Gtk::Separator separator1;                              /*!< Seperator */
  Gtk::Grid detail_grid;                                  /*!< Grid layout container to have multiple rows & columns below the toolbar */
  Gtk::TreeView application_list_treeview;                /*!< List of applications put inside a tree view */

  // Test
  Gtk::CellRendererText name_desc_renderer_text;
  Gtk::TreeView::Column name_desc_column;

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
  GeneralConfigData general_config_data_;

  // Signal handlers
  virtual void on_row_clicked(Gtk::ListBoxRow* row);
  virtual void on_new_bottle_apply();

  // Private methods

  void set_detailed_info(BottleItem& bottle);
  void set_application_list(const string& prefix_path);
  void check_version_update(bool show_equal = false);
  void load_stored_window_settings();
  void create_left_panel();
  void create_right_panel();
  void set_sensitive_toolbar_buttons(bool sensitive);
  static void cc_list_box_update_header_func(Gtk::ListBoxRow* list_box_row, Gtk::ListBoxRow* before);
  void treeview_set_cell_data_name_desc(Gtk::CellRenderer* renderer, const Gtk::TreeModel::iterator& iter);
};
