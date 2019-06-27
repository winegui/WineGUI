/**
 * Copyright (c) 2019 WineGUI
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
#include "main_window.h"
#include "helper.h"

#include <stdexcept>

/**
 * \brief Constructor
 * \param mainWindow Address to the main Window
 */
BottleManager::BottleManager(MainWindow& mainWindow): mainWindow(mainWindow)
{
  // TODO: Make it configurable via settings
  std::vector<std::string> dirs{Glib::get_home_dir(), ".winegui", "prefixes"};
  BOTTLE_LOCATION = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
}

/**
 * \brief Destructor
 */
BottleManager::~BottleManager() {}

/**
 * \brief Prepare method, called during initial start-up of the app
 */
void BottleManager::Prepare()
{
  // Install winetricks if not yet present,
  // Winetricks script is used by WineGUI.
  if(!Helper::FileExists("winetricks"))
  {
    try {
      Helper::InstallOrUpdateWinetricks();
    }
    catch(const std::runtime_error& error)
    {
      mainWindow.ShowErrorMessage(error.what());
    }
  }
  else
  {
    // Update existing script
    try {
      Helper::SelfUpdateWinetricks();
    }
    catch(const std::runtime_error& error)
    {
      string ver = Helper::GetWinetricksVersion();
      std::cout << "WARN: Could not update winetricks, however winetrick version " << ver << " is already present." << std::endl;
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
  if(bottles.size() > 0) {
    // Clean-up
    bottles.clear();
  }

  // Read bottles from disk and create classes from it
  std::map<string, unsigned long> bottleDirs = ReadBottles();
  if(bottleDirs.size() > 0) {
    // Create wine bottles from bottle directories and wine version
    CreateWineBottles(GetWineVersion(), bottleDirs);
  
    if(bottles.size() > 0)
    {
      // Update main Window
      mainWindow.SetWineBottles(bottles);

      // Set first element as active (details)  
      // TODO : Improve this
      auto front = bottles.begin();      
      mainWindow.SetDetailedInfo(*front);
    }
  }
}

/**
 * \brief Create a new Wine Bottle
 * \param[in] name                        - Bottle Name
 * \param[in] virtual_desktop_resolution  - Virtual desktop resolution (empty if disabled)
 * \param[in] windows_version             - Windows OS version
 * \param[in] bit                         - Windows Bit (32/64-bit)
 * \param[in] audio                       - Audio Driver type
 */
void BottleManager::NewBottle(
    Glib::ustring& name,
    Glib::ustring& virtual_desktop_resolution,
    BottleTypes::Windows windows_version,
    BottleTypes::Bit bit,
    BottleTypes::AudioDriver audio)
{
  // Calculate prefix
  std::vector<std::string> dirs{BOTTLE_LOCATION, name};
  auto wine_prefix = Glib::build_path(G_DIR_SEPARATOR_S, dirs);
  try {
    // First create a new Wine Bottle
    Helper::CreateWineBottle(wine_prefix, bit);

    std::cout << "\nBottle created!\n" 
    << "Name: " << name
    << "\nRes: "  << virtual_desktop_resolution 
    << "\nWindows: "  << BottleTypes::toString(windows_version) 
    << "\nBit: " << BottleTypes::toString(bit)
    << "\nAudio: " << BottleTypes::toString(audio)
    << std::endl;

    // TODO: Next update Windows version, audio and virtual desktop...
  }
  catch (const std::runtime_error& error)
  {
    mainWindow.ShowErrorMessage(error.what());
  }
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
    wineVersion = Helper::GetWineVersion();
  }
  catch (const std::runtime_error& error)
  {
    mainWindow.ShowErrorMessage(error.what());
  }
  return wineVersion;
}

/**
 * \brief Read Wine bottles from disk at start-up
 * \return Return a map of bottle paths (string) and modification time (in ms)
 */
std::map<string, unsigned long> BottleManager::ReadBottles()
{
  if(!Helper::DirExists(BOTTLE_LOCATION)) {
    // Create directory if not exist yet
    if(g_mkdir_with_parents(BOTTLE_LOCATION.c_str(), 0775) < 0 && errno != EEXIST) {
      printf("Failed to create WineGUI directory \"%s\": %s\n", BOTTLE_LOCATION.c_str(), g_strerror(errno));
    }
  }
  if(Helper::DirExists(BOTTLE_LOCATION)) {
    // Continue
    return Helper::GetBottlesPaths(BOTTLE_LOCATION);
  }
  else {
    mainWindow.ShowErrorMessage("Configuration directory not found (could not create):\n" + BOTTLE_LOCATION);
  }
  // Otherwise empty
  return std::map<string, unsigned long>();
}

/**
 * \brief Create wine bottle classes and add them to the private bottles variable
 * \param[in] wineVersion The current wine version used
 * \param[in] bottleDirs  The list of bottle directories
 */
void BottleManager::CreateWineBottles(string wineVersion, std::map<string, unsigned long> bottleDirs)
{
  string name = "";
  string virtualDesktop = BottleTypes::VIRTUAL_DESKTOP_DISABLED;
  bool status = false;
  BottleTypes::Windows windows = BottleTypes::Windows::WindowsXP;
  BottleTypes::Bit bit = BottleTypes::Bit::win32;
  string cDriveLocation = "";
  string lastTimeWineUpdated = "";
  BottleTypes::AudioDriver audioDriver = BottleTypes::AudioDriver::pulseaudio;
  
  // Retrieve detailed info for each wine bottle prefix
  for (const auto &[prefix, _]: bottleDirs ) {
    std::ignore = _;
    // Reset variables
    name = "";
    virtualDesktop = BottleTypes::VIRTUAL_DESKTOP_DISABLED;
    status = false;
    windows = BottleTypes::Windows::WindowsXP;
    bit = BottleTypes::Bit::win32;
    cDriveLocation = "- Unknown -";
    lastTimeWineUpdated = "- Unknown -";
    audioDriver = BottleTypes::AudioDriver::pulseaudio;

    try {
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

    BottleItem* bottle = new BottleItem(
      name,
      status,
      windows,
      bit,
      wineVersion,
      prefix,
      cDriveLocation,
      lastTimeWineUpdated,
      audioDriver,
      virtualDesktop);
    bottles.push_back(*bottle);
  }
}
