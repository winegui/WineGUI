/**
 * Copyright (c) 2019-2024 WineGUI
 *
 * \file    bottle_clone_window.h
 * \brief   Wine bottle clone window
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

#include "bottle_types.h"
#include "busy_dialog.h"
#include <gtkmm.h>

using std::string;

// Forward declaration
class BottleItem;

struct CloneBottleStruct
{
  Glib::ustring orginal_folder_name; // Source bottle folder name
  Glib::ustring name;
  Glib::ustring folder_name;
  Glib::ustring description;
};

/**
 * \class BottleCloneWindow
 * \brief Clone Wine bottle GTK Window class
 */
class BottleCloneWindow : public Gtk::Window
{
public:
  // Signals
  sigc::signal<void, CloneBottleStruct&> clone_bottle; /*!< save button clicked signal */

  explicit BottleCloneWindow(Gtk::Window& parent);
  virtual ~BottleCloneWindow();

  void show();
  void set_active_bottle(BottleItem* bottle);
  void reset_active_bottle();

  // Signal handlers
  virtual void on_bottle_cloned();

protected:
  // Child widgets
  Gtk::Box vbox;         /*!< main vertical box */
  Gtk::Box hbox_buttons; /*!< box for buttons */
  Gtk::Grid clone_grid;  /*!< grid layout for form */

  Gtk::Label header_clone_label;                   /*!< header clone label */
  Gtk::Label name_label;                           /*!< name label */
  Gtk::Label folder_name_label;                    /*!< folder name label */
  Gtk::Label description_label;                    /*!< description label */
  Gtk::Entry name_entry;                           /*!< name input field */
  Gtk::Entry folder_name_entry;                    /*!< folder name input field */
  Gtk::ScrolledWindow description_scrolled_window; /*!< description scrolled window */
  Gtk::TextView description_text_view;             /*!< description text view */
  Gtk::Button clone_button;                        /*!< clone button */
  Gtk::Button cancel_button;                       /*!< cancel button */
  // Busy dialog
  BusyDialog busy_dialog; /*!< Busy dialog, when the user should wait until install is finished */
private:
  // Signal handlers
  void on_cancel_button_clicked();
  void on_clone_button_clicked();

  // Member functions

  BottleItem* active_bottle_; /*!< Current active bottle */
};
