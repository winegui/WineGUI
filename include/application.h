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

#include <gtkmm.h>

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
  void create_window();

  void on_action_preferences();
  void on_menu_file_quit();
  void on_menu_help_about();
  void on_action_quit();

  Glib::RefPtr<Gtk::Builder> m_refBuilder;
};