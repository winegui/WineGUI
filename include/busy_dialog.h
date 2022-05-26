/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    busy_dialog.h
 * \brief   Busy Dialog (showing a loading process bar)
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
 * \class BusyDialog
 * \brief GTK+ Window class for the settings
 */
class BusyDialog : public Gtk::Dialog
{
public:
  explicit BusyDialog(Gtk::Window& parent);
  virtual ~BusyDialog();

  void show();
  void close();

  void set_message(const Glib::ustring& heading_text, const Glib::ustring& message);

protected:
  Gtk::Label heading_label;     /*!< Heading label */
  Gtk::Label message_label;     /*!< Message box label */
  Gtk::ProgressBar loading_bar; /*!< Loading bar */

private:
  sigc::connection timer_; /*!< Timer connection */
  Gtk::Window& default_parent_;

  virtual bool pulsing();
};
