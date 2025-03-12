/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    signal_controller.cc
 * \brief   Manager and connect different signals and dispatchers
 *          (eg. Menu button clicks and new bottle wizard signals) them to the proper calls within WineGUI
 * \author  Melroy van den Berg <melroy@melroy.org>
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
#include "signal_controller.h"

#include "about_dialog.h"
#include "add_app_window.h"
#include "bottle_clone_window.h"
#include "bottle_configure_env_var_window.h"
#include "bottle_configure_window.h"
#include "bottle_edit_window.h"
#include "bottle_manager.h"
#include "helper.h"
#include "main_window.h"
#include "menu.h"
#include "preferences_window.h"
#include "remove_app_window.h"

/**
 * \brief Signal Dispatcher Constructor
 */
SignalController::SignalController(BottleManager& manager,
                                   Menu& menu,
                                   PreferencesWindow& preferences_window,
                                   AboutDialog& about_dialog,
                                   BottleEditWindow& edit_window,
                                   BottleCloneWindow& clone_window,
                                   BottleConfigureEnvVarWindow& configure_env_var_window,
                                   BottleConfigureWindow& configure_window,
                                   AddAppWindow& add_app_window,
                                   RemoveAppWindow& remove_app_window)
    : main_window_(nullptr),
      manager_(manager),
      menu_(menu),
      preferences_window_(preferences_window),
      about_dialog_(about_dialog),
      edit_window_(edit_window),
      clone_window_(clone_window),
      configure_env_var_window_(configure_env_var_window),
      configure_window_(configure_window),
      add_app_window_(add_app_window),
      remove_app_window_(remove_app_window)
{
  // Nothing
}

/**
 * \brief Destructor, join (wait for) running threads, if applicable, to avoid zombies
 */
SignalController::~SignalController()
{
  // To avoid zombie threads
  this->cleanup_bottle_manager_thread();
}

/**
 * \brief Set main window pointer to Signal Dispatcher
 */
void SignalController::set_main_window(MainWindow* main_window)
{
  if (main_window_ == nullptr)
  {
    main_window_ = main_window;
  }
  else
  {
    g_error("Something really strange is going on with setting the main window!");
  }
}

/**
 * \brief This method does all the signal connections between classes/emits/signals
 */
