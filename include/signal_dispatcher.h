/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    signal_dispatcher.h
 * \brief   Gtkmm signal dispatcher
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

// Forward declaration
class MainWindow;
class Menu;
class AboutDialog;

/**
 * \class Window
 * \brief GTK+ Window class
 */
class SignalDispatcher : public Gtk::Window
{
  friend class MainWindow;

public:
  SignalDispatcher(Menu& menu, AboutDialog& about);
  virtual ~SignalDispatcher();

  void setMainWindow(MainWindow* mainWindow);
  
protected:

private:
  MainWindow* mainWindow;
};
