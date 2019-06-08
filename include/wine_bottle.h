/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    wine_bottle.h
 * \brief   Wine Bottle class definition (only header file)
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

#include <string>
#include "bottle_types.h"

using std::string;


/**
 * \class WineBottle
 * \brief Class object definition for a wine bottle (only header file)
 */
class WineBottle
{
public:
  WineBottle(
    string name, 
    string wine_version,
    string wine_location,
    string wine_c_drive,
    string wine_last_changed)
  : _name(name),
    _is_status_ok(true),
    _win(BottleTypes::Windows::WindowsXP),
    _bit(BottleTypes::Bit::win32),
    _wine_version(wine_version),
    _wine_location(wine_location),
    _wine_c_drive(wine_c_drive),
    _wine_last_changed(wine_last_changed),
    _audio_driver(BottleTypes::AudioDriver::pulseaudio),
    _virtual_desktop("disabled") {};

  WineBottle(
    string name,
    bool status,
    BottleTypes::Windows win,
    BottleTypes::Bit bit,
    string wine_version,
    string wine_location,
    string wine_c_drive,
    string wine_last_changed,
    BottleTypes::AudioDriver audio_driver,
    string virtual_desktop)
  : _name(name),
    _is_status_ok(status),
    _win(win),
    _bit(bit),
    _wine_version(wine_version),
    _wine_location(wine_location),
    _wine_c_drive(wine_c_drive),
    _wine_last_changed(wine_last_changed),
    _audio_driver(audio_driver),
    _virtual_desktop(virtual_desktop) {};

  ~WineBottle() {};

  void name(const string name) { _name = name; };
  const string& name() const { return _name; };
  void status(const bool status) { _is_status_ok = status; };
  const bool status() const { return _is_status_ok; };
  void windows(const BottleTypes::Windows win) { _win = win; };
  const BottleTypes::Windows windows() const { return _win; };
  void bit(const BottleTypes::Bit bit) { _bit = bit; };
  const BottleTypes::Bit bit() const { return _bit; };
  void wine_version(const string wine_version) { _wine_version = wine_version; };
  const string& wine_version() const { return _wine_version; };
  void wine_location(const string wine_location) { _wine_location = wine_location; };
  const string& wine_location() const { return _wine_location; };
  void wine_c_drive(const string wine_c_drive) { _wine_c_drive = wine_c_drive; };
  const string& wine_c_drive() const { return _wine_c_drive; };
  // TODO: Changed to datetime iso string
  void wine_last_changed(const string wine_last_changed) { _wine_last_changed = wine_last_changed; };
  const string& wine_last_changed() const { return _wine_last_changed; };
  void audio_driver(const BottleTypes::AudioDriver audio_driver) { _audio_driver = audio_driver; };
  const BottleTypes::AudioDriver audio_driver() const { return _audio_driver; };
  void virtual_desktop(const string virtual_desktop) { _virtual_desktop = virtual_desktop; };
  const string& virtual_desktop() const { return _virtual_desktop; };
  

private:
  string _name;
  bool _is_status_ok;
  BottleTypes::Windows _win;
  BottleTypes::Bit _bit;
  string _wine_version;
  string _wine_location;
  string _wine_c_drive;
  string _wine_last_changed;
  BottleTypes::AudioDriver _audio_driver;
  string _virtual_desktop;
};