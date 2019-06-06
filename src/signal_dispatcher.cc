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

//Menu& menu
SignalDispatcher::SignalDispatcher(Menu& menu, AboutDialog& about)
{
  menu.signalShowAbout.connect(sigc::mem_fun(about, &AboutDialog::show));
}

SignalDispatcher::~SignalDispatcher()
{}

void SignalDispatcher::setMainWindow(MainWindow* mainWindow)
{
  this->mainWindow = mainWindow;
}