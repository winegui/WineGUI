/**
 * Copyright (c) 2019-2021 WineGUI
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
 * \param mainWindow Address to the main Window
 */
BottleManager::BottleManager(MainWindow& mainWindow)
    : m_Mutex(),
      mainWindow(mainWindow),
      active_bottle(nullptr),
      is_wine64_bit(false),
      m_error_message()
{
  // TODO: Make it configurable via settings
  std::vector<std::string> dirs{Glib::get_home_dir(), ".winegui", "prefixes"};
  bottle_location = Glib::build_path(G_DIR_SEPARATOR_S, dirs);

  int wineStatus = Helper::DetermineWineExecutable();
  if (wineStatus == 1)
  {
    is_wine64_bit = true;
  }

  // TODO: Enable/disable tracing for the RunProgram commands (and make it configurable)
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
void BottleManager::Prepare()
{
  // Install winetricks if not yet present,
  // Winetricks script is used by WineGUI.
  if (!Helper::FileExists(Helper::GetWinetricksLocation()))
  {
    try
    {
      Helper::InstallOrUpdateWinetricks();
    }
    catch (const std::runtime_error& error)
    {
      mainWindow.ShowErrorMessage(error.what());
    }
  }
  else
  {
    // Update existing script
    try
    {
      Helper::SelfUpdateWinetricks();
    }
    catch (const std::invalid_argument& msg)
    {
      std::cout << "WARN: " << msg.what() << std::endl;
    }
    catch (const std::runtime_error& error)
    {
      mainWindow.ShowErrorMessage(error.what());
    }
  }

  // Start the initial read from disk to fetch the bottles & update GUI
  UpdateBottles();
}

/**
 * \brief Update bottles by reading the Wine Bottles from disk and update GUI
 */
void BottleManager::UpdateBottles()
{
  // Clear bottles
  if (!bottles.empty())
    bottles.clear();

  // Get the bottle directories
  std::map<string, unsigned long> bottleDirs;
  try
  {
    bottleDirs = GetBottlePaths();
  }
  catch (const std::runtime_error& error)
  {
    mainWindow.ShowErrorMessage(error.what());
    return; // stop
  }

  if (bottleDirs.size() > 0)
  {
    try
    {
      // Create wine bottles from bottle directories and wine version
      bottles = CreateWineBottles(GetWineVersion(), bottleDirs);
    }
    catch (const std::runtime_error& error)
    {
      mainWindow.ShowErrorMessage(error.what());
      return; // stop
    }

    if (!bottles.empty())
    {
      // Update main Window
      mainWindow.SetWineBottles(bottles);

      // Set first Bottle in the detailed info panel,
      // begin() gives you an iterator
      auto first = bottles.begin();
      mainWindow.SetDetailedInfo(*first);
      // Set active bottle at the first
      this->active_bottle = &(*first);
    }
    else
    {
      mainWindow.ShowErrorMessage("Could not create an overview of Windows Machines. Empty list.");

      // Send reset signal to reset the active bottle to NULL
      resetActiveBottle.emit();
      // Reset locally
      this->active_bottle = nullptr;
    }
  }
  else
  {
    // Send reset signal to reset the active bottle to NULL
    resetActiveBottle.emit();
    // Reset locally
    this->active_bottle = nullptr;
  }
}

/**
 * \brief Create a new Wine Bottle (runs in thread!)
 * \param[in] caller                      - Signal Dispatcher pointer, in order to signal back events
 * \param[in] name                        - Bottle Name
 * \param[in] virtual_desktop_resolution  - Virtual desktop resolution (empty if disabled)
 * \param[in] disable_gecko_mono          - Disable Gecko/Mono install
 * \param[in] windows_version             - Windows OS version
 * \param[in] bit                         - Windows Bit (32/64-bit)
 * \param[in] audio                       - Audio Driver type
 */
void BottleManager::NewBottle(SignalDispatcher* caller,
                              Glib::ustring name,
                              Glib::ustring virtual_desktop_resolution,
                              bool disable_gecko_mono,
                              BottleTypes::Windows windows_version,
                              BottleTypes::Bit bit,
                              BottleTypes::AudioDriver audio)
{
  // First check if wine is installed
  int wineStatus = Helper::DetermineWineExecutable();
  if (wineStatus == -1)
  {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_error_message = "Could not find wine binary. Please first install wine on your machine.";
    }
    caller->SignalErrorMessage();
    return; // Stop thread
  }

  // Calculate prefix
  std::vector<std::string> dirs{bottle_location, name};
  auto wine_prefix = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
  bool bottle_created = false;
  try
  {
    // Now create a new Wine Bottle
    Helper::CreateWineBottle(is_wine64_bit, wine_prefix, bit, disable_gecko_mono);
    bottle_created = true;
  }
  catch (const std::runtime_error& error)
  {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_error_message =
          ("Something went wrong during creation of a new Windows machine!\n" + Glib::ustring(error.what()));
    }
    caller->SignalErrorMessage();
    return; // Stop thread
  }

  // Continue with additional settings
  if (bottle_created)
  {
    // Only change Windows OS when NOT default
    if (windows_version != WineDefaults::WINDOWS_OS)
    {
      try
      {
        Helper::SetWindowsVersion(wine_prefix, windows_version);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(m_Mutex);
          m_error_message =
              ("Something went wrong during setting another Windows version.\n" + Glib::ustring(error.what()));
        }
        caller->SignalErrorMessage();
        return; // Stop thread
      }
    }

    // Only if virtual desktop is not empty, enable it
    if (!virtual_desktop_resolution.empty())
    {
      try
      {
        Helper::SetVirtualDesktop(wine_prefix, virtual_desktop_resolution);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(m_Mutex);
          m_error_message =
              ("Something went wrong during enabling virtual desktop mode.\n" + Glib::ustring(error.what()));
        }
        caller->SignalErrorMessage();
        return; // Stop thread
      }
    }

    // Only if Audio driver is not default, change it
    if (audio != WineDefaults::AUDIO_DRIVER)
    {
      try
      {
        Helper::SetAudioDriver(wine_prefix, audio);
      }
      catch (const std::runtime_error& error)
      {
        {
          std::lock_guard<std::mutex> lock(m_Mutex);
          m_error_message =
              ("Something went wrong during setting another audio driver.\n" + Glib::ustring(error.what()));
        }
        caller->SignalErrorMessage();
        return; // Stop thread
      }
    }

    // TODO: Finally add name to WineGUI config file
  }

  // Wait until wineserver terminates
  Helper::WaitUntilWineserverIsTerminated(wine_prefix);

  // Trigger finish signal!
  caller->SignalBottleCreated();
}

