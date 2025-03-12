/**
 * Copyright (c) 2020-2025 WineGUI
 *
 * \file    bottle_configure_window.cc
 * \brief   Wine bottle configure window
 * \author  Melroy van den Berg <melroy@melroy.org>
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
#include "bottle_configure_window.h"
#include "bottle_item.h"
#include "helper.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleConfigureWindow::BottleConfigureWindow(Gtk::Window& parent) : active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_default_size(1100, 500);
  set_modal(true);

  add(configure_grid);
  configure_grid.set_margin_top(5);
  configure_grid.set_margin_end(5);
  configure_grid.set_margin_bottom(6);
  configure_grid.set_margin_start(6);
  configure_grid.set_column_spacing(6);
  configure_grid.set_row_spacing(8);

  first_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  first_toolbar.set_halign(Gtk::ALIGN_CENTER);
  first_toolbar.set_valign(Gtk::ALIGN_CENTER);
  first_toolbar.set_hexpand(true);
  first_toolbar.set_vexpand(true);
  second_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  second_toolbar.set_halign(Gtk::ALIGN_CENTER);
  second_toolbar.set_valign(Gtk::ALIGN_CENTER);
  second_toolbar.set_hexpand(true);
  second_toolbar.set_vexpand(true);
  third_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  third_toolbar.set_halign(Gtk::ALIGN_CENTER);
  third_toolbar.set_valign(Gtk::ALIGN_CENTER);
  third_toolbar.set_hexpand(true);
  third_toolbar.set_vexpand(true);
  fourth_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  fourth_toolbar.set_halign(Gtk::ALIGN_CENTER);
  fourth_toolbar.set_valign(Gtk::ALIGN_CENTER);
  fourth_toolbar.set_hexpand(true);
  fourth_toolbar.set_vexpand(true);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_label;
  attr_list_label.insert(font_label);

  hint_label.set_markup("<big><b>Tip:</b> Hover the mouse over the buttons for more info.</big>");
  hint_label.set_margin_top(8);
  hint_label.set_margin_bottom(4);

  first_row_label.set_text("Graphics packages");
  first_row_label.set_attributes(attr_list_label);
  first_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  second_row_label.set_text("Font packages");
  second_row_label.set_attributes(attr_list_label);
  second_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  third_row_label.set_text("Visual C++ packages");
  third_row_label.set_attributes(attr_list_label);
  third_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  fourth_row_label.set_text(".NET packages");
  fourth_row_label.set_attributes(attr_list_label);
  fourth_row_label.set_halign(Gtk::Align::ALIGN_CENTER);

  configure_grid.attach(hint_label, 0, 0, 2, 1);
  configure_grid.attach(first_row_label, 0, 1);
  configure_grid.attach(first_toolbar, 0, 2);
  configure_grid.attach(second_row_label, 1, 1);
  configure_grid.attach(second_toolbar, 1, 2);
  configure_grid.attach(third_row_label, 0, 3, 2, 1);
  configure_grid.attach(third_toolbar, 0, 4, 2, 1);
  configure_grid.attach(fourth_row_label, 0, 5, 2, 1);
  configure_grid.attach(fourth_toolbar, 0, 6, 2, 1);

  // TODO: Inform the user to disable desktop effects of the compositor. And set CPU to performance.

  // First row buttons, graphics packages
  install_d3dx9_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(directx9, *this, ""));
  install_d3dx9_button.set_tooltip_text("Installs MS D3DX9: Ideal for DirectX 9 games, by using OpenGL API");
  first_toolbar.insert(install_d3dx9_button, 0);
  install_dxvk_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dxvk, *this, "latest"));
  install_dxvk_button.set_tooltip_text("Installs DXVK: Ideal for DirectX 9, 10 or 11 games, by using Vulkan API");
  first_toolbar.insert(install_dxvk_button, 1);
  install_vkd3d_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(vkd3d, *this));
  install_vkd3d_button.set_tooltip_text("Installs VKD3D-Proton: Ideal for DirectX 12 games, by using Vulkan API");
  first_toolbar.insert(install_vkd3d_button, 2);

  // Second row, Font packages
  install_liberation_fonts_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(liberation_fonts, *this));
  install_liberation_fonts_button.set_tooltip_text("Installs Liberation open-source Fonts, alternative for Core fonts");
  second_toolbar.insert(install_liberation_fonts_button, 0);
  install_core_fonts_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(corefonts, *this));
  install_core_fonts_button.set_tooltip_text("Installs Microsoft Core Fonts");
  second_toolbar.insert(install_core_fonts_button, 1);

  // Third row, Visual C++ packages
  install_visual_cpp_2013_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2013"));
  install_visual_cpp_2013_button.set_tooltip_text("Installs Visual C++ 2013");
  third_toolbar.insert(install_visual_cpp_2013_button, 0);
  install_visual_cpp_2015_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2015"));
  install_visual_cpp_2015_button.set_tooltip_text("Installs Visual C++ 2015");
  third_toolbar.insert(install_visual_cpp_2015_button, 1);
  install_visual_cpp_2017_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2017"));
  install_visual_cpp_2017_button.set_tooltip_text("Installs Visual C++ 2017");
  third_toolbar.insert(install_visual_cpp_2017_button, 2);
  install_visual_cpp_2019_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2019"));
  install_visual_cpp_2019_button.set_tooltip_text("Installs Visual C++ 2015-2019");
  third_toolbar.insert(install_visual_cpp_2019_button, 3);
  install_visual_cpp_2022_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2022"));
  install_visual_cpp_2022_button.set_tooltip_text("Installs Visual C++ 2015-2022");
  third_toolbar.insert(install_visual_cpp_2022_button, 4);

  // Fourth row, .NET packages
  install_dotnet4_0_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "40"));
  install_dotnet4_0_button.set_tooltip_text("Installs .NET 4.0 from 2011");
  fourth_toolbar.insert(install_dotnet4_0_button, 0);
  install_dotnet4_5_2_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "452"));
  install_dotnet4_5_2_button.set_tooltip_text("Installs .NET 4.5.2 from 2012");
  fourth_toolbar.insert(install_dotnet4_5_2_button, 1);
  install_dotnet4_7_2_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "472"));
  install_dotnet4_7_2_button.set_tooltip_text("Installs .NET 4.7.2 from 2018");
  fourth_toolbar.insert(install_dotnet4_7_2_button, 2);
  install_dotnet4_8_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "48"));
  install_dotnet4_8_button.set_tooltip_text("Installs .NET 4.8 from 2019");
  fourth_toolbar.insert(install_dotnet4_8_button, 3);
  install_dotnet6_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "6"));
  install_dotnet6_button.set_tooltip_text("Installs .NET 6.0 from 2023");
  fourth_toolbar.insert(install_dotnet6_button, 4);

  show_all_children();
}

/**
 * \brief Destructor
 */
