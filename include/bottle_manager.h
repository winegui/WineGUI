/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    bottle_manager.h
 * \brief   Bottle manager controller
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

#include <vector>
#include <string>
#include <glibmm/main.h>
#include "wine_bottle.h"
#include "helper.h"

using std::string;

// Forward declaration
class MainWindow;

/**
 * \class BottleManager
 * \brief Controller that controls it all
 */
class BottleManager
{
public:
  BottleManager(MainWindow& mainWindow);
  virtual ~BottleManager();

  void UpdateBottles();
  // Signals are possible:
  // Eg. sigc::signal<void> some_name; /*!< signal bla */

private:
  string BOTTLE_LOCATION;
  MainWindow& mainWindow;
  std::vector<WineBottle> bottles;
  WineBottle* current_bottle;

  string GetWineVersion();
  std::vector<string> ReadBottles();
  void CreateWineBottles(string wineVersion, std::vector<string> bottleDirs);
  void SetCurrentBottle(WineBottle* bottle);
};