void SignalController::dispatch_signals()
{
  // Menu buttons
  menu_.preferences.connect(sigc::mem_fun(preferences_window_, &PreferencesWindow::show));
  menu_.quit.connect(
      sigc::mem_fun(*main_window_, &MainWindow::on_hide_window)); /*!< When quit button is pressed, hide main window and therefore closes the app */
  menu_.refresh_view.connect(sigc::bind(sigc::mem_fun(manager_, &BottleManager::update_config_and_bottles), "", false));
  menu_.new_bottle.connect(sigc::mem_fun(*main_window_, &MainWindow::on_new_bottle_button_clicked));
  menu_.run.connect(sigc::mem_fun(*main_window_, &MainWindow::on_run_button_clicked));
  menu_.edit_bottle.connect(sigc::mem_fun(edit_window_, &BottleEditWindow::show));
  menu_.clone_bottle.connect(sigc::mem_fun(clone_window_, &BottleCloneWindow::show));
  menu_.configure_bottle.connect(sigc::mem_fun(configure_window_, &BottleConfigureWindow::show));
  menu_.remove_bottle.connect(sigc::mem_fun(manager_, &BottleManager::delete_bottle));
  menu_.open_c_drive.connect(sigc::mem_fun(manager_, &BottleManager::open_c_drive));
  menu_.open_log_file.connect(sigc::mem_fun(manager_, &BottleManager::open_log_file));
  menu_.give_feedback.connect(sigc::mem_fun(*main_window_, &MainWindow::on_give_feedback));
  menu_.list_issues.connect(sigc::mem_fun(*main_window_, &MainWindow::on_issue_tickets));
  menu_.check_version.connect(sigc::mem_fun(main_window_, &MainWindow::on_check_version));
  menu_.show_about.connect(sigc::mem_fun(about_dialog_, &AboutDialog::run_dialog));
  about_dialog_.signal_response().connect(sigc::mem_fun(about_dialog_, &AboutDialog::hide_dialog));

  // Distribute the active bottle signal from Main Window
  main_window_->active_bottle.connect(sigc::mem_fun(manager_, &BottleManager::set_active_bottle));
  main_window_->active_bottle.connect(sigc::mem_fun(edit_window_, &BottleEditWindow::set_active_bottle));
  main_window_->active_bottle.connect(sigc::mem_fun(clone_window_, &BottleCloneWindow::set_active_bottle));
  main_window_->active_bottle.connect(sigc::mem_fun(configure_env_var_window_, &BottleConfigureEnvVarWindow::set_active_bottle));
  main_window_->active_bottle.connect(sigc::mem_fun(configure_window_, &BottleConfigureWindow::set_active_bottle));
  main_window_->active_bottle.connect(sigc::mem_fun(add_app_window_, &AddAppWindow::set_active_bottle));
  main_window_->active_bottle.connect(sigc::mem_fun(remove_app_window_, &RemoveAppWindow::set_active_bottle));
  // Distribute the reset bottle signal from the manager
  manager_.reset_active_bottle.connect(sigc::mem_fun(edit_window_, &BottleEditWindow::reset_active_bottle));
  manager_.reset_active_bottle.connect(sigc::mem_fun(clone_window_, &BottleCloneWindow::reset_active_bottle));
  manager_.reset_active_bottle.connect(sigc::mem_fun(configure_env_var_window_, &BottleConfigureEnvVarWindow::reset_active_bottle));
  manager_.reset_active_bottle.connect(sigc::mem_fun(configure_window_, &BottleConfigureWindow::reset_active_bottle));
  manager_.reset_active_bottle.connect(sigc::mem_fun(add_app_window_, &AddAppWindow::reset_active_bottle));
  manager_.reset_active_bottle.connect(sigc::mem_fun(remove_app_window_, &RemoveAppWindow::reset_active_bottle));
  manager_.reset_active_bottle.connect(sigc::mem_fun(*main_window_, &MainWindow::reset_detailed_info));
  manager_.reset_active_bottle.connect(sigc::mem_fun(*main_window_, &MainWindow::reset_application_list));
  // Removed bottle signal from the manager
  manager_.bottle_removed.connect(sigc::mem_fun(edit_window_, &BottleEditWindow::bottle_removed));
  // Package install finished (in settings window), close the busy dialog & refresh the settings window
  manager_.finished_package_install_dispatcher.connect(sigc::mem_fun(*main_window_, &MainWindow::close_busy_dialog));
  manager_.finished_package_install_dispatcher.connect(sigc::mem_fun(configure_window_, &BottleConfigureWindow::update_installed));

  // Menu / Toolbar actions
  main_window_->new_bottle.connect(sigc::mem_fun(this, &SignalController::on_new_bottle));
  main_window_->finished_new_bottle.connect(sigc::bind<1>(sigc::mem_fun(manager_, &BottleManager::update_config_and_bottles), false));
  main_window_->run_executable.connect(sigc::mem_fun(manager_, &BottleManager::run_executable));
  main_window_->run_program.connect(sigc::mem_fun(manager_, &BottleManager::run_program));
  main_window_->show_edit_window.connect(sigc::mem_fun(edit_window_, &BottleEditWindow::show));
  main_window_->show_clone_window.connect(sigc::mem_fun(clone_window_, &BottleCloneWindow::show));
  main_window_->show_configure_window.connect(sigc::mem_fun(configure_window_, &BottleConfigureWindow::show));
  main_window_->open_c_drive.connect(sigc::mem_fun(manager_, &BottleManager::open_c_drive));
  main_window_->reboot_bottle.connect(sigc::mem_fun(manager_, &BottleManager::reboot));
  main_window_->update_bottle.connect(sigc::mem_fun(manager_, &BottleManager::update));
  main_window_->open_log_file.connect(sigc::mem_fun(manager_, &BottleManager::open_log_file));
  main_window_->kill_running_processes.connect(sigc::mem_fun(manager_, &BottleManager::kill_processes));
  // App list
  main_window_->show_add_app_window.connect(sigc::mem_fun(add_app_window_, &AddAppWindow::show));
  main_window_->show_remove_app_window.connect(sigc::mem_fun(remove_app_window_, &RemoveAppWindow::show));

  // Edit Window
  edit_window_.configure_environment_variables.connect(sigc::mem_fun(configure_env_var_window_, &BottleConfigureEnvVarWindow::show));
  edit_window_.update_bottle.connect(sigc::mem_fun(this, &SignalController::on_update_bottle));
  edit_window_.remove_bottle.connect(sigc::mem_fun(manager_, &BottleManager::delete_bottle));

  // Clone Window
  clone_window_.clone_bottle.connect(sigc::mem_fun(this, &SignalController::on_clone_bottle));

  // Right click menu in listbox
  main_window_->right_click_menu.connect(sigc::mem_fun(this, &SignalController::on_mouse_button_pressed));

  // When bottle created, the finish (or error message) event is called
  bottle_created_dispatcher_.connect(sigc::mem_fun(this, &SignalController::on_new_bottle_created));
  bottle_updated_dispatcher_.connect(sigc::mem_fun(this, &SignalController::on_bottle_updated));
  bottle_cloned_dispatcher_.connect(sigc::mem_fun(this, &SignalController::on_bottle_cloned));
  error_message_created_dispatcher_.connect(sigc::mem_fun(this, &SignalController::on_error_message_created));
  error_message_updated_dispatcher_.connect(sigc::mem_fun(this, &SignalController::on_error_message_updated));
  error_message_cloned_dispatcher_.connect(sigc::mem_fun(this, &SignalController::on_error_message_cloned));

  // When the WineExec() results into a non-zero exit code the failure_on_exec it triggered
  Helper& helper = Helper::get_instance();
  // Using Dispatcher instead of signal, will result in that the message box runs in the main thread.
  helper.failure_on_exec.connect(sigc::mem_fun(*main_window_, &MainWindow::on_exec_failure));

  // Settings gaming package buttons
  configure_window_.directx9.connect(sigc::mem_fun(manager_, &BottleManager::install_d3dx9));
  configure_window_.dxvk.connect(sigc::mem_fun(manager_, &BottleManager::install_dxvk));
  configure_window_.vkd3d.connect(sigc::mem_fun(manager_, &BottleManager::install_vkd3d));
  // Settings additional package buttons
  configure_window_.liberation_fonts.connect(sigc::mem_fun(manager_, &BottleManager::install_liberation));
  configure_window_.corefonts.connect(sigc::mem_fun(manager_, &BottleManager::install_core_fonts));
  configure_window_.dotnet.connect(sigc::mem_fun(manager_, &BottleManager::install_dot_net));
  configure_window_.visual_cpp_package.connect(sigc::mem_fun(manager_, &BottleManager::install_visual_cpp_package));

  // Add new application Window
  add_app_window_.config_saved.connect(sigc::bind(sigc::mem_fun(manager_, &BottleManager::update_config_and_bottles), "", false));

  // Configure environment variables Window
  configure_env_var_window_.config_saved.connect(sigc::bind(sigc::mem_fun(manager_, &BottleManager::update_config_and_bottles), "", false));

  // Remove application Window
  remove_app_window_.config_saved.connect(sigc::bind(sigc::mem_fun(manager_, &BottleManager::update_config_and_bottles), "", false));

  // WineGUI Preference Window
  preferences_window_.config_saved.connect(sigc::bind(sigc::mem_fun(manager_, &BottleManager::update_config_and_bottles), "", false));
}

