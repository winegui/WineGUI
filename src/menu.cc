/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    menu.cc
 * \brief   Main menu bar
 * \author  Melroy van den Berg <melroy@melroy.org>
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
 * \brief Constructor
 */
Menu::Menu() : file("_File", true), view("_View", true), machine("_Machine", true), help("_Help", true)
{
  // Add sub-menu's to menu items
  file.set_submenu(file_submenu);
  view.set_submenu(view_submenu);
  machine.set_submenu(machine_submenu);
  help.set_submenu(help_submenu);

  // File submenu
  // Using text + image
  auto preferences_menuitem = create_image_menu_item("Preferences", "system-run");
  preferences_menuitem->signal_activate().connect(preferences);
  auto exit_menuitem = create_image_menu_item("Exit", "application-exit");
  exit_menuitem->signal_activate().connect(quit);

  // View submenu
  auto refresh_menuitem = create_image_menu_item("Refresh List", "view-refresh");
  refresh_menuitem->signal_activate().connect(refresh_view);

  // Machine submenu
  auto newitem_menuitem = create_image_menu_item("New", "list-add");
  newitem_menuitem->signal_activate().connect(new_bottle);
  auto edit_menuitem = create_image_menu_item("Edit", "document-edit");
  edit_menuitem->signal_activate().connect(edit_bottle);
  auto clone_menuitem = create_image_menu_item("Clone", "edit-copy");
  clone_menuitem->signal_activate().connect(clone_bottle);
  auto configure_menuitem = create_image_menu_item("Configure", "preferences-other");
  configure_menuitem->signal_activate().connect(configure_bottle);
  auto run_menuitem = create_image_menu_item("Run...", "media-playback-start");
  run_menuitem->signal_activate().connect(run);
  auto remove_menuitem = create_image_menu_item("Remove", "edit-delete");
  remove_menuitem->signal_activate().connect(remove_bottle);
  auto open_drive_c_menuitem = create_image_menu_item("Open C: Drive", "drive-harddisk");
  open_drive_c_menuitem->signal_activate().connect(open_c_drive);
  auto open_log_file_menuitem = create_image_menu_item("Open Log file", "text-x-generic");
  open_log_file_menuitem->signal_activate().connect(open_log_file);

  // Help submenu
  auto feedback_menuitem = create_image_menu_item("Give feedback", "help-faq");
  feedback_menuitem->signal_activate().connect(give_feedback);
  auto list_issues_menuitem = create_image_menu_item("List work items", "emblem-package");
  list_issues_menuitem->signal_activate().connect(list_issues);
  auto check_update_menuitem = create_image_menu_item("Check for updates", "system-software-update");
  check_update_menuitem->signal_activate().connect(check_version);
  auto about_menuitem = create_image_menu_item("About WineGUI", "help-about");
  about_menuitem->signal_activate().connect(show_about);

  // Add items to sub-menu
  // File menu
  file_submenu.append(*preferences_menuitem);
  file_submenu.append(separator1);
  file_submenu.append(*exit_menuitem);

  // View menu
  view_submenu.append(*refresh_menuitem);

  // Machine menu
  machine_submenu.append(*newitem_menuitem);
  machine_submenu.append(separator2);
  machine_submenu.append(*edit_menuitem);
  machine_submenu.append(*clone_menuitem);
  machine_submenu.append(*configure_menuitem);
  machine_submenu.append(*run_menuitem);
  machine_submenu.append(*remove_menuitem);
  machine_submenu.append(separator3);
  machine_submenu.append(*open_drive_c_menuitem);
  machine_submenu.append(*open_log_file_menuitem);

  // Help menu
  help_submenu.append(*feedback_menuitem);
  help_submenu.append(*list_issues_menuitem);
  help_submenu.append(*check_update_menuitem);
  help_submenu.append(separator4);
  help_submenu.append(*about_menuitem);

  // Add menu items to menu bar
  append(file);
  append(view);
  append(machine);
  append(help);
}

/**
 * \brief Destructor
 */
Menu::~Menu()
{
}

/**
 * \brief Return the machine sub menu only
 * \return GTK::Menu pointer of the machine menu
 */
Gtk::Menu* Menu::get_machine_menu()
{
  return &machine_submenu;
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem* Menu::create_image_menu_item(const Glib::ustring& label_text, const Glib::ustring& icon_name)
{
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
