/**
 * Copyright (c) 2019 WineGUI
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

#include <gtkmm.h>
#include <string>
#include "bottle_types.h"

/**
 * \class BottleItem
 * \brief Class object definition for a wine bottle item
 */

class BottleItem : public Gtk::ListBoxRow
{
public:
  BottleItem();
  /// Copy constructor
  BottleItem(const BottleItem& bottleItem);

  /// Copy-&-swap equal overloader method
  BottleItem& operator=(BottleItem tempBottleItem) {
    this->swap(*this, tempBottleItem);
    return *this;
  }
  
  /// swap helper method
  void swap(BottleItem& a, BottleItem& b) {
    using std::swap;
    swap(a._name, b._name);
    swap(a._is_status_ok, b._is_status_ok);
    swap(a._win, b._win);
    swap(a._bit, b._bit);
    swap(a._wine_version, b._wine_version);
    swap(a._wine_c_drive, b._wine_c_drive);
    swap(a._wine_last_changed, b._wine_last_changed);
    swap(a._audio_driver, b._audio_driver);
    swap(a._virtual_desktop, b._virtual_desktop);
  }

  BottleItem(Glib::ustring name, 
    Glib::ustring wine_version,
    Glib::ustring wine_location,
    Glib::ustring wine_c_drive,
    Glib::ustring wine_last_changed);

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
  ~BottleItem() {};

  /*
   *  Getters & setters
   */
  /// set name
  void name(const Glib::ustring name) { _name = name; };
  /// get name
  const Glib::ustring& name() const { return _name; };
  /// set status
  void status(const bool status) { _is_status_ok = status; };
  /// get status
  bool status() const { return _is_status_ok; };
  /// set windows 
  void windows(const BottleTypes::Windows win) { _win = win; };
  /// get windows 
  BottleTypes::Windows windows() const { return _win; };
  /// set bit
  void bit(const BottleTypes::Bit bit) { _bit = bit; };
  /// get bit
  BottleTypes::Bit bit() const { return _bit; };
  /// set Wine version
  void wine_version(const Glib::ustring wine_version) { _wine_version = wine_version; };
  /// set Wine version
  const Glib::ustring& wine_version() const { return _wine_version; };
  /// set Wine location
  void wine_location(const Glib::ustring wine_location) { _wine_location = wine_location; };
  /// get Wine location
  const Glib::ustring& wine_location() const { return _wine_location; };
  /// set Wine c:\ drive location
  void wine_c_drive(const Glib::ustring wine_c_drive) { _wine_c_drive = wine_c_drive; };
  /// get Wine c:\ drive location
  const Glib::ustring& wine_c_drive() const { return _wine_c_drive; };
  // TODO: Changed to datetime iso Glib::ustring
  /// set Wine last changed date
  void wine_last_changed(const Glib::ustring wine_last_changed) { _wine_last_changed = wine_last_changed; };
  /// get Wine last changed date
  const Glib::ustring& wine_last_changed() const { return _wine_last_changed; };
  /// set Wine audio driver
  void audio_driver(const BottleTypes::AudioDriver audio_driver) { _audio_driver = audio_driver; };
  /// get Wine audio driver
  BottleTypes::AudioDriver audio_driver() const { return _audio_driver; };
  /// set Wine emulate virtual desktop
  void virtual_desktop(const Glib::ustring virtual_desktop) { _virtual_desktop = virtual_desktop; };
  /// get Wine emulate virtual desktop
  const Glib::ustring& virtual_desktop() const { return _virtual_desktop; };

protected:
  // Widgets
  Gtk::Grid grid; /*!< The main grid for the listbox item */
  Gtk::Image image; /*!< Windows logo of the Wine bottle */
  Gtk::Label name_label; /*!< Name of the Wine Bottle */
  Gtk::Image status_icon; /*!< Status icon of the Wine Bottle */
  Gtk::Label status_label; /*!< Status of the Wine Bottle */

private:
  Glib::ustring _name;
  bool _is_status_ok;
  BottleTypes::Windows _win;
  BottleTypes::Bit _bit;
  Glib::ustring _wine_version;
  Glib::ustring _wine_location;
  Glib::ustring _wine_c_drive;
  Glib::ustring _wine_last_changed;
  BottleTypes::AudioDriver _audio_driver;
  Glib::ustring _virtual_desktop;

  void CreateUI();
  static std::string str_tolower(std::string s);
};
