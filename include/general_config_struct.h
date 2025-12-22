/**
 * Copyright (c) 2023-2025 WineGUI
 *
 * \file    general_config_struct.h
 * \brief   General configuration struct
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

#include <string>

/**
 * \struct GeneralConfigData
 * \brief Custom struct for general config data (stored in/retrieved from: ~/.config/winegui/config.ini)
 */
struct GeneralConfigData
{
  std::string default_folder;
  bool display_default_wine_machine;
  bool enable_logging_stderr;
  bool check_for_updates_startup;
};
