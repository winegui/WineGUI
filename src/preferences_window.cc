/**
 * Copyright (c) 2019-2021 WineGUI
 *
 * \file    preferences_window.cc
 * \brief   Application preferences GTK+ window class
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
#include "preferences_window.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
PreferencesWindow::PreferencesWindow(Gtk::Window& parent)
{
  set_transient_for(parent);
  set_title("Application Preferences");
  set_default_size(650, 400);
  set_modal(true);

  text.set_text("Not implemented yet. Sorry :\\");
  vbox.pack_start(text);
  add(vbox);

  show_all_children();
}

/**
 * \brief Destructor
 */
PreferencesWindow::~PreferencesWindow()
{
}
