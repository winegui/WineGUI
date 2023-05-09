/**
 * Copyright (c) 2023 WineGUI
 *
 * \file    remove_app_window.h
 * \brief   Remove application shortcut to the app list Window
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
 * \class RemoveAppWindow
 * \brief Remove application GTK Window class for the bottle app list
 */
class RemoveAppWindow : public Gtk::Window
{
public:
  // Signals
  sigc::signal<void> config_saved; /*!< bottle config is saved signal */

  explicit RemoveAppWindow(Gtk::Window& parent);
  virtual ~RemoveAppWindow();

  void show();
  void set_active_bottle(BottleItem* bottle);
  void reset_active_bottle();

protected:
  // Child widgets
  Gtk::Box vbox;                              /*!< main vertical box */
  Gtk::Box hbox_buttons;                      /*!< box for buttons */
  Gtk::Label header_remove_app_label;         /*!< header add app label */
  Gtk::Label header_remove_description_label; /*!< header explanation description label */

  Gtk::ListBox app_list_box;          /*!< application list box */
  Gtk::Button select_all_button;      /*!< select all button */
  Gtk::Button unselect_all_button;    /*!< unselect all button */
  Gtk::Button remove_selected_button; /*!< remove selected button */
  Gtk::Button cancel_button;          /*!< cancel button */

private:
  BottleItem* active_bottle_; /*!< Current active bottle */

  // Signal handlers
  void on_cancel_button_clicked();
  void on_remove_selected_button_clicked();
  void item();
};
