/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    main_window.h
 * \brief   GTK+ Main window class
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
#include "menu.h"
#include "bottle_item.h"
#include "new_bottle_assistant.h"

using std::string;


// Forward declaration
class SignalDispatcher;

/**
 * \class MainWindow
 * \brief GTK+ Window class
 */
class MainWindow : public Gtk::Window
{
public:
  // Signals
  sigc::signal<void, BottleItem*> activeBottle; /*!< Set the active bottle in manager, based on the selected bottle */
  sigc::signal<void, Glib::ustring&, Glib::ustring&, BottleTypes::Windows, BottleTypes::Bit, BottleTypes::AudioDriver> newBottle; /*!< Create new Wine Bottle Signal */
  sigc::signal<void, string, bool> runProgram; /*!< Run an EXE or MSI application in Wine with provided filename */
  
  MainWindow(Menu& menu);
  virtual ~MainWindow();
  void SetDispatcher(SignalDispatcher& signalDispatcher);

  void AppendWineBottle(BottleItem& bottle);
  void SetWineBottles(std::list<BottleItem>& bottles);
  void SetDetailedInfo(BottleItem& bottle);
  void ShowErrorMessage(const Glib::ustring& message);
  bool ShowConfirmDialog(const Glib::ustring& message);
  
  // Signal handers
  virtual void on_new_bottle_button_clicked();
  virtual void on_new_bottle_created();
  virtual void on_run_button_clicked();

protected:
  // Child widgets
  Gtk::Box vbox; /*!< The main vertical box */
  Gtk::Paned paned; /*!< The main paned panel (horizontal) */

  // Left widgets
  Gtk::ScrolledWindow scrolled_window; /*!< Scrolled Window container, which contains the listbox */
  Gtk::ListBox listbox; /*!< Listbox in the left panel */

  // Right widgets
  Gtk::Box right_box; /*!< Right panel horizontal box */
  Gtk::Toolbar toolbar; /*!< Toolbar at top */
  Gtk::Separator separator1; /*!< Seperator */
  Gtk::Grid detail_grid; /*!< Grid layout container to have multiple rows & columns below the toolbar */
  // Detailed info labels on the right panel
  Gtk::Label name; /*!< Bottle name */
  Gtk::Label window_version; /*!< Windows version text */
  Gtk::Label wine_version; /*!< Wine version text */
  Gtk::Label wine_location; /*!< Wine location text */
  Gtk::Label c_drive_location; /*!< C:\ drive location text */
  Gtk::Label wine_last_changed; /*!< Last changed text */
  Gtk::Label audio_driver; /*!< Audio driver text */
  Gtk::Label virtual_desktop; /*!< Virtual desktop text */

  // Toolbar buttons
  Gtk::ToolButton new_button; /*!< New toolbar button */
  Gtk::ToolButton run_button; /*!< Run... toolbar button */
  Gtk::ToolButton settings_button; /*!< Settings toolbar button */
  Gtk::ToolButton manage_button; /*!< Manage toolbar button */
  Gtk::ToolButton reboot_button; /*!< Reboot toolbar button */

private:
  NewBottleAssistant newBottleAssistant;  /*!< New bottle wizard (behind: new button toolbar) */

  // Signal handlers
  virtual void on_hide_window();
  virtual void on_row_clicked(Gtk::ListBoxRow* row);
  virtual void on_new_bottle_apply();

  // Private methods
  void CreateLeftPanel();
  void CreateRightPanel();
  static void cc_list_box_update_header_func(Gtk::ListBoxRow* row, Gtk::ListBoxRow* before);
};
