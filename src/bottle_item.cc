/**
 * Copyright (c) 2019-2022 WineGUI
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

#include "helper.h"

/**
 * \brief Default contructor
 */
BottleItem::BottleItem()
{
  // Gui will be created during the copy contructor called by Gtk
}

/**
 * \brief Copy contructor, used by GTK+
 */
BottleItem::BottleItem(const BottleItem& bottle_item) : BottleItem()
{
  if (this != &bottle_item)
  {
    name_ = bottle_item.name();
    is_status_ok_ = bottle_item.status();
    win_ = bottle_item.windows();
    bit_ = bottle_item.bit();
    wine_version_ = bottle_item.wine_version();
    wine_location_ = bottle_item.wine_location();
    wine_c_drive_ = bottle_item.wine_c_drive();
    wine_last_changed_ = bottle_item.wine_last_changed();
    audio_driver_ = bottle_item.audio_driver();
    virtual_desktop_ = bottle_item.virtual_desktop();
  }
  CreateUI();
}

/**
 * \brief Contruct a new Wine Bottle Item with limited inputs
 */
BottleItem::BottleItem(
    Glib::ustring name, Glib::ustring wine_version, Glib::ustring wine_location, Glib::ustring wine_c_drive, Glib::ustring wine_last_changed)
    : name_(name),
      is_status_ok_(true),
      win_(BottleTypes::Windows::WindowsXP),
      bit_(BottleTypes::Bit::win32),
      wine_version_(wine_version),
      wine_location_(wine_location),
      wine_c_drive_(wine_c_drive),
      wine_last_changed_(wine_last_changed),
      audio_driver_(BottleTypes::AudioDriver::pulseaudio),
      virtual_desktop_(""){
          // Gui will be created during the copy contructor called by Gtk
      };

/**
 * \brief Contruct a new Wine Bottle Item
 */
BottleItem::BottleItem(Glib::ustring name,
                       bool status,
                       BottleTypes::Windows win,
                       BottleTypes::Bit bit,
                       Glib::ustring wine_version,
                       Glib::ustring wine_location,
                       Glib::ustring wine_c_drive,
                       Glib::ustring wine_last_changed,
                       BottleTypes::AudioDriver audio_driver,
                       Glib::ustring virtual_desktop)
    : name_(name),
      is_status_ok_(status),
      win_(win),
      bit_(bit),
      wine_version_(wine_version),
      wine_location_(wine_location),
      wine_c_drive_(wine_c_drive),
      wine_last_changed_(wine_last_changed),
      audio_driver_(audio_driver),
      virtual_desktop_(virtual_desktop){
          // Gui will be created during the copy contructor called by Gtk
      };

void BottleItem::CreateUI()
{
  // To lower case
  std::string windows = BottleItem::str_tolower(BottleTypes::to_string(this->windows()));
  // Remove spaces
  windows.erase(std::remove_if(std::begin(windows), std::end(windows), [l = std::locale{}](auto ch) { return std::isspace(ch, l); }), end(windows));
  Glib::ustring bit = BottleTypes::to_string(this->bit());
  Glib::ustring filename = windows + "_" + bit + ".png";
  Glib::ustring name = this->name();
  bool status = this->status();

  // Set left side of the GUI
  image.set(Helper::get_image_location("windows/" + filename));
  image.set_margin_top(8);
  image.set_margin_end(8);
  image.set_margin_bottom(8);
  image.set_margin_start(8);

  name_label.set_xalign(0.0);
  name_label.set_markup("<span size=\"medium\"><b>" + Glib::Markup::escape_text(name) + "</b></span>");

  Glib::ustring status_text = "Ready";
  if (status)
  {
    status_icon.set(Helper::get_image_location("ready.png"));
  }
  else
  {
    status_text = "Not Ready";
    status_icon.set(Helper::get_image_location("not_ready.png"));
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
std::string BottleItem::str_tolower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}
