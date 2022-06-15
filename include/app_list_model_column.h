/**
 * Copyright (c) 2022 WineGUI
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

#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelcolumn.h>
#include <string>

class AppListModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
  AppListModelColumns()
  {
    add(icon);
    add(name);
    add(description);
    add(command);
  }

  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
  Gtk::TreeModelColumn<Glib::ustring> name;
  Gtk::TreeModelColumn<Glib::ustring> description;
  Gtk::TreeModelColumn<std::string> command;
};
