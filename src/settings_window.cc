/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    settings_window.cc
 * \brief   Setting GTK+ Window class
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
#include "settings_window.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
SettingsWindow::SettingsWindow(Gtk::Window& parent)
{
  set_transient_for(parent);
  set_title("Settings - Windows Machine");
  set_default_size(750, 540);
  set_modal(true);

}

/**
 * \brief Destructor
 */
SettingsWindow::~SettingsWindow() {}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void SettingsWindow::SetActiveBottle(BottleItem* bottle)
{
  this->currentBottle = bottle;
}