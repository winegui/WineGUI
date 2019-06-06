/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    window.h
 * \brief   GTK+ Window class
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
#include "menu.h"

#define READY_IMAGE "../images/ready.png"
#define WRONG_IMAGE "../images/wrong.png"

/**
 * \class Window
 * \brief GTK+ Window class
 */
class Window : public Gtk::Window {
public:
  Window();
  virtual ~Window();

protected:
  // Child widgets
  Gtk::Box vbox;
  Gtk::Paned paned;

private:
  void CreateLeftPanel();
  void CreateRightPanel();

  static void cc_list_box_update_header_func(Gtk::ListBoxRow* row, Gtk::ListBoxRow* before);
};
