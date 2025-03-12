/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    preferences_window.cc
 * \brief   Application preferences GTK3 window class
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
#include "preferences_window.h"
#include "general_config_file.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
PreferencesWindow::PreferencesWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_preferences_label("Preferences"),
      default_folder_label("Machine folder location: "),
      display_default_wine_machine_label("Show default Wine machine: "),
      logging_stderr_label("Log standard error:"),
      display_default_wine_machine_check("Display default Wine prefix bottle (at: ~/.wine)"),
      enable_logging_stderr_check("Also log standard error (if logging is enabled)"),
      select_folder_button("Select folder..."),
      save_button("Save"),
      cancel_button("Cancel")
{
  set_transient_for(parent);
  set_title("WineGUI Preferences");
  set_default_size(560, 300);
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

  logging_label_heading.set_markup("<big><b>Logging</b></big>");
  default_folder_label.set_halign(Gtk::Align::ALIGN_END);
  display_default_wine_machine_label.set_halign(Gtk::Align::ALIGN_END);
  logging_stderr_label.set_halign(Gtk::Align::ALIGN_END);
  default_folder_entry.set_hexpand(true);

  settings_grid.attach(default_folder_label, 0, 0);
  settings_grid.attach(default_folder_entry, 1, 0);
  settings_grid.attach(select_folder_button, 2, 0);
  settings_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 1, 3);
  settings_grid.attach(display_default_wine_machine_label, 0, 2);
  settings_grid.attach(display_default_wine_machine_check, 1, 2, 2);
  settings_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 4, 3);
  settings_grid.attach(logging_label_heading, 0, 5, 3);
  settings_grid.attach(logging_stderr_label, 0, 6);
  settings_grid.attach(enable_logging_stderr_check, 1, 6, 2);

  hbox_buttons.pack_end(save_button, false, false, 4);
  hbox_buttons.pack_end(cancel_button, false, false, 4);

  vbox.pack_start(header_preferences_label, false, false, 4);
  vbox.pack_start(settings_grid, true, true, 4);
  vbox.pack_start(hbox_buttons, false, false, 4);
  add(vbox);

  // Signals
  select_folder_button.signal_clicked().connect(sigc::mem_fun(*this, &PreferencesWindow::on_select_folder));
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
  GeneralConfigData general_config = GeneralConfigFile::read_config_file();
  default_folder_entry.set_text(general_config.default_folder);
  display_default_wine_machine_check.set_active(general_config.display_default_wine_machine);
  enable_logging_stderr_check.set_active(general_config.enable_logging_stderr);
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Triggered when select folder button is clicked
 */
void PreferencesWindow::on_select_folder()
{
  auto* folder_chooser =
      new Gtk::FileChooserDialog(*this, "Choose a folder", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER, Gtk::DialogFlags::DIALOG_MODAL);
  folder_chooser->set_modal(true);
  folder_chooser->signal_response().connect(sigc::bind(sigc::mem_fun(*this, &PreferencesWindow::on_select_dialog_response), folder_chooser));
  folder_chooser->add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
  folder_chooser->add_button("_Select folder", Gtk::ResponseType::RESPONSE_OK);
  folder_chooser->set_current_folder(default_folder_entry.get_text());
  folder_chooser->show();
}

/**
 * \brief when folder is selected
 */
void PreferencesWindow::on_select_dialog_response(int response_id, Gtk::FileChooserDialog* dialog)
{
  switch (response_id)
  {
  case Gtk::ResponseType::RESPONSE_OK:
  {
    // Get current older and update folder entry
    auto folder = dialog->get_current_folder();
    default_folder_entry.set_text(folder);
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
  GeneralConfigData general_config;
  general_config.default_folder = default_folder_entry.get_text();
  general_config.display_default_wine_machine = display_default_wine_machine_check.get_active();
  general_config.enable_logging_stderr = enable_logging_stderr_check.get_active();
  if (!GeneralConfigFile::write_config_file(general_config))
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
