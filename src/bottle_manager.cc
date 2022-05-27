/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    bottle_manager.cc
 * \brief   The controller controls it all
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
#include "bottle_manager.h"
#include "bottle_item.h"
#include "config_file.h"
#include "dll_override_types.h"
#include "helper.h"
#include "main_window.h"
#include "signal_dispatcher.h"
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
    : mutex_(),
      main_window_(main_window),
      active_bottle_(nullptr),
      is_wine64_bit_(false),
      error_message_()
{
  // Connect internal dispatcher(s)
  update_bottles_dispatcher_.connect(sigc::mem_fun(this, &BottleManager::update_config_and_bottles));

  // TODO: Enable/disable tracing for the run_program commands (and make it configurable)
}

/**
 * \brief Destructor
 */
BottleManager::~BottleManager()
{
}

/**
 * \brief Prepare method, called during initial start-up of the app
 */
void BottleManager::prepare()
{
  // Install winetricks if not yet present,
  // Winetricks script is used by WineGUI.
  if (!Helper::file_exists(Helper::get_winetricks_location()))
  {
    try
    {
      Helper::install_or_update_winetricks();
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
  }
  else
  {
    // Update existing script
    try
    {
      Helper::self_update_winetricks();
    }
    catch (const std::invalid_argument& msg)
    {
      std::cout << "WARN: " << msg.what() << std::endl;
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }
  }

  // Start the initial read from disk to fetch the bottles & update GUI
  update_config_and_bottles();
}

/**
 * \brief Update WineGUI Config and update bottles by reading the Wine Bottles from disk and update GUI
 */
void BottleManager::update_config_and_bottles()
{
  // Update config
  load_config();

  // Clear bottles
  if (!bottles_.empty())
    bottles_.clear();

  // Get the bottle directories
  std::map<string, unsigned long> bottle_dirs;
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

      // Set first Bottle in the detailed info panel,
      // begin() gives you an iterator
      auto first = bottles_.begin();
      main_window_.set_detailed_info(*first);
      // Set active bottle at the first
      active_bottle_ = &(*first);
    }
    else
    {
      main_window_.show_error_message("Could not create an overview of Windows Machines. Empty list.");

      // Send reset signal to reset the active bottle to NULL
      reset_acctive_bottle.emit();
      // Reset locally
      active_bottle_ = nullptr;
    }
  }
  else
  {
    // Send reset signal to reset the active bottle to NULL
    reset_acctive_bottle.emit();
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
void BottleManager::new_bottle(SignalDispatcher* caller,
                               Glib::ustring name,
                               BottleTypes::Windows windows_version,
                               BottleTypes::Bit bit,
                               Glib::ustring virtual_desktop_resolution,
                               bool disable_gecko_mono,
                               BottleTypes::AudioDriver audio)
{
  // First check if wine is installed
  int wineStatus = Helper::determine_wine_executable();
  if (wineStatus == -1)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      error_message_ = "Could not find wine binary. Please first install wine on your machine.";
    }
    caller->signal_error_message_during_create();
    return; // Stop thread prematurely
  }

  // Build prefix
  std::vector<std::string> dirs{bottle_location_, name};
  auto prefix_path = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
  bool bottle_created = false;
  try
  {
    // Now create a new Wine Bottle
    Helper::create_wine_bottle(is_wine64_bit_, prefix_path, bit, disable_gecko_mono);
    bottle_created = true;
  }
  catch (const std::runtime_error& error)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      error_message_ = ("Something went wrong during creation of a new Windows machine!\n" + Glib::ustring(error.what()));
    }
    caller->signal_error_message_during_create();
    return; // Stop thread prematurely
  }

  // Continue with additional settings
  if (bottle_created)
  {
    // Only change Windows OS when NOT default
    if (windows_version != WineDefaults::WindowsOs)
    {
      try
      {
        Helper::set_windows_version(prefix_path, windows_version);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          error_message_ = ("Something went wrong during setting another Windows version.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_create();
        return; // Stop thread prematurely
      }
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
          std::lock_guard<std::mutex> lock(mutex_);
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
          std::lock_guard<std::mutex> lock(mutex_);
          error_message_ = ("Something went wrong during setting another audio driver.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_create();
        return; // Stop thread prematurely
      }
    }
  }

  // Wait until wineserver terminates
  Helper::wait_until_wineserver_is_terminated(prefix_path);

  // Trigger done signal
  caller->signal_bottle_created();
}

