/**
 * Copyright (c) 2019-2022 WineGUI
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

#include "about_dialog.h"
#include "bottle_manager.h"
#include "edit_window.h"
#include "helper.h"
#include "main_window.h"
#include "menu.h"
#include "preferences_window.h"
#include "settings_window.h"

/**
 * \brief Signal Dispatcher Constructor
 */
SignalDispatcher::SignalDispatcher(BottleManager& manager,
                                   Menu& menu,
                                   PreferencesWindow& preferencesWindow,
                                   AboutDialog& about,
                                   EditWindow& editWindow,
                                   SettingsWindow& settingsWindow)
    : mainWindow(nullptr),
      manager(manager),
      menu(menu),
      preferencesWindow(preferencesWindow),
      about(about),
      editWindow(editWindow),
      settingsWindow(settingsWindow),
      m_bottleCreatedDispatcher(),
      m_errorMessageCreatedDispatcher(),
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
  this->CleanUpBottleManagerThread();
}

/**
 * \brief Set main window pointer to Signal Dispatcher
 */
void SignalDispatcher::SetMainWindow(MainWindow* mainWindow)
{
  if (this->mainWindow == nullptr)
  {
    this->mainWindow = mainWindow;
  }
  else
  {
    g_error("Something really strange is going on with setting the main window!");
  }
}

/**
 * \brief This method does all the signal connections between classes/emits/signals
 */
