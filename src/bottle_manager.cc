/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    bottle_manager.cc
 * \brief   The controller controls it all
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
#include "bottle_manager.h"
#include "bottle_config_file.h"
#include "bottle_item.h"
#include "dll_override_types.h"
#include "general_config_file.h"
#include "helper.h"
#include "main_window.h"
#include "signal_controller.h"
#include "wine_defaults.h"

#include <chrono>
#include <stdexcept>

/*************************************************************
 * Public member functions                                   *
 *************************************************************/

/**
 * \brief Constructor
 * \param main_window Address to the main Window
 */
BottleManager::BottleManager(MainWindow& main_window)
    : error_message_mutex_(),
      output_loging_mutex_(),
      error_message_winetricks_mutex_(),
      main_window_(main_window),
      active_bottle_(nullptr),
      is_wine64_bit_(false),
      is_logging_stderr_(true),
      error_message_(),
      error_message_winetricks_()
{
  // Connect internal dispatcher(s)
  update_bottles_dispatcher_.connect(sigc::bind(sigc::mem_fun(this, &BottleManager::update_config_and_bottles), "", false));
  write_log_dispatcher_.connect(sigc::mem_fun(this, &BottleManager::write_log_to_file));
  error_message_winetricks_dispatcher_.connect(sigc::mem_fun(this, &BottleManager::on_error_winetricks));
  winetricks_finished_dispatcher_.connect(sigc::mem_fun(this, &BottleManager::cleanup_install_update_winetricks_thread));
}

/**
 * \brief Destructor
 */
BottleManager::~BottleManager()
{
  // Avoid zombie thread
  this->cleanup_install_update_winetricks_thread();
}

/**
 * \brief Prepare method, called during initial start-up of the app
 */
void BottleManager::prepare()
{
  // Install or self-update winetricks if not yet present within a thread (async),
  // Winetricks script is used by WineGUI.
  if (!Helper::file_exists(Helper::get_winetricks_location()))
  {
    install_or_update_winetricks_thread(true);
  }
  else
  {
    install_or_update_winetricks_thread(false);
  }

  // Start the initial read from disk to fetch the bottles & update GUI
  // "" - during startup (no bottle name to select)
  // true - during startup
  // TODO: Run in thread, not blocking the main thread
  update_config_and_bottles("", true);
}

/**
 * \brief Write debugging logging to file handler
 */
void BottleManager::write_log_to_file()
{
  std::lock_guard<std::mutex> lock(error_message_mutex_);
  char last_char = output_logging_.back();
  // Needs new line at end of string?
  if (last_char != '\n')
  {
    output_logging_ += '\n';
  }
  Helper::write_to_log_file(logging_bottle_prefix_, output_logging_);
}

/**
 * \brief Helper method for cleaning the winetricks thread.
 */
void BottleManager::cleanup_install_update_winetricks_thread()
{
  if (thread_install_update_winetricks_ && thread_install_update_winetricks_->joinable())
  {
    thread_install_update_winetricks_->join();
    thread_install_update_winetricks_.reset();
  }
}

/**
 * \brief Show error winetricks error messages to the main window
 */
void BottleManager::on_error_winetricks()
{
  this->cleanup_install_update_winetricks_thread();

  {
    std::lock_guard<std::mutex> lock(error_message_winetricks_mutex_);
    main_window_.show_error_message(error_message_winetricks_);
  }
}

/**
 * \brief Install or self-update Winetricks within a thread.
 * \param install True to install/update winetricks, false to self-update
 */
void BottleManager::install_or_update_winetricks_thread(bool install)
{
  if (thread_install_update_winetricks_ == nullptr)
  {
    // Start the update winetricks thread
    thread_install_update_winetricks_ = std::make_unique<std::thread>(
        [this, install]
        {
          try
          {
            if (install)
            {
              Helper::install_or_update_winetricks();
            }
            else
            {
              Helper::self_update_winetricks();
            }
          }
          catch (const std::invalid_argument& msg)
          {
            std::cout << "WARN: " << msg.what() << std::endl;
          }
          catch (const std::runtime_error& error)
          {
            {
              std::lock_guard<std::mutex> lock(error_message_winetricks_mutex_);
              error_message_winetricks_ = error.what();
            }
            this->error_message_winetricks_dispatcher_.emit();
            return; // Stop thread prematurely
          }
          this->winetricks_finished_dispatcher_.emit(); // Clean-up the thread pointer
        });
  }
}

/**
 * \brief Update WineGUI Config and update bottles by reading the Wine Bottles from disk and update GUI
 * \param select_bottle_name If set, try to find the bottle with this name and set it as active bottle (used for newly created bottles)
 * \param is_startup Set to true if this function is called during start-up, otherwise false
 */
