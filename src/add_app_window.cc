/**
 * Copyright (c) 2023-2025 WineGUI
 *
 * \file    add_app_window.cc
 * \brief   Add new application shortcut for app list GTK3 window class
 * \author  Melroy van den Berg <melroy@melroy.org>
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
#include "add_app_window.h"
#include "bottle_config_file.h"
#include "bottle_item.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
AddAppWindow::AddAppWindow(Gtk::Window& parent)
    : vbox(Gtk::Orientation::VERTICAL, 4),
      hbox_buttons(Gtk::Orientation::HORIZONTAL, 4),
      header_add_app_label("Add Application shortcut"),
      name_label("Application name: "),
      description_label("Description: "),
      command_label("Command: "),
      select_executable_button("Select executable..."),
      save_button("Save"),
      cancel_button("Cancel"),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_title("Add new Application shortcut");
  set_default_size(500, 200);
  set_modal(true);

  set_default_values();

  create_layout();

  // Signals
  select_executable_button.signal_clicked().connect(sigc::mem_fun(*this, &AddAppWindow::on_select_file));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &AddAppWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &AddAppWindow::on_save_button_clicked));
  // Hide window instead of destroy
  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        set_default_values();
        return true; // stop default destroy
      },
      false);
}

/**
 * \brief Destructor
 */
AddAppWindow::~AddAppWindow()
{
}

void AddAppWindow::create_layout()
{
  add_app_grid.set_margin_top(5);
  add_app_grid.set_margin_end(5);
  add_app_grid.set_margin_bottom(6);
  add_app_grid.set_margin_start(6);
  add_app_grid.set_column_spacing(6);
  add_app_grid.set_row_spacing(8);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_add_app_label.set_attributes(attr_list_header_label);
  header_add_app_label.set_margin_top(5);
  header_add_app_label.set_margin_bottom(5);

  name_label.set_halign(Gtk::Align::END);
  description_label.set_halign(Gtk::Align::END);
  command_label.set_halign(Gtk::Align::END);
  name_entry.set_hexpand(true);
  description_entry.set_hexpand(true);
  command_entry.set_hexpand(true);

  add_app_grid.attach(name_label, 0, 0);
  add_app_grid.attach(name_entry, 1, 0, 2);
  add_app_grid.attach(description_label, 0, 1);
  add_app_grid.attach(description_entry, 1, 1, 2);
  add_app_grid.attach(command_label, 0, 2);
  add_app_grid.attach(command_entry, 1, 2);
  add_app_grid.attach(select_executable_button, 2, 2);
  add_app_grid.set_hexpand(true);
  add_app_grid.set_vexpand(true);
  add_app_grid.set_halign(Gtk::Align::FILL);

  hbox_buttons.set_halign(Gtk::Align::END);
  hbox_buttons.set_margin(6);
  hbox_buttons.append(save_button);
  hbox_buttons.append(cancel_button);

  vbox.append(header_add_app_label);
  vbox.append(add_app_grid);
  vbox.append(hbox_buttons);
  set_child(vbox);
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle Current active bottle
 */
void AddAppWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void AddAppWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

void AddAppWindow::set_default_values()
{
  name_entry.set_text("");
  description_entry.set_text("");
  command_entry.set_text("");
}

/**
 * \brief Triggered when select file button is clicked
 */
void AddAppWindow::on_select_file()
{
  auto dialog = Gtk::FileDialog::create();
  dialog->set_title("Please choose a file");
  dialog->set_modal(true);
  {
    if (active_bottle_ != nullptr)
    {
      auto folder = Gio::File::create_for_path(active_bottle_->wine_c_drive());
      if (!folder->get_path().empty())
      {
        dialog->set_initial_folder(folder);
      }
    }
  }

  // Filters
  const auto filters = Gio::ListStore<Gtk::FileFilter>::create();
  auto filter_win = Gtk::FileFilter::create();
  filter_win->set_name("Windows Executable/MSI Installer");
  filter_win->add_mime_type("application/x-ms-dos-executable");
  filter_win->add_mime_type("application/x-msi");
  filters->append(filter_win);
  auto filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any file");
  filter_any->add_pattern("*");
  filters->append(filter_any);
  // Set the filters
  dialog->set_filters(filters);

  dialog->open(*this,
               [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result)
               {
                 try
                 {
                   const auto file = dialog->open_finish(result);
                   command_entry.set_text(file->get_path());
                 }
                 catch (const Gtk::DialogError& err)
                 {
                   // Do nothing
                 }
                 catch (const Glib::Error& err)
                 {
                   // Do nothing
                 }
               });
}

/**
 * \brief Triggered when cancel button is clicked
 */
void AddAppWindow::on_cancel_button_clicked()
{
  set_visible(false);
}

/**
 * \brief Triggered when save button is clicked
 */
void AddAppWindow::on_save_button_clicked()
{
  if (active_bottle_ != nullptr)
  {
    // Check if all fields are filled-in
    if (name_entry.get_text().empty() || command_entry.get_text().empty())
    {
      Gtk::MessageDialog dialog(*this, "You forgot to fill-in the name and command (only the description is optional).", false,
                                Gtk::MessageType::ERROR, Gtk::ButtonsType::OK);
      dialog.set_title("Error during new application saving");
      dialog.set_modal(true);
      dialog.present();
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
      new_app.name = name_entry.get_text();
      new_app.description = description_entry.get_text();
      new_app.command = command_entry.get_text();
      app_list.insert(std::pair<int, ApplicationData>(new_index, new_app));

      // Save application to bottle config
      if (!BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list))
      {
        Gtk::MessageDialog dialog(*this, "Error occurred during saving bottle config file.", false, Gtk::MessageType::ERROR, Gtk::ButtonsType::OK);
        dialog.set_title("An error has occurred!");
        dialog.set_modal(true);
        dialog.present();
      }
      else
      {
        // Hide new application window
        set_visible(false);
        // Reset entry fields
        set_default_values();
        // Trigger manager update & UI update
        config_saved.emit();
      }
    }
  }
  else
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving, because there is no active Windows machine set.", false, Gtk::MessageType::ERROR,
                              Gtk::ButtonsType::OK);
    dialog.set_title("Error during new application saving");
    dialog.set_modal(true);
    dialog.present();
  }
}
