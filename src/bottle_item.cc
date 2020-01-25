/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    bottle_item.cc
 * \brief   Wine Bottle Item
 * \author  Melroy van den Berg <webmaster1989@gmail.com>
 * \note https://github.com/pirobtumen/Remember/blob/master/src/gui/taskitem.cpp
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
#include "bottle_item.h"

/**
 * \brief Default contructor
 */
BottleItem::BottleItem() {
  // Gui will be created during the copy contructor called by Gtk
}

/**
 * \brief Copy contructor, used by GTK+
 */
BottleItem::BottleItem(const BottleItem& bottleItem) : BottleItem() {
  if ( this != &bottleItem ) {
    _name = bottleItem.name();
    _is_status_ok = bottleItem.status();
    _win = bottleItem.windows();
    _bit = bottleItem.bit();
    _wine_version = bottleItem.wine_version();
    _wine_location = bottleItem.wine_location();
    _wine_c_drive = bottleItem.wine_c_drive();
    _wine_last_changed = bottleItem.wine_last_changed();
    _audio_driver = bottleItem.audio_driver();
    _virtual_desktop = bottleItem.virtual_desktop();
  }
  CreateUI();
}

/**
 * \brief Contruct a new Wine Bottle Item with limited inputs
 */
BottleItem::BottleItem(
  Glib::ustring name, 
  Glib::ustring wine_version,
  Glib::ustring wine_location,
  Glib::ustring wine_c_drive,
  Glib::ustring wine_last_changed)
:
  _name(name),
  _is_status_ok(true),
  _win(BottleTypes::Windows::WindowsXP),
  _bit(BottleTypes::Bit::win32),
  _wine_version(wine_version),
  _wine_location(wine_location),
  _wine_c_drive(wine_c_drive),
  _wine_last_changed(wine_last_changed),
  _audio_driver(BottleTypes::AudioDriver::pulseaudio),
  _virtual_desktop("disabled")
{
  // Gui will be created during the copy contructor called by Gtk
};

/**
  * \brief Contruct a new Wine Bottle Item
  */
BottleItem::BottleItem(
  Glib::ustring name,
  bool status,
  BottleTypes::Windows win,
  BottleTypes::Bit bit,
  Glib::ustring wine_version,
  Glib::ustring wine_location,
  Glib::ustring wine_c_drive,
  Glib::ustring wine_last_changed,
  BottleTypes::AudioDriver audio_driver,
  Glib::ustring virtual_desktop)
:
  _name(name),
  _is_status_ok(status),
  _win(win),
  _bit(bit),
  _wine_version(wine_version),
  _wine_location(wine_location),
  _wine_c_drive(wine_c_drive),
  _wine_last_changed(wine_last_changed),
  _audio_driver(audio_driver),
  _virtual_desktop(virtual_desktop)
{
  // Gui will be created during the copy contructor called by Gtk
};

void BottleItem::CreateUI()
{
  // To lower case
  std::string windows = BottleItem::str_tolower(BottleTypes::toString(this->windows()));
  // Remove spaces
  windows.erase(std::remove_if (
    std::begin(windows), std::end(windows),
    [l = std::locale{}](auto ch) { return std::isspace(ch, l); }
  ), end(windows));
  Glib::ustring bit = BottleTypes::toString(this->bit());
  Glib::ustring filename = windows + "_" + bit + ".png";
  Glib::ustring name = this->name();
  bool status = this->status();

  // Set left side of the GUI
  image.set(IMAGE_LOCATION "windows/" + filename);
  image.set_margin_top(8);
  image.set_margin_end(8);
  image.set_margin_bottom(8);
  image.set_margin_start(8);

  name_label.set_xalign(0.0);
  name_label.set_markup("<span size=\"medium\"><b>" + name + "</b></span>");
  
  Glib::ustring status_text = "Ready";
  if (status) {
    status_icon.set(IMAGE_LOCATION "ready.png");
  } else {
    status_text = "Not Ready";
    status_icon.set(IMAGE_LOCATION "not_ready.png");
  }
  status_icon.set_size_request(2, -1);
  status_icon.set_halign(Gtk::Align::ALIGN_START);

  status_label.set_text(status_text);
  status_label.set_xalign(0.0);

  grid.set_column_spacing(8);
  grid.set_row_spacing(5);
  grid.set_border_width(4);

  grid.attach(image, 0, 0, 1, 2);
  // Agh, stupid GTK! Width 2 would be enough, add 8 extra = 10
  // I can't control the gtk grid cell width
  grid.attach_next_to(name_label, image, Gtk::PositionType::POS_RIGHT, 10, 1);

  grid.attach(status_icon, 1, 1, 1, 1);
  grid.attach_next_to(status_label, status_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Finally at the grid to the ListBoxRow
  add(grid);
}

/**
 * \brief String to lower string helper method
 * \param[in] string that needs lower case
 * \return lower case string
 */
std::string BottleItem::str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), 
    [](unsigned char c){ return std::tolower(c); }
  );
  return s;
}