/**
 * \brief Update existing Wine bottle (runs in thread)
 * \param[in] caller                      - Signal Dispatcher pointer, in order to signal back events
 * \param[in] name                        - Bottle Name
 * \param[in] windows_version             - Windows OS version
 * \param[in] virtual_desktop_resolution  - Virtual desktop resolution (empty if disabled)ze
 * \param[in] audio                       - Audio Driver type
 */
void BottleManager::update_bottle(SignalDispatcher* caller,
                                  Glib::ustring name,
                                  BottleTypes::Windows windows_version,
                                  Glib::ustring virtual_desktop_resolution,
                                  BottleTypes::AudioDriver audio)
{
  if (active_bottle_ != nullptr)
  {
    Glib::ustring prefix_path = active_bottle_->wine_location();

    if (active_bottle_->windows() != windows_version)
    {
      try
      {
        Helper::set_windows_version(prefix_path, windows_version);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(mutex_);
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
            std::lock_guard<std::mutex> lock(mutex_);
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
            std::lock_guard<std::mutex> lock(mutex_);
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
          std::lock_guard<std::mutex> lock(mutex_);
          error_message_ = ("Something went wrong during setting another audio driver.\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_update();
        return; // Stop thread prematurely
      }
    }

    // Wait until wineserver terminates
    Helper::wait_until_wineserver_is_terminated(prefix_path);

    // LAST but not least, rename Wine bottle (=renaming folder)
    // Do this after the wait on wineserver, since otherwise renaming may break the Wine installation during update
    if (active_bottle_->name() != name)
    {
      // Build new prefix
      std::vector<std::string> dirs{bottle_location_, name};
      auto new_prefix_path = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
      try
      {
        Helper::rename_wine_bottle_folder(prefix_path, new_prefix_path);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          error_message_ = ("Something went wrong during during changing the name (= renaming the folder).\n" + Glib::ustring(error.what()));
        }
        caller->signal_error_message_during_update();
        return; // Stop thread prematurely
      }
    }
  }
  else
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      error_message_ = "No current Windows Machine was set?";
    }
    caller->signal_error_message_during_update();
    return; // Stop thread prematurely
  }

  // Trigger done signal
  caller->signal_bottle_updated();
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
      Glib::ustring prefix_path = active_bottle_->wine_location();
      string windows = BottleTypes::to_string(active_bottle_->windows());
      // Are you sure?
      Glib::ustring confirm_message = "Are you sure you want to <b>PERMANENTLY</b> remove machine named '" +
                                      Glib::Markup::escape_text(Helper::get_name(prefix_path)) + "' running " + windows +
                                      "?\n\n<i>Note:</i> This action cannot be undone!";
      if (main_window_.show_confirm_dialog(confirm_message, true))
      {
        // Signal that bottle is removed
        bottle_removed.emit();
        Helper::remove_wine_bottle(prefix_path);
        this->update_config_and_bottles();
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
const Glib::ustring& BottleManager::get_error_message()
{
  std::lock_guard<std::mutex> lock(mutex_);
  return error_message_;
}

/**
 * \brief Run a program in Wine (using the active selected bottle)
 * \param[in] filename - Filename location of the program, selected by user
 * \param[in] is_msi_file - Is the program you try to run a Windows Installer (MSI)? False is EXE.
 */
void BottleManager::run_program(string filename, bool is_msi_file = false)
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program_prefix = is_msi_file ? "msiexec /i" : "start /unix";
    // Be-sure to execute the filename also between brackets (in case of spaces)
    Glib::ustring program = program_prefix + " \"" + filename + "\"";
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, program, true, false);
    t.detach();
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
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "wineboot -r", false, false);
    t.detach();
    main_window_.show_info_message("Machine reboot emulated");
  }
}

/**
 * \brief Update bottle
 */
void BottleManager::update()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t([this, wine64 = std::move(is_wine64_bit_), wine_prefix] {
      Helper::run_program_under_wine(wine64, wine_prefix, "wineboot -u", false, false);
      // Emit update bottles (via dispatcher, so it can run in the GUI thread)
      this->update_bottles_dispatcher_.emit();
    });
    t.detach();
  }
}

/**
 * \brief Kill running processes in bottle
 */
void BottleManager::kill_processes()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "wineboot -k", false, false);
    t.detach();
    main_window_.show_info_message("Kill processes requested");
  }
}

/**
 * \brief Open Explorer browser
 */
