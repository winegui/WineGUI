/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    bottle_types.h
 * \brief   Bottle type enum definitions (like Windows OS, audio driver, supported Windows list)
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
#include <vector>

/**
 * \brief Bottle type enum definitions
 */
namespace BottleTypes {
  //// Emulate Virtual Desktop disabled string
  static const std::string VIRTUAL_DESKTOP_DISABLED = "Disabled";

  /**
   * \enum Windows
   * \brief List of Windows versions.
   * \note Don't forget to update the toString methods if required!
   * \note Don't forget to update the hardcoded size below! C++ -,-
   */
  enum class Windows
  {
    Windows20 = 0,
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

  //// Size of Windows enum class
  static const unsigned int WINDOWS_ENUM_SIZE = 18;

  /**
   * \enum Bit
   * \brief Windows bit options
   */
  enum class Bit
  {
    win32,
    win64
  };

  typedef std::pair<Windows, Bit> WindowsAndBit; /*!< Windows + Bit pair, used within the list of supported Windows versions */
  
  /**
   * \brief Supported list of Windows version with their bit support
   */
  inline std::vector<WindowsAndBit> SupportedWindowsVersions =
  {
    std::pair(Windows::Windows20, Bit::win32),
    std::pair(Windows::Windows30, Bit::win32),
    std::pair(Windows::Windows31, Bit::win32),
    std::pair(Windows::WindowsNT351, Bit::win32),
    std::pair(Windows::WindowsNT40, Bit::win32),
    std::pair(Windows::Windows95, Bit::win32),
    std::pair(Windows::Windows98, Bit::win32),
    std::pair(Windows::WindowsME, Bit::win32),
    std::pair(Windows::Windows2000, Bit::win32),
    std::pair(Windows::WindowsXP, Bit::win32),
    std::pair(Windows::WindowsXP, Bit::win64),
    std::pair(Windows::Windows2003, Bit::win32),
    std::pair(Windows::Windows2003, Bit::win64),
    std::pair(Windows::WindowsVista, Bit::win32),
    std::pair(Windows::WindowsVista, Bit::win64),
    std::pair(Windows::Windows2008, Bit::win32),
    std::pair(Windows::Windows2008, Bit::win64),
    std::pair(Windows::Windows7, Bit::win32),
    std::pair(Windows::Windows7, Bit::win64),
    std::pair(Windows::Windows2008R2, Bit::win32),
    std::pair(Windows::Windows2008R2, Bit::win64),
    std::pair(Windows::Windows8, Bit::win32),
    std::pair(Windows::Windows8, Bit::win64),
    std::pair(Windows::Windows81, Bit::win32),
    std::pair(Windows::Windows81, Bit::win64),
    std::pair(Windows::Windows10, Bit::win32),
    std::pair(Windows::Windows10, Bit::win64),
  };

  //// Default Windows version (Windows XP 32-bit) as WineGUI Bottle
  static const int DefaultBottleIndex = 9;

  /**
   * \enum AudioDriver
   * \brief Wine supported audio drivers
   */
  enum class AudioDriver 
  {
    pulseaudio = 0,
    alsa,
    coreaudio,
    oss,
    disabled
  };
  
  //// Enum AudioDriver Start iterator
  static const int AudioDriverStart = (int)AudioDriver::pulseaudio;
  //// Enum AudioDriver End iterator
  static const int AudioDriverEnd = (int)AudioDriver::disabled + 1;

  //// Default AudioDriver for WineGui Bottle
  static const int DefaultAudioDriverIndex = (int)AudioDriver::pulseaudio;

  // Bit enum to string
  inline static std::string toString(Bit bit) {
    switch(bit) {
      case Bit::win32:
        return "32-bit";
      case Bit::win64:
        return "64-bit";
      default:
        return "- Unkown OS bit -";
    }
  }

  // Windows enum to string
  // TODO: Move the helper.cc windows list to bottle_types,
  // in order to have a single point of definition of Windows names
  inline static std::string toString(Windows win) {
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

  /**
   * Get Winetricks Windows OS version string
   */
  inline static std::string getWinetricksString(Windows win) {
    switch(win) {
      case Windows::Windows20:
        return "win20"; // Not yet implemented by Winetrick?
      case Windows::Windows30:
        return "win30"; // Not yet implemented by Winetrick?
      case Windows::Windows31:
        return "win31";
      case Windows::WindowsNT351:
        return "nt351"; // Not yet implemented by Winetricks?
      case Windows::WindowsNT40:
        return "nt40";
      case Windows::Windows95:
        return "win95";
      case Windows::Windows98:
        return "win98";
      case Windows::WindowsME:
        return "winme";
      case Windows::Windows2000:
        return "win2k";
      case Windows::WindowsXP:
        return "winxp";
      case Windows::Windows2003:
        return "win2k3";
      case Windows::WindowsVista:
        return "vista";
      case Windows::Windows2008:
        return "win2k8"; // Not yet implemented by Winetricks? Use 2008R2 atm (should be 'win2k8' eventually)
        // TODO: Fix after PR is merged into master: https://github.com/Winetricks/winetricks/pull/1488
      case Windows::Windows7:
        return "win7";
      case Windows::Windows2008R2:
        return "win2k8"; // Bug in Winetricks, should be 'win2k8r2' to be unique with Windows 2000!
        // TODO: Fix after PR is merged into master: https://github.com/Winetricks/winetricks/pull/1488
      case Windows::Windows8:
        return "win8";
      case Windows::Windows81:
        return "win81";
      case Windows::Windows10:
        return "win10";
      default:
        return "winxp";
    }
  }

  // AudioDriver enum to string
  inline static std::string toString(AudioDriver audio) {
    switch(audio) {
      case AudioDriver::pulseaudio:
        return "PulseAudio";
      case AudioDriver::alsa:
        return "Advanced Linux Sound Architecture (ALSA)";
      case AudioDriver::coreaudio:
        return "Mac Core Audio";
      case AudioDriver::oss:
        return "Open Sound System (OSS)";
      default:
        return "Disabled";
    }
  }

  /**
   * \brief Get Winetricks Audio driver string
   */
  inline static std::string getWinetricksString(AudioDriver audio) {
    switch(audio) {
      case AudioDriver::pulseaudio:
        return "pulse";
      case AudioDriver::alsa:
        return "alsa";
      case AudioDriver::coreaudio:
        return "coreaudio";
      case AudioDriver::oss:
        return "oss";
      default:
        return "disabled";
    }
  }
};