/**
 * \brief Remove the current active Wine bottle
 */
void BottleManager::DeleteBottle()
{
  if (active_bottle != nullptr)
  {
    try
    {
      Glib::ustring prefix_path = active_bottle->wine_location();
      string windows = BottleTypes::toString(active_bottle->windows());
      // Are you sure?'
      Glib::ustring confirmMessage = "Are you sure you want to <b>PERMANENTLY</b> remove machine named '" +
                                     Glib::Markup::escape_text(Helper::GetName(prefix_path)) + "' running " + windows +
                                     "?\n\n<i>Note:</i> This action cannot be undone!";
      if (mainWindow.ShowConfirmDialog(confirmMessage, true))
      {
        // Signal that bottle is removed
        bottleRemoved.emit();
        Helper::RemoveWineBottle(prefix_path);
        this->UpdateBottles();
      }
      else
      {
        // Nothing, canceled
      }
    }
    catch (const std::runtime_error& error)
    {
      mainWindow.ShowErrorMessage(error.what());
    }
  }
  else
  {
    mainWindow.ShowErrorMessage("No Windows Machine to remove, empty/no selection.");
  }
}

/**
 * \brief Signal handler when the active bottle changes, update active bottle
 * \param[in] bottle - New bottle
 */
void BottleManager::SetActiveBottle(BottleItem* bottle)
{
  if (bottle != nullptr)
  {
    this->active_bottle = bottle;
  }
}

/**
 * \brief Get error message (stored from manager thread)
 * \return Return the error message
 */
const Glib::ustring& BottleManager::GetErrorMessage()
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_error_message;
}

/**
 * \brief Run a program in Wine (using the active selected bottle)
 * \param[in] filename - Filename location of the program, selected by user
 * \param[in] is_msi_file - Is the program you try to run a Windows Installer (MSI)? False is EXE.
 */
