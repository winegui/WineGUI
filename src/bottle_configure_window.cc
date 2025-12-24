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
#include "dll_override_types.h"
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

  configure_grid.set_margin_top(5);
  configure_grid.set_margin_end(5);
  configure_grid.set_margin_bottom(5);
  configure_grid.set_margin_start(5);
  configure_grid.set_column_spacing(6);
  configure_grid.set_row_spacing(8);

  set_child(configure_grid);

  create_layout();

  // Signals
  // Hide window instead of destroy
  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        return true; // stop default destroy
      },
      false);
}

/**
 * \brief Destructor
 */
BottleConfigureWindow::~BottleConfigureWindow()
{
}

void BottleConfigureWindow::create_layout()
{
  first_toolbar.set_orientation(Gtk::Orientation::HORIZONTAL);
  first_toolbar.set_halign(Gtk::Align::CENTER);
  first_toolbar.set_valign(Gtk::Align::CENTER);
  first_toolbar.set_expand();
  first_toolbar.set_spacing(10);

  second_toolbar.set_orientation(Gtk::Orientation::HORIZONTAL);
  second_toolbar.set_halign(Gtk::Align::CENTER);
  second_toolbar.set_valign(Gtk::Align::CENTER);
  second_toolbar.set_expand();
  second_toolbar.set_spacing(10);

  third_toolbar.set_orientation(Gtk::Orientation::HORIZONTAL);
  third_toolbar.set_halign(Gtk::Align::CENTER);
  third_toolbar.set_valign(Gtk::Align::CENTER);
  third_toolbar.set_expand();
  third_toolbar.set_spacing(10);

  fourth_toolbar.set_orientation(Gtk::Orientation::HORIZONTAL);
  fourth_toolbar.set_halign(Gtk::Align::CENTER);
  fourth_toolbar.set_valign(Gtk::Align::CENTER);
  fourth_toolbar.set_expand();
  fourth_toolbar.set_spacing(10);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_label;
  attr_list_label.insert(font_label);

  hint_label.set_markup("<big><b>Tip:</b> Hover the mouse over the buttons for more info.</big>");
  hint_label.set_margin_top(8);
  hint_label.set_margin_bottom(4);

  first_row_label.set_text("Graphics packages");
  first_row_label.set_attributes(attr_list_label);
  first_row_label.set_halign(Gtk::Align::CENTER);
  second_row_label.set_text("Font packages");
  second_row_label.set_attributes(attr_list_label);
  second_row_label.set_halign(Gtk::Align::CENTER);
  third_row_label.set_text("Visual C++ packages");
  third_row_label.set_attributes(attr_list_label);
  third_row_label.set_halign(Gtk::Align::CENTER);
  fourth_row_label.set_text(".NET packages");
  fourth_row_label.set_attributes(attr_list_label);
  fourth_row_label.set_halign(Gtk::Align::CENTER);

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

  static const int button_height = 85;
  // First row buttons, graphics packages
  install_d3dx9_button.signal_clicked().connect(sigc::bind(directx9, this, ""));
  install_d3dx9_button.set_size_request(-1, button_height);
  install_d3dx9_button.set_tooltip_text("Installs MS D3DX9: Ideal for DirectX 9 games, by using OpenGL API");
  first_toolbar.append(install_d3dx9_button);
  install_dxvk_button.signal_clicked().connect(sigc::bind(dxvk, this, "latest"));
  install_dxvk_button.set_size_request(-1, button_height);
  install_dxvk_button.set_tooltip_text("Installs DXVK: Ideal for DirectX 9, 10 or 11 games, by using Vulkan API");
  first_toolbar.append(install_dxvk_button);
  install_vkd3d_button.signal_clicked().connect(sigc::bind(vkd3d, this));
  install_vkd3d_button.set_size_request(-1, button_height);
  install_vkd3d_button.set_tooltip_text("Installs VKD3D-Proton: Ideal for DirectX 12 games, by using Vulkan API");
  first_toolbar.append(install_vkd3d_button);

  // // Second row, Font packages
  install_liberation_fonts_button.signal_clicked().connect(sigc::bind(liberation_fonts, this));
  install_liberation_fonts_button.set_size_request(-1, button_height);
  install_liberation_fonts_button.set_tooltip_text("Installs Liberation open-source Fonts, alternative for Core fonts");
  second_toolbar.append(install_liberation_fonts_button);
  install_core_fonts_button.signal_clicked().connect(sigc::bind(corefonts, this));
  install_core_fonts_button.set_size_request(-1, button_height);
  install_core_fonts_button.set_tooltip_text("Installs Microsoft Core Fonts");
  second_toolbar.append(install_core_fonts_button);

  // // Third row, Visual C++ packages
  install_visual_cpp_2013_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2013"));
  install_visual_cpp_2013_button.set_size_request(-1, button_height);
  install_visual_cpp_2013_button.set_tooltip_text("Installs Visual C++ 2013");
  third_toolbar.append(install_visual_cpp_2013_button);
  install_visual_cpp_2015_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2015"));
  install_visual_cpp_2015_button.set_size_request(-1, button_height);
  install_visual_cpp_2015_button.set_tooltip_text("Installs Visual C++ 2015");
  third_toolbar.append(install_visual_cpp_2015_button);
  install_visual_cpp_2017_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2017"));
  install_visual_cpp_2017_button.set_size_request(-1, button_height);
  install_visual_cpp_2017_button.set_tooltip_text("Installs Visual C++ 2017");
  third_toolbar.append(install_visual_cpp_2017_button);
  install_visual_cpp_2019_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2019"));
  install_visual_cpp_2019_button.set_size_request(-1, button_height);
  install_visual_cpp_2019_button.set_tooltip_text("Installs Visual C++ 2015-2019");
  third_toolbar.append(install_visual_cpp_2019_button);
  install_visual_cpp_2022_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2022"));
  install_visual_cpp_2022_button.set_size_request(-1, button_height);
  install_visual_cpp_2022_button.set_tooltip_text("Installs Visual C++ 2015-2022");
  third_toolbar.append(install_visual_cpp_2022_button);

  // // Fourth row, .NET packages
  install_dotnet4_0_button.signal_clicked().connect(sigc::bind(dotnet, this, "40"));
  install_dotnet4_0_button.set_size_request(-1, button_height);
  install_dotnet4_0_button.set_tooltip_text("Installs .NET 4.0 from 2011");
  fourth_toolbar.append(install_dotnet4_0_button);
  install_dotnet4_5_2_button.signal_clicked().connect(sigc::bind(dotnet, this, "452"));
  install_dotnet4_5_2_button.set_size_request(-1, button_height);
  install_dotnet4_5_2_button.set_tooltip_text("Installs .NET 4.5.2 from 2012");
  fourth_toolbar.append(install_dotnet4_5_2_button);
  install_dotnet4_7_2_button.signal_clicked().connect(sigc::bind(dotnet, this, "472"));
  install_dotnet4_7_2_button.set_size_request(-1, button_height);
  install_dotnet4_7_2_button.set_tooltip_text("Installs .NET 4.7.2 from 2018");
  fourth_toolbar.append(install_dotnet4_7_2_button);
  install_dotnet4_8_button.signal_clicked().connect(sigc::bind(dotnet, this, "48"));
  install_dotnet4_8_button.set_size_request(-1, button_height);
  install_dotnet4_8_button.set_tooltip_text("Installs .NET 4.8 from 2019");
  fourth_toolbar.append(install_dotnet4_8_button);
  install_dotnet6_button.signal_clicked().connect(sigc::bind(dotnet, this, "6"));
  install_dotnet6_button.set_size_request(-1, button_height);
  install_dotnet6_button.set_tooltip_text("Installs .NET 6.0 from 2023");
  fourth_toolbar.append(install_dotnet6_button);
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
  // Call parent present
  present();
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
    add_name_and_icon_to_button(install_d3dx9_button, "Reinstall DirectX v9 (OpenGL)", true);
  }
  else
  {
    add_name_and_icon_to_button(install_d3dx9_button, "Install DirectX v9 (OpenGL)", false);
  }

  if (is_dxvk_installed())
  {
    add_name_and_icon_to_button(install_dxvk_button, "Reinstall DirectX v9/v10/v11 (Vulkan)", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dxvk_button, "Install DirectX v9/v10/v11 (Vulkan)", false);
  }

  if (is_vkd3d_installed())
  {
    add_name_and_icon_to_button(install_vkd3d_button, "Reinstall DirectX v12 (Vulkan)", true);
  }
  else
  {
    add_name_and_icon_to_button(install_vkd3d_button, "Install DirectX v12 (Vulkan)", false);
  }

  if (is_liberation_installed())
  {
    add_name_and_icon_to_button(install_liberation_fonts_button, "Reinstall Liberation fonts", true);
  }
  else
  {
    add_name_and_icon_to_button(install_liberation_fonts_button, "Install Liberation fonts", false);
  }

  if (is_core_fonts_installed())
  {
    add_name_and_icon_to_button(install_core_fonts_button, "Reinstall Core Fonts", true);
  }
  else
  {
    add_name_and_icon_to_button(install_core_fonts_button, "Install Core Fonts", false);
  }

  // Check for Visual C++ 2013
  if (is_visual_cpp_2013_installed())
  {
    add_name_and_icon_to_button(install_visual_cpp_2013_button, "Reinstall Visual C++ 2013", true);
  }
  else
  {
    add_name_and_icon_to_button(install_visual_cpp_2013_button, "Install Visual C++ 2013", false);
  }

  // Check for Visual C++ 2015
  if (is_visual_cpp_2015_installed())
  {
    add_name_and_icon_to_button(install_visual_cpp_2015_button, "Reinstall Visual C++ 2015", true);
  }
  else
  {
    add_name_and_icon_to_button(install_visual_cpp_2015_button, "Install Visual C++ 2015", false);
  }

  // Check for Visual C++ 2017
  if (is_visual_cpp_2017_installed())
  {
    add_name_and_icon_to_button(install_visual_cpp_2017_button, "Reinstall Visual C++ 2017", true);
  }
  else
  {
    add_name_and_icon_to_button(install_visual_cpp_2017_button, "Install Visual C++ 2017", false);
  }

  // Check for Visual C++ 2019
  if (is_visual_cpp_2019_installed())
  {
    add_name_and_icon_to_button(install_visual_cpp_2019_button, "Reinstall Visual C++ 2019", true);
  }
  else
  {
    add_name_and_icon_to_button(install_visual_cpp_2019_button, "Install Visual C++ 2019", false);
  }

  // Check for Visual C++ 2022
  if (is_visual_cpp_2022_installed())
  {
    add_name_and_icon_to_button(install_visual_cpp_2022_button, "Reinstall Visual C++ 2022", true);
  }
  else
  {
    add_name_and_icon_to_button(install_visual_cpp_2022_button, "Install Visual C++ 2022", false);
  }

  // Check for .NET 4.0
  if (is_dotnet_installed("Microsoft .NET Framework 4 Extended", "Microsoft .NET Framework 4 Extended"))
  {
    add_name_and_icon_to_button(install_dotnet4_0_button, "Reinstall .NET v4", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet4_0_button, "Install .NET v4", false);
  }

  // Check for .NET 4.5.2
  if (is_dotnet_installed("{92FB6C44-E685-45AD-9B20-CADF4CABA132}", "Microsoft .NET Framework 4.5.2"))
  {
    add_name_and_icon_to_button(install_dotnet4_5_2_button, "Reinstall .NET v4.5.2", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet4_5_2_button, "Install .NET v4.5.2", false);
  }

  // Check for .NET 4.7.2
  if (is_dotnet_installed("{92FB6C44-E685-45AD-9B20-CADF4CABA132} - 1033", "Microsoft .NET Framework 4.7.2"))
  {
    add_name_and_icon_to_button(install_dotnet4_7_2_button, "Reinstall .NET v4.7.2", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet4_7_2_button, "Install .NET v4.7.2", false);
  }

  // Check for .NET 4.8
  if (is_dotnet_installed("{92FB6C44-E685-45AD-9B20-CADF4CABA132} - 1033", "Microsoft .NET Framework 4.8"))
  {
    add_name_and_icon_to_button(install_dotnet4_8_button, "Reinstall .NET v4.8", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet4_8_button, "Install .NET v4.8", false);
  }

  // Check for .NET 6.0 LTS
  if (is_dotnet_6_installed())
  {
    add_name_and_icon_to_button(install_dotnet6_button, "Reinstall .NET v6.0 LTS", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet6_button, "Install .NET v6.0 LTS", false);
  }
}

/**
 * \brief Add name and icon to a button
 * \param[in,out] button Reference to GTK Button
 * \param[in] label Label text
 * \param[in] is_installed True if package is installed otherwise False
 */
void BottleConfigureWindow::add_name_and_icon_to_button(Gtk::Button& button, const std::string& label, bool is_installed)
{
  Gtk::Image* button_imag = Gtk::manage(new Gtk::Image());
  button_imag->set_from_icon_name(is_installed ? "view-refresh" : "system-software-install");
  button_imag->set_icon_size(Gtk::IconSize::LARGE);
  Gtk::Label* button_label = Gtk::manage(new Gtk::Label(label));
  Gtk::Box* button_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  button_box->set_valign(Gtk::Align::CENTER);
  button_box->set_vexpand(true);
  button_box->append(*button_imag);
  button_box->append(*button_label);
  button.set_child(*button_box);
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
    try
    {
      Glib::ustring wine_prefix = active_bottle_->wine_location();
      BottleTypes::Bit bit = active_bottle_->bit();
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
    try
    {
      Glib::ustring wine_prefix = active_bottle_->wine_location();
      BottleTypes::Bit bit = active_bottle_->bit();
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