/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    settings_window.h
 * \brief   Settings GTK+ window class
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

#include "bottle_item.h"
#include <gtkmm.h>

using std::string;

// Forward declaration
class SignalDispatcher;

/**
 * \class SettingsWindow
 * \brief GTK+ Window class for the settings
 */
class SettingsWindow : public Gtk::Window
{
public:
  SettingsWindow(Gtk::Window& parent);
  virtual ~SettingsWindow();
  void SetDispatcher(SignalDispatcher& signalDispatcher);

  void SetActiveBottle(BottleItem* bottle);

protected:
  // Child widgets
  Gtk::Box vbox; /*!< The main vertical box */

private:
  BottleItem* currentBottle;  /*!< Current bottle to manage settings */

  // Private methods

};
