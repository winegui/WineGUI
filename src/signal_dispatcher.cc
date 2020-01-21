/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    signal_dispatcher.cc
 * \brief   Connect different signals and dispatch 
 *          (eg. Menu button clicks and new bottle wizard signals) them to the proper calls within WineGUI
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
#include "signal_dispatcher.h"

#include "main_window.h"
#include "bottle_manager.h"
#include "menu.h"
#include "preferences_window.h"
#include "about_dialog.h"
#include "edit_window.h"
#include "settings_window.h"
#include "helper.h"

/**
 * \brief Signal Dispatcher Constructor
 */
SignalDispatcher::SignalDispatcher(
  BottleManager& manager,
  Menu& menu, 
  PreferencesWindow& preferencesWindow,
  AboutDialog& about, 
  EditWindow& editWindow,
  SettingsWindow& settingsWindow)
: 
  mainWindow(nullptr),
  manager(manager),
  menu(menu),
  preferencesWindow(preferencesWindow),
  about(about),
  editWindow(editWindow),
  settingsWindow(settingsWindow),
  m_FinishDispatcher(),
  m_ErrorMessageDispatcher(),
  m_threadBottleManager(nullptr)
{
  // Nothing
}

/**
 * \brief Destructor, join (wait for) running threads, if applicable, to avoid zombies
 */
SignalDispatcher::~SignalDispatcher()
{
  // To avoid zombie threads
  CleanUpBottleManagerThread();
}

/**
 * \brief Set main window pointer to Signal Dispatcher
 */
void SignalDispatcher::SetMainWindow(MainWindow* mainWindow)
{
  this->mainWindow = mainWindow;
}

/**
 * \brief This method does all the signal connections between classes/emits/signals
 */
void SignalDispatcher::DispatchSignals()
{
  // Menu signals
  menu.signal_preferences.connect(sigc::mem_fun(preferencesWindow, &PreferencesWindow::show));
  menu.signal_quit.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_hide_window)); /*!< When quit button is pressed, hide main window and therefor closes the app */
  menu.signal_show_about.connect(sigc::mem_fun(about, &AboutDialog::show));
  menu.signal_refresh.connect(sigc::mem_fun(manager, &BottleManager::UpdateBottles));
  menu.signal_new_machine.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_new_bottle_button_clicked));
  menu.signal_run.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_run_button_clicked));
  menu.signal_open_drive_c.connect(sigc::mem_fun(manager, &BottleManager::OpenDriveC));
  menu.signal_edit_machine.connect(sigc::mem_fun(editWindow, &EditWindow::show));
  menu.signal_settings_machine.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::Show));
  menu.signal_remove_machine.connect(sigc::mem_fun(manager, &BottleManager::DeleteBottle));

  signal_show_edit_window.connect(sigc::mem_fun(editWindow, &EditWindow::Show));
  signal_show_settings_window.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::Show));

  // Distribute the active bottle signal
  mainWindow->activeBottle.connect(sigc::mem_fun(manager, &BottleManager::SetActiveBottle));
  mainWindow->activeBottle.connect(sigc::mem_fun(editWindow, &EditWindow::SetActiveBottle));
  mainWindow->activeBottle.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::SetActiveBottle));
  // Distribute the reset bottle signal
  manager.resetActiveBottle.connect(sigc::mem_fun(editWindow, &EditWindow::ResetActiveBottle));
  manager.resetActiveBottle.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::ResetActiveBottle));
  manager.resetActiveBottle.connect(sigc::mem_fun(*mainWindow, &MainWindow::ResetDetailedInfo));

  mainWindow->newBottle.connect(sigc::mem_fun(this, &SignalDispatcher::on_new_bottle));
  mainWindow->runProgram.connect(sigc::mem_fun(manager, &BottleManager::RunProgram));
  mainWindow->openDriveC.connect(sigc::mem_fun(manager, &BottleManager::OpenDriveC));
  mainWindow->rebootBottle.connect(sigc::mem_fun(manager, &BottleManager::Reboot));
  mainWindow->updateBottle.connect(sigc::mem_fun(manager, &BottleManager::Update));
  mainWindow->killRunningProcesses.connect(sigc::mem_fun(manager, &BottleManager::KillProcesses));

  // When bottle created, the finish (or error message) event is called
  m_FinishDispatcher.connect(sigc::mem_fun(this, &SignalDispatcher::on_new_bottle_created));
  m_ErrorMessageDispatcher.connect(sigc::mem_fun(this, &SignalDispatcher::on_error_message));

  // When the WineExec() results into a non-zero exit code the failureOnExec it triggered
  Helper& helper = Helper::getInstance();
  // Using Dispatcher instead of signal, will result in that the message box runs in the main thread.
  helper.failureOnExec.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_exec_failure));
}