void BottleManager::update_config_and_bottles(const Glib::ustring& select_bottle_name, bool is_startup)
{
  // Read general & save config in bottle manager
  GeneralConfigData config_data = load_and_save_general_config();
  // Set/update main window about the latest general config data
  main_window_.set_general_config(config_data);

  bool try_to_restore = (active_bottle_ != nullptr);
  if (try_to_restore)
  {
    previous_active_bottle_index_ = active_bottle_->get_index();
    // Save the current bottle list size
    previous_bottles_list_size_ = bottles_.size();
  }

  // Clear bottles
  if (!bottles_.empty())
    bottles_.clear();

  // Get the bottle directories
  std::vector<string> bottle_dirs;
  try
  {
    bottle_dirs = get_bottle_paths();
  }
  catch (const std::runtime_error& error)
  {
    main_window_.show_error_message(error.what());
    return; // stop
  }

  if (bottle_dirs.size() > 0)
  {
    try
    {
      // Create wine bottles from bottle directories and wine version
      bottles_ = create_wine_bottles(bottle_dirs);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
      return; // stop
    }

    if (!bottles_.empty())
    {
      // Update main Window
      main_window_.set_wine_bottles(bottles_);

      // Is select_bottle_name set?
      if (!select_bottle_name.empty())
      {
        // Check if there is a bottle with the same name and select as active bottle
        auto it = std::find_if(bottles_.begin(), bottles_.end(),
                               [&select_bottle_name](const BottleItem& bottle) { return bottle.name() == select_bottle_name; });
        if (it != bottles_.end())
        {
          main_window_.select_row_bottle(*it);
          active_bottle_ = &(*it);
        }
      }
      // Is try_to_restore boolean true?
      // And: Is the bottle list size the same?
      // And: Is the previous index not bigger than the list size?
      else if (try_to_restore && (bottles_.size() == previous_bottles_list_size_) && ((size_t)previous_active_bottle_index_ < bottles_.size()))
      {
        // Let's reset the previous state!
        auto front = bottles_.begin();
        std::advance(front, previous_active_bottle_index_);
        main_window_.select_row_bottle(*front);
        // Set active bottle at the previous index
        active_bottle_ = &(*front);
      }
      else
      {
        // Default behaviour: Bottle list is changed, let's set the first bottle in the detailed info panel.
        // begin() gives us an iterator with the first element
        auto first = bottles_.begin();
        // Trigger select row, except during start-up (show_all will auto-select the first listbox item in GTK)
        if (!is_startup)
          main_window_.select_row_bottle(*first);
        // Set active bottle at the first
        active_bottle_ = &(*first);
      }
    }
    else
    {
      main_window_.show_error_message("Could not create an overview of Windows Machines. Empty list.");

      // Send reset signal to reset the active bottle to NULL
      reset_active_bottle.emit();
      // Reset locally
      active_bottle_ = nullptr;
    }
  }
  else
  {
    // Send reset signal to reset the active bottle to NULL
    reset_active_bottle.emit();
    // Reset locally
    active_bottle_ = nullptr;
  }
}

/**
 * \brief Create a new Wine Bottle (runs in thread!)
 * \param[in] caller                      - Signal Dispatcher pointer, in order to signal back events
 * \param[in] name                        - Bottle Name
 * \param[in] windows_version             - Windows OS version
 * \param[in] bit                         - Windows Bit (32/64-bit)
 * \param[in] virtual_desktop_resolution  - Virtual desktop resolution (empty if disabled)
 * \param[in] disable_gecko_mono          - Disable Gecko/Mono install
 * \param[in] audio                       - Audio Driver type
 */
