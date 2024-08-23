/**
 * Copyright (c) 2024 WineGUI
 *
 * \file    bottle_configure_env_var_window.h
 * \brief   Configure bottle environment variables
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

// Forward declaration
class BottleItem;

/**
 * \class BottleConfigureEnvVarWindow
 * \brief Add new application GTK Window class for the bottle environment variable
 */
class BottleConfigureEnvVarWindow : public Gtk::Window
{
public:
  // Signals
  // sigc::signal<void> config_saved; /*!< bottle config is saved signal */

  explicit BottleConfigureEnvVarWindow(Gtk::Window& parent);
  virtual ~BottleConfigureEnvVarWindow();

  void set_active_bottle(BottleItem* bottle);
  void reset_active_bottle();

protected:
  // Child widgets
  Gtk::Box vbox;          /*!< main vertical box */
  Gtk::Box hbox_buttons;  /*!< box for buttons */
  Gtk::Grid add_app_grid; /*!< grid layout for settings */

  Gtk::Label header_configure_env_var_label; /*!< header config env var label */
  Gtk::Label name_label;                     /*!< app name label */
  Gtk::Label description_label;              /*!< app description label */
  Gtk::Label command_label;                  /*!< app command label */
  Gtk::Entry name_entry;                     /*!< app name input field */
  Gtk::Entry description_entry;              /*!< app description input field */
  Gtk::Entry command_entry;                  /*!< app command input field */
  Gtk::Button select_executable_button;      /*!< select file executable button */
  Gtk::Button save_button;                   /*!< save button */
  Gtk::Button cancel_button;                 /*!< cancel button */

private:
  BottleItem* active_bottle_; /*!< Current active bottle */

  // Signal handlers
  void on_select_file();
  void on_select_dialog_response(int response_id, Gtk::FileChooserDialog* dialog);
  void on_cancel_button_clicked();
  void on_save_button_clicked();

  // Member functions
  void set_default_values();
};
