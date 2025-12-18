/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    bottle_configure_window.h
 * \brief   Wine bottle configure window
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

using std::string;

// Forward declaration
class BottleItem;

/**
 * \class BottleConfigureWindow
 * \brief Wine Bottle Configure GTK Window class
 */
class BottleConfigureWindow : public Gtk::Window
{
public:
  // Signals
  sigc::signal<void(Gtk::Window*, const string&)> directx9;           /*!< Install d3dx9 for Direct3D 9 signal */
  sigc::signal<void(Gtk::Window*, const string&)> dxvk;               /*!< Install DXVK for Direct3D 9/10/11 using Vulkan signal */
  sigc::signal<void(Gtk::Window*)> vkd3d;                             /*!< Install VKD3D-proton for Direct3D 12 using Vulkan signal */
  sigc::signal<void(Gtk::Window*)> liberation_fonts;                  /*!< Install Liberation fonts signal */
  sigc::signal<void(Gtk::Window*)> corefonts;                         /*!< Install Core fonts signal */
  sigc::signal<void(Gtk::Window*, const string&)> visual_cpp_package; /*!< Install Visual C++ package signal */
  sigc::signal<void(Gtk::Window*, const string&)> dotnet;             /*!< Install .NET signal */

  explicit BottleConfigureWindow(Gtk::Window& parent);
  virtual ~BottleConfigureWindow();

  void show();
  void set_active_bottle(BottleItem* bottle);
  void reset_active_bottle();
  void update_installed();

protected:
  // Child widgets
  Gtk::Grid configure_grid; /*!< The overall grid */

  Gtk::Box first_toolbar;  /*!< 1st row toolbar */
  Gtk::Box second_toolbar; /*!< 2nd row toolbar */
  Gtk::Box third_toolbar;  /*!< 3rd row toolbar */
  Gtk::Box fourth_toolbar; /*!< 4th row toolbar */

  Gtk::Label first_row_label;  /*!< 1st row label */
  Gtk::Label hint_label;       /*!< Extra hint label info for user */
  Gtk::Label second_row_label; /*!< 2nd row label */
  Gtk::Label third_row_label;  /*!< 3rd row label */
  Gtk::Label fourth_row_label; /*!< 4th row label */

  // Buttons First row
  Gtk::Button install_d3dx9_button; /*!< d3dx9 install button */
  Gtk::Button install_dxvk_button;  /*!< DXVK install button */
  Gtk::Button install_vkd3d_button; /*!< DKD3D install button */
  // Buttons Second row
  Gtk::Button install_liberation_fonts_button; /*!< Liberation fonts install button */
  Gtk::Button install_core_fonts_button;       /*!< Core fonts install button */
  // Buttons Third row
  Gtk::Button install_visual_cpp_2013_button; /*!< MS Visual C++ 2013 Redistributable Package install button */
  Gtk::Button install_visual_cpp_2015_button; /*!< MS Visual C++ 2015 Redistributable Package install button */
  Gtk::Button install_visual_cpp_2017_button; /*!< MS Visual C++ 2017 Redistributable Package install button */
  Gtk::Button install_visual_cpp_2019_button; /*!< MS Visual C++ 2019 Redistributable Package install button */
  Gtk::Button install_visual_cpp_2022_button; /*!< MS Visual C++ 2022 Redistributable Package install button */
  // Buttons Fourth row
  Gtk::Button install_dotnet4_0_button;   /*!< .NET v4.0 install button */
  Gtk::Button install_dotnet4_5_2_button; /*!< .NET v4.5.2 install button */
  Gtk::Button install_dotnet4_7_2_button; /*!< .NET v4.7.2 install button */
  Gtk::Button install_dotnet4_8_button;   /*!< .NET v4.8 install button */
  Gtk::Button install_dotnet6_button;     /*!< .NET v6.0 install button */

private:
  BottleItem* active_bottle_; /*!< Current active bottle */

  bool is_d3dx9_installed();
  bool is_dxvk_installed();
  bool is_vkd3d_installed();
  bool is_liberation_installed();
  bool is_core_fonts_installed();
  bool is_visual_cpp_2013_installed();
  bool is_visual_cpp_2015_installed();
  bool is_visual_cpp_2017_installed();
  bool is_visual_cpp_2019_installed();
  bool is_visual_cpp_2022_installed();
  bool is_dotnet_installed(const string& uninstaller_key, const string& uninstaller_name);
  bool is_dotnet_6_installed();
};
