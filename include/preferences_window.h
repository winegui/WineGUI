/**
 * Copyright (c) 2019-2023 WineGUI
 *
 * \file    preferences_window.h
 * \brief   WineGUI Application preferences window
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

/**
 * \class PreferencesWindow
 * \brief WineGUI preferences GTK Window class
 */
class PreferencesWindow : public Gtk::Window
{
public:
  // Signals
  sigc::signal<void()> config_saved; /*!< config is saved signal */

  explicit PreferencesWindow(Gtk::Window& parent);
  virtual ~PreferencesWindow();

  void show();

protected:
  // Child widgets
  Gtk::Box vbox;         /*!< main vertical box */
  Gtk::Box hbox_buttons; /*!< box for buttons */

  Gtk::Label header_preferences_label;             /*!< Preferences header label */
  Gtk::Label default_wine_location_header;         /*!< Default Wine storage location header */
  Gtk::Label logging_stderr_header;                /*!< Logging stderr header */
  Gtk::Label default_wine_machine_header;          /*!< Default Wine machine header */
  Gtk::Label check_for_updates_header;             /*!< Check for updates header */
  Gtk::Entry default_folder_entry;                 /*!< Default Wine storage location input field */
  Gtk::Switch display_default_wine_machine_switch; /*!< Display default Wine machine switch */
  Gtk::Switch enable_logging_stderr_switch;        /*!< Debug logging switch */
  Gtk::Switch check_for_updates_switch;            /*!< Check for updates during startup switch */
  Gtk::Button select_folder_button;                /*!< Select folder button */
  Gtk::Button save_button;                         /*!< Save button */
  Gtk::Button cancel_button;                       /*!< Cancel button */

private:
  // Signal handlers
  void on_select_folder();
  void on_cancel_button_clicked();
  void on_save_button_clicked();
};
