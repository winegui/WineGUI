/**
 * Copyright (c) 2019-2021 WineGUI
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
    auto preferences_menuitem = CreateImageMenuItem("Preferences", "system-run");
    preferences_menuitem->signal_activate().connect(preferences);
    auto exit_menuitem = CreateImageMenuItem("Exit", "application-exit");
    exit_menuitem->signal_activate().connect(quit);

    // View submenu
    auto refresh_menuitem = CreateImageMenuItem("Refresh List", "view-refresh");
    refresh_menuitem->signal_activate().connect(refresh_view);

    // Machine submenu
    auto newitem_menuitem = CreateImageMenuItem("New", "list-add");
    newitem_menuitem->signal_activate().connect(new_machine);
    auto run_menuitem = CreateImageMenuItem("Run...", "media-playback-start");
    run_menuitem->signal_activate().connect(run);
    auto open_drive_c_menuitem = CreateImageMenuItem("Open C: Drive", "drive-harddisk");
    open_drive_c_menuitem->signal_activate().connect(open_drive_c);
    auto edit_menuitem = CreateImageMenuItem("Edit", "document-edit");
    edit_menuitem->signal_activate().connect(edit_machine);
    auto settings_menuitem = CreateImageMenuItem("Settings", "preferences-other");
    settings_menuitem->signal_activate().connect(settings_machine);
    auto remove_menuitem = CreateImageMenuItem("Remove", "edit-delete");
    remove_menuitem->signal_activate().connect(remove_machine);

    // Help submenu
    auto feedback_menuitem = CreateImageMenuItem("Give feedback", "help-faq");
    feedback_menuitem->signal_activate().connect(give_feedback);
    auto about_menuitem = CreateImageMenuItem("About WineGUI...", "help-about");
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
    machine_submenu.append(*run_menuitem);
    machine_submenu.append(*open_drive_c_menuitem);
    machine_submenu.append(*edit_menuitem);
    machine_submenu.append(*settings_menuitem);
    machine_submenu.append(*remove_menuitem);

    // Help menu
    help_submenu.append(*feedback_menuitem);
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
Gtk::Menu *Menu::GetMachineMenu()
{
    return &machine_submenu;
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
Gtk::MenuItem *Menu::CreateImageMenuItem(const Glib::ustring &label_text, const Glib::ustring &icon_name)
{
    Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem());
    Gtk::Box *helper_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2));
    Gtk::Image *icon = Gtk::manage(new Gtk::Image());
    icon->set_from_icon_name(icon_name, Gtk::IconSize(Gtk::ICON_SIZE_MENU));
    helper_box->add(*icon);
    Gtk::Label *label = Gtk::manage(new Gtk::Label(label_text, 0.0, 0.0));
    helper_box->pack_end(*label, true, true, 0U);
    item->add(*helper_box);
    return item;
}
