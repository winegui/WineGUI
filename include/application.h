/**
 * Copyright (c) 2025 WineGUI
 *
 * \file    application.h
 * \brief   Application class
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

#include "signal_controller.h"
#include <gtkmm.h>

class MainWindow;
class BottleManager;
class PreferencesWindow;
class AboutDialog;
class BottleEditWindow;
class BottleCloneWindow;
class BottleConfigureEnvVarWindow;
class BottleConfigureWindow;
class AddAppWindow;
class RemoveAppWindow;
class SignalController;

/**
 * \class Application
 * \brief The main application class starting point
 */
class Application : public Gtk::Application
{
protected:
  Application();

public:
  static Glib::RefPtr<Application> create();

protected:
  void on_startup() override;
  void on_activate() override;
  void on_shutdown() override;

private:
  MainWindow* main_window_;
  BottleManager* manager_;
  PreferencesWindow* preferences_window_;
  AboutDialog* about_dialog_;
  BottleEditWindow* edit_window_;
  BottleCloneWindow* clone_window_;
  BottleConfigureEnvVarWindow* configure_env_var_window_;
  BottleConfigureWindow* configure_window_;
  AddAppWindow* add_app_window_;
  RemoveAppWindow* remove_app_window_;
  SignalController* signal_controller_;

  void on_action_quit();
};