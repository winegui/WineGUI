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

  Gtk::Label header_configure_env_var_label;  /*!< header config env var label */
  Gtk::Label key_label;                       /*!< env key label */
  Gtk::Label value_label;                     /*!< env value label */
  Gtk::Label environment_variables_label;     /*!< Environment variables label */
  Gtk::Entry key_entry;                       /*!< env key input field */
  Gtk::Entry value_entry;                     /*!< env value input field */
  Gtk::Button add_button;                     /*!< Add button */
  Gtk::ListBox environment_variables_listbox; /*!< Environment variables list box */

private:
  BottleItem* active_bottle_; /*!< Current active bottle */

  // Signal handlers
  void on_add_button_clicked();

  // Member functions
  void set_default_values();
};
