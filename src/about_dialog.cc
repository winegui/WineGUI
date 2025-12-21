/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    about_dialog.cc
 * \brief   The About dialog
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
#include "about_dialog.h"
#include "helper.h"
#include "project_config.h"

/**
 * \brief Constructor
 */
AboutDialog::AboutDialog(Gtk::Window& parent)
{
  // Set logo
  logo.set(Helper::get_image_location("logo.png"));
  // Set version
  std::vector<Glib::ustring> devs;
  devs.emplace_back("Melroy van den Berg <melroy@melroy.org>");

  set_transient_for(parent);
  set_program_name("WineGUI");
  set_comments("The most user-friendly WINE manager.");

  set_logo(logo.get_paintable());
  set_authors(devs);
  set_artists(devs);
  set_version(PROJECT_VER);
  set_copyright("Copyright © 2019-2025 Melroy van den Berg");
  set_license_type(Gtk::License::AGPL_3_0);
  set_website_label("Official GitLab Project");
  set_website("https://gitlab.melroy.org/melroy/winegui");

  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        return true; // stop default destroy
      },
      false);
}

AboutDialog::~AboutDialog()
{
}

/**
 * \brief Open about dialog window
 */
void AboutDialog::run_dialog()
{
  present();
}

/**
 * \brief Retrieve the app version
 * \return Version number
 */
std::string AboutDialog::get_version()
{
  return PROJECT_VER;
}