void BottleManager::new_bottle(SignalController* caller,
                               const Glib::ustring& name,
                               BottleTypes::Windows windows_version,
                               BottleTypes::Bit bit,
                               const Glib::ustring& virtual_desktop_resolution,
                               bool disable_gecko_mono,
                               BottleTypes::AudioDriver audio)
{
  // First check if wine is installed
  int wineStatus = Helper::determine_wine_executable();
  if (wineStatus == -1)
  {
    {
      std::lock_guard<std::mutex> lock(error_message_mutex_);
      error_message_ = "Could not find wine binary. Please first install wine on your machine.";
    }
    caller->signal_error_message_during_create();
    return; // Stop thread prematurely
  }

  // Build prefix
  // Name of the bottle we be used as folder name as well
  std::vector<string> dirs{bottle_location_, name};
  string prefix_path = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
  bool bottle_created = false;

  // Check if prefix_path already exists, if so, abort and show error message
  if (Helper::dir_exists(prefix_path))
  {
    {
      std::lock_guard<std::mutex> lock(error_message_mutex_);
      error_message_ = "A Wine bottle with the same name already exists. Try another name.";
    }
    caller->signal_error_message_during_create();
    return; // Stop thread prematurely
  }

  try
  {
    // Now create a new Wine Bottle
    Helper::create_wine_bottle(is_wine64_bit_, prefix_path, bit, disable_gecko_mono);
    // Create default Bottle config data struct
    BottleConfigData bottle_config;
    bottle_config.name = name;
    bottle_config.description = "";        // By default empty description
    bottle_config.logging_enabled = false; // By default disable logging
    bottle_config.debug_log_level = 1;     // 1 (default) = Normal debug log level
    // Create empty custom app list
    std::map<int, ApplicationData> app_list;
    // Next, write the WineGUI bottle config file
    if (!BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list))
    {
      // TODO: Maybe a warning message to the user?
      // No critical failure, only log an error to console.
      std::cout << "Error: Could not write bottle config file." << std::endl;
    }
    bottle_created = true;
  }
  catch (const std::runtime_error& error)
  {
    {
      std::lock_guard<std::mutex> lock(error_message_mutex_);
      error_message_ = ("Something went wrong during creation of a new Windows machine!\n" + Glib::ustring(error.what()));
    }
    caller->signal_error_message_during_create();
    return; // Stop thread prematurely
  }

  // Continue with additional settings
  if (bottle_created)
  {
    // Always set the Windows Version (we do not know which Wine version the user is using)
    // Only change Windows OS when NOT default
    try
    {
      Helper::set_windows_version(prefix_path, windows_version);
    }
    catch (const std::runtime_error& error)
    {
      {
        std::lock_guard<std::mutex> lock(error_message_mutex_);
        error_message_ = ("Something went wrong during setting another Windows version.\n" + Glib::ustring(error.what()));
      }
      caller->signal_error_message_during_create();
      return; // Stop thread prematurely
    }

    // Only if virtual desktop is not empty, enable it
    if (!virtual_desktop_resolution.empty())
    {
      try
      {
        Helper::set_virtual_desktop(prefix_path, virtual_desktop_resolution);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(error_message_mutex_);
          error_message_ = ("Something went wrong during enabling virtual desktop mode.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_create();
        return; // Stop thread prematurely
      }
    }

    // Only if Audio driver is not default, change it
    if (audio != WineDefaults::AudioDriver)
    {
      try
      {
        Helper::set_audio_driver(prefix_path, audio);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(error_message_mutex_);
          error_message_ = ("Something went wrong during setting another audio driver.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_create();
        return; // Stop thread prematurely
      }
    }
  }

  // Wait until wineserver terminates
  Helper::wait_until_wineserver_is_terminated(prefix_path);

  // Trigger done signal, which will eventually use a Glib dispatcher to signal back to the GUI thread
  caller->signal_bottle_created();
}

/**
 * \brief Update existing Wine bottle (runs in thread)
 * \param[in] caller                      Signal Dispatcher pointer, in order to signal back events
 * \param[in] name                        Bottle Name
 * \param[in] folder_name                 Bottle Folder Name
 * \param[in] description                 Description text
 * \param[in] windows_version             Windows OS version
 * \param[in] virtual_desktop_resolution  Virtual desktop resolution (empty if disabled)ze
 * \param[in] audio                       Audio Driver type
 * \param[in] is_debug_logging            Enable/disable debug logging to disk
 * \param[in] debug_log_level             Bottle Debug Log Level
 */
void BottleManager::update_bottle(SignalController* caller,
                                  const Glib::ustring& name,
                                  const Glib::ustring& folder_name,
                                  const Glib::ustring& description,
                                  BottleTypes::Windows windows_version,
                                  const Glib::ustring& virtual_desktop_resolution,
                                  BottleTypes::AudioDriver audio,
                                  bool is_debug_logging,
                                  int debug_log_level)
{
  if (active_bottle_ != nullptr)
  {
    string prefix_path = active_bottle_->wine_location();

    bool need_update_bottle_config_file = false;
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> app_list; // App list is never dirty, so no need to check
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);
    if (active_bottle_->name() != name)
    {
      bottle_config.name = name;
      need_update_bottle_config_file = true;
    }
    if (active_bottle_->description() != description)
    {
      bottle_config.description = description;
      need_update_bottle_config_file = true;
    }
    if (active_bottle_->is_debug_logging() != is_debug_logging)
    {
      bottle_config.logging_enabled = is_debug_logging;
      need_update_bottle_config_file = true;
    }
    if (active_bottle_->debug_log_level() != debug_log_level)
    {
      bottle_config.debug_log_level = debug_log_level;
      need_update_bottle_config_file = true;
    }

    if (need_update_bottle_config_file)
    {
      if (!BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list))
      {
        // Silent error
        std::cout << "Error: Could not update bottle config file." << std::endl;
      }
    }

    if (active_bottle_->windows() != windows_version)
    {
      try
      {
        Helper::set_windows_version(prefix_path, windows_version);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(error_message_mutex_);
          error_message_ = ("Something went wrong during setting another Windows version.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_update();
        return; // Stop thread prematurely
      }
    }

    if (active_bottle_->virtual_desktop() != virtual_desktop_resolution)
    {
      if (!virtual_desktop_resolution.empty())
      {
        try
        {
          Helper::set_virtual_desktop(prefix_path, virtual_desktop_resolution);
        }
        catch (const std::runtime_error& error)
        {
          {
            std::lock_guard<std::mutex> lock(error_message_mutex_);
            error_message_ = ("Something went wrong during enabling virtual desktop mode.\n" + Glib::ustring(error.what()));
          }
          caller->signal_error_message_during_update();
          return; // Stop thread prematurely
        }
      }
      else
      {
        try
        {
          Helper::disable_virtual_desktop(prefix_path);
        }
        catch (const std::runtime_error& error)
        {
          {
            std::lock_guard<std::mutex> lock(error_message_mutex_);
            error_message_ = ("Something went wrong during disabling virtual desktop mode.\n" + Glib::ustring(error.what()));
          }
          caller->signal_error_message_during_update();
          return; // Stop thread prematurely
        }
      }
    }
    if (active_bottle_->audio_driver() != audio)
    {
      try
      {
        Helper::set_audio_driver(prefix_path, audio);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(error_message_mutex_);
          error_message_ = ("Something went wrong during setting another audio driver.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_update();
        return; // Stop thread prematurely
      }
    }

    // Wait until wineserver terminates
    Helper::wait_until_wineserver_is_terminated(prefix_path);

    // LAST but not least, rename Wine bottle folder
    // Do this after the wait on wineserver, since otherwise renaming may break the Wine installation during update
    if (active_bottle_->folder_name() != folder_name)
    {
      // Build new prefix
      std::vector<string> dirs{bottle_location_, folder_name};
      string new_prefix_path = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
      try
      {
        Helper::rename_wine_bottle_folder(prefix_path, new_prefix_path);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(error_message_mutex_);
          error_message_ = ("Something went wrong during during changing the folder name.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_update();
        return; // Stop thread prematurely
      }
    }
  }
  else
  {
    {
      std::lock_guard<std::mutex> lock(error_message_mutex_);
      error_message_ = "No current Windows Machine was set?";
    }
    caller->signal_error_message_during_update();
    return; // Stop thread prematurely
  }

  // Trigger done signal
  caller->signal_bottle_updated();
}

/**
 * \brief Clone an existing Wine bottle (runs in thread)
 * \param[in] caller                      Signal Dispatcher pointer, in order to signal back events
 * \param[in] name                        New Bottle Name
 * \param[in] folder_name                 New Bottle Folder Name
 * \param[in] description                 New Description text
 */
void BottleManager::clone_bottle(SignalController* caller,
                                 const Glib::ustring& name,
                                 const Glib::ustring& folder_name,
                                 const Glib::ustring& description)
{
  if (active_bottle_ != nullptr)
  {
    string orginal_prefix_path = active_bottle_->wine_location();

    // First do a clone of the bottle, using the new folder name as new prefix
    std::vector<string> dirs{bottle_location_, folder_name};
    string clone_prefix_path = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
    try
    {
      Helper::copy_wine_bottle_folder(orginal_prefix_path, clone_prefix_path);
    }
    catch (const std::runtime_error& error)
    {
      {
        std::lock_guard<std::mutex> lock(error_message_mutex_);
        error_message_ = ("Something went wrong during during the clone.\n" + Glib::ustring(error.what()));
      }
      caller->signal_error_message_during_clone();
      return; // Stop thread prematurely
    }

    // Now we update the cloned Wine Bottle config file
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> app_list; // App list is never dirty, so no need to check
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(clone_prefix_path);

    // Set new cloned name (and description)
    bottle_config.name = name;
    bottle_config.description = description;
    if (!BottleConfigFile::write_config_file(clone_prefix_path, bottle_config, app_list))
    {
      std::cout << "Error: Could not update bottle cloned config file." << std::endl;
      {
        std::lock_guard<std::mutex> lock(error_message_mutex_);
        error_message_ = "Could not update new bottle cloned configuration file.";
      }
      caller->signal_error_message_during_clone();
      return; // Stop thread prematurely
    }
  }
  else
  {
    {
      std::lock_guard<std::mutex> lock(error_message_mutex_);
      error_message_ = "No current Windows Machine was set? Unable to clone.";
    }
    caller->signal_error_message_during_clone();
    return; // Stop thread prematurely
  }

  // Trigger done signal
  caller->signal_bottle_cloned();
}

/**
 * \brief Remove the current active Wine bottle
 */
void BottleManager::delete_bottle()
{
  if (active_bottle_ != nullptr)
  {
    try
    {
      string prefix_path = active_bottle_->wine_location();
      Glib::ustring windows = BottleTypes::to_string(active_bottle_->windows());
      // Are you sure?
      Glib::ustring confirm_message = "Are you sure you want to <b>PERMANENTLY</b> remove machine named '" +
                                      Glib::Markup::escape_text(Helper::get_folder_name(prefix_path)) + "' running " + windows +
                                      "?\n\n<i>Note:</i> This action cannot be undone!";
      if (main_window_.show_confirm_dialog(confirm_message, true))
      {
        // Signal that bottle is removed
        bottle_removed.emit();
        Helper::remove_wine_bottle(prefix_path);
        this->update_config_and_bottles("", false);
      }
      else
      {
        // Nothing, canceled
      }
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
  }
  else
  {
    main_window_.show_error_message("No Windows Machine to remove, empty/no selection.");
  }
}

/**
 * \brief Signal handler when the active bottle changes, update active bottle
 * \param[in] bottle - New bottle
 */
void BottleManager::set_active_bottle(BottleItem* bottle)
{
  if (bottle != nullptr)
  {
    active_bottle_ = bottle;
  }
}

/**
 * \brief Get error message (stored from manager thread)
 * \return Return the error message
 */
const Glib::ustring& BottleManager::get_error_message() const
{
  std::lock_guard<std::mutex> lock(error_message_mutex_);
  return error_message_;
}

/**
 * \brief Run an executable (exe) or MSI file in Wine (using the current active bottle)
 * \param[in] program Path of the program (selected by the user)
 * \param[in] is_msi_file True, if you running a Windows Installer (MSI), otherwise False for EXE
 */
void BottleManager::run_executable(string program, bool is_msi_file = false)
{
  if (is_bottle_not_null())
  {
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program_prefix = is_msi_file ? "msiexec /i" : "start /unix";
    string working_directory = Glib::path_get_dirname(program);
    // Be-sure to execute the program between quotes (due to spaces)
    program = program_prefix + " \"" + program + "\"";
    auto& env_vars = active_bottle_->env_vars();

    std::thread t(
        [wine64 = std::move(is_wine64_bit_), wine_prefix, debug_log_level, program, working_directory, env_vars,
         logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_]
        {
          string output =
              Helper::run_program_under_wine(wine64, wine_prefix, debug_log_level, program, working_directory, env_vars, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
        });
    t.detach();
  }
}

/**
 * \brief Run a program in Wine (using the active selected bottle)
 * \param[in] program Program name you want to run/start
 */
void BottleManager::run_program(string program)
{
  if (is_bottle_not_null())
  {
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    // For all programs (except winetricks)
    if (!program.ends_with("winetricks --gui -q"))
    {
      string working_directory = "";
      // Be-sure to execute the program between quotes (due to spaces).
      if (program.starts_with("/"))
      {
        // TODO: Provide the user the option whether or not the working directory need to be set.
        // If true, we can use: working_directory = Glib::path_get_dirname(program);
        // And pass it alone with run_program_under_wine() below.

        // Add 'start /unix' for Unit style command, like application shortcuts
        program = "start /unix \"" + program + "\"";
      }
      else
      {
        // Add 'start' for Windows style commands, like 'notepad'
        program = "start \"" + program + "\"";
      }
      auto& env_vars = active_bottle_->env_vars();

      std::thread t(
          [wine64 = std::move(is_wine64_bit_), wine_prefix, debug_log_level, program, working_directory, env_vars,
           logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
           output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
           output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_]
          {
            string output =
                Helper::run_program_under_wine(wine64, wine_prefix, debug_log_level, program, working_directory, env_vars, true, logging_stderr);
            if (debug_logging && !output.empty())
            {
              {
                std::lock_guard<std::mutex> lock(output_logging_mutex);
                logging_bottle_prefix.get() = wine_prefix;
                output_logging.get() = output;
              }
              write_log_dispatcher->emit();
            }
          });
      t.detach();
    }
    else
    {
      // We have an exception for winetricks, since that doesn't need the wine command
      std::thread t(
          [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
           output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
           output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_]
          {
            string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
            if (debug_logging && !output.empty())
            {
              {
                std::lock_guard<std::mutex> lock(output_logging_mutex);
                logging_bottle_prefix.get() = wine_prefix;
                output_logging.get() = output;
              }
              write_log_dispatcher->emit();
            }
          });
      t.detach();
    }
  }
}

/**
 * \brief Open the Wine C: drive on the current active bottle
 */
void BottleManager::open_c_drive()
{
  if (is_bottle_not_null())
  {
    if (!Gio::AppInfo::launch_default_for_uri(Glib::filename_to_uri(active_bottle_->wine_c_drive())))
    {
      main_window_.show_error_message("Could not open the C:/ drive.");
    }
  }
}

/**
 * \brief Reboot bottle
 */
void BottleManager::reboot()
{
  if (is_bottle_not_null())
  {
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    std::thread t(
        [wine64 = std::move(is_wine64_bit_), wine_prefix, debug_log_level, logging_stderr = std::move(is_logging_stderr_),
         debug_logging = std::move(is_debug_logging), output_logging_mutex = std::ref(output_loging_mutex_),
         logging_bottle_prefix = std::ref(logging_bottle_prefix_), output_logging = std::ref(output_logging_),
         write_log_dispatcher = &write_log_dispatcher_]
        {
          string output = Helper::run_program_under_wine(wine64, wine_prefix, debug_log_level, "wineboot -r", "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
        });
    t.detach();
    main_window_.show_info_message("Machine emulate reboot requested.");
  }
}

/**
 * \brief Update bottle
 */
void BottleManager::update()
{
  if (is_bottle_not_null())
  {
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    std::thread t(
        [wine64 = std::move(is_wine64_bit_), wine_prefix, debug_log_level, update_bottles_dispatcher = &update_bottles_dispatcher_,
         logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_]
        {
          string output = Helper::run_program_under_wine(wine64, wine_prefix, debug_log_level, "wineboot -u", "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          // Emit update bottles (via dispatcher, so the GUI update can take place in the GUI thread)
          update_bottles_dispatcher->emit();
        });
    t.detach();
  }
}

/**
 * \brief Open debug log of current bottle
 */
void BottleManager::open_log_file()
{
  if (is_bottle_not_null())
  {
    string log_file_path = Helper::get_log_file_path(active_bottle_->wine_location());
    if (Helper::file_exists(log_file_path))
    {
      if (!Gio::AppInfo::launch_default_for_uri(Glib::filename_to_uri(log_file_path)))
      {
        main_window_.show_error_message("Could not open log file.");
      }
    }
    else
    {
      main_window_.show_warning_message("There is no log file present (yet).\n\nPlease, be sure you <b>enabled</b> debug logging in the Edit "
                                        "window.\n\n Also did you ran something already?",
                                        true);
    }
  }
}

/**
 * \brief Kill running processes in bottle
 */
void BottleManager::kill_processes()
{
  if (is_bottle_not_null())
  {
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    std::thread t(
        [wine64 = std::move(is_wine64_bit_), wine_prefix, debug_log_level, logging_stderr = std::move(is_logging_stderr_),
         debug_logging = std::move(is_debug_logging), output_logging_mutex = std::ref(output_loging_mutex_),
         logging_bottle_prefix = std::ref(logging_bottle_prefix_), output_logging = std::ref(output_logging_),
         write_log_dispatcher = &write_log_dispatcher_]
        {
          string output = Helper::run_program_under_wine(wine64, wine_prefix, debug_log_level, "wineboot -k", "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
        });
    t.detach();
    main_window_.show_info_message("Kill processes requested.");
  }
}

/**
 * \brief Install D3DX9 (DirectX 9 OpenGL)
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of additional DirectX 9 DLLs, eg. 26 (for default use: "")
 */
void BottleManager::install_d3dx9(Gtk::Window& parent, const string& version)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing D3DX9 (OpenGL implementation of DirectX 9).");

    string package = "d3dx9";
    if (version != "")
    {
      package += "_" + version;
    }
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
    std::thread t(
        [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
         finish_dispatcher = &finished_package_install_dispatcher]
        {
          string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          finish_dispatcher->emit();
        });
    t.detach();
  }
}

/**
 * \brief Install DXVK (Vulkan based DirectX 9/10/11)
 * Note: initially only Direct3D 10 & 11 was supported by DXVK. But now also Direct3D 9.
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of DXVK, eg. 151 (for default use: "latest")
 */
void BottleManager::install_dxvk(Gtk::Window& parent, const string& version)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing DXVK (Vulkan-based implementation of DirectX 9, 10 and 11).\n");

    string package = "dxvk";
    if (version != "latest")
    {
      package += version;
    }
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
    std::thread t(
        [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
         finish_dispatcher = &finished_package_install_dispatcher]
        {
          string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          finish_dispatcher->emit();
        });
    t.detach();
  }
}

/**
 * \brief Install vkd3d-proton (Vulkan based DirectX 12)
 * \param[in] parent Parent GTK window were the request is coming from
 */
void BottleManager::install_vkd3d(Gtk::Window& parent)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing VKD3D (Vulkan-based implementation of DirectX 12).\n");

    string package = "vkd3d";
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
    std::thread t(
        [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
         finish_dispatcher = &finished_package_install_dispatcher]
        {
          string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          finish_dispatcher->emit();
        });
    t.detach();
  }
}

/**
 * \brief Install MS Visual C++ Redistributable Package
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of Visual C++, eg. 6, 2010, 2013, 2015, 2019 (no default)
 */
void BottleManager::install_visual_cpp_package(Gtk::Window& parent, const string& version)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing Visual C++ package (" + version + ").");

    string package = "vcrun" + version;
    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
    std::thread t(
        [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
         finish_dispatcher = &finished_package_install_dispatcher]
        {
          string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          finish_dispatcher->emit();
        });
    t.detach();
  }
}