void BottleManager::open_explorer()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "explorer", false, false);
    t.detach();
  }
}

/**
 * \brief Open (Wine) Windows Console
 */
void BottleManager::open_console()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "wineconsole", false, false);
    t.detach();
  }
}

/**
 * \brief Open Winecfg tool
 */
void BottleManager::open_winecfg()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "winecfg", false, false);
    t.detach();
  }
}

/**
 * \brief Open Winetricks tool in GUI mode
 */
void BottleManager::open_winetricks()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program = Helper::get_winetricks_location() + " --gui";
    std::thread t(&Helper::run_program, wine_prefix, program, true, false);
    t.detach();
  }
}

/**
 * \brief Open Uninstaller
 */
void BottleManager::open_uninstaller()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "uninstaller", false, false);
    t.detach();
  }
}

/**
 * \brief Open Task manager
 */
void BottleManager::open_task_manager()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "taskmgr", false, false);
    t.detach();
  }
}

/**
 * \brief Open Registery Editor
 */
void BottleManager::open_registery_editor()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "regedit", false, false);
    t.detach();
  }
}

/**
 * \brief Open Notepad
 */
void BottleManager::open_notepad()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "notepad", false, false);
    t.detach();
  }
}

/**
 * \brief Open Wordpad
 */
void BottleManager::open_wordpad()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "wordpad", false, false);
    t.detach();
  }
}

/**
 * \brief Open (Wine) Internet Explorer
 */
void BottleManager::open_iexplorer()
{
  if (is_bottle_not_null())
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    std::thread t(&Helper::run_program_under_wine, is_wine64_bit_, wine_prefix, "iexplore", false, false);
    t.detach();
  }
}

/**
 * \brief Install D3DX9 (DirectX 9 OpenGL)
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of additional DirectX 9 DLLs, eg. 26 (for default use: "")
 */
void BottleManager::install_d3dx9(Gtk::Window& parent, const Glib::ustring& version)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing D3DX9 (OpenGL implementation of DirectX 9).");

    Glib::ustring package = "d3dx9";
    if (version != "")
    {
      package += "_" + version;
    }
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install signal is needed in order to close the busy dialog again
    std::thread t(&Helper::run_program_with_finish_callback, wine_prefix, program, &finished_package_install, true, false);
    t.detach();
  }
}

/**
 * \brief Install DXVK (Vulkan based DirectX 9/10/11)
 * Note: initially only Direct3D 10 & 11 was supported by DXVK. But now also Direct3D 9.
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of DXVK, eg. 151 (for default use: "latest")
 */
void BottleManager::install_dxvk(Gtk::Window& parent, const Glib::ustring& version)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing DXVK (Vulkan-based implementation of DirectX 9, 10 and 11).\n");

    Glib::ustring package = "dxvk";
    if (version != "latest")
    {
      package += version;
    }
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install signal is needed in order to close the busy dialog again
    std::thread t(&Helper::run_program_with_finish_callback, wine_prefix, program, &finished_package_install, true, false);
    t.detach();
  }
}

/**
 * \brief Install MS Visual C++ Redistributable Package
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of Visual C++, eg. 2010, 2013, 2015 (no default)
 */
void BottleManager::install_visual_cpp_package(Gtk::Window& parent, const Glib::ustring& version)
{
  if (is_bottle_not_null())
  {
    // Before we execute the install, show busy dialog
    main_window_.show_busy_install_dialog(parent, "Installing Visual C++ package.");

    Glib::ustring package = "vcrun" + version;
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program = Helper::get_winetricks_location() + " -q " + package;
    // finished_package_install signal is needed in order to close the busy dialog again
    std::thread t(&Helper::run_program_with_finish_callback, wine_prefix, program, &finished_package_install, true, false);
    t.detach();
  }
}

/**
 * \brief Install MS .NET (deinstall Mono first if needed)
 * Idea: Install dotnet_verifier by default with it?
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of .NET, eg. '35' for 3.5, '471' for 4.7.1 or '35sp1' for 3.5 SP1 (no default)
 */
