/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    preferences_window.cc
 * \brief   Application preferences GTK+ window class
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
#include "preferences_window.h"
#include "generic_config_file.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
PreferencesWindow::PreferencesWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_preferences_label("Preferences - Settings"),
      default_folder_label("Default Machine folder: "),
      prefer_wine64_label("Prefer Wine 64-bit:"),
      debug_logging_label("Enable logging:"),
      logging_stderr_label("Log standard error:"),
      prefer_wine64_check("Prefer Wine 64-bit executable (over 32-bit)"),
      enable_debug_logging_check("Enable debug logging"),
      enable_logging_stderr_check("Also log standard error to logging (if logging is enabled)"),
      save_button("Save"),
      cancel_button("Cancel")
{
  set_transient_for(parent);
  set_title("WineGUI Preferences");
  set_default_size(550, 300);
  set_modal(true);

  settings_grid.set_margin_top(5);
  settings_grid.set_margin_end(5);
  settings_grid.set_margin_bottom(6);
  settings_grid.set_margin_start(6);
  settings_grid.set_column_spacing(6);
  settings_grid.set_row_spacing(8);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_preferences_label.set_attributes(attr_list_header_label);
  header_preferences_label.set_margin_top(5);
  header_preferences_label.set_margin_bottom(5);

  default_folder_label.set_halign(Gtk::Align::ALIGN_END);
  prefer_wine64_label.set_halign(Gtk::Align::ALIGN_END);
  debug_logging_label.set_halign(Gtk::Align::ALIGN_END);
  logging_stderr_label.set_halign(Gtk::Align::ALIGN_END);

  default_folder_entry.set_hexpand(true);

  settings_grid.attach(default_folder_label, 0, 0);
  settings_grid.attach(default_folder_entry, 1, 0);
  settings_grid.attach(prefer_wine64_label, 0, 1);
  settings_grid.attach(prefer_wine64_check, 1, 1);
  settings_grid.attach(debug_logging_label, 0, 2);
  settings_grid.attach(enable_debug_logging_check, 1, 2);
  settings_grid.attach(logging_stderr_label, 0, 3);
  settings_grid.attach(enable_logging_stderr_check, 1, 3);

  hbox_buttons.pack_end(save_button, false, false, 4);
  hbox_buttons.pack_end(cancel_button, false, false, 4);

  vbox.pack_start(header_preferences_label, false, false, 4);
  vbox.pack_start(settings_grid, true, true, 4);
  vbox.pack_start(hbox_buttons, false, false, 4);
  add(vbox);

  // Signals
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &PreferencesWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &PreferencesWindow::on_save_button_clicked));

  show_all_children();
}

/**
 * \brief Destructor
 */
PreferencesWindow::~PreferencesWindow()
{
}

/**
 * \brief Same as show() but will also load the WineGUI preferences from disk
 */
void PreferencesWindow::show()
{
  GenericConfigData generic_config = GenericConfigFile::read_config_file();
  default_folder_entry.set_text(generic_config.default_folder);
  prefer_wine64_check.set_active(generic_config.prefer_wine64);
  enable_debug_logging_check.set_active(generic_config.enable_debug_logging);
  enable_logging_stderr_check.set_active(generic_config.enable_logging_stderr);
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Triggered when cancel button is clicked
 */
void PreferencesWindow::on_cancel_button_clicked()
{
  hide();
}

/**
 * \brief Triggered when save button is clicked
 */
void PreferencesWindow::on_save_button_clicked()
{
  // Save preferences to disk
  GenericConfigData generic_config;
  generic_config.default_folder = default_folder_entry.get_text();
  generic_config.prefer_wine64 = prefer_wine64_check.get_active();
  generic_config.enable_debug_logging = enable_debug_logging_check.get_active();
  generic_config.enable_logging_stderr = enable_logging_stderr_check.get_active();
  if (!GenericConfigFile::write_config_file(generic_config))
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving generic config file.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
    dialog.set_title("An error has occurred!");
    dialog.set_modal(true);
    dialog.run();
  }
  else
  {
    // Hide preferences window
    hide();
    // Trigger manager update & UI update
    config_saved.emit();
  }
}
