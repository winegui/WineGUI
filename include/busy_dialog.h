/**
 * Copyright (c) 2020 WineGUI
 *
 * \file    busy_dialog.h
 * \brief   GTK+ Busy Dialog
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

#include <gtkmm.h>

using std::string;

/**
 * \class SettingsWindow
 * \brief GTK+ Window class for the settings
 */
class BusyDialog : public Gtk::Dialog
{
public:
  explicit BusyDialog(Gtk::Window& parent);
  virtual ~BusyDialog();

  void SetMessage(const Glib::ustring& message);
protected:
  Gtk::Label message_label;

private:
};
