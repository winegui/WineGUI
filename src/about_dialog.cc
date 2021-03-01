/**
 * Copyright (c) 2019-2021 WineGUI
 *
 * \file    about_dialog.cc
 * \brief   The About dialog
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
#include "about_dialog.h"
#include "helper.h"

/**
 * \brief Contructor
 */
AboutDialog::AboutDialog(Gtk::Window &parent)
{
    // Set logo
    logo.set(Helper::GetImageLocation("logo.png"));
    // Set version
    std::vector<Glib::ustring> devs;
    devs.push_back("Melroy van den Berg <melroy@melroy.org>");

    set_transient_for(parent);
    set_program_name("WineGui");
    set_comments("The most user-friendly WINE manager.");
    set_logo(logo.get_pixbuf());
    set_authors(devs);
    set_artists(devs);
    set_version(VERSION);
    set_copyright("Copyright © 2019-2020 Melroy van den Berg");
    set_license_type(Gtk::LICENSE_AGPL_3_0);
}

AboutDialog::~AboutDialog() {}

void AboutDialog::run_dialog()
{
    run();
}

void AboutDialog::hide_dialog(__attribute__((unused)) int response)
{
    hide();
}

/**
 * \brief Retrieve the app version
 * \return Version number
 */
std::string AboutDialog::GetVersion()
{
    return VERSION;
}
