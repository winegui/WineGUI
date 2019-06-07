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

#include <iostream>

BottleManager::BottleManager(MainWindow& mainWindow): mainWindow(mainWindow) {
  current_bottle = NULL;

  // TODO: Make it configurable via settings
  WINE_PREFIX = Glib::get_home_dir() + "/.winegui/prefixes";
  ReadBottles();
}

BottleManager::~BottleManager() {}

/**
 * \brief Read Wine bottles from disk
 */
void BottleManager::ReadBottles() {
  if(!Helper::exists(WINE_PREFIX)) {
    // Create directory if not exist yet
    if(g_mkdir_with_parents(WINE_PREFIX.c_str(), 0775) < 0 && errno != EEXIST) {
      printf("Failed to create WineGUI directory \"%s\": %s\n", WINE_PREFIX.c_str(), g_strerror(errno));
    }
  }

  // Continue
  if(Helper::exists(WINE_PREFIX)) {

  }
  else {
    mainWindow.ShowErrorMessage("Configuration directory not found (could not create):\n" + WINE_PREFIX);
  }

  //std::vector<string> bottles = Helper::retrieveBottles("home/melroy/");
  
  //catch(const fs::filesystem_error& e)

}

void BottleManager::SetCurrentBottle(WineBottle* bottle) {
  current_bottle = bottle;
}