void BottleManager::RunProgram(string filename, bool is_msi_file = false)
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program_prefix = is_msi_file ? "msiexec /i" : "start /unix";
    // Be-sure to execute the filename also between brackets (in case of spaces)
    Glib::ustring program = program_prefix + " \"" + filename + "\"";
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, program, true, false);
    t.detach();
  }
}

/**
 * \brief Open the Wine C: drive on the current active bottle
 */
void BottleManager::OpenDriveC()
{
  if (isBottleNotNull())
  {
    GError* error = NULL;
    if (!g_app_info_launch_default_for_uri(("file://" + active_bottle->wine_c_drive()).c_str(), NULL, &error))
    {
      g_warning("Failed to open uri: %s", error->message);
      mainWindow.ShowErrorMessage("Could not open the C:/ drive.");
    }
  }
}

/**
 * \brief Reboot bottle
 */
void BottleManager::Reboot()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "wineboot -r", false, false);
    t.detach();
  }
}

/**
 * \brief Update bottle
 */
void BottleManager::Update()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "wineboot -u", false, false);
    t.detach();
  }
}

/**
 * \brief Kill running processes in bottle
 */
void BottleManager::KillProcesses()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "wineboot -k", false, false);
    t.detach();
  }
}

/**
 * \brief Open Explorer browser
 */
void BottleManager::OpenExplorer()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "explorer", false, false);
    t.detach();
  }
}

/**
 * \brief Open (Wine) Windows Console
 */
void BottleManager::OpenConsole()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "wineconsole", false, false);
    t.detach();
  }
}

/**
 * \brief Open Winecfg tool
 */
void BottleManager::OpenWinecfg()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "winecfg", false, false);
    t.detach();
  }
}

/**
 * \brief Open Winetricks tool in GUI mode
 */
void BottleManager::OpenWinetricks()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program = Helper::GetWinetricksLocation() + " --gui";
    std::thread t(&Helper::RunProgram, wine_prefix, program, true, false);
    t.detach();
  }
}

/**
 * \brief Open Uninstaller
 */
void BottleManager::OpenUninstaller()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "uninstaller", false, false);
    t.detach();
  }
}

/**
 * \brief Open Task manager
 */
void BottleManager::OpenTaskManager()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "taskmgr", false, false);
    t.detach();
  }
}

/**
 * \brief Open Registery Editor
 */
void BottleManager::OpenRegistertyEditor()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "regedit", false, false);
    t.detach();
  }
}

/**
 * \brief Open Notepad
 */
void BottleManager::OpenNotepad()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "notepad", false, false);
    t.detach();
  }
}

/**
 * \brief Open Wordpad
 */
void BottleManager::OpenWordpad()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "wordpad", false, false);
    t.detach();
  }
}

/**
 * \brief Open (Wine) Internet Explorer
 */
void BottleManager::OpenIexplore()
{
  if (isBottleNotNull())
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    std::thread t(&Helper::RunProgramUnderWine, is_wine64_bit, wine_prefix, "iexplore", false, false);
    t.detach();
  }
}

/**
 * \brief Install D3DX9 (DirectX 9 OpenGL)
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of additional DirectX 9 DLLs, eg. 26 (for default use: "")
 */
void BottleManager::InstallD3DX9(Gtk::Window& parent, const Glib::ustring& version)
{
  if (isBottleNotNull())
  {
    // Before we execute the install, show busy dialog
    mainWindow.ShowBusyDialog(parent, "Installing D3DX9 (OpenGL implementation of DirectX 9).");

    Glib::ustring package = "d3dx9";
    if (version != "")
    {
      package += "_" + version;
    }
    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program = Helper::GetWinetricksLocation() + " -q " + package;
    // finishedPackageInstall signal is needed in order to close the busy dialog again
    std::thread t(&Helper::RunProgramWithFinishCallback, wine_prefix, program, &finishedPackageInstall, true, false);
    t.detach();
  }
}

/**
 * \brief Install DXVK (Vulkan based DirectX 9/10/11)
 * Note: initially only Direct3D 10 & 11 was supported by DXVK. But now also Direct3D 9.
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of DXVK, eg. 151 (for default use: "latest")
 */
