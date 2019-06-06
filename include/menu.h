/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    menu.h
 * \brief   The main-menu
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
 * \class Menu
 * \brief The top main-menu
 */
class Menu: public Gtk::MenuBar {
public:
  Menu();
  virtual ~Menu();

protected:
  // Child widgets
  Gtk::MenuItem file;
  Gtk::MenuItem help;

  Gtk::Menu file_submenu;
  Gtk::Menu help_submenu;

private:
  Gtk::MenuItem CreateImageMenuItem(const Glib::ustring& label_text, const Glib::ustring& icon_name);
  void ShowAbout();
};
