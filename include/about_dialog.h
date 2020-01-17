/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    about_dialog.h
 * \brief   About Dialog
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

#include <gtkmm/aboutdialog.h>
#include <gtkmm/image.h>
#include <string>

// Use major.minor.patch syntax, don't forget to update CMakeLists.txt file as well!
static const std::string VERSION = "v1.1.0";

#if defined(PRODUCTION)
  #define IMAGE_LOCATION "/usr/share/winegui/images/" /*!< Image location */
#else
  #define IMAGE_LOCATION "../images/" /*!< Image location */
#endif

/**
 * \class AboutDialog
 * \brief The About dialog
 */
class AboutDialog : public Gtk::AboutDialog
{
public:
  explicit AboutDialog(Gtk::Window& parent);
  virtual ~AboutDialog();
  
  static std::string GetVersion();
protected:
  Gtk::Image logo; /*!< The logo of the app for the about window */
};
