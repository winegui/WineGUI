/**
 * Copyright (c) 2024 WineGUI
 *
 * \file    bottle_configure_env_var_window.cc
 * \brief   Configure bottle environment variables GTK3 Window class
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
#include "bottle_configure_env_var_window.h"
#include "bottle_config_file.h"
#include "bottle_item.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleConfigureEnvVarWindow::BottleConfigureEnvVarWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_configure_env_var_label("Configure Environment Variables"),
      key_label("Environment variable name: "),
      value_label("Environment variable value: "),
      environment_variables_label("Current environment variables set:"),
      add_button("Add"),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_title("Configure Environment Variables");
  set_default_size(500, 500);
  set_modal(true);

  set_default_values();

  add_app_grid.set_margin_top(5);
  add_app_grid.set_margin_end(5);
  add_app_grid.set_margin_bottom(6);
  add_app_grid.set_margin_start(6);
  add_app_grid.set_column_spacing(6);
  add_app_grid.set_row_spacing(8);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_configure_env_var_label.set_attributes(attr_list_header_label);
  header_configure_env_var_label.set_margin_top(5);
  header_configure_env_var_label.set_margin_bottom(5);

  key_label.set_halign(Gtk::Align::ALIGN_END);
  value_label.set_halign(Gtk::Align::ALIGN_END);
  environment_variables_label.set_halign(Gtk::Align::ALIGN_START);
  environment_variables_label.set_margin_start(6);
  key_entry.set_hexpand(true);
  value_entry.set_hexpand(true);
  environment_variables_listbox.set_margin_start(6);
  environment_variables_listbox.set_margin_end(6);
  environment_variables_listbox.set_margin_bottom(5);
  key_entry.set_tooltip_text("Set the name of the environment variable");
  value_entry.set_tooltip_text("Set the value of the environment variable");

  add_app_grid.attach(key_label, 0, 0);
  add_app_grid.attach(key_entry, 1, 0, 2);
  add_app_grid.attach(value_label, 0, 1);
  add_app_grid.attach(value_entry, 1, 1, 2);
  hbox_buttons.pack_end(add_button, false, false, 4);

  vbox.pack_start(header_configure_env_var_label, false, false, 4);
  vbox.pack_start(add_app_grid, false, true, 4);
  vbox.pack_start(hbox_buttons, false, false, 4);
  vbox.pack_start(environment_variables_label, false, false, 4);
  vbox.pack_start(environment_variables_listbox, true, true, 4);

  add(vbox);

  // Signals
  add_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_add_button_clicked));

  show_all_children();
}

/**
 * \brief Destructor
 */
BottleConfigureEnvVarWindow::~BottleConfigureEnvVarWindow()
{
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle Current active bottle
 */
void BottleConfigureEnvVarWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleConfigureEnvVarWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

void BottleConfigureEnvVarWindow::set_default_values()
{
  key_entry.set_text("");
  value_entry.set_text("");
}

/**
 * \brief Triggered when add button is clicked
 */
void BottleConfigureEnvVarWindow::on_add_button_clicked()
{
  if (active_bottle_ != nullptr)
  {
    // Check if all fields are filled-in
    if (key_entry.get_text().empty() || value_entry.get_text().empty())
    {
      Gtk::MessageDialog dialog(*this, "Please fill-in the key and value of the environment variable you wish to add.", false, Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK);
      dialog.set_title("Error during adding a new environment variable");
      dialog.set_modal(true);
      dialog.run();
    }
    else
    {
      std::string prefix_path = active_bottle_->wine_location();
      // Read existing config data
      BottleConfigData bottle_config;
      std::map<int, ApplicationData> app_list;
      std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);

      int new_index = (!app_list.empty()) ? std::prev(app_list.end())->first + 1 : 0;
      // Append new app
      ApplicationData new_app;
      new_app.name = key_entry.get_text();
      new_app.command = value_entry.get_text();
      app_list.insert(std::pair<int, ApplicationData>(new_index, new_app));

      // Save application to bottle config
      if (!BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list))
      {
        Gtk::MessageDialog dialog(*this, "Error occurred during saving generic config file.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        dialog.set_title("An error has occurred!");
        dialog.set_modal(true);
        dialog.run();
      }
      else
      {
        // Reset entry fields
        set_default_values();
        // Trigger manager update & UI update
        // config_saved.emit();
      }
    }
  }
  else
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving, because there is no active Windows machine set.", false, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
    dialog.set_title("Error during new application saving");
    dialog.set_modal(true);
    dialog.run();
    std::cout << "Error: No current Windows machine is set. Change won't be saved." << std::endl;
  }
}
