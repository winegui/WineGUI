/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    menu.cc
 * \brief   The menu
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
#include "menu.h"

/**
 * \brief Contructor
 */
Menu::Menu()
: file("_File", true),  
  view("_View", true),
  machine("_Machine", true),
  help("_Help", true)
{
  // Add sub-menu's to menu items
  file.set_submenu(file_submenu);
  view.set_submenu(view_submenu);
  machine.set_submenu(machine_submenu);
  help.set_submenu(help_submenu);

  // File submenu
  // Using text + image
  auto preferences = CreateImageMenuItem("Preferences", "preferences-other");
  auto save_item = CreateImageMenuItem("Save", "document-save");
  auto exit = CreateImageMenuItem("Exit", "application-exit");
  // Add appliaction quit signal to the exit button
  exit->signal_activate().connect(signalQuit);
  
  // View submenu
  auto refresh = CreateImageMenuItem("Refresh List", "view-refresh");
  refresh->signal_activate().connect(signalRefresh);

  // Machine submenu
  auto remove = CreateImageMenuItem("Remove...", "edit-delete");

  // Help submenu
  auto about = CreateImageMenuItem("About WineGUI...", "help-about");
  about->signal_activate().connect(signalShowAbout);
  // Template for creating a seperate method if addition actions are required:
  //    about->signal_activate().connect(sigc::mem_fun(*this, &Menu::on_help_about));
  
  // Add items to sub-menu
  // File menu
  file_submenu.append(*preferences);
  file_submenu.append(separator1);
  file_submenu.append(*save_item);
  file_submenu.append(separator2);
  file_submenu.append(*exit);

  // View menu
  view_submenu.append(*refresh);

  // Machine menu
  machine_submenu.append(*remove);

  // Help menu
  help_submenu.append(*about);
  
  // Add menu items to menu bar
  append(file);
  append(view);
  append(machine);
  append(help);
}

/**
 * \brief Destructor
 */
Menu::~Menu() {
}

/**
 * \brief Return the machine sub menu only
 * \return GTK::Menu pointer of the machine menu
 */
Gtk::Menu* Menu::getMachineMenu() {
  return &machine_submenu;
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem* Menu::CreateImageMenuItem(const Glib::ustring& label_text, const Glib::ustring& icon_name) {
  Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem());
  Gtk::Box* helper_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2));
  Gtk::Image* icon = Gtk::manage(new Gtk::Image());
  icon->set_from_icon_name(icon_name, Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  helper_box->add(*icon);
  Gtk::Label* label = Gtk::manage(new Gtk::Label(label_text, 0.0, 0.0));
  helper_box->pack_end(*label, true, true, 0U);
  item->add(*helper_box);
  return item;
}