/**
 * \brief Signal finish is called from within thread, 
 *  which can trigger the dispatcher so it can run a method 
 * (connected to the dispatcher signal) in the GUI thread
 */
void SignalDispatcher::SignalBottleCreated()
{
  m_FinishDispatcher.emit();
}

/**
 * \brief Helper method for Signal error message
 */
void SignalDispatcher::SignalErrorMessage()
{
  // Show error message
  m_ErrorMessageDispatcher.emit();
}

/**
 * \brief Helper method for cleaning the manage thread
 */
void SignalDispatcher::CleanUpBottleManagerThread()
{
  if (m_threadBottleManager)
  {
    if (m_threadBottleManager->joinable())
        m_threadBottleManager->join();
    delete m_threadBottleManager;
    m_threadBottleManager = nullptr;
  }
}

/************************************
 * Dispatch events from Main Window *
 ************************************/

bool SignalDispatcher::on_mouse_button_pressed(GdkEventButton* event)
{
  // Single click with right mouse button?
  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
  {
    Gtk::Menu* popup = menu.GetMachineMenu();
    if (popup)
    {
      popup->popup(event->button, event->time);
    }
    return true;
  }

  // Event has not been handled
  return false;
}

/**
 * \brief Update bottles in GUI (typically when the new wizard is finished)
 */
void SignalDispatcher::on_update_bottles()
{
  manager.UpdateBottles();
}

/**
 * \brief New Bottle signal, starting NewBottle() within thread
 */
void SignalDispatcher::on_new_bottle(
  Glib::ustring& name,
  Glib::ustring& virtual_desktop_resolution,
  BottleTypes::Windows windows_version,
  BottleTypes::Bit bit,
  BottleTypes::AudioDriver audio)
{
  if (m_threadBottleManager)
  {
    this->mainWindow->ShowErrorMessage("There is already running a thread. Please wait...");
    // Always close the wizard (signal 'finish')
    m_FinishDispatcher.emit();  
  }
  else
  {
    // Start a new manager thread (executing NewBottle())
    m_threadBottleManager = new std::thread(
      [this, name, virtual_desktop_resolution, windows_version, bit, audio]
      {
        manager.NewBottle(this, name, virtual_desktop_resolution, windows_version, bit, audio);
      });
  }
}

/******************************************
 * Dispatch events from dispatcher itself *
 * (indirectly from other classes)        *
 ******************************************/

/**
 * \brief Signal handler when a new bottle is created, dispatched from the manager thread
 */
void SignalDispatcher::on_new_bottle_created()
{
  CleanUpBottleManagerThread();

  this->mainWindow->on_new_bottle_created();
}

/**
 * \brief Fetch the error message from the manager (in a thread-safe manner), 
 * and report it to the main window (runs on the GUI thread).
 */
void SignalDispatcher::on_error_message()
{
  CleanUpBottleManagerThread();

  this->mainWindow->ShowErrorMessage(manager.GetErrorMessage());

  // Always close the wizard (signal 'finish')
  m_FinishDispatcher.emit();  
}