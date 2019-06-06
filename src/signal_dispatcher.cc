/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    signal_dispatcher.cc
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
#include "signal_dispatcher.h"

#include "main_window.h"
#include "menu.h"
#include "about_dialog.h"

SignalDispatcher::SignalDispatcher(Menu& menu, AboutDialog& about)
: menu(menu),
  about(about) {}

SignalDispatcher::~SignalDispatcher() {}

void SignalDispatcher::SetMainWindow(MainWindow* mainWindow)
{
  this->mainWindow = mainWindow;
}

/**
 * \brief This method does all the signal connections between classes/emits/signals
 */
void SignalDispatcher::DispatchSignals()
{
  menu.signalQuit.connect(sigc::mem_fun(*this, &SignalDispatcher::on_quit));
  menu.signalShowAbout.connect(sigc::mem_fun(about, &AboutDialog::show));
}

void SignalDispatcher::on_quit()
{
  // Signal hide main window
  hideMainWindow();
  // needed at all?
  //Gtk::Main::quit();
}