/**
 * \brief Install MS .NET (deinstall Mono first if needed)
 * Idea: Install dotnet_verifier by default with it?
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of .NET, eg. '35' for 3.5, '471' for 4.7.1 or '35sp1' for 3.5 SP1 (no default)
 */
void BottleManager::install_dot_net(Gtk::Window& parent, const string& version)
{
  if (is_bottle_not_null())
  {
    if (main_window_.show_confirm_dialog("<i>Important note:</i> Wine Mono &amp; Gecko support is often sufficient enough.\n\nWine Mono will be "
                                         "<b>uninstalled</b> before native .NET will be installed.\n\nAre you sure you want to continue?",
                                         true))
    {
      // Before we execute the install, show busy dialog
      main_window_.show_busy_install_dialog(parent, "Installing Native .NET package (v" + version + ").\nThis may take quite some time!\n");

      string deinstall_command = this->get_deinstall_mono_command();

      string package = "dotnet" + version;
      string wine_prefix = active_bottle_->wine_location();
      bool is_debug_logging = active_bottle_->is_debug_logging();
      int debug_log_level = active_bottle_->debug_log_level();
      // I can't use -q with .NET installs
      string install_command = Helper::get_winetricks_location() + " " + package;
      string program = "";
      if (!deinstall_command.empty())
      {
        // First deinstall Mono then install native .NET
        program = deinstall_command + "; " + install_command;
      }
      else
      {
        program = install_command;
      }
      // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
      std::thread t(
          [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
           output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
           output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
           finish_dispatcher = &finished_package_install_dispatcher]
          {
            string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
            if (debug_logging && !output.empty())
            {
              {
                std::lock_guard<std::mutex> lock(output_logging_mutex);
                logging_bottle_prefix.get() = wine_prefix;
                output_logging.get() = output;
              }
              write_log_dispatcher->emit();
            }
            Helper::wait_until_wineserver_is_terminated(wine_prefix);
            finish_dispatcher->emit();
          });
      t.detach();
    }
    else
    {
      // Nothing, canceled
    }
  }
}

