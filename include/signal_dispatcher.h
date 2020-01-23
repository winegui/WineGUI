/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    signal_dispatcher.h
 * \brief   Handles different (Gtkmm) signals and dispatch or connect them to other methods within the App
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
#include <thread>
#include "bottle_types.h"

// Forward declaration
class MainWindow;
class BottleManager;
class Menu;
class PreferencesWindow;
class AboutDialog;
class EditWindow;
class SettingsWindow;

/**
 * \class SignalDispatcher
 * \brief Dispatch and manage GTK signals across the app
 */
class SignalDispatcher : public Gtk::Window
{
  friend class MainWindow;

public:
  // Signals
  sigc::signal<void> signal_show_edit_window; /*!< show Edit window signal */
  sigc::signal<void> signal_show_settings_window; /*!< show Settings window signal */

  SignalDispatcher(BottleManager& manager, 
  Menu& menu,
  PreferencesWindow& preferencesWindow,
  AboutDialog& about,
  EditWindow& editWindow,
  SettingsWindow& settingsWindow);
  virtual ~SignalDispatcher();
  void SetMainWindow(MainWindow* mainWindow);
  void DispatchSignals();  

  // SignalBottleCreated() is called from the thread bottle manager,
  // it's executed in the that thread. And can trigger the dispatcher (=thread safe), which gets executed in the GUI thread.
  void SignalBottleCreated();
  void SignalErrorMessage();
protected:

private:
  void CleanUpBottleManagerThread();

  // slots
  virtual bool on_mouse_button_pressed(GdkEventButton* event);
  virtual void on_update_bottles();
  virtual void on_new_bottle(Glib::ustring& name,
    Glib::ustring& virtual_desktop_resolution,
    bool& disable_geck_mono,
    BottleTypes::Windows windows_version,
    BottleTypes::Bit bit,
    BottleTypes::AudioDriver audio);
  virtual void on_new_bottle_created();
  virtual void on_error_message();

  MainWindow* mainWindow;
  BottleManager& manager;
  Menu& menu;
  PreferencesWindow& preferencesWindow;
  AboutDialog& about;
  EditWindow& editWindow;
  SettingsWindow& settingsWindow;

  // Dispatcher for handling signals from the thread towards a GUI thread
  Glib::Dispatcher m_FinishDispatcher;
  Glib::Dispatcher m_ErrorMessageDispatcher;
  // Thread for Bottle Manager (so it doesn't block the GUI thread)
  std::thread* m_threadBottleManager;
};