/**
 * \brief Signal bottle creation is finished, called from the thread.
 * Now we can trigger the dispatcher so it can run a method
 * (connected to the dispatcher signal) in the GUI thread
 */
void SignalController::signal_bottle_created()
{
  bottle_created_dispatcher_.emit();
}

/**
 * \brief Signal bottle updated is finished, called from the thread.
 *  Now we can trigger the dispatcher so it can run a method
 * (connected to the dispatcher signal) in the GUI thread
 */
void SignalController::signal_bottle_updated()
{
  bottle_updated_dispatcher_.emit();
}

/**
 * \brief Signal bottle cloned is finished, called from the thread.
 *  Now we can trigger the dispatcher so it can run a method
 * (connected to the dispatcher signal) in the GUI thread
 */
void SignalController::signal_bottle_cloned()
{
  bottle_cloned_dispatcher_.emit();
}

/**
 * \brief Signal error message during bottle creation,
 * called from the thread.
 */
void SignalController::signal_error_message_during_create()
{
  // Dispatch during error
  error_message_created_dispatcher_.emit();
}

/**
 * \brief Signal error message during bottle update,
 *  called from the thread.
 */
void SignalController::signal_error_message_during_update()
{
  // Dispatch during error
  error_message_updated_dispatcher_.emit();
}

/**
 * \brief Signal error message during bottle clone,
 *  called from the thread.
 */
void SignalController::signal_error_message_during_clone()
{
  // Dispatch during error
  error_message_cloned_dispatcher_.emit();
}

/**
 * \brief Helper method for cleaning the manage thread.
 */
void SignalController::cleanup_bottle_manager_thread()
{
  if (thread_bottle_manager_ && thread_bottle_manager_->joinable())
  {
    thread_bottle_manager_->join();
    thread_bottle_manager_.reset();
  }
}

/************************************
 * Dispatch events from Main Window *
 ************************************/