/**
 * \brief Install core fonts (which is often enough)
 * \param[in] parent Parent GTK window were the request is coming from
 */
void BottleManager::install_core_fonts(Gtk::Window& parent)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing MS Core fonts.");

    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program = Helper::get_winetricks_location() + " -q corefonts";
    // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
    std::thread t(
        [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
         finish_dispatcher = &finished_package_install_dispatcher]
        {
          string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          finish_dispatcher->emit();
        });
    t.detach();
  }
}

/**
 * \brief Install liberation fonts, open-source (which is often enough)
 * \param[in] parent Parent GTK window were the request is coming from
 */
void BottleManager::install_liberation(Gtk::Window& parent)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing Liberation open-source fonts.");

    string wine_prefix = active_bottle_->wine_location();
    bool is_debug_logging = active_bottle_->is_debug_logging();
    int debug_log_level = active_bottle_->debug_log_level();
    string program = Helper::get_winetricks_location() + " -q liberation";
    // finished_package_install_dispatcher signal is needed in order to close the busy dialog again
    std::thread t(
        [wine_prefix, debug_log_level, program, logging_stderr = std::move(is_logging_stderr_), debug_logging = std::move(is_debug_logging),
         output_logging_mutex = std::ref(output_loging_mutex_), logging_bottle_prefix = std::ref(logging_bottle_prefix_),
         output_logging = std::ref(output_logging_), write_log_dispatcher = &write_log_dispatcher_,
         finish_dispatcher = &finished_package_install_dispatcher]
        {
          string output = Helper::run_program(wine_prefix, debug_log_level, program, "", {}, true, logging_stderr);
          if (debug_logging && !output.empty())
          {
            {
              std::lock_guard<std::mutex> lock(output_logging_mutex);
              logging_bottle_prefix.get() = wine_prefix;
              output_logging.get() = output;
            }
            write_log_dispatcher->emit();
          }
          Helper::wait_until_wineserver_is_terminated(wine_prefix);
          finish_dispatcher->emit();
        });
    t.detach();
  }
}

