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

  void get_result(bool& virtual_desktop_enabled,
    Glib::ustring& virtual_desktop_resolution,
    Glib::ustring& name,
    Glib::ustring& windows_version);

private:
  // Signal handlers:
  void on_assistant_apply();
  void on_assistant_cancel();
  void on_assistant_close();
  void on_assistant_prepare(Gtk::Widget* widget);
  void on_entry_changed();
  void on_virtual_desktop_toggle();
  bool apply_changes_gradually();

  // Member functions:
  void setDefaultValues();
  void createFirstPage();
  void createSecondPage();
  void createThirdPage();
  void print_status();

  // Child widgets:
  Gtk::Box m_vbox;
  Gtk::Box m_vbox2;
  Gtk::Box m_vbox3;
  Gtk::Box m_hbox_name;
  Gtk::Box m_hbox_win;
  Gtk::Box m_hbox_audio;
  Gtk::Box m_hbox_virtual_desktop;
  Gtk::Label intro_label;
  Gtk::Label name_label;
  Gtk::Label windows_version_label;
  Gtk::Label additional_label;
  Gtk::Label audiodriver_label;
  Gtk::Label virtual_desktop_resolution_label;
  Gtk::Label confirm_label;
  Gtk::Label apply_label;
  Gtk::ComboBoxText windows_version_combobox;
  Gtk::ComboBoxText audiodriver_combobox;
  Gtk::CheckButton virtual_desktop_check;
  Gtk::Entry name_entry;
  Gtk::Entry virtual_desktop_resolution_entry;
  Gtk::ProgressBar loading_bar;
};
