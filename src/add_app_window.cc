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
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
AddAppWindow::AddAppWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
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
  header_add_app_label.set_attributes(attr_list_header_label);
  header_add_app_label.set_margin_top(5);
  header_add_app_label.set_margin_bottom(5);

  name_label.set_halign(Gtk::Align::ALIGN_END);
  description_label.set_halign(Gtk::Align::ALIGN_END);
  command_label.set_halign(Gtk::Align::ALIGN_END);
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

  hbox_buttons.pack_end(save_button, false, false, 4);
  hbox_buttons.pack_end(cancel_button, false, false, 4);

  vbox.pack_start(header_add_app_label, false, false, 4);
  vbox.pack_start(add_app_grid, true, true, 4);
  vbox.pack_start(hbox_buttons, false, false, 4);
  add(vbox);

  // Signals
  select_executable_button.signal_clicked().connect(sigc::mem_fun(*this, &AddAppWindow::on_select_file));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &AddAppWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &AddAppWindow::on_save_button_clicked));

  show_all_children();
}

/**
 * \brief Destructor
 */
AddAppWindow::~AddAppWindow()
{
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
  auto filter_win = Gtk::FileFilter::create();
  filter_win->set_name("Windows Executable/MSI Installer");
  filter_win->add_mime_type("application/x-ms-dos-executable");
  filter_win->add_mime_type("application/x-msi");
  auto filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any file");
  filter_any->add_pattern("*");

  auto* file_chooser =
      new Gtk::FileChooserDialog(*this, "Choose a folder", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN, Gtk::DialogFlags::DIALOG_MODAL);
  file_chooser->set_modal(true);
  file_chooser->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &AddAppWindow::on_select_dialog_response), file_chooser));
  file_chooser->add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
  file_chooser->add_button("_Select file", Gtk::ResponseType::RESPONSE_OK);
  if (active_bottle_ != nullptr)
  {
    file_chooser->set_current_folder(active_bottle_->wine_c_drive());
  }
  file_chooser->add_filter(filter_win);
  file_chooser->add_filter(filter_any);
  file_chooser->show();
}

/**
 * \brief when file is selected
 */
void AddAppWindow::on_select_dialog_response(int response_id, Gtk::FileChooserDialog* dialog)
{
  switch (response_id)
  {
  case Gtk::ResponseType::RESPONSE_OK:
  {
    // Update the command entry
    auto filename = dialog->get_filename();
    command_entry.set_text(filename);
    break;
  }
  case Gtk::ResponseType::RESPONSE_CANCEL:
  {
    break; // ignore
  }
  default:
  {
    std::cout << "Error: Unexpected button clicked." << std::endl;
    break;
  }
  }
  delete dialog;
}

/**
 * \brief Triggered when cancel button is clicked
 */
void AddAppWindow::on_cancel_button_clicked()
{
  hide();
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
      Gtk::MessageDialog dialog(*this, "You forgot to fill-in the name and command (only the description is optional).", false, Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK);
      dialog.set_title("Error during new application saving");
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
      new_app.name = name_entry.get_text();
      new_app.description = description_entry.get_text();
      new_app.command = command_entry.get_text();
      app_list.insert(std::pair<int, ApplicationData>(new_index, new_app));

      // Save application to bottle config
      if (!BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list))
      {
        Gtk::MessageDialog dialog(*this, "Error occurred during saving bottle config file.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        dialog.set_title("An error has occurred!");
        dialog.set_modal(true);
        dialog.run();
      }
      else
      {
        // Hide new application window
        hide();
        // Reset entry fields
        set_default_values();
        // Trigger manager update & UI update
        config_saved.emit();
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
