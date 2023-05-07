/**
 * Copyright (c) 2019-2023 WineGUI
 *
 * \file    bottle_new_assistant.h
 * \brief   New Wine bottle assistant (Wizard with steps)
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

#include "bottle_types.h"
#include <gtkmm.h>

/**
 * \class BottleNewAssistant
 * \brief New Wine Bottle GTK Assistant class
 */
class BottleNewAssistant : public Gtk::Assistant
{
public:
  // Signal
  sigc::signal<void> new_bottle_finished; /*!< Signal when New Bottle Assistant is finished */

  BottleNewAssistant();
  virtual ~BottleNewAssistant();

  void get_result(Glib::ustring& name,
                  BottleTypes::Windows& windows_version,
                  BottleTypes::Bit& bit,
                  Glib::ustring& virtual_desktop_resolution,
                  bool& disable_gecko_mono,
                  BottleTypes::AudioDriver& audio);

  void bottle_created();

  // Child widgets
  Gtk::Box vbox;
  Gtk::Box vbox2;
  Gtk::Box vbox3;
  Gtk::Box hbox_name;
  Gtk::Box hbox_win;
  Gtk::Box hbox_audio;
  Gtk::Box hbox_virtual_desktop;
  Gtk::Label intro_label;
  Gtk::Label name_label;
  Gtk::Label windows_version_label;
  Gtk::Label additional_label;
  Gtk::Label audio_driver_label;
  Gtk::Label virtual_desktop_resolution_label;
  Gtk::Label confirm_label;
  Gtk::Label apply_label;
  Gtk::ComboBoxText windows_version_combobox;
  Gtk::ComboBoxText audio_driver_combobox;
  Gtk::CheckButton virtual_desktop_check;
  Gtk::CheckButton disable_gecko_mono_check;
  Gtk::Entry name_entry;
  Gtk::Entry virtual_desktop_resolution_entry;
  Gtk::ProgressBar loading_bar;

private:
  sigc::connection timer_; /*!< Timer connection */

  // Signal handlers
  void on_assistant_apply();
  void on_assistant_cancel();
  void on_assistant_close();
  void on_assistant_prepare(Gtk::Widget* widget);
  void on_entry_changed();
  void on_virtual_desktop_toggle();
  bool apply_changes_gradually();

  // Member functions
  void set_default_values();
  void create_first_page();
  void create_second_page();
  void create_third_page();
};
