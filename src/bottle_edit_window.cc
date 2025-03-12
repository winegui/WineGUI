/**
 * Copyright (c) 2020-2025 WineGUI
 *
 * \file    bottle_edit_window.cc
 * \brief   Wine bottle edit window
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
#include "bottle_edit_window.h"
#include "bottle_item.h"
#include "wine_defaults.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleEditWindow::BottleEditWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_edit_label("Edit Machine"),
      name_label("Name: "),
      folder_name_label("Folder Name: "),
      windows_version_label("Windows Version: "),
      audio_driver_label("Audio Driver:"),
      virtual_desktop_resolution_label("Window Resolution:"),
      log_level_label("Log Level:"),
      description_label("Description:"),
      environment_variables_label("Environment Variables:"),
      virtual_desktop_check("Enable Virtual Desktop Window"),
      enable_logging_check("Enable debug logging"),
      configure_environment_variables_button("Configure Environment Variables"),
      save_button("Save"),
      cancel_button("Cancel"),
      delete_button("Delete Machine"),
      busy_dialog(*this),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_default_size(540, 540);
  set_modal(true);

  edit_grid.set_margin_top(5);
  edit_grid.set_margin_end(5);
  edit_grid.set_margin_bottom(6);
  edit_grid.set_margin_start(6);
  edit_grid.set_column_spacing(6);
  edit_grid.set_row_spacing(8);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_edit_label.set_attributes(attr_list_header_label);
  header_edit_label.set_margin_top(5);
  header_edit_label.set_margin_bottom(5);

  name_label.set_halign(Gtk::Align::ALIGN_END);
  folder_name_label.set_halign(Gtk::Align::ALIGN_END);
  windows_version_label.set_halign(Gtk::Align::ALIGN_END);
  audio_driver_label.set_halign(Gtk::Align::ALIGN_END);
  virtual_desktop_resolution_label.set_halign(Gtk::Align::ALIGN_END);
  log_level_label.set_halign(Gtk::Align::ALIGN_END);
  environment_variables_label.set_halign(Gtk::Align::ALIGN_END);
  description_label.set_halign(Gtk::Align::ALIGN_START);
  name_label.set_tooltip_text("Change the machine name");
  folder_name_label.set_tooltip_text("Change the folder. NOTE: This break your shortcuts!");
  windows_version_label.set_tooltip_text("Change the Windows version");
  audio_driver_label.set_tooltip_text("Change the audio driver");
  virtual_desktop_resolution_label.set_tooltip_text("Set the emulated desktop resolution");
  log_level_label.set_tooltip_text("Change the Wine debug messages for logging");
  environment_variables_label.set_tooltip_text("Set one or more environment variables");
  description_label.set_tooltip_text("Add an additional description text to your machine");

  // Fill-in Audio drivers in combobox
  for (int i = BottleTypes::AudioDriverStart; i < BottleTypes::AudioDriverEnd; i++)
  {
    audio_driver_combobox.append(std::to_string(i), BottleTypes::to_string(BottleTypes::AudioDriver(i)));
  }
  virtual_desktop_check.set_active(false);
  virtual_desktop_resolution_entry.set_text("1024x768");
  enable_logging_check.set_active(false);

  log_level_combobox.append("0", "Off");
  log_level_combobox.append("1", "Error + Fixme (Default)");
  log_level_combobox.append("2", "Only Errors (Could improve performance)");
  log_level_combobox.append("3", "Also log warnings (recommended for debugging)");
  log_level_combobox.append("4", "Log Frames per second)");
  log_level_combobox.append("5", "Disable D3D/GL messages (could improve performance)");
  log_level_combobox.append("6", "Relay + Heap");
  log_level_combobox.append("7", "Relay + Message box");
  log_level_combobox.append("8", "All Except relay (too verbose)");
  log_level_combobox.append("9", "All (most likely too verbose)");
  log_level_combobox.set_tooltip_text("More info: https://wiki.winehq.org/Debug_Channels");
  name_entry.set_hexpand(true);
  folder_name_entry.set_hexpand(true);
  windows_version_combobox.set_hexpand(true);
  audio_driver_combobox.set_hexpand(true);
  log_level_combobox.set_hexpand(true);
  description_text_view.set_hexpand(true);
  virtual_desktop_check.set_tooltip_text("Enable emulate virtual desktop resolution");
  enable_logging_check.set_tooltip_text("Enable output logging to disk");
  folder_name_entry.set_tooltip_text("Important: This will break your shortcuts! Consider changing the name instead, see above.");

  description_scrolled_window.add(description_text_view);
  description_scrolled_window.set_hexpand(true);
  description_scrolled_window.set_vexpand(true);

  edit_grid.attach(name_label, 0, 0);
  edit_grid.attach(name_entry, 1, 0);
  edit_grid.attach(folder_name_label, 0, 1);
  edit_grid.attach(folder_name_entry, 1, 1);
  edit_grid.attach(windows_version_label, 0, 2);
  edit_grid.attach(windows_version_combobox, 1, 2);
  edit_grid.attach(audio_driver_label, 0, 3);
  edit_grid.attach(audio_driver_combobox, 1, 3);
  edit_grid.attach(virtual_desktop_check, 0, 4, 2);
  edit_grid.attach(virtual_desktop_resolution_label, 0, 5);
  edit_grid.attach(virtual_desktop_resolution_entry, 1, 5);
  edit_grid.attach(enable_logging_check, 0, 6, 2);
  edit_grid.attach(log_level_label, 0, 7);
  edit_grid.attach(log_level_combobox, 1, 7);
  edit_grid.attach(environment_variables_label, 0, 8);
  edit_grid.attach(configure_environment_variables_button, 1, 8);
  edit_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 9, 2);
  edit_grid.attach(description_label, 0, 10, 2);
  edit_grid.attach(description_scrolled_window, 0, 11, 2);

  hbox_buttons.pack_start(delete_button, false, false, 4);
  hbox_buttons.pack_end(save_button, false, false, 4);
  hbox_buttons.pack_end(cancel_button, false, false, 4);

  vbox.pack_start(header_edit_label, false, false, 4);
  vbox.pack_start(edit_grid, true, true, 4);
  vbox.pack_start(hbox_buttons, false, false, 4);
  add(vbox);

  // Gray-out virtual desktop & log level by default
  virtual_desktop_resolution_sensitive(false);
  log_level_sensitive(false);

  // Signals
  configure_environment_variables_button.signal_clicked().connect(configure_environment_variables);
  delete_button.signal_clicked().connect(remove_bottle);
  virtual_desktop_check.signal_toggled().connect(sigc::mem_fun(*this, &BottleEditWindow::on_virtual_desktop_toggle));
  enable_logging_check.signal_toggled().connect(sigc::mem_fun(*this, &BottleEditWindow::on_debug_logging_toggle));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleEditWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleEditWindow::on_save_button_clicked));

  show_all_children();
}

/**
 * \brief Destructor
 */
