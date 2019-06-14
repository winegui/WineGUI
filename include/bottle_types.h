/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    bottle_types.h
 * \brief   Bottle enum definitions
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

/**
 * \brief Bottle type enum definitions
 */
namespace BottleTypes {
  //// Emulate Virtual Desktop disabled string
  inline static const std::string VIRTUAL_DESKTOP_DISABLED = "Disabled";
  
  /**
   * \enum Windows
   * \brief List of Windows versions.
   * \note Don't forget to update the toString methods if required!
   */
  enum class Windows
  {
    Windows20,
    Windows30,
    Windows31,
    WindowsNT351,
    WindowsNT40,
    Windows95,
    Windows98,
    WindowsME,
    Windows2000,
    WindowsXP,
    Windows2003,
    WindowsVista,
    Windows2008,
    Windows7,
    Windows2008R2,
    Windows8,
    Windows81,
    Windows10
  };

  /**
   * \enum Bit
   * \brief Windows bit options
   */
  enum class Bit
  {
    win32,
    win64
  };
  
  /**
   * \enum AudioDriver
   * \brief Wine supported audio drivers
   */
  enum class AudioDriver 
  {
    pulseaudio,
    alsa,
    coreaudio,
    oss,
    disabled
  };
  
  // Bit enum to string
  static std::string toString(Bit bit) {
    switch(bit) {
      case Bit::win32:
        return "32";
      case Bit::win64:
        return "64";
      default:
        return "";
    }
  }

  // Windows enum to string
  // TODO: Move the helper.cc windows list to bottle_types,
  // in order to have a single point of definition of Windows names
  static std::string toString(Windows win) {
    switch(win) {
      case Windows::Windows20:
        return "Windows 2.0";    
      case Windows::Windows30:
        return "Windows 3.0";
      case Windows::Windows31:
        return "Windows 3.1";
      case Windows::WindowsNT351:
        return "Windows NT 3.51";
      case Windows::WindowsNT40:
        return "Windows NT 4.0";
      case Windows::Windows95:
        return "Windows 95";
      case Windows::Windows98:
        return "Windows 98";
      case Windows::WindowsME:
        return "Windows ME";
      case Windows::Windows2000:
        return "Windows 2000";
      case Windows::WindowsXP:
        return "Windows XP";
      case Windows::Windows2003:
        return "Windows 2003";
      case Windows::WindowsVista:
        return "Windows Vista";
      case Windows::Windows2008:
        return "Windows 2008";
      case Windows::Windows7:
        return "Windows 7";
      case Windows::Windows2008R2:
        return "Windows 2008 R2";
      case Windows::Windows8:
        return "Windows 8";
      case Windows::Windows81:
        return "Windows 8.1";
      case Windows::Windows10:
        return "Windows 10";
      default:
        return "- Unknown Windows OS -";
    }
  }

  // AudioDriver enum to string
  static std::string toString(AudioDriver audio) {
    switch(audio) {
      case AudioDriver::pulseaudio:
        return "PulseAudio";
      case AudioDriver::alsa:
        return "Advanced Linux Sound Architecture (ALSA)";
      case AudioDriver::coreaudio:
        return "Core Audio";
      case AudioDriver::oss:
        return "Open Sound System (OSS)";
      default:
        return "disabled";
    }
  }
};