void SignalDispatcher::DispatchSignals()
{
  // Menu buttons
  menu.preferences.connect(sigc::mem_fun(preferencesWindow, &PreferencesWindow::show));
  menu.quit.connect(
      sigc::mem_fun(*mainWindow, &MainWindow::on_hide_window)); /*!< When quit button is pressed, hide main window and therefor closes the app */
  menu.refreshView.connect(sigc::mem_fun(manager, &BottleManager::UpdateBottles));
  menu.newBottle.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_new_bottle_button_clicked));
  menu.run.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_run_button_clicked));
  menu.openDriveC.connect(sigc::mem_fun(manager, &BottleManager::OpenDriveC));
  menu.editBottle.connect(sigc::mem_fun(editWindow, &EditWindow::show));
  menu.settingsBottle.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::Show));
  menu.removeMachine.connect(sigc::mem_fun(manager, &BottleManager::DeleteBottle));
  menu.giveFeedback.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_give_feedback));
  menu.showAbout.connect(sigc::mem_fun(about, &AboutDialog::RunDialog));
  about.signal_response().connect(sigc::mem_fun(about, &AboutDialog::HideDialog));

  // Distribute the active bottle signal from Main Window
  mainWindow->activeBottle.connect(sigc::mem_fun(manager, &BottleManager::SetActiveBottle));
  mainWindow->activeBottle.connect(sigc::mem_fun(editWindow, &EditWindow::SetActiveBottle));
  mainWindow->activeBottle.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::SetActiveBottle));
  // Distribute the reset bottle signal from the manager
  manager.resetActiveBottle.connect(sigc::mem_fun(editWindow, &EditWindow::ResetActiveBottle));
  manager.resetActiveBottle.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::ResetActiveBottle));
  manager.resetActiveBottle.connect(sigc::mem_fun(*mainWindow, &MainWindow::ResetDetailedInfo));
  // Removed bottle signal from the manager
  manager.bottleRemoved.connect(sigc::mem_fun(editWindow, &EditWindow::BottleRemoved));
  // Package install finished (in settings window), close the busy dialog & refresh the settings window
  manager.finishedPackageInstall.connect(sigc::mem_fun(*mainWindow, &MainWindow::CloseBusyDialog));
  manager.finishedPackageInstall.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::UpdateInstalled));

  // Menu / Toolbar actions
  mainWindow->newBottle.connect(sigc::mem_fun(this, &SignalDispatcher::on_new_bottle));
  mainWindow->finishedNewBottle.connect(sigc::mem_fun(manager, &BottleManager::UpdateBottles));
  mainWindow->showEditWindow.connect(sigc::mem_fun(editWindow, &EditWindow::Show));
  mainWindow->showSettingsWindow.connect(sigc::mem_fun(settingsWindow, &SettingsWindow::Show));
  mainWindow->runProgram.connect(sigc::mem_fun(manager, &BottleManager::RunProgram));
  mainWindow->openDriveC.connect(sigc::mem_fun(manager, &BottleManager::OpenDriveC));
  mainWindow->rebootBottle.connect(sigc::mem_fun(manager, &BottleManager::Reboot));
  mainWindow->updateBottle.connect(sigc::mem_fun(manager, &BottleManager::Update));
  mainWindow->killRunningProcesses.connect(sigc::mem_fun(manager, &BottleManager::KillProcesses));

  // Edit Window
  editWindow.updateBottle.connect(sigc::mem_fun(this, &SignalDispatcher::on_update_bottle));
  editWindow.removeBottle.connect(sigc::mem_fun(manager, &BottleManager::DeleteBottle));

  // Right click menu in listbox
  mainWindow->rightClickMenu.connect(sigc::mem_fun(this, &SignalDispatcher::on_mouse_button_pressed));

  // When bottle created, the finish (or error message) event is called
  m_bottleCreatedDispatcher.connect(sigc::mem_fun(this, &SignalDispatcher::on_new_bottle_created));
  m_bottleUpdatedDispatcher.connect(sigc::mem_fun(this, &SignalDispatcher::on_bottle_updated));
  m_errorMessageCreatedDispatcher.connect(sigc::mem_fun(this, &SignalDispatcher::on_error_message_created));
  m_errorMessageUpdatedDispatcher.connect(sigc::mem_fun(this, &SignalDispatcher::on_error_message_updated));

  // When the WineExec() results into a non-zero exit code the failureOnExec it triggered
  Helper& helper = Helper::getInstance();
  // Using Dispatcher instead of signal, will result in that the message box runs in the main thread.
  helper.failureOnExec.connect(sigc::mem_fun(*mainWindow, &MainWindow::on_exec_failure));

  // Settings gaming package buttons
  settingsWindow.directx9.connect(sigc::mem_fun(manager, &BottleManager::InstallD3DX9));
  settingsWindow.vulkan.connect(sigc::mem_fun(manager, &BottleManager::InstallDXVK));

  // Settings additional package buttons
  settingsWindow.liberation_fonts.connect(sigc::mem_fun(manager, &BottleManager::InstallLiberation));
  settingsWindow.corefonts.connect(sigc::mem_fun(manager, &BottleManager::InstallCoreFonts));
  settingsWindow.dotnet.connect(sigc::mem_fun(manager, &BottleManager::InstallDotNet));
  settingsWindow.visual_cpp_package.connect(sigc::mem_fun(manager, &BottleManager::InstallVisualCppPackage));

  // Settings additional tool buttons
  settingsWindow.uninstaller.connect(sigc::mem_fun(manager, &BottleManager::OpenUninstaller));
  settingsWindow.notepad.connect(sigc::mem_fun(manager, &BottleManager::OpenNotepad));
  settingsWindow.wordpad.connect(sigc::mem_fun(manager, &BottleManager::OpenWordpad));
  settingsWindow.iexplore.connect(sigc::mem_fun(manager, &BottleManager::OpenIexplore));
  settingsWindow.task_manager.connect(sigc::mem_fun(manager, &BottleManager::OpenTaskManager));
  settingsWindow.regedit.connect(sigc::mem_fun(manager, &BottleManager::OpenRegistertyEditor));

  // Settings fallback tool buttons
  settingsWindow.explorer.connect(sigc::mem_fun(manager, &BottleManager::OpenExplorer));
  settingsWindow.console.connect(sigc::mem_fun(manager, &BottleManager::OpenConsole));
  settingsWindow.winetricks.connect(sigc::mem_fun(manager, &BottleManager::OpenWinetricks));
  settingsWindow.winecfg.connect(sigc::mem_fun(manager, &BottleManager::OpenWinecfg));
}

/**
 * \brief Signal bottle creation is finished, called from the thread.
 * Now we can trigger the dispatcher so it can run a method
 * (connected to the dispatcher signal) in the GUI thread
 */
