/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    gui.h
 * \brief   GTK+ User Interface Class
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
#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

/**
 * \class GUI
 * \brief GTK+ User Interface Class
 */
class GUI {
public:
  GUI();

  int start(int argc, char **argv);

private:
  static GtkWidget* CreateImageMenuItem(const gchar* label_text, const gchar* icon_name);
  static void cbShowAbout(GtkButton *btn, gpointer parent_window);
  static GtkWidget* SetupMenu(GtkWidget *window);
  static GtkWidget* CreateFoundation(GtkWidget *window);
  static void CreateLeftPanel(GtkWidget *paned);
  static void CreateRightPanel(GtkWidget *paned);
  static void activate(GtkApplication *app, gpointer user_data);

  static void cc_list_box_update_header_func(GtkListBoxRow *row,
                                GtkListBoxRow *before,
                                gpointer user_data);
  static void print_hello(GtkWidget *widget, gpointer data);
  static void add_css();
};

#endif