BottleEditWindow::~BottleEditWindow()
{
}

/**
 * \brief Same as show() but will also update the Window title, set name,
 * update list of windows versions, set active windows, audio driver and virtual desktop
 */
void BottleEditWindow::show()
{
  if (active_bottle_ != nullptr)
  {
    set_title("Edit Machine - " +
              ((!active_bottle_->name().empty()) ? active_bottle_->name() : active_bottle_->folder_name())); // Fallback to folder name
    // Enable save button (again)
    save_button.set_sensitive(true);

    // Set name
    name_entry.set_text(active_bottle_->name());
    // Set folder name
    folder_name_entry.set_text(active_bottle_->folder_name());
    // Set description
    description_text_view.get_buffer()->set_text(active_bottle_->description());

    // Clear list
    windows_version_combobox.remove_all();
    // Fill-in Windows versions in combobox
    for (std::vector<BottleTypes::WindowsAndBit>::iterator it = BottleTypes::SupportedWindowsVersions.begin();
         it != BottleTypes::SupportedWindowsVersions.end(); ++it)
    {
      // Only show the same bitness Windows versions
      if (active_bottle_->bit() == (*it).second)
      {
        auto index = std::distance(BottleTypes::SupportedWindowsVersions.begin(), it);
        windows_version_combobox.append(std::to_string(index),
                                        BottleTypes::to_string((*it).first) + " (" + BottleTypes::to_string((*it).second) + ')');
      }
    }
    windows_version_combobox.set_active_text(BottleTypes::to_string(active_bottle_->windows()) + " (" +
                                             BottleTypes::to_string(active_bottle_->bit()) + ")");
    audio_driver_combobox.set_active_id(std::to_string((int)active_bottle_->audio_driver()));
    if (!active_bottle_->virtual_desktop().empty())
    {
      virtual_desktop_resolution_entry.set_text(active_bottle_->virtual_desktop());
      virtual_desktop_check.set_active(true);
    }
    else
    {
      virtual_desktop_check.set_active(false);
    }

    enable_logging_check.set_active(active_bottle_->is_debug_logging());
    log_level_combobox.set_active_id(std::to_string((int)active_bottle_->debug_log_level()));

    show_all_children();
  }
  else
  {
    set_title("Edit Machine (Unknown machine)");
  }
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void BottleEditWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleEditWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Triggered when bottle is actually confirmed to be removed
 */
void BottleEditWindow::bottle_removed()
{
  hide(); // Close the edit window
}

/**
 * \brief Handler when the bottle is updated.
 */
void BottleEditWindow::on_bottle_updated()
{
  busy_dialog.hide();
  hide(); // Close the edit Window
}

/**
 * \brief Enable/disable desktop resolution fields.
 * \param sensitive Set true to enable, false for disable
 */
void BottleEditWindow::virtual_desktop_resolution_sensitive(bool sensitive)
{
  virtual_desktop_resolution_label.set_sensitive(sensitive);
  virtual_desktop_resolution_entry.set_sensitive(sensitive);
}

/**
 * \brief Enable/disable debug log level
 * \param sensitive Set true to enable, false for disable
 */
void BottleEditWindow::log_level_sensitive(bool sensitive)
{
  log_level_label.set_sensitive(sensitive);
  log_level_combobox.set_sensitive(sensitive);
}

/**
 * \brief Signal handler when the virtual desktop checkbox is checked.
 * It will show the additional resolution input field.
 */
void BottleEditWindow::on_virtual_desktop_toggle()
{
  virtual_desktop_resolution_sensitive(virtual_desktop_check.get_active());
}

/**
 * \brief Signal handler when the debug logging checkbox is checked.
 * It will show the additional log level input field.
 */
void BottleEditWindow::on_debug_logging_toggle()
{
  log_level_sensitive(enable_logging_check.get_active());
}

/**
 * \brief Triggered when cancel button is clicked
 */
void BottleEditWindow::on_cancel_button_clicked()
{
  hide();
}

/**
 * \brief Triggered when save button is clicked
 */
void BottleEditWindow::on_save_button_clicked()
{
  std::string::size_type sz;

  UpdateBottleStruct update_bottle_struct;
  update_bottle_struct.windows_version = WineDefaults::WindowsOs; // Fallback
  update_bottle_struct.audio = WineDefaults::AudioDriver;         // Fallback
  update_bottle_struct.virtual_desktop_resolution = "";           // Empty string default (= disabled windowed mode)
  update_bottle_struct.debug_log_level = 1;                       // // 1 = Default wine debug logging

  // First disable save button (avoid multiple presses)
  save_button.set_sensitive(false);

  // Show busy dialog
  busy_dialog.set_message("Updating Windows Machine", "Busy applying all your changes currently.");
  busy_dialog.show();

  update_bottle_struct.name = name_entry.get_text();
  update_bottle_struct.folder_name = folder_name_entry.get_text();
  update_bottle_struct.description = description_text_view.get_buffer()->get_text();
  bool is_desktop_enabled = virtual_desktop_check.get_active();
  if (is_desktop_enabled)
  {
    update_bottle_struct.virtual_desktop_resolution = virtual_desktop_resolution_entry.get_text();
  }
  update_bottle_struct.is_debug_logging = enable_logging_check.get_active();
  try
  {
    update_bottle_struct.debug_log_level = std::stoi(log_level_combobox.get_active_id(), &sz);
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  // Ignore the catches
  try
  {
    size_t win_bit_index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    const auto& [currentWindowsVersionName, _] = BottleTypes::SupportedWindowsVersions.at(win_bit_index);
    update_bottle_struct.windows_version = currentWindowsVersionName;
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  // Ignore the catches
  try
  {
    size_t audio_index = size_t(std::stoi(audio_driver_combobox.get_active_id(), &sz));
    update_bottle_struct.audio = BottleTypes::AudioDriver(audio_index);
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  // Ignore the catches

  update_bottle.emit(update_bottle_struct);
}