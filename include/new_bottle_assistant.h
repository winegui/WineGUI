/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    new_bottle_assistant.h
 * \brief   New Bottle Assistant (Wizard)
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

/**
 * \class NewBottleAssistant
 * \brief New Bottle GTK+ Assistant (Wizard)
 */
class NewBottleAssistant : public Gtk::Assistant
{
public:
  NewBottleAssistant();
  virtual ~NewBottleAssistant();

  void get_result(bool& check_state, Glib::ustring& name, Glib::ustring& windows_version);

private:
  // Signal handlers:
  void on_assistant_apply();
  void on_assistant_cancel();
  void on_assistant_close();
  void on_assistant_prepare(Gtk::Widget* widget);
  void on_entry_changed();

  // Member functions:
  void createFirstPage();
  void createSecondPage();
  void createThirdPage();
  void print_status();

  // Child widgets:
  Gtk::Box m_vbox;
  Gtk::Box m_hbox_name;
  Gtk::Box m_hbox_win;
  Gtk::ComboBoxText windows_version_combobox;
  Gtk::Label intro_label, name_label, windows_version_label, confirm_label;
  Gtk::CheckButton m_check;
  Gtk::Entry name_entry;
};