void BottleManager::install_dot_net(Gtk::Window& parent, const Glib::ustring& version)
{
  if (is_bottle_not_null())
  {
    if (main_window_.show_confirm_dialog("<i>Important note:</i> Wine Mono &amp; Gecko support is often sufficient enough.\n\nWine Mono will be "
                                         "<b>uninstalled</b> before native .NET will be installed.\n\nAre you sure you want to continue?",
                                         true))
    {
      // Before we execute the install, show busy dialog
      main_window_.show_busy_install_dialog(parent, "Installing Native .NET redistributable packages (v" + version +
                                                        ").\nThis may take quite some time...\n");

      Glib::ustring deinstall_command = this->get_deinstall_mono_command();

      Glib::ustring package = "dotnet" + version;
      Glib::ustring wine_prefix = active_bottle_->wine_location();
      // I can't use -q with .NET installs
      Glib::ustring install_command = Helper::get_winetricks_location() + " " + package;

      Glib::ustring program = "";
      if (!deinstall_command.empty())
      {
        // First deinstall Mono then install native .NET
        program = deinstall_command + "; " + install_command;
      }
      else
      {
        program = install_command;
      }
      // finished_package_install signal is needed in order to close the busy dialog again
      std::thread t(&Helper::run_program_with_finish_callback, wine_prefix, program, &finished_package_install, true, false);
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

    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program = Helper::get_winetricks_location() + " -q corefonts";
    // finished_package_install signal is needed in order to close the busy dialog again
    std::thread t(&Helper::run_program_with_finish_callback, wine_prefix, program, &finished_package_install, true, false);
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

    Glib::ustring wine_prefix = active_bottle_->wine_location();
    Glib::ustring program = Helper::get_winetricks_location() + " -q liberation";
    // finished_package_install signal is needed in order to close the busy dialog again
    std::thread t(&Helper::run_program_with_finish_callback, wine_prefix, program, &finished_package_install, true, false);
    t.detach();
  }
}

/*************************************************************
 * Private member functions                                  *
 *************************************************************/

/**
 * \brief Load configuration values from file and apply
 */
void BottleManager::load_config()
{
  ConfigData config = ConfigFile::read_config_file();
  bottle_location_ = config.default_folder;
  is_wine64_bit_ = ((Helper::determine_wine_executable() == 1) || config.prefer_wine64);
  is_debug_logging_ = config.enable_debug_logging;
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
Glib::ustring BottleManager::get_deinstall_mono_command()
{
  string command = "";
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    string guid = Helper::get_wine_guid(is_wine64_bit_, wine_prefix, "Wine Mono Runtime");

    if (!guid.empty())
    {
      Glib::ustring uninstaller = "";
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
 * \return Return a map of bottle paths (string) and modification time (in ms)
 */
std::map<string, unsigned long> BottleManager::get_bottle_paths()
{
  if (!Helper::dir_exists(bottle_location_))
  {
    // Create directory if not exist yet
    if (!Helper::create_dir(bottle_location_))
    {
      throw std::runtime_error("Failed to create the Wine bottles directory: " + bottle_location_);
    }
  }
  if (Helper::dir_exists(bottle_location_))
  {
    // Continue
    return Helper::get_bottles_paths(bottle_location_);
  }
  else
  {
    throw std::runtime_error("Configuration directory still not found (probably no permissions):\n" + bottle_location_);
  }
  // Otherwise empty
  return std::map<string, unsigned long>();
}

/**
 * \brief Create wine bottle classes and add them to the private bottles variable
 * \param[in] bottle_dirs  The list of bottle directories
 */
std::list<BottleItem> BottleManager::create_wine_bottles(std::map<string, unsigned long> bottle_dirs)
{
  std::list<BottleItem> bottles;
  string wine_version = get_wine_version();

  // Retrieve detailed info for each wine bottle prefix
  for (const auto& [prefix, _] : bottle_dirs)
  {
    std::ignore = _;
    // Reset variables
    string name = "";
    string virtualDesktop = "";
    BottleTypes::Bit bit = BottleTypes::Bit::win32;
    string c_drive_location = "- Unknown -";
    string last_time_wine_updated = "- Unknown -";
    BottleTypes::AudioDriver audio_driver = BottleTypes::AudioDriver::pulseaudio;
    BottleTypes::Windows windows = WineDefaults::WindowsOs;
    bool status = false;

    try
    {
      name = Helper::get_name(prefix);
    }
    catch (const std::runtime_error& error)
    {
      main_window_.show_error_message(error.what());
    }

    try
    {
      virtualDesktop = Helper::get_virtual_desktop(prefix);
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

    BottleItem* bottle = new BottleItem(name, status, windows, bit, wine_version, is_wine64_bit_, prefix, c_drive_location, last_time_wine_updated,
                                        audio_driver, virtualDesktop);
    bottles.push_back(*bottle);
  }
  return bottles;
}