/*************************************************************
 * Private member functions                                  *
 *************************************************************/

/**
 * \brief Load general configuration values from file and save them
 * \return GeneralConfigData
 */
GeneralConfigData BottleManager::load_and_save_general_config()
{
  GeneralConfigData general_config = GeneralConfigFile::read_config_file();
  bottle_location_ = general_config.default_folder;
  is_display_default_wine_machine_ = general_config.display_default_wine_machine;
  is_wine64_bit_ = Helper::determine_wine_executable() == 1;
  is_logging_stderr_ = general_config.enable_logging_stderr;
  return general_config;
}

bool BottleManager::is_bottle_not_null()
{
  bool is_null = (active_bottle_ == nullptr);
  if (is_null)
  {
    main_window_.show_error_message("No Windows Machine selected/empty. First create a new machine!\n\nAborted.");
  }
  return !is_null;
}

/**
 * \brief Wine Mono deinstall command, run before installing native .NET
 * \return uninstall Mono command
 * Note: When nothing todo, the command will be an empty string.
 */
string BottleManager::get_deinstall_mono_command()
{
  string command = "";
  if (active_bottle_ != nullptr)
  {
    string wine_prefix = active_bottle_->wine_location();
    string guid = Helper::get_wine_guid(is_wine64_bit_, wine_prefix, "Wine Mono Runtime");

    if (!guid.empty())
    {
      string uninstaller = "";
      switch (active_bottle_->bit())
      {
      case BottleTypes::Bit::win32:
        uninstaller = "wine uninstaller --remove";
        break;
      case BottleTypes::Bit::win64:
        uninstaller = "wine64 uninstaller --remove";
        break;
      }
      command = uninstaller + " '{" + guid + "}'";
    }
  }
  return command;
}

