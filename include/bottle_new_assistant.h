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
#include <map>

/**
 * \struct NewBottleStruct
 * \brief Custom struct for new bottle data information
 */
struct NewBottleStruct
{
  Glib::ustring name;
  BottleTypes::Windows windows_version = BottleTypes::Windows::Unknown;
  BottleTypes::Bit bit = BottleTypes::Bit::win32;
  Glib::ustring virtual_desktop_resolution;
  bool disable_gecko_mono = false;
  BottleTypes::AudioDriver audio = BottleTypes::AudioDriver::pulseaudio;
  Glib::ustring wine_bin_path; /*!< Wine binary directory of the selected Wine runner (empty string = system Wine) */
};

/**
 * \class BottleNewAssistant
 * \brief New Wine Bottle GTK Assistant class
 */
class BottleNewAssistant : public Gtk::Assistant
{
public:
  // Signals
  sigc::signal<void(const Glib::ustring&)> new_bottle_finished; /*!< Signal when New Bottle Assistant is finished with the just created bottle name */
  sigc::signal<void(Gtk::Window*)> manage_runners;              /*!< Signal when the manage wine runners button is clicked */

  BottleNewAssistant();
  virtual ~BottleNewAssistant();

  NewBottleStruct get_result();
  void bottle_created();
  void refresh_wine_runner_list();

  // Child widgets
  Gtk::Box vbox;
  Gtk::Box vbox_runner;
  Gtk::Box vbox2;
  Gtk::Box vbox3;
  Gtk::Box hbox_name;
  Gtk::Box hbox_win;
  Gtk::Box hbox_runner;
  Gtk::Box hbox_audio;
  Gtk::Box hbox_virtual_desktop;
  Gtk::Label intro_label;
  Gtk::Label name_label;
  Gtk::Label windows_version_label;
  Gtk::Label runner_intro_label;
  Gtk::Label runner_label;
  Gtk::Label additional_label;
  Gtk::Label audio_driver_label;
  Gtk::Label virtual_desktop_resolution_label;
  Gtk::Label confirm_label;
  Gtk::Label apply_label;
  Gtk::ComboBoxText windows_version_combobox;
  Gtk::ComboBoxText wine_runner_combobox;
  Gtk::ComboBoxText audio_driver_combobox;
  Gtk::CheckButton virtual_desktop_check;
  Gtk::CheckButton disable_gecko_mono_check;
  Gtk::Entry name_entry;
  Gtk::Entry virtual_desktop_resolution_entry;
  Gtk::Button manage_runners_button;
  Gtk::ProgressBar loading_bar;

private:
  sigc::connection timer_;                        /*!< Timer connection */
  std::map<Glib::ustring, bool> runner_is_wow64_; /*!< Wine runner bin dir -> WoW64 (64-bit only), used to filter the Windows version list */

  // Signal handlers
  void on_assistant_apply();
  void on_assistant_cancel();
  void on_assistant_prepare(Gtk::Widget* widget);
  void on_entry_changed();
  void on_wine_runner_changed();
  void on_virtual_desktop_toggle();
  bool apply_changes_gradually();

  // Member functions
  void set_default_values();
  void refresh_windows_version_list();
  void create_first_page();
  void create_runner_page();
  void create_second_page();
  void create_third_page();
};