void SignalDispatcher::SignalBottleCreated()
{
  m_bottleCreatedDispatcher.emit();
}

/**
 * \brief Signal bottle updated is finished, called from the thread.
 *  Now we can trigger the dispatcher so it can run a method
 * (connected to the dispatcher signal) in the GUI thread
 */
void SignalDispatcher::SignalBottleUpdated()
{
  m_bottleUpdatedDispatcher.emit();
}

/**
 * \brief Signal error message during bottle creation,
 * called from the thread.
 */
void SignalDispatcher::SignalErrorMessageDuringCreate()
{
  // Show error message
  m_errorMessageCreatedDispatcher.emit();
}

/**
 * \brief Signal error message during bottle update,
 *  called from the thread.
 */
void SignalDispatcher::SignalErrorMessageDuringUpdate()
{
  // Show error message
  m_errorMessageUpdatedDispatcher.emit();
}

/**
 * \brief Helper method for cleaning the manage thread.
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
 * \brief New Bottle signal, starting NewBottle() within thread
 */
void SignalDispatcher::on_new_bottle(Glib::ustring& name,
                                     BottleTypes::Windows windows_version,
                                     BottleTypes::Bit bit,
                                     Glib::ustring& virtual_desktop_resolution,
                                     bool& disable_geck_mono,
                                     BottleTypes::AudioDriver audio)
{
  if (m_threadBottleManager)
  {
    this->mainWindow->ShowErrorMessage("There is already running a thread. Please wait...");
    // Always close the wizard (signal as if the bottle was created)
    m_bottleCreatedDispatcher.emit();
  }
  else
  {
    // Start a new manager thread (executing NewBottle())
    m_threadBottleManager = new std::thread([this, name, windows_version, bit, virtual_desktop_resolution, disable_geck_mono, audio] {
      manager.NewBottle(this, name, windows_version, bit, virtual_desktop_resolution, disable_geck_mono, audio);
    });
  }
}

/**
 * \brief Update existing bottle signal, starting UpdateBottle() within thread
 */
void SignalDispatcher::on_update_bottle(Glib::ustring& name,
                                        BottleTypes::Windows windows_version,
                                        Glib::ustring& virtual_desktop_resolution,
                                        BottleTypes::AudioDriver audio)
{
  if (m_threadBottleManager)
  {
    this->mainWindow->ShowErrorMessage("There is already running a thread. Please wait...");
    // Close the edit window (signal as if the bottle was updated)
    m_errorMessageUpdatedDispatcher.emit();
  }
  else
  {
    // Start a new manager thread (executing NewBottle())
    m_threadBottleManager = new std::thread([this, name, windows_version, virtual_desktop_resolution, audio] {
      manager.UpdateBottle(this, name, windows_version, virtual_desktop_resolution, audio);
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
  this->CleanUpBottleManagerThread();

  // Inform the main window (which will inform the new bottle assistant)
  this->mainWindow->on_new_bottle_created();
}

/**
 * \brief Signal handler when bottle is updated, dispatched from the manager thread
 */
void SignalDispatcher::on_bottle_updated()
{
  this->CleanUpBottleManagerThread();

  // Inform the edit window
  this->editWindow.on_bottle_updated();

  // Update bottle list
  manager.UpdateBottles();
}

/**
 * \brief Fetch the error message from the manager during bottle creation (in a thread-safe manner),
 * and report it to the main window (runs on the GUI thread).
 */
void SignalDispatcher::on_error_message_created()
{
  this->CleanUpBottleManagerThread();

  this->mainWindow->ShowErrorMessage(manager.GetErrorMessage());

  // Always close the wizard (signal as if the bottle was updated)
  m_bottleCreatedDispatcher.emit();
}

/**
 * \brief Fetch the error message from the manager during bottle update (in a thread-safe manner),
 * and report it to the main window (runs on the GUI thread).
 */
void SignalDispatcher::on_error_message_updated()
{
  this->CleanUpBottleManagerThread();

  this->mainWindow->ShowErrorMessage(manager.GetErrorMessage());

  // Always close the edit window (signal as if the bottle was updated)
  m_bottleUpdatedDispatcher.emit();
}
