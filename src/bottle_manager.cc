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
#include <stdexcept>

/**
 * \brief Constructor
 */
BottleManager::BottleManager(MainWindow& mainWindow): mainWindow(mainWindow)
{
  // Set NULL during init
  current_bottle = NULL;

  // TODO: Make it configurable via settings
  WINE_PREFIX = Glib::get_home_dir() + "/.winegui/prefixes";
  
  // Read wine version (is always the same for all bottles atm)
  string wineVersion = "";
  try
  {
    wineVersion = Helper::GetWineVersion();
  }
  catch (const std::runtime_error& error)
  {
      mainWindow.ShowErrorMessage(error.what());
  }
  // Read bottles from disk and create classes from it
  std::vector<string> bottleDirs = ReadBottles();
  CreateWineBottles(wineVersion, bottleDirs);
}

/**
 * \brief Destructor
 */
BottleManager::~BottleManager() {}

/**
 * \brief Read Wine bottles from disk at start-up
 */
std::vector<string> BottleManager::ReadBottles()
{
  if(!Helper::Exists(WINE_PREFIX)) {
    // Create directory if not exist yet
    if(g_mkdir_with_parents(WINE_PREFIX.c_str(), 0775) < 0 && errno != EEXIST) {
      printf("Failed to create WineGUI directory \"%s\": %s\n", WINE_PREFIX.c_str(), g_strerror(errno));
    }
  }

  if(Helper::Exists(WINE_PREFIX)) {
    // Continue
    return Helper::GetBottles(WINE_PREFIX);
  }
  else {
    mainWindow.ShowErrorMessage("Configuration directory not found (could not create):\n" + WINE_PREFIX);
  }
  // Otherwise empty
  return std::vector<string>();
}

/**
 * \brief Create wine bottle classes and add them to the private bottles variable
 */
void BottleManager::CreateWineBottles(string wineVersion, std::vector<string> bottleDirs)
{
  // Retrieve detailed info for each wine bottle prefix
  for(string prefix: bottleDirs) {
    string name = Helper::GetName(prefix);
    std::cout << name;
  }
}

/**
 * \brief Set the current selected bottle, the one you are working with
 * TODO: Should this be managed by the manager or GUI?
 */
void BottleManager::SetCurrentBottle(WineBottle* bottle)
{
  current_bottle = bottle;
}