void BottleManager::InstallDXVK(Gtk::Window& parent, const Glib::ustring& version)
{
  if (isBottleNotNull())
  {
    // Before we execute the install, show busy dialog
    mainWindow.ShowBusyDialog(parent, "Installing DXVK (Vulkan-based implementation of DirectX 9, 10 and 11).\n");

    Glib::ustring package = "dxvk";
    if (version != "latest")
    {
      package += version;
    }
    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program = Helper::GetWinetricksLocation() + " -q " + package;
    // finishedPackageInstall signal is needed in order to close the busy dialog again
    std::thread t(&Helper::RunProgramWithFinishCallback, wine_prefix, program, &finishedPackageInstall, true, false);
    t.detach();
  }
}

/**
 * \brief Install MS Visual C++ Redistributable Package
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of Visual C++, eg. 2010, 2013, 2015 (no default)
 */
void BottleManager::InstallVisualCppPackage(Gtk::Window& parent, const Glib::ustring& version)
{
  if (isBottleNotNull())
  {
    // Before we execute the install, show busy dialog
    mainWindow.ShowBusyDialog(parent, "Installing Visual C++ package.");

    Glib::ustring package = "vcrun" + version;
    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program = Helper::GetWinetricksLocation() + " -q " + package;
    // finishedPackageInstall signal is needed in order to close the busy dialog again
    std::thread t(&Helper::RunProgramWithFinishCallback, wine_prefix, program, &finishedPackageInstall, true, false);
    t.detach();
  }
}

/**
 * \brief Install MS .NET (deinstall Mono first if needed)
 * Idea: Install dotnet_verifier by default with it?
 * \param[in] parent Parent GTK window were the request is coming from
 * \param[in] version Version of .NET, eg. '35' for 3.5, '471' for 4.7.1 or '35sp1' for 3.5 SP1 (no default)
 */
