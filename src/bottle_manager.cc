/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    controller.cc
 * \brief   The controller controls it all
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
#include "bottle_manager.h"
#include "main_window.h"

BottleManager::BottleManager(MainWindow& mainWindow): mainWindow(mainWindow) {
  current_bottle = NULL;

}

BottleManager::~BottleManager() {}

void BottleManager::SetCurrentBottle(WineBottle* bottle) {
  current_bottle = bottle;
}