BottleConfigureWindow::~BottleConfigureWindow()
{
}

/**
 * \brief Same as show() but will also update the Window title
 */
void BottleConfigureWindow::show()
{
  this->update_installed();

  if (active_bottle_ != nullptr)
    set_title("Configure machine - " +
              ((!active_bottle_->name().empty()) ? active_bottle_->name() : active_bottle_->folder_name())); // Fall-back to folder name
  else
    set_title("Configure machine (Unknown machine)");
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle Current active bottle
 */
void BottleConfigureWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleConfigureWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Update GUI state depending on the packages installed
 */
void BottleConfigureWindow::update_installed()
{
  if (is_d3dx9_installed())
  {
    Gtk::Image* reinstall_d3dx9_image = Gtk::manage(new Gtk::Image());
    reinstall_d3dx9_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_d3dx9_button.set_label("Reinstall DirectX v9 (OpenGL)");
    install_d3dx9_button.set_icon_widget(*reinstall_d3dx9_image);
  }
  else
  {
    Gtk::Image* install_d3dx9_image = Gtk::manage(new Gtk::Image());
    install_d3dx9_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_d3dx9_button.set_label("Install DirectX v9 (OpenGL)");
    install_d3dx9_button.set_icon_widget(*install_d3dx9_image);
  }

  if (is_dxvk_installed())
  {
    Gtk::Image* reinstall_dxvk_image = Gtk::manage(new Gtk::Image());
    reinstall_dxvk_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dxvk_button.set_label("Reinstall DirectX v9/v10/v11 (Vulkan)");
    install_dxvk_button.set_icon_widget(*reinstall_dxvk_image);
  }
  else
  {
    Gtk::Image* install_dxvk_image = Gtk::manage(new Gtk::Image());
    install_dxvk_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dxvk_button.set_label("Install DirectX v9/v10/v11 (Vulkan)");
    install_dxvk_button.set_icon_widget(*install_dxvk_image);
  }

  if (is_vkd3d_installed())
  {
    Gtk::Image* reinstall_vkd3d_image = Gtk::manage(new Gtk::Image());
    reinstall_vkd3d_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_vkd3d_button.set_label("Reinstall DirectX v12 (Vulkan)");
    install_vkd3d_button.set_icon_widget(*reinstall_vkd3d_image);
  }
  else
  {
    Gtk::Image* install_vkd3d_image = Gtk::manage(new Gtk::Image());
    install_vkd3d_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_vkd3d_button.set_label("Install DirectX v12 (Vulkan)");
    install_vkd3d_button.set_icon_widget(*install_vkd3d_image);
  }

  if (is_liberation_installed())
  {
    Gtk::Image* reinstall_liberation_image = Gtk::manage(new Gtk::Image());
    reinstall_liberation_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_liberation_fonts_button.set_label("Reinstall Liberation fonts");
    install_liberation_fonts_button.set_icon_widget(*reinstall_liberation_image);
  }
  else
  {
    Gtk::Image* install_liberation_image = Gtk::manage(new Gtk::Image());
    install_liberation_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_liberation_fonts_button.set_label("Install Liberation fonts");
    install_liberation_fonts_button.set_icon_widget(*install_liberation_image);
  }

  if (is_core_fonts_installed())
  {
    Gtk::Image* reinstall_core_fonts_image = Gtk::manage(new Gtk::Image());
    reinstall_core_fonts_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_core_fonts_button.set_label("Reinstall Core Fonts");
    install_core_fonts_button.set_icon_widget(*reinstall_core_fonts_image);
  }
  else
  {
    Gtk::Image* install_core_fonts_image = Gtk::manage(new Gtk::Image());
    install_core_fonts_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_core_fonts_button.set_label("Install Core Fonts");
    install_core_fonts_button.set_icon_widget(*install_core_fonts_image);
  }

  // Check for Visual C++ 2013
  if (is_visual_cpp_2013_installed())
  {
    Gtk::Image* reinstall_visual_cpp_2013_image = Gtk::manage(new Gtk::Image());
    reinstall_visual_cpp_2013_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2013_button.set_label("Reinstall Visual C++ 2013");
    install_visual_cpp_2013_button.set_icon_widget(*reinstall_visual_cpp_2013_image);
  }
  else
  {
    Gtk::Image* install_visual_cpp_2013_image = Gtk::manage(new Gtk::Image());
    install_visual_cpp_2013_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2013_button.set_label("Install Visual C++ 2013");
    install_visual_cpp_2013_button.set_icon_widget(*install_visual_cpp_2013_image);
  }

  // Check for Visual C++ 2015
  if (is_visual_cpp_2015_installed())
  {
    Gtk::Image* reinstall_visual_cpp_2015_image = Gtk::manage(new Gtk::Image());
    reinstall_visual_cpp_2015_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2015_button.set_label("Reinstall Visual C++ 2015");
    install_visual_cpp_2015_button.set_icon_widget(*reinstall_visual_cpp_2015_image);
  }
  else
  {
    Gtk::Image* install_visual_cpp_2015_image = Gtk::manage(new Gtk::Image());
    install_visual_cpp_2015_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2015_button.set_label("Install Visual C++ 2015");
    install_visual_cpp_2015_button.set_icon_widget(*install_visual_cpp_2015_image);
  }

  // Check for Visual C++ 2017
  if (is_visual_cpp_2017_installed())
  {
    Gtk::Image* reinstall_visual_cpp_2017_image = Gtk::manage(new Gtk::Image());
    reinstall_visual_cpp_2017_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2017_button.set_label("Reinstall Visual C++ 2017");
    install_visual_cpp_2017_button.set_icon_widget(*reinstall_visual_cpp_2017_image);
  }
  else
  {
    Gtk::Image* install_visual_cpp_2017_image = Gtk::manage(new Gtk::Image());
    install_visual_cpp_2017_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2017_button.set_label("Install Visual C++ 2017");
    install_visual_cpp_2017_button.set_icon_widget(*install_visual_cpp_2017_image);
  }

  // Check for Visual C++ 2019
  if (is_visual_cpp_2019_installed())
  {
    Gtk::Image* reinstall_visual_cpp_2019_image = Gtk::manage(new Gtk::Image());
    reinstall_visual_cpp_2019_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2019_button.set_label("Reinstall Visual C++ 2019");
    install_visual_cpp_2019_button.set_icon_widget(*reinstall_visual_cpp_2019_image);
  }
  else
  {
    Gtk::Image* install_visual_cpp_2019_image = Gtk::manage(new Gtk::Image());
    install_visual_cpp_2019_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2019_button.set_label("Install Visual C++ 2019");
    install_visual_cpp_2019_button.set_icon_widget(*install_visual_cpp_2019_image);
  }

  // Check for Visual C++ 2022
  if (is_visual_cpp_2022_installed())
  {
    Gtk::Image* reinstall_visual_cpp_2022_image = Gtk::manage(new Gtk::Image());
    reinstall_visual_cpp_2022_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2022_button.set_label("Reinstall Visual C++ 2022");
    install_visual_cpp_2022_button.set_icon_widget(*reinstall_visual_cpp_2022_image);
  }
  else
  {
    Gtk::Image* install_visual_cpp_2022_image = Gtk::manage(new Gtk::Image());
    install_visual_cpp_2022_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_2022_button.set_label("Install Visual C++ 2022");
    install_visual_cpp_2022_button.set_icon_widget(*install_visual_cpp_2022_image);
  }

  // Check for .NET 4.0
  if (is_dotnet_installed("Microsoft .NET Framework 4 Extended", "Microsoft .NET Framework 4 Extended"))
  {
    Gtk::Image* reinstall_dotnet4_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet4_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_0_button.set_label("Reinstall .NET v4");
    install_dotnet4_0_button.set_icon_widget(*reinstall_dotnet4_image);
  }
  else
  {
    Gtk::Image* install_dotnet4_image = Gtk::manage(new Gtk::Image());
    install_dotnet4_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_0_button.set_label("Install .NET v4");
    install_dotnet4_0_button.set_icon_widget(*install_dotnet4_image);
  }

  // Check for .NET 4.5.2
  if (is_dotnet_installed("{92FB6C44-E685-45AD-9B20-CADF4CABA132}", "Microsoft .NET Framework 4.5.2"))
  {
    Gtk::Image* reinstall_dotnet4_5_2_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet4_5_2_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_5_2_button.set_label("Reinstall .NET v4.5.2");
    install_dotnet4_5_2_button.set_icon_widget(*reinstall_dotnet4_5_2_image);
  }
  else
  {
    Gtk::Image* install_dotnet4_5_2_image = Gtk::manage(new Gtk::Image());
    install_dotnet4_5_2_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_5_2_button.set_label("Install .NET v4.5.2");
    install_dotnet4_5_2_button.set_icon_widget(*install_dotnet4_5_2_image);
  }

  // Check for .NET 4.7.2
  if (is_dotnet_installed("{92FB6C44-E685-45AD-9B20-CADF4CABA132} - 1033", "Microsoft .NET Framework 4.7.2"))
  {
    Gtk::Image* reinstall_dotnet4_7_2_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet4_7_2_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_7_2_button.set_label("Reinstall .NET v4.7.2");
    install_dotnet4_7_2_button.set_icon_widget(*reinstall_dotnet4_7_2_image);
  }
  else
  {
    Gtk::Image* install_dotnet4_7_2_image = Gtk::manage(new Gtk::Image());
    install_dotnet4_7_2_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_7_2_button.set_label("Install .NET v4.7.2");
    install_dotnet4_7_2_button.set_icon_widget(*install_dotnet4_7_2_image);
  }

  // Check for .NET 4.8
  if (is_dotnet_installed("{92FB6C44-E685-45AD-9B20-CADF4CABA132} - 1033", "Microsoft .NET Framework 4.8"))
  {
    Gtk::Image* reinstall_dotnet4_8_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet4_8_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_8_button.set_label("Reinstall .NET v4.8");
    install_dotnet4_8_button.set_icon_widget(*reinstall_dotnet4_8_image);
  }
  else
  {
    Gtk::Image* install_dotnet4_8_image = Gtk::manage(new Gtk::Image());
    install_dotnet4_8_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_8_button.set_label("Install .NET v4.8");
    install_dotnet4_8_button.set_icon_widget(*install_dotnet4_8_image);
  }

  // Check for .NET 6.0 LTS
  if (is_dotnet_6_installed())
  {
    Gtk::Image* reinstall_dotnet6_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet6_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet6_button.set_label("Reinstall .NET v6.0 LTS");
    install_dotnet6_button.set_icon_widget(*reinstall_dotnet6_image);
  }
  else
  {
    Gtk::Image* install_dotnet6_image = Gtk::manage(new Gtk::Image());
    install_dotnet6_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet6_button.set_label("Install .NET v6.0 LTS");
    install_dotnet6_button.set_icon_widget(*install_dotnet6_image);
  }

  show_all_children();
}

/**
 * \brief Check is D3DX9 (DirectX 9 OpenGL) is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_d3dx9_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native' load order
      is_installed = Helper::get_dll_override(wine_prefix, "*d3dx9_43");
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check is DXVK (Vulkan based DirectX 9/10/11) is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_dxvk_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native' load order
      is_installed = Helper::get_dll_override(wine_prefix, "*dxgi");
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check is VKD3D (Vulkan based DirectX 12) is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_vkd3d_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native' load order
      is_installed = Helper::get_dll_override(wine_prefix, "*d3d12");
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if Liberation fonts are installed
 * As fallback: Wine is looking for the liberation font on the local unix system (in the /usr/share/fonts/.. directory)
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_liberation_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    BottleTypes::Bit bit = active_bottle_->bit();
    try
    {
      string fontFilename = Helper::get_font_filename(wine_prefix, bit, "Liberation Mono (TrueType)");
      is_installed = (fontFilename == "liberationmono-regular.ttf");
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS Core fonts are installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_core_fonts_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    BottleTypes::Bit bit = active_bottle_->bit();
    try
    {
      string fontFilename = Helper::get_font_filename(wine_prefix, bit, "Comic Sans MS (TrueType)");
      is_installed = (fontFilename == "comic.ttf");
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS Visual C++ 2013 installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_2013_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native, builtin' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*msvcp120", DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{61087a79-ac85-455c-934d-1fa22cc64f36}");
        // Strings has last occurrence
        is_installed = (name.rfind("Microsoft Visual C++ 2013 Redistributable") == 0);

        // Try the 64-bit package (fallback)
        if (!is_installed)
        {
          name = Helper::get_uninstaller(wine_prefix, "{ef6b00ec-13e1-4c25-9064-b2f383cb8412}");
          is_installed = (name.rfind("Microsoft Visual C++ 2013 Redistributable") == 0);
        }
      }
      else
      {
        is_installed = false;
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS Visual C++ 2015 installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_2015_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native, builtin' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*msvcp140", DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{462f63a8-6347-4894-a1b3-dbfe3a4c981d}");
        // Strings has last occurrence
        is_installed = (name.rfind("Microsoft Visual C++ 2015 Redistributable") == 0);

        // Try the 64-bit package (fallback)
        if (!is_installed)
        {
          name = Helper::get_uninstaller(wine_prefix, "{F20396E5-D84E-3505-A7A8-7358F0155F6C}");
          is_installed = (name.rfind("Microsoft Visual C++ 2015 Redistributable") == 0);
        }
      }
      else
      {
        is_installed = false;
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS Visual C++ 2017 installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_2017_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native, builtin' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*msvcp140", DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{624ba875-fdfc-4efa-9c66-b170dfebc3ec}");
        // Strings has last occurrence
        is_installed = (name.rfind("Microsoft Visual C++ 2017 Redistributable") == 0);

        // Try the 64-bit package (fallback)
        if (!is_installed)
        {
          name = Helper::get_uninstaller(wine_prefix, "{65835E57-3712-4382-990A-8D39008A8E0B}");
          is_installed = (name.rfind("Microsoft Visual C++ 2017") == 0);
        }
      }
      else
      {
        is_installed = false;
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS Visual C++ 2019 installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_2019_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native, builtin' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*msvcp140", DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{e3aefa8b-a2ea-42b8-a384-95f2ff6df681}");
        // Strings has last occurrence
        is_installed = (name.rfind("Microsoft Visual C++ 2015-2019 Redistributable") == 0);

        // Try the 64-bit package (fallback)
        if (!is_installed)
        {
          name = Helper::get_uninstaller(wine_prefix, "{0F03096E-F81F-48D0-AEE0-9F8513CD883F}");
          is_installed = (name.rfind("Microsoft Visual C++ 2019") == 0);
        }
      }
      else
      {
        is_installed = false;
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS Visual C++ 2022 installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_2022_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native, builtin' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*msvcp140", DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{2cfeba4a-21f8-4ea7-9927-c5a5c6f13cc9}");
        // Strings has last occurrence
        is_installed = (name.rfind("Microsoft Visual C++ 2015-2022 Redistributable") == 0);

        // Try the 64-bit package (fallback)
        if (!is_installed)
        {
          name = Helper::get_uninstaller(wine_prefix, "{1CA7421F-A225-4A9C-B320-A36981A2B789}");
          is_installed = (name.rfind("Microsoft Visual C++ 2022") == 0);
        }
      }
      else
      {
        is_installed = false;
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS .NET is installed using the uninstaller key & display name.
 * Note: Can not be used to check for .NET 6
 * \param[in] uninstaller_key The uninstaller register key
 * \param[in] uninstaller_name The uninstaller display name
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_dotnet_installed(const string& uninstaller_key, const string& uninstaller_name)
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*mscoree");
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, uninstaller_key);
        // Check the display name
        is_installed = (name == uninstaller_name);
      }
      else
      {
        is_installed = false;
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}

/**
 * \brief Check if MS .NET v6 is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_dotnet_6_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if package can be found to be uninstalled
      string name = Helper::get_uninstaller(wine_prefix, "{5DEFBDBE-FF1A-4EB2-8DFB-17A26A7E6442}");
      // Strings has first occurrence of display name
      is_installed = (name.find("Microsoft .NET Runtime - 6") == 0);
      // Try the 64-bit package (fallback)
      if (!is_installed)
      {
        name = Helper::get_uninstaller(wine_prefix, "{3CC763AD-93B3-41EF-ABF8-CFE63A1DC3A6}");
        // Strings has first occurrence of display name
        is_installed = (name.find("Microsoft .NET Runtime - 6") == 0);
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}