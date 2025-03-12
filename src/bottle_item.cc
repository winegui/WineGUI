/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    bottle_item.cc
 * \brief   Wine bottle item class
 * \author  Melroy van den Berg <melroy@melroy.org>
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
#include <glibmm/markup.h>

#include "helper.h"
#include "wine_defaults.h"

/**
 * \brief Default Constructor
 */
BottleItem::BottleItem()
{
  // Gui will be created during the copy constructor called by GTK
}

/**
 * \brief Copy constructor, used by GTK
 */
BottleItem::BottleItem(const BottleItem& bottle_item) : BottleItem()
{
  if (this != &bottle_item)
  {
    name_ = bottle_item.name();
    folder_name_ = bottle_item.folder_name();
    description_ = bottle_item.description();
    is_status_ok_ = bottle_item.status();
    win_ = bottle_item.windows();
    bit_ = bottle_item.bit();
    wine_version_ = bottle_item.wine_version();
    is_wine64_bit_ = bottle_item.is_wine64_bit();
    wine_location_ = bottle_item.wine_location();
    wine_c_drive_ = bottle_item.wine_c_drive();
    wine_last_changed_ = bottle_item.wine_last_changed();
    audio_driver_ = bottle_item.audio_driver();
    virtual_desktop_ = bottle_item.virtual_desktop();
    is_debug_logging_ = bottle_item.is_debug_logging();
    debug_log_level_ = bottle_item.debug_log_level();
    env_vars_ = bottle_item.env_vars();
    app_list_ = bottle_item.app_list();
  }

  CreateUI();
}

/**
 * \brief Construct a new Wine Bottle Item with limited inputs
 */
BottleItem::BottleItem(Glib::ustring& name,
                       Glib::ustring& folder_name,
                       Glib::ustring& wine_version,
                       bool is_wine64_bit,
                       Glib::ustring& wine_location,
                       Glib::ustring& wine_c_drive,
                       Glib::ustring& wine_last_changed)
    : name_(name),
      folder_name_(folder_name),
      description_(""),
      is_status_ok_(true),
      win_(WineDefaults::WindowsOs),
      bit_(BottleTypes::Bit::win32),
      wine_version_(wine_version),
      is_wine64_bit_(is_wine64_bit),
      wine_location_(wine_location),
      wine_c_drive_(wine_c_drive),
      wine_last_changed_(wine_last_changed),
      audio_driver_(WineDefaults::AudioDriver),
      virtual_desktop_(""),
      is_debug_logging_(false),
      debug_log_level_(1){
          // Gui will be created during the copy constructor called by Gtk
      };

/**
 * \brief Construct a new Wine Bottle Item
 */
BottleItem::BottleItem(Glib::ustring& name,
                       Glib::ustring& folder_name,
                       Glib::ustring& description,
                       bool status,
                       BottleTypes::Windows win,
                       BottleTypes::Bit bit,
                       Glib::ustring& wine_version,
                       bool is_wine64_bit,
                       Glib::ustring& wine_location,
                       Glib::ustring& wine_c_drive,
                       Glib::ustring& wine_last_changed,
                       BottleTypes::AudioDriver audio_driver,
                       Glib::ustring& virtual_desktop,
                       bool is_debug_logging,
                       int debug_log_level,
                       std::vector<std::pair<std::string, std::string>>& env_vars,
                       std::map<int, ApplicationData>& app_list)
    : name_(name),
      folder_name_(folder_name),
      description_(description),
      is_status_ok_(status),
      win_(win),
      bit_(bit),
      wine_version_(wine_version),
      is_wine64_bit_(is_wine64_bit),
      wine_location_(wine_location),
      wine_c_drive_(wine_c_drive),
      wine_last_changed_(wine_last_changed),
      audio_driver_(audio_driver),
      virtual_desktop_(virtual_desktop),
      is_debug_logging_(is_debug_logging),
      debug_log_level_(debug_log_level),
      env_vars_(env_vars),
      app_list_(app_list){
          // Gui will be created during the copy constructor called by Gtk
      };

void BottleItem::CreateUI()
{
  // To lower case
  std::string windows_str = BottleItem::str_tolower(BottleTypes::to_string(this->windows()));
  // Remove spaces
  windows_str.erase(std::remove_if(std::begin(windows_str), std::end(windows_str), [l = std::locale{}](auto ch) { return std::isspace(ch, l); }),
                    end(windows_str));
  Glib::ustring bit_str = BottleTypes::to_string(this->bit());
  Glib::ustring filename_str = windows_str + "_" + bit_str + ".png";
  Glib::ustring name_str = this->name();
  Glib::ustring folder_name_str = this->folder_name();
  Glib::ustring name_label_text = (!name_str.empty()) ? name_str : folder_name_str; // Fallback to folder name
  bool is_status = this->status();

  // Set left side of the GUI
  image.set(Helper::get_image_location("windows/" + filename_str));
  image.set_margin_top(8);
  image.set_margin_end(8);
  image.set_margin_bottom(8);
  image.set_margin_start(8);

  name_label.set_xalign(0.0);
  name_label.set_markup("<span size=\"medium\"><b>" + Glib::Markup::escape_text(name_label_text) + "</b></span>");

  Glib::ustring status_text = "Ready";
  if (is_status)
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
