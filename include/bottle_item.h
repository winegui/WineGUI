/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    bottle_item.h
 * \brief   Wine Bottle item class definition
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
#pragma once

#include "bottle_types.h"
#include <gtkmm.h>
#include <string>

/**
 * \class BottleItem
 * \brief Class object definition for a wine bottle item
 */

class BottleItem : public Gtk::ListBoxRow
{
public:
  BottleItem();
  /// Copy constructor
  BottleItem(const BottleItem& bottle_item);

  /// Copy-&-swap equal overloader method
  BottleItem& operator=(BottleItem temp_bottle_item)
  {
    this->swap(*this, temp_bottle_item);
    return *this;
  }

  /// swap helper method
  void swap(BottleItem& a, BottleItem& b)
  {
    using std::swap;
    swap(a.name_, b.name_);
    swap(a.is_status_ok_, b.is_status_ok_);
    swap(a.win_, b.win_);
    swap(a.bit_, b.bit_);
    swap(a.wine_version_, b.wine_version_);
    swap(a.wine_c_drive_, b.wine_c_drive_);
    swap(a.wine_last_changed_, b.wine_last_changed_);
    swap(a.audio_driver_, b.audio_driver_);
    swap(a.virtual_desktop_, b.virtual_desktop_);
  }

  BottleItem(
      Glib::ustring name, Glib::ustring wine_version, Glib::ustring wine_location, Glib::ustring wine_c_drive, Glib::ustring wine_last_changed);

  BottleItem(Glib::ustring name,
             bool status,
             BottleTypes::Windows win,
             BottleTypes::Bit bit,
             Glib::ustring wine_version,
             Glib::ustring wine_location,
             Glib::ustring wine_c_drive,
             Glib::ustring wine_last_changed,
             BottleTypes::AudioDriver audio_driver,
             Glib::ustring virtual_desktop);

  /**
   * \brief Destruct
   */
  ~BottleItem(){};

  /*
   *  Getters & setters
   */
  /// set name
  void name(const Glib::ustring name)
  {
    name_ = name;
  };
  /// get name
  const Glib::ustring& name() const
  {
    return name_;
  };
  /// set status
  void status(const bool status)
  {
    is_status_ok_ = status;
  };
  /// get status
  bool status() const
  {
    return is_status_ok_;
  };
  /// set windows
  void windows(const BottleTypes::Windows win)
  {
    win_ = win;
  };
  /// get windows
  BottleTypes::Windows windows() const
  {
    return win_;
  };
  /// set bit
  void bit(const BottleTypes::Bit bit)
  {
    bit_ = bit;
  };
  /// get bit
  BottleTypes::Bit bit() const
  {
    return bit_;
  };
  /// set Wine version
  void wine_version(const Glib::ustring wine_version)
  {
    wine_version_ = wine_version;
  };
  /// set Wine version
  const Glib::ustring& wine_version() const
  {
    return wine_version_;
  };
  /// set Wine location
  void wine_location(const Glib::ustring wine_location)
  {
    wine_location_ = wine_location;
  };
  /// get Wine location
  const Glib::ustring& wine_location() const
  {
    return wine_location_;
  };
  /// set Wine c:\ drive location
  void wine_c_drive(const Glib::ustring wine_c_drive)
  {
    wine_c_drive_ = wine_c_drive;
  };
  /// get Wine c:\ drive location
  const Glib::ustring& wine_c_drive() const
  {
    return wine_c_drive_;
  };
  // TODO: Changed to datetime iso Glib::ustring
  /// set Wine last changed date
  void wine_last_changed(const Glib::ustring wine_last_changed)
  {
    wine_last_changed_ = wine_last_changed;
  };
  /// get Wine last changed date
  const Glib::ustring& wine_last_changed() const
  {
    return wine_last_changed_;
  };
  /// set Wine audio driver
  void audio_driver(const BottleTypes::AudioDriver audio_driver)
  {
    audio_driver_ = audio_driver;
  };
  /// get Wine audio driver
  BottleTypes::AudioDriver audio_driver() const
  {
    return audio_driver_;
  };
  /// set Wine emulate virtual desktop (set to empty string to disable)
  void virtual_desktop(const Glib::ustring virtual_desktop)
  {
    virtual_desktop_ = virtual_desktop;
  };
  /// get Wine emulate virtual desktop (empty string is disabled)
  const Glib::ustring& virtual_desktop() const
  {
    return virtual_desktop_;
  };

protected:
  // Widgets
  Gtk::Grid grid;          /*!< The main grid for the listbox item */
  Gtk::Image image;        /*!< Windows logo of the Wine bottle */
  Gtk::Label name_label;   /*!< Name of the Wine Bottle */
  Gtk::Image status_icon;  /*!< Status icon of the Wine Bottle */
  Gtk::Label status_label; /*!< Status of the Wine Bottle */

private:
  Glib::ustring name_;
  bool is_status_ok_;
  BottleTypes::Windows win_;
  BottleTypes::Bit bit_;
  Glib::ustring wine_version_;
  Glib::ustring wine_location_;
  Glib::ustring wine_c_drive_;
  Glib::ustring wine_last_changed_;
  BottleTypes::AudioDriver audio_driver_;
  Glib::ustring virtual_desktop_;

  void CreateUI();
  static std::string str_tolower(std::string s);
};
