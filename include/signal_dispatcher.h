/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    signal_dispatcher.h
 * \brief   Handles different GTK signals and dispatch or connect them to other methods/handlers within WineGUI
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
#include <thread>

// Forward declaration
class MainWindow;
class BottleManager;
class Menu;
class PreferencesWindow;
class AboutDialog;
class BottleEditWindow;
class BottleSettingsWindow;
struct UpdateBottleStruct;

/**
 * \class SignalDispatcher
 * \brief Dispatch and manage GTK signals across the app
 */
class SignalDispatcher : public Gtk::Window
{
  friend class MainWindow;

public:
  // Signals
  sigc::signal<void> signal_show_edit_window;     /*!< show Edit window signal */
  sigc::signal<void> signal_show_settings_window; /*!< show Settings window signal */

  SignalDispatcher(BottleManager& manager,
                   Menu& menu,
                   PreferencesWindow& preferences_window,
                   AboutDialog& about_dialog,
                   BottleEditWindow& edit_window,
                   BottleSettingsWindow& settings_window);
  virtual ~SignalDispatcher();
  void set_main_window(MainWindow* main_window);
  void dispatch_signals();

  // signal_bottle_created() and signal_bottle_updated() are called from the thread bottle manager,
  // it's executed in the that thread. And can trigger the dispatcher (=thread safe), which gets executed in the GUI
  // thread.
  void signal_bottle_created();
  void signal_bottle_updated();
  void signal_error_message_during_create();
  void signal_error_message_during_update();

protected:
private:
  void cleanup_bottle_manager_thread();

  // slots
  virtual bool on_mouse_button_pressed(GdkEventButton* event);
  virtual void on_new_bottle(Glib::ustring& name,
                             BottleTypes::Windows windows_version,
                             BottleTypes::Bit bit,
                             Glib::ustring& virtual_desktop_resolution,
                             bool& disable_geck_mono,
                             BottleTypes::AudioDriver audio);
  virtual void on_update_bottle(const UpdateBottleStruct& update_bottle_struct);
  virtual void on_new_bottle_created();
  virtual void on_bottle_updated();
  virtual void on_error_message_created();
  virtual void on_error_message_updated();

  MainWindow* main_window_;
  BottleManager& manager_;
  Menu& menu_;
  PreferencesWindow& preferences_window_;
  AboutDialog& about_dialog_;
  BottleEditWindow& edit_window_;
  BottleSettingsWindow& settings_window_;

  // Dispatcher for handling signals from the thread towards a GUI thread
  Glib::Dispatcher bottle_created_dispatcher_;
  Glib::Dispatcher bottle_updated_dispatcher_;
  Glib::Dispatcher error_message_created_dispatcher_;
  Glib::Dispatcher error_message_updated_dispatcher_;
  // Thread for Bottle Manager (so it doesn't block the GUI thread)
  std::thread* thread_bottle_manager_;
};
