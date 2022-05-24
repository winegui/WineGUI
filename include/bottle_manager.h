/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    bottle_manager.h
 * \brief   Bottle manager controller
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
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "bottle_types.h"

using std::string;

// Forward declaration
class MainWindow;
class SignalDispatcher;
class BottleItem;

/**
 * \class BottleManager
 * \brief Controller that controls it all
 */
class BottleManager
{
public:
  // Signals
  sigc::signal<void> resetActiveBottle;    /*!< Send signal: Clear the current active bottle */
  sigc::signal<void> bottleRemoved;        /*!< Send signal: When the bottle is confirmed to be removed */
  Glib::Dispatcher finishedPackageInstall; /*!< Signal that Wine package install is completed */

  explicit BottleManager(MainWindow& mainWindow);
  virtual ~BottleManager();

  void Prepare();
  void UpdateBottles();
  void NewBottle(SignalDispatcher* caller,
                 Glib::ustring name,
                 BottleTypes::Windows windows_version,
                 BottleTypes::Bit bit,
                 Glib::ustring virtual_desktop_resolution,
                 bool disable_gecko_mono,
                 BottleTypes::AudioDriver audio);
  void UpdateBottle(SignalDispatcher* caller,
                    Glib::ustring name,
                    BottleTypes::Windows windows_version,
                    BottleTypes::Bit bit,
                    Glib::ustring virtual_desktop_resolution,
                    BottleTypes::AudioDriver audio);
  void DeleteBottle();
  void SetActiveBottle(BottleItem* bottle);
  const Glib::ustring& GetErrorMessage();

  // Signal handlers
  void RunProgram(string filename, bool is_msi_file);
  void OpenDriveC();
  void Reboot();
  void Update();
  void KillProcesses();
  void OpenExplorer();
  void OpenConsole();
  void OpenWinecfg();
  void OpenWinetricks();
  void OpenUninstaller();
  void OpenTaskManager();
  void OpenRegistertyEditor();
  void OpenNotepad();
  void OpenWordpad();
  void OpenIexplore();
  void InstallD3DX9(Gtk::Window& parent, const Glib::ustring& version);
  void InstallDXVK(Gtk::Window& parent, const Glib::ustring& version);
  void InstallVisualCppPackage(Gtk::Window& parent, const Glib::ustring& version);
  void InstallDotNet(Gtk::Window& parent, const Glib::ustring& version);
  void InstallCoreFonts(Gtk::Window& parent);
  void InstallLiberation(Gtk::Window& parent);

private:
  // Synchronizes access to data members
  mutable std::mutex m_Mutex;

  MainWindow& mainWindow;
  string bottle_location;
  std::list<BottleItem> bottles;
  BottleItem* active_bottle;
  bool is_wine64_bit;

  //// error_message is used by both the GUI thread and NewBottle thread (used a 'temp' location)
  Glib::ustring m_error_message;

  bool isBottleNotNull();
  Glib::ustring GetDeinstallMonoCommand();
  string GetWineVersion();
  std::map<string, unsigned long> GetBottlePaths();
  std::list<BottleItem> CreateWineBottles(string wineVersion, std::map<string, unsigned long> bottleDirs);
};
