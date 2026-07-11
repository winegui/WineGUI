/**
 * Copyright (c) 2025 WineGUI
 *
 * \file    create_shortcut_window.h
 * \brief   Create menu or desktop shortcuts (.desktop files) for applications Window
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
#include <string>
#include <tuple>
#include <vector>

// Forward declaration
class BottleItem;

/**
 * \typedef ShortcutAppData
 * \brief One application in the create shortcut list: name, description and command
 */
using ShortcutAppData = std::tuple<Glib::ustring, Glib::ustring, std::string>;

/**
 * \class CreateShortcutWindow
 * \brief List of bottle applications, each offering a one-click "To Menu" / "To Desktop" host shortcut
 */
class CreateShortcutWindow : public Gtk::Window
{
public:
  explicit CreateShortcutWindow(Gtk::Window& parent);
  virtual ~CreateShortcutWindow();

  void set_active_bottle(BottleItem* bottle);
  void reset_active_bottle();
  void set_applications(const std::vector<ShortcutAppData>& applications);
  void show();

protected:
  // Child widgets
  Gtk::Box vbox;                       /*!< main vertical box */
  Gtk::Label header_label;             /*!< header label */
  Gtk::Label description_label;        /*!< explanation label */
  Gtk::ScrolledWindow scrolled_window; /*!< scrolled window around the app list */
  Gtk::ListBox app_list_box;           /*!< list of applications */
  Gtk::Box hbox_buttons;               /*!< box for the bottom buttons */
  Gtk::Button close_button;            /*!< close button */

private:
  BottleItem* active_bottle_;                 /*!< Current active bottle */
  std::vector<ShortcutAppData> applications_; /*!< Applications to show in the list */

  // Signal handlers
  void on_close_button_clicked();
  void on_create_clicked(const Glib::ustring& name, const Glib::ustring& description, const std::string& command, bool to_desktop);

  // Member functions
  void clear_list();
  void populate_list();
};
