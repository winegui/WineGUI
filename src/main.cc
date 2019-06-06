/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    main.cc
 * \brief   Main, where it all starts
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
#include "main_window.h"
#include "menu.h"
#include "bottle_manager.h"
#include "about_dialog.h"
#include "signal_dispatcher.h"

#include <gtkmm/application.h>

/**
 * \brief The beginning, start the main loop
 * \return Status code
 */
int main(int argc, char *argv[])
{
  auto app = Gtk::Application::create("org.melroy.winegui");
  
  // Constructing the top level objects:
  Menu menu;
  MainWindow mainWindow(menu);
  AboutDialog about(mainWindow);
  BottleManager bottleManager(mainWindow);

  SignalDispatcher signalDispatcher(bottleManager, menu, about);

  mainWindow.SetDispatcher(signalDispatcher);
  signalDispatcher.SetMainWindow(&mainWindow);
  // Do all the signal connections
  signalDispatcher.DispatchSignals();

  // Start main loop
  return app->run(mainWindow, argc, argv);
}