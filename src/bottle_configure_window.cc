/**
 * Copyright (c) 2020-2023 WineGUI
 *
 * \file    bottle_configure_window.cc
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
  set_default_size(900, 350);
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
  second_toolbar.set_halgn(Gtk::ALIGN_CENTER);
  second_toolbar.set_valign(Gtk::ALIGN_CENTER);
  second_toolbar.set_hexpand(true);
  second_toolbar.set_vexpand(true);

  // third_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  // third_toolbar.set_halign(Gtk::ALIGN_CENTER);
  // third_toolbar.set_valign(Gtk::ALIGN_CENTER);
  // third_toolbar.set_hexpand(true);
  // third_toolbar.set_vexpand(true);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_label;
  attr_list_label.insert(font_label);

  first_row_label.set_text("Gaming packages");
  first_row_label.set_attributes(attr_list_label);
  first_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  hint_label.set_markup("<b>Hint:</b> Hover the mouse over the buttons for more info...");
  hint_label.set_margin_top(8);
  hint_label.set_margin_bottom(4);
  second_row_label.set_text("Additional packages");
  second_row_label.set_attributes(attr_list_label);
  second_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  // third_row_label.set_text("Other tools..");
  // third_row_label.set_attributes(attr_list_label);
  // third_row_label.set_halign(Gtk::Align::ALIGN_CENTER);

  configure_grid.attach(first_row_label, 0, 0);
  configure_grid.attach(first_toolbar, 0, 1);
  configure_grid.attach(hint_label, 0, 2);
  configure_grid.attach(second_row_label, 0, 3);
  configure_grid.attach(second_toolbar, 0, 4);

  // TODO: Inform the user to disable desktop effects of the compositor. And set CPU to performance.

  // First row buttons, 1-button installs
  install_d3dx9_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(directx9, *this, ""));
  install_d3dx9_button.set_tooltip_text("Installs MS D3DX9: Ideal for DirectX 9 games, by using OpenGL API");
  first_toolbar.insert(install_d3dx9_button, 0);

  install_dxvk_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(vulkan, *this, "latest"));
  install_dxvk_button.set_tooltip_text("Installs DXVK: Ideal for DirectX 9/10/11 games, by using Vulkan API");
  first_toolbar.insert(install_dxvk_button, 1);

  // Second row, additional packages
  install_liberation_fonts_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(liberation_fonts, *this));
  install_liberation_fonts_button.set_tooltip_text("Installs Liberation open-source Fonts, alternative for Core fonts");
  second_toolbar.insert(install_liberation_fonts_button, 0);

  install_core_fonts_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(corefonts, *this));
  install_core_fonts_button.set_tooltip_text("Installs Microsoft Core Fonts");
  second_toolbar.insert(install_core_fonts_button, 1);

  install_visual_cpp_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2013"));
  install_visual_cpp_button.set_tooltip_text("Installs Visual C++ 2013 package");
  second_toolbar.insert(install_visual_cpp_button, 2);

  install_dotnet4_0_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "40"));
  install_dotnet4_0_button.set_tooltip_text("Installs .NET 4.0");
  second_toolbar.insert(install_dotnet4_0_button, 4);

  install_dotnet4_5_2_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "452"));
  install_dotnet4_5_2_button.set_tooltip_text("Installs .NET 4.0 and .NET 4.5.2");
  second_toolbar.insert(install_dotnet4_5_2_button, 5);

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
    Gtk::Image* reinstall_d3dx9_image = Gtk::manage(new Gtk::Image());
    reinstall_d3dx9_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dxvk_button.set_label("Reinstall DirectX v9/v10/v11 (Vulkan)");
    install_dxvk_button.set_icon_widget(*reinstall_d3dx9_image);
  }
  else
  {
    Gtk::Image* install_d3dx9_image = Gtk::manage(new Gtk::Image());
    install_d3dx9_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dxvk_button.set_label("Install DirectX v9/v10/v11 (Vulkan)");
    install_dxvk_button.set_icon_widget(*install_d3dx9_image);
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
    install_liberation_fonts_button.set_label("Install Liberation open-source fonts");
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

  if (is_visual_cpp_installed())
  {
    Gtk::Image* reinstall_visual_cpp_image = Gtk::manage(new Gtk::Image());
    reinstall_visual_cpp_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_button.set_label("Reinstall Visual C++ 2013");
    install_visual_cpp_button.set_icon_widget(*reinstall_visual_cpp_image);
  }
  else
  {
    Gtk::Image* install_visual_cpp_image = Gtk::manage(new Gtk::Image());
    install_visual_cpp_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_visual_cpp_button.set_label("Install Visual C++ 2013");
    install_visual_cpp_button.set_icon_widget(*install_visual_cpp_image);
  }

  if (is_dotnet_4_0_installed())
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

  if (is_dotnet_4_5_2_installed())
  {
    Gtk::Image* reinstall_dotnet452_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet452_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_5_2_button.set_label("Reinstall .NET v4.5.2");
    install_dotnet4_5_2_button.set_icon_widget(*reinstall_dotnet452_image);
  }
  else
  {
    Gtk::Image* install_dotnet452_image = Gtk::manage(new Gtk::Image());
    install_dotnet452_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_5_2_button.set_label("Install .NET v4.5.2");
    install_dotnet4_5_2_button.set_icon_widget(*install_dotnet452_image);
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
    // Check if set to 'native' load order
    try
    {
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
    // Check if set to 'native' load order
    try
    {
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
 * \brief Check if MS Visual C++ installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native, builtin' load order
    try
    {
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*msvcp120", DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{61087a79-ac85-455c-934d-1fa22cc64f36}");
        // String starts with
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
 * \brief Check if MS .NET v4.0 is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_dotnet_4_0_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native' load order
    try
    {
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*mscoree");
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "Microsoft .NET Framework 4 Extended");
        is_installed = (name == "Microsoft .NET Framework 4 Extended");
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
 * \brief Check ife MS .NET v4.5.2 is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_dotnet_4_5_2_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native' load order
    try
    {
      bool is_dll_override = Helper::get_dll_override(wine_prefix, "*mscoree");
      if (is_dll_override)
      {
        // Next, check if package can be found to be uninstalled
        string name = Helper::get_uninstaller(wine_prefix, "{92FB6C44-E685-45AD-9B20-CADF4CABA132}");
        is_installed = (name == "Microsoft .NET Framework 4.5.2");
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
