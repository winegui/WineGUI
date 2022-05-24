/**
 * Copyright (c) 2020-2022 WineGUI
 *
 * \file    settings_window.cc
 * \brief   Setting GTK+ Window class
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
#include "edit_window.h"
#include "bottle_item.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
EditWindow::EditWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_edit_label("Edit Machine"),
      name_label("Name: "),
      windows_version_label("Windows Version: "),
      audiodriver_label("Audio Driver:"),
      virtual_desktop_resolution_label("Window Resolution:"),
      virtual_desktop_check("Enable Virtual Desktop Window"),
      save_button("Save"),
      cancel_button("Cancel"),
      delete_button("Delete Machine"),
      wine_config_button("WineCfg"),
      activeBottle(nullptr)
{
  set_transient_for(parent);
  set_default_size(550, 250);
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
  windows_version_label.set_halign(Gtk::Align::ALIGN_END);

  // Fill-in Audio drivers in combobox
  for (int i = BottleTypes::AudioDriverStart; i < BottleTypes::AudioDriverEnd; i++)
  {
    audiodriver_combobox.insert(-1, std::to_string(i), BottleTypes::toString(BottleTypes::AudioDriver(i)));
  }
  virtual_desktop_check.set_active(false);
  virtual_desktop_resolution_entry.set_text("960x540");

  name_entry.set_hexpand(true);
  windows_version_combobox.set_hexpand(true);
  audiodriver_combobox.set_hexpand(true);

  edit_grid.attach(name_label, 0, 0);
  edit_grid.attach(name_entry, 1, 0);
  edit_grid.attach(windows_version_label, 0, 1);
  edit_grid.attach(windows_version_combobox, 1, 1);
  edit_grid.attach(audiodriver_label, 0, 2);
  edit_grid.attach(audiodriver_combobox, 1, 2);
  edit_grid.attach(virtual_desktop_check, 0, 3, 2);

  hbox_buttons.pack_start(delete_button, false, false, 4);
  hbox_buttons.pack_end(save_button, false, false, 4);
  hbox_buttons.pack_end(cancel_button, false, false, 4);

  // vbox.pack_start(vbox_delete, true, true, 4);
  vbox.pack_start(header_edit_label, false, false, 4);
  vbox.pack_start(edit_grid, true, true, 4);
  vbox.pack_start(hbox_buttons, false, false, 4);
  add(vbox);

  // Signals
  delete_button.signal_clicked().connect(remove_machine);
  virtual_desktop_check.signal_toggled().connect(sigc::mem_fun(*this, &EditWindow::on_virtual_desktop_toggle));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &EditWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &EditWindow::on_save_button_clicked));

  show_all_children();
}

/**
 * \brief Destructor
 */
EditWindow::~EditWindow()
{
}

/**
 * \brief Same as show() but will also update the Window title, set name,
 * update list of windows versions, set active windows, audio driver and virtual desktop
 */
void EditWindow::Show()
{
  if (activeBottle != nullptr)
  {
    set_title("Edit Machine - " + activeBottle->name());

    // Set name
    name_entry.set_text(activeBottle->name());

    // Clear list
    windows_version_combobox.remove_all();
    // Fill-in Windows versions in combobox
    for (std::vector<BottleTypes::WindowsAndBit>::iterator it = BottleTypes::SupportedWindowsVersions.begin();
         it != BottleTypes::SupportedWindowsVersions.end(); ++it)
    {
      // Only show the same bitness Windows versions
      if (activeBottle->bit() == (*it).second)
      {
        auto index = std::distance(BottleTypes::SupportedWindowsVersions.begin(), it);
        windows_version_combobox.insert(-1, std::to_string(index),
                                        BottleTypes::toString((*it).first) + " (" +
                                            BottleTypes::toString((*it).second) + ')');
      }
    }
    windows_version_combobox.set_active_text(BottleTypes::toString(activeBottle->windows()) + " (" +
                                             BottleTypes::toString(activeBottle->bit()) + ")");
    audiodriver_combobox.set_active_id(std::to_string((int)activeBottle->audio_driver()));
    if (!activeBottle->virtual_desktop().empty())
    {
      virtual_desktop_resolution_entry.set_text(activeBottle->virtual_desktop());
      virtual_desktop_check.set_active(true);
    }
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
void EditWindow::SetActiveBottle(BottleItem* bottle)
{
  this->activeBottle = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void EditWindow::ResetActiveBottle()
{
  this->activeBottle = nullptr;
}

/**
 * \brief Triggered when bottle is actually confirmed to be removed
 */
void EditWindow::BottleRemoved()
{
  // Close the edit window
  hide();
}

/**
 * \brief Show (add) the additional virtual desktop label + input field
 */
void EditWindow::ShowVirtualDesktopResolution()
{
  edit_grid.attach(virtual_desktop_resolution_label, 0, 4);
  edit_grid.attach(virtual_desktop_resolution_entry, 1, 4);
}

/**
 * \brief Hide (remove) the virtual desktop section from grid
 */
void EditWindow::HideVirtualDesktopResolution()
{
  edit_grid.remove_row(4);
}

/**
 * \brief Signal handler when the virtual desktop checkbox is checked.
 * It will show the additional resolution input field.
 */
void EditWindow::on_virtual_desktop_toggle()
{
  if (virtual_desktop_check.get_active())
  {
    ShowVirtualDesktopResolution();
  }
  else
  {
    HideVirtualDesktopResolution();
  }
  show_all_children();
}

/**
 * \brief Triggered when cancel button is clicked
 */
void EditWindow::on_cancel_button_clicked()
{
  hide();
}

/**
 * \brief Triggered when save button is clicked
 */
void EditWindow::on_save_button_clicked()
{
  std::string::size_type sz;
  BottleTypes::Windows windows_version = BottleTypes::Windows::WindowsXP; // Fallback
  BottleTypes::Bit bit = BottleTypes::Bit::win32;                         // Fallback
  BottleTypes::AudioDriver audio = BottleTypes::AudioDriver::pulseaudio;  // Fallback
  Glib::ustring virtual_desktop_resolution = "";                          // Default empty string
  Glib::ustring name = name_entry.get_text();
  bool isDesktopEnabled = virtual_desktop_check.get_active();
  if (isDesktopEnabled)
  {
    virtual_desktop_resolution = virtual_desktop_resolution_entry.get_text();
  }

  try
  {
    size_t win_bit_index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    const auto currentWindowsBit = BottleTypes::SupportedWindowsVersions.at(win_bit_index);
    windows_version = currentWindowsBit.first;
    bit = currentWindowsBit.second;
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (std::invalid_argument& e)
  {
  }
  catch (std::out_of_range& e)
  {
  }
  // Ignore the catches

  try
  {
    size_t audio_index = size_t(std::stoi(audiodriver_combobox.get_active_id(), &sz));
    audio = BottleTypes::AudioDriver(audio_index);
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (std::invalid_argument& e)
  {
  }
  catch (std::out_of_range& e)
  {
  }
  // Ignore the catches
}