bool SignalController::on_mouse_button_pressed(GdkEventButton* event)
{
  // Single click with right mouse button?
  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
  {
    Gtk::Menu* popup = menu_.get_machine_menu();
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
 * \brief New Bottle signal, starting new_bottle() within thread
 */
void SignalController::on_new_bottle(Glib::ustring& name,
                                     BottleTypes::Windows windows_version,
                                     BottleTypes::Bit bit,
                                     Glib::ustring& virtual_desktop_resolution,
                                     bool& disable_geck_mono,
                                     BottleTypes::AudioDriver audio)
{
  if (thread_bottle_manager_)
  {
    main_window_->show_error_message("There is already running a thread. Please wait...");
    // Always close the wizard (signal as if the bottle was created)
    bottle_created_dispatcher_.emit();
  }
  else
  {
    // Start a new manager thread
    thread_bottle_manager_ = std::make_unique<std::thread>(
        [this, name, windows_version, bit, virtual_desktop_resolution, disable_geck_mono, audio]
        { manager_.new_bottle(this, name, windows_version, bit, virtual_desktop_resolution, disable_geck_mono, audio); });
  }
}

/**
 * \brief Update existing bottle signal, starting update_bottle() within thread
 */
void SignalController::on_update_bottle(const UpdateBottleStruct& update_bottle_struct)
{
  if (thread_bottle_manager_)
  {
    main_window_->show_error_message("There is already running a thread. Please wait...");
    // Close the edit window (signal as if the bottle was updated)
    error_message_updated_dispatcher_.emit();
  }
  else
  {
    // Start a new manager thread
    thread_bottle_manager_ = std::make_unique<std::thread>(
        [this, update_bottle_struct]
        {
          manager_.update_bottle(this, update_bottle_struct.name, update_bottle_struct.folder_name, update_bottle_struct.description,
                                 update_bottle_struct.windows_version, update_bottle_struct.virtual_desktop_resolution, update_bottle_struct.audio,
                                 update_bottle_struct.is_debug_logging, update_bottle_struct.debug_log_level);
        });
  }
}

/**
 * \brief Clone existing bottle signal, starting clone_bottle() within thread
 */
void SignalController::on_clone_bottle(const CloneBottleStruct& clone_bottle_struct)
{
  if (thread_bottle_manager_)
  {
    main_window_->show_error_message("There is already running a thread. Please wait...");
    // Close the clone window (signal as if the bottle was updated)
    error_message_cloned_dispatcher_.emit();
  }
  else
  {
    // Start a new manager thread
    thread_bottle_manager_ = std::make_unique<std::thread>(
        [this, clone_bottle_struct]
        { manager_.clone_bottle(this, clone_bottle_struct.name, clone_bottle_struct.folder_name, clone_bottle_struct.description); });
  }
}

/******************************************
 * Dispatch events from dispatcher itself *
 * (indirectly from other classes)        *
 ******************************************/

/**
 * \brief Signal handler when a new bottle is created, dispatched from the manager thread
 */
void SignalController::on_new_bottle_created()
{
  this->cleanup_bottle_manager_thread();

  // Inform the main window (which will inform the new bottle assistant)
  main_window_->on_new_bottle_created();
}

/**
 * \brief Signal handler when bottle is updated, dispatched from the manager thread
 */
void SignalController::on_bottle_updated()
{
  this->cleanup_bottle_manager_thread();

  // Inform the edit window
  edit_window_.on_bottle_updated();

  // Update bottle list
  manager_.update_config_and_bottles("", false);
}

/**
 * \brief Signal handler when bottle is cloned, dispatched from the manager thread
 */
void SignalController::on_bottle_cloned()
{
  this->cleanup_bottle_manager_thread();

  // Inform the clone window, returns newly cloned bottle name
  Glib::ustring new_cloned_bottle_name = clone_window_.on_bottle_cloned();

  // Update bottle list and select the cloned bottle
  manager_.update_config_and_bottles(new_cloned_bottle_name, false);
}

/**
 * \brief Fetch the error message from the manager during bottle creation (in a thread-safe manner),
 * and report it to the main window (runs on the GUI thread).
 */
void SignalController::on_error_message_created()
{
  this->cleanup_bottle_manager_thread();

  // Always close the wizard (signal as if the bottle was updated)
  bottle_created_dispatcher_.emit();

  main_window_->show_error_message(manager_.get_error_message());
}

/**
 * \brief Fetch the error message from the manager during bottle update (in a thread-safe manner),
 * and report it to the main window (runs on the GUI thread).
 */
void SignalController::on_error_message_updated()
{
  this->cleanup_bottle_manager_thread();

  // Always close the edit window (signal as if the bottle was updated)
  bottle_updated_dispatcher_.emit();

  main_window_->show_error_message(manager_.get_error_message());
}

/**
 * \brief Fetch the error message from the manager during bottle clone (in a thread-safe manner),
 * and report it to the main window (runs on the GUI thread).
 */
void SignalController::on_error_message_cloned()
{
  this->cleanup_bottle_manager_thread();

  // Always close the clone window (signal as if the bottle was updated)
  bottle_cloned_dispatcher_.emit();

  main_window_->show_error_message(manager_.get_error_message());
}
