/**
 * Copyright (c) 2024 WineGUI
 *
 * \file    bottle_configure_env_var_window.h
 * \brief   Configure bottle environment variables
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

// Forward declaration
class BottleItem;

// Tree model columns
class ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
  ModelColumns()
  {
    add(m_col_name);
    add(m_col_value);
  }

  Gtk::TreeModelColumn<Glib::ustring> m_col_name;
  Gtk::TreeModelColumn<Glib::ustring> m_col_value;
};

/**
 * \class BottleConfigureEnvVarWindow
 * \brief Add new application GTK Window class for the bottle environment variable
 */
class BottleConfigureEnvVarWindow : public Gtk::Window
{
public:
  // Signals
  // sigc::signal<void> config_saved; /*!< bottle config is saved signal */

  explicit BottleConfigureEnvVarWindow(Gtk::Window& parent);
  virtual ~BottleConfigureEnvVarWindow();

  void set_active_bottle(BottleItem* bottle);
  void reset_active_bottle();

protected:
  // Child widgets
  Gtk::Box vbox;           /*!< main vertical box */
  Gtk::Box hbox_buttons;   /*!< box for add/remove buttons */
  Gtk::Box hbox_2_buttons; /*!< box save button and cancel button */

  Gtk::Label header_configure_env_var_label; /*!< header config env var label */
  Gtk::Label environment_variables_label;    /*!< Environment variables label */
  Gtk::Button add_button;                    /*!< Add button */
  Gtk::Button remove_button;                 /*!< Remove button */
  Gtk::Button save_button;                   /*!< Save button */
  Gtk::Button cancel_button;                 /*!< Save button */

  ModelColumns m_Columns;
  Gtk::ScrolledWindow m_ScrolledWindow;
  Gtk::TreeView m_TreeView;
  Glib::RefPtr<Gtk::ListStore> m_refTreeModel;

private:
  BottleItem* active_bottle_; /*!< Current active bottle */

  // Signal handlers
  void on_add_button_clicked();
  void on_remove_button_clicked();
  void on_save_button_clicked();
  void on_cancel_button_clicked();
};