/**
 * \brief Get Wine version
 * \return Wine version
 */
string BottleManager::get_wine_version()
{
  // Read wine version (is always the same for all bottles atm)
  string wine_version = "";
  try
  {
    wine_version = Helper::get_wine_version(is_wine64_bit_);
  }
  catch (const std::runtime_error& error)
  {
    main_window_.show_error_message(error.what());
  }
  return wine_version;
}

/**
 * \brief Get Bottle Paths
 * \throws runtime_error when we can not created a Wine bottle directory or configuration folder could not be found
 * \return Return a map of bottle paths (string) and modification time (in ms)
 */
std::vector<string> BottleManager::get_bottle_paths()
{
  if (!Helper::dir_exists(bottle_location_))
  {
    // Create bottle prefix directory if not exist yet
    if (!Helper::create_dir(bottle_location_))
    {
      throw std::runtime_error("Failed to create the Wine bottle directory: " + bottle_location_);
    }
  }
  if (Helper::dir_exists(bottle_location_))
  {
    // Continue
    return Helper::get_bottles_paths(bottle_location_, is_display_default_wine_machine_);
  }
  else
  {
    throw std::runtime_error("Configuration directory still not found (probably no permissions):\n" + bottle_location_);
  }
  // Otherwise empty
  return std::vector<string>();
}

