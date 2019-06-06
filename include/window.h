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
#include <vector>
#include "menu.h"
#include "wine_bottle.h"

#define READY_IMAGE "../images/ready.png"
#define WRONG_IMAGE "../images/wrong.png"

using std::string;

/**
 * \class Window
 * \brief GTK+ Window class
 */
class Window : public Gtk::Window
{
public:
  Window();
  virtual ~Window();
  void SetWineBottles(std::vector<WineBottle> bottles);
  void SetDetailedInfo(WineBottle bottle);
  void ShowAbout();
  
protected:
  // Child widgets
  Gtk::Box vbox;
  Gtk::Paned paned;
  Gtk::Image logo;
  Gtk::AboutDialog about;

  // Left widgets
  Gtk::ScrolledWindow scrolled_window;
  Gtk::ListBox listbox;

  // Right widgets
  Gtk::Box right_box;
  Gtk::Toolbar toolbar;
  Gtk::Separator separator1;
  Gtk::Grid detail_grid;
  // Detailed info labels on the right panel
  Gtk::Label name;
  Gtk::Label window_version;
  Gtk::Label wine_version;
  Gtk::Label wine_location;
  Gtk::Label c_drive_location;
  Gtk::Label wine_last_changed;
  Gtk::Label audio_driver;
  Gtk::Label virtual_desktop;

private:
  void CreateLeftPanel();
  void CreateRightPanel();

  static void cc_list_box_update_header_func(Gtk::ListBoxRow* row, Gtk::ListBoxRow* before);
};
