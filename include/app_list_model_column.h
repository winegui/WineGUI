/**
 * Copyright (c) 2022-2023 WineGUI
 *
 * \file    app_list_model_column.h
 * \brief   Application list columns for treeview
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

#include "gdkmm/texture.h"
#include <gdkmm/pixbuf.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <string>

/**
 * \class AppListModelColumns
 * \brief List model for the application list, just based on Glib Object.
 */
class AppListModelColumns : public Glib::Object
{
public:
  Glib::ustring name;
  Glib::ustring description;
  Glib::RefPtr<Gdk::Texture> icon;
  std::string command;

  static Glib::RefPtr<AppListModelColumns> create(const Glib::ustring& col_name,
                                                  const Glib::ustring& col_description,
                                                  const Glib::RefPtr<Gdk::Texture>& col_icon,
                                                  const std::string& col_command)
  {
    return Glib::make_refptr_for_instance<AppListModelColumns>(new AppListModelColumns(col_name, col_description, col_icon, col_command));
  }

protected:
  AppListModelColumns(const Glib::ustring& col_name,
                      const Glib::ustring& col_description,
                      const Glib::RefPtr<Gdk::Texture>& col_icon,
                      const std::string& col_command)
      : name(col_name),
        description(col_description),
        icon(col_icon),
        command(col_command)
  {
  }
};