void BottleManager::InstallDotNet(Gtk::Window& parent, const Glib::ustring& version)
{
  if (isBottleNotNull())
  {
    if (mainWindow.ShowConfirmDialog(
            "<i>Important note:</i> Wine Mono &amp; Gecko support is often sufficient enough.\n\nWine Mono will be "
            "<b>uninstalled</b> before native .NET will be installed.\n\nAre you sure you want to continue?",
            true))
    {
      // Before we execute the install, show busy dialog
      mainWindow.ShowBusyDialog(parent, "Installing Native .NET redistributable packages (v" + version +
                                            ").\nThis may take quite some time...\n");

      Glib::ustring deinstallCommand = this->GetDeinstallMonoCommand();

      Glib::ustring package = "dotnet" + version;
      Glib::ustring wine_prefix = active_bottle->wine_location();
      // I can't use -q with .NET installs
      Glib::ustring installCommand = Helper::GetWinetricksLocation() + " " + package;

      Glib::ustring program = "";
      if (!deinstallCommand.empty())
      {
        // First deinstall Mono then install native .NET
        program = deinstallCommand + "; " + installCommand;
      }
      else
      {
        program = installCommand;
      }
      // finishedPackageInstall signal is needed in order to close the busy dialog again
      std::thread t(&Helper::RunProgramWithFinishCallback, wine_prefix, program, &finishedPackageInstall, true, false);
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
void BottleManager::InstallCoreFonts(Gtk::Window& parent)
{
  if (isBottleNotNull())
  {
    // Before we execute the install, show busy dialog
    mainWindow.ShowBusyDialog(parent, "Installing MS Core fonts.");

    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program = Helper::GetWinetricksLocation() + " -q corefonts";
    // finishedPackageInstall signal is needed in order to close the busy dialog again
    std::thread t(&Helper::RunProgramWithFinishCallback, wine_prefix, program, &finishedPackageInstall, true, false);
    t.detach();
  }
}

/**
 * \brief Install liberation fonts, open-source (which is often enough)
 * \param[in] parent Parent GTK window were the request is coming from
 */
void BottleManager::InstallLiberation(Gtk::Window& parent)
{
  if (isBottleNotNull())
  {
    // Before we execute the install, show busy dialog
    mainWindow.ShowBusyDialog(parent, "Installing Liberation open-source fonts.");

    Glib::ustring wine_prefix = active_bottle->wine_location();
    Glib::ustring program = Helper::GetWinetricksLocation() + " -q liberation";
    // finishedPackageInstall signal is needed in order to close the busy dialog again
    std::thread t(&Helper::RunProgramWithFinishCallback, wine_prefix, program, &finishedPackageInstall, true, false);
    t.detach();
  }
}

/*************************************************************
 * Private member functions                                  *
 *************************************************************/

bool BottleManager::isBottleNotNull()
{
  bool isNull = (active_bottle == nullptr);
  if (isNull)
  {
    mainWindow.ShowErrorMessage("No Windows Machine selected/empty. First create a new machine!\n\nAborted.");
  }
  return !isNull;
}

/**
 * \brief Wine Mono deinstall command, run before installing native .NET
 * \return uninstall Mono command
 * Note: When nothing todo, the command will be an empty string.
 */
Glib::ustring BottleManager::GetDeinstallMonoCommand()
{
  string command = "";
  if (active_bottle != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle->wine_location();
    string GUID = Helper::GetWineGUID(is_wine64_bit, wine_prefix, "Wine Mono Runtime");

    if (!GUID.empty())
    {
      Glib::ustring uninstaller = "";
      switch (active_bottle->bit())
      {
      case BottleTypes::Bit::win32:
        uninstaller = "wine uninstaller --remove";
        break;
      case BottleTypes::Bit::win64:
        uninstaller = "wine64 uninstaller --remove";
        break;
      }
      command = uninstaller + " '{" + GUID + "}'";
    }
  }
  return command;
}

/**
 * \brief Get Wine version
 * \return Wine version
 */
string BottleManager::GetWineVersion()
{
  // Read wine version (is always the same for all bottles atm)
  string wineVersion = "";
  try
  {
    wineVersion = Helper::GetWineVersion(is_wine64_bit);
  }
  catch (const std::runtime_error& error)
  {
    mainWindow.ShowErrorMessage(error.what());
  }
  return wineVersion;
}

/**
 * \brief Get Bottle Paths
 * \return Return a map of bottle paths (string) and modification time (in ms)
 */
std::map<string, unsigned long> BottleManager::GetBottlePaths()
{
  if (!Helper::DirExists(bottle_location))
  {
    // Create directory if not exist yet
    if (!Helper::CreateDir(bottle_location))
    {
      throw std::runtime_error("Failed to create the Wine bottles directory: " + bottle_location);
    }
  }
  if (Helper::DirExists(bottle_location))
  {
    // Continue
    return Helper::GetBottlesPaths(bottle_location);
  }
  else
  {
    throw std::runtime_error("Configuration directory still not found (probably no permissions):\n" + bottle_location);
  }
  // Otherwise empty
  return std::map<string, unsigned long>();
}

/**
 * \brief Create wine bottle classes and add them to the private bottles variable
 * \param[in] wineVersion The current wine version used (currently the same version for all bottles)
 * \param[in] bottleDirs  The list of bottle directories
 */
std::list<BottleItem> BottleManager::CreateWineBottles(string wineVersion, std::map<string, unsigned long> bottleDirs)
{
  std::list<BottleItem> bottles;

  // Retrieve detailed info for each wine bottle prefix
  for (const auto& [prefix, _] : bottleDirs)
  {
    std::ignore = _;
    // Reset variables
    string name = "";
    string virtualDesktop = "";
    bool status = false;
    BottleTypes::Windows windows = BottleTypes::Windows::WindowsXP;
    BottleTypes::Bit bit = BottleTypes::Bit::win32;
    string cDriveLocation = "- Unknown -";
    string lastTimeWineUpdated = "- Unknown -";
    BottleTypes::AudioDriver audioDriver = BottleTypes::AudioDriver::pulseaudio;

    try
    {
      name = Helper::GetName(prefix);
      virtualDesktop = Helper::GetVirtualDesktop(prefix);
      status = Helper::GetBottleStatus(prefix);
      windows = Helper::GetWindowsOSVersion(prefix);
      bit = Helper::GetSystemBit(prefix);
      cDriveLocation = Helper::GetCLetterDrive(prefix);
      lastTimeWineUpdated = Helper::GetLastWineUpdated(prefix);
      audioDriver = Helper::GetAudioDriver(prefix);
    }
    catch (const std::runtime_error& error)
    {
      mainWindow.ShowErrorMessage(error.what());
    }

    BottleItem* bottle = new BottleItem(name, status, windows, bit, wineVersion, prefix, cDriveLocation,
                                        lastTimeWineUpdated, audioDriver, virtualDesktop);
    bottles.push_back(*bottle);
  }
  return bottles;
}