/**
 * \brief Create wine BottleItem objects and add them to a list.
 * \param[in] bottle_dirs  The list of bottle directories
 * \returns Array of Bottle Items
 */
std::list<BottleItem> BottleManager::create_wine_bottles(const std::vector<string>& bottle_dirs)
{
  std::list<BottleItem> bottles;
  Glib::ustring wine_version = get_wine_version();

  // Retrieve detailed info for each wine bottle prefix
  for (const string& prefix : bottle_dirs)
  {
    // Reset variables
    Glib::ustring folder_name = "";
    Glib::ustring virtual_desktop = "";
    BottleTypes::Bit bit = BottleTypes::Bit::win32;
    Glib::ustring c_drive_location = "- Unknown -";
    Glib::ustring last_time_wine_updated = "- Unknown -";
    BottleTypes::AudioDriver audio_driver = BottleTypes::AudioDriver::pulseaudio;
    BottleTypes::Windows windows = WineDefaults::WindowsOs;
    bool status = false;

    // Retrieve bottle config data & custom app list
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> bottle_app_list;
    std::tie(bottle_config, bottle_app_list) = BottleConfigFile::read_config_file(prefix);

    try
    {
      folder_name = Helper::get_folder_name(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }

    try
    {
      bit = Helper::get_windows_bitness(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
    try
    {
      c_drive_location = Helper::get_c_letter_drive(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
    try
    {
      last_time_wine_updated = Helper::get_last_wine_updated(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
    try
    {
      audio_driver = Helper::get_audio_driver(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
    try
    {
      windows = Helper::get_windows_version(prefix);
      status = Helper::get_bottle_status(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
    try
    {
      virtual_desktop = Helper::get_virtual_desktop(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }

    // Convert to Glib ustrings
    Glib::ustring name(bottle_config.name);
    Glib::ustring description(bottle_config.description);
    Glib::ustring prefix_path(prefix);
    BottleItem* bottle = new BottleItem(name, folder_name, description, status, windows, bit, wine_version, is_wine64_bit_, prefix_path,
                                        c_drive_location, last_time_wine_updated, audio_driver, virtual_desktop, bottle_config.logging_enabled,
                                        bottle_config.debug_log_level, bottle_config.env_vars, bottle_app_list);
    bottles.emplace_back(*bottle);
  }
  return bottles;
}
