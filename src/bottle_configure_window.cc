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
  set_default_size(1000, 550);
  set_modal(true);

  configure_box.set_orientation(Gtk::Orientation::VERTICAL);
  configure_box.set_margin_top(5);
  configure_box.set_margin_end(5);
  configure_box.set_margin_bottom(5);
  configure_box.set_margin_start(5);
  configure_box.set_spacing(8);

  set_child(configure_box);

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
  hint_label.set_markup("<big><b>Tip:</b> Hover the mouse over the buttons for more info.</big>");
  hint_label.set_margin_top(4);
  hint_label.set_margin_bottom(4);
  hint_label.set_halign(Gtk::Align::CENTER);

  // Sidebar (categories) on the left, the stack with buttons on the right
  sidebar_stack_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  sidebar_stack_box.set_spacing(6);
  sidebar_stack_box.set_expand();
  stack.set_expand();
  stack.set_transition_type(Gtk::StackTransitionType::CROSSFADE);
  sidebar.set_stack(stack);

  // Scroll the buttons instead of clipping them when the window is made small
  stack_scroll.set_child(stack);
  stack_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
  stack_scroll.set_expand();

  // Helper to prepare a per-category FlowBox page
  auto setup_flowbox = [](Gtk::FlowBox& flowbox)
  {
    flowbox.set_orientation(Gtk::Orientation::HORIZONTAL);
    flowbox.set_selection_mode(Gtk::SelectionMode::NONE);
    flowbox.set_homogeneous(true);
    flowbox.set_row_spacing(10);
    flowbox.set_column_spacing(10);
    // Cap at 3 columns so the FlowBox reports a predictable (row-based) height instead of
    // stacking every button in one tall column. Allow it to reflow down to 1 column so the
    // buttons re-wrap (rather than clip) when the window is made narrower.
    flowbox.set_min_children_per_line(1);
    flowbox.set_max_children_per_line(3);
    flowbox.set_halign(Gtk::Align::CENTER);
    flowbox.set_valign(Gtk::Align::START);
    flowbox.set_margin(10);
  };
  setup_flowbox(graphics_flowbox);
  setup_flowbox(fonts_flowbox);
  setup_flowbox(visual_cpp_flowbox);
  setup_flowbox(dotnet_flowbox);

  static const int button_width = 210;
  static const int button_height = 85;
  // Helper to configure a package button and append it to the given category FlowBox
  auto add_button = [](Gtk::FlowBox& flowbox, Gtk::Button& button, const std::string& tooltip)
  {
    button.set_size_request(button_width, button_height);
    button.set_tooltip_text(tooltip);
    flowbox.append(button);
  };

  // Graphics packages
  install_d3dx9_button.signal_clicked().connect(sigc::bind(directx9, this, ""));
  add_button(graphics_flowbox, install_d3dx9_button, "Installs MS D3DX9: Ideal for DirectX 9 games, by using OpenGL API");
  install_gallium_nine_button.signal_clicked().connect(sigc::bind(gallium_nine, this));
  add_button(graphics_flowbox, install_gallium_nine_button,
             "Installs Gallium Nine: Runs DirectX 9 games directly on your Mesa graphics driver, "
             "giving a smoother gaming experience and higher FPS (requires a Mesa driver, eg. AMD or Intel GPUs)");
  install_dxvk_button.signal_clicked().connect(sigc::bind(dxvk, this, "latest"));
  add_button(graphics_flowbox, install_dxvk_button, "Installs DXVK: Ideal for DirectX 9, 10 or 11 games, by using Vulkan API");
  install_vkd3d_button.signal_clicked().connect(sigc::bind(vkd3d, this));
  add_button(graphics_flowbox, install_vkd3d_button, "Installs VKD3D-Proton: Ideal for DirectX 12 games, by using Vulkan API");

  // Font packages
  install_liberation_fonts_button.signal_clicked().connect(sigc::bind(liberation_fonts, this));
  add_button(fonts_flowbox, install_liberation_fonts_button, "Installs Liberation open-source Fonts, alternative for Core fonts");
  install_core_fonts_button.signal_clicked().connect(sigc::bind(corefonts, this));
  add_button(fonts_flowbox, install_core_fonts_button, "Installs Microsoft Core Fonts");

  // Visual C++ packages
  install_visual_cpp_2003_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2003"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2003_button, "Installs Visual C++ 2003 libraries");
  install_visual_cpp_2005_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2005"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2005_button, "Installs Visual C++ 2005");
  install_visual_cpp_2008_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2008"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2008_button, "Installs Visual C++ 2008");
  install_visual_cpp_2010_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2010"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2010_button, "Installs Visual C++ 2010");
  install_visual_cpp_2012_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2012"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2012_button, "Installs Visual C++ 2012");
  install_visual_cpp_2013_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2013"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2013_button, "Installs Visual C++ 2013");
  install_visual_cpp_2015_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2015"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2015_button, "Installs Visual C++ 2015");
  install_visual_cpp_2017_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2017"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2017_button, "Installs Visual C++ 2017");
  install_visual_cpp_2019_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2019"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2019_button, "Installs Visual C++ 2015-2019");
  install_visual_cpp_2022_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2022"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2022_button, "Installs Visual C++ 2015-2022");
  install_visual_cpp_2026_button.signal_clicked().connect(sigc::bind(visual_cpp_package, this, "2026"));
  add_button(visual_cpp_flowbox, install_visual_cpp_2026_button, "Installs Visual C++ 2017-2026");

  // Wine Mono & .NET packages
  install_mono_button.signal_clicked().connect(sigc::bind(mono, this));
  add_button(dotnet_flowbox, install_mono_button, "(Re)installs Wine Mono: fixes a broken Mono/.NET support by fetching the matching Wine Mono MSI");
  install_dotnet4_0_button.signal_clicked().connect(sigc::bind(dotnet, this, "40"));
  add_button(dotnet_flowbox, install_dotnet4_0_button, "Installs .NET 4.0 from 2011");
  install_dotnet4_5_2_button.signal_clicked().connect(sigc::bind(dotnet, this, "452"));
  add_button(dotnet_flowbox, install_dotnet4_5_2_button, "Installs .NET 4.5.2 from 2012");
  install_dotnet4_7_2_button.signal_clicked().connect(sigc::bind(dotnet, this, "472"));
  add_button(dotnet_flowbox, install_dotnet4_7_2_button, "Installs .NET 4.7.2 from 2018");
  install_dotnet4_8_button.signal_clicked().connect(sigc::bind(dotnet, this, "48"));
  add_button(dotnet_flowbox, install_dotnet4_8_button, "Installs .NET 4.8 from 2019");
  install_dotnet6_button.signal_clicked().connect(sigc::bind(dotnet, this, "6"));
  add_button(dotnet_flowbox, install_dotnet6_button, "Installs .NET 6.0 LTS from 2021");
  install_dotnet7_button.signal_clicked().connect(sigc::bind(dotnet, this, "7"));
  add_button(dotnet_flowbox, install_dotnet7_button, "Installs .NET 7.0 from 2022");
  install_dotnet8_button.signal_clicked().connect(sigc::bind(dotnet, this, "8"));
  add_button(dotnet_flowbox, install_dotnet8_button, "Installs .NET 8.0 LTS from 2023");
  install_dotnet9_button.signal_clicked().connect(sigc::bind(dotnet, this, "9"));
  add_button(dotnet_flowbox, install_dotnet9_button, "Installs .NET 9.0 from 2024");

  // TODO: Inform the user to disable desktop effects of the compositor. And set CPU to performance.

  // Add the category pages to the stack (title shows up in the sidebar)
  stack.add(graphics_flowbox, "graphics", "Graphics");
  stack.add(fonts_flowbox, "fonts", "Fonts");
  stack.add(visual_cpp_flowbox, "visual_cpp", "Visual C++");
  stack.add(dotnet_flowbox, "dotnet", "Wine Mono & .NET");

  sidebar_stack_box.append(sidebar);
  sidebar_stack_box.append(stack_scroll);

  configure_box.append(hint_label);
  configure_box.append(sidebar_stack_box);
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

  if (is_gallium_nine_installed())
  {
    add_name_and_icon_to_button(install_gallium_nine_button, "Reinstall Gallium Nine DirectX v9", true);
  }
  else
  {
    add_name_and_icon_to_button(install_gallium_nine_button, "Install Gallium Nine DirectX v9", false);
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

  // Check for Visual C++ 2003 (no DLL override nor uninstaller entry; only DLL files are extracted)
  {
    bool installed = is_visual_cpp_2003_installed();
    add_name_and_icon_to_button(install_visual_cpp_2003_button, (installed ? "Reinstall" : "Install") + string(" Visual C++ 2003"), installed);
  }

  // Check for the Visual C++ Redistributable packages, using the msvcp DLL override + the uninstaller display name.
  // Note: 2015 and up all share the same msvcp140 DLL family; only the uninstaller display name differs
  // (and later versions like 2015-2019/2015-2022/2017-2026 supersede the earlier ones)
  auto update_visual_cpp_button =
      [this](Gtk::Button& button, const string& label_year, const string& msvcp_dll_name, const string& uninstaller_display_name_prefix)
  {
    bool installed = is_visual_cpp_installed(msvcp_dll_name, uninstaller_display_name_prefix);
    add_name_and_icon_to_button(button, (installed ? "Reinstall" : "Install") + string(" Visual C++ ") + label_year, installed);
  };
  update_visual_cpp_button(install_visual_cpp_2005_button, "2005", "*msvcp80", "Microsoft Visual C++ 2005 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2008_button, "2008", "*msvcp90", "Microsoft Visual C++ 2008 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2010_button, "2010", "*msvcp100", "Microsoft Visual C++ 2010");
  update_visual_cpp_button(install_visual_cpp_2012_button, "2012", "*msvcp110", "Microsoft Visual C++ 2012 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2013_button, "2013", "*msvcp120", "Microsoft Visual C++ 2013 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2015_button, "2015", "*msvcp140", "Microsoft Visual C++ 2015 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2017_button, "2017", "*msvcp140", "Microsoft Visual C++ 2017 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2019_button, "2019", "*msvcp140", "Microsoft Visual C++ 2015-2019 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2022_button, "2022", "*msvcp140", "Microsoft Visual C++ 2015-2022 Redistributable");
  update_visual_cpp_button(install_visual_cpp_2026_button, "2026", "*msvcp140", "Microsoft Visual C++ 2017-2026 Redistributable");

  // Wine Mono can always be (re)installed to repair a broken Mono/.NET support
  add_name_and_icon_to_button(install_mono_button, "Reinstall Wine Mono", true);

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
  if (is_dotnet_runtime_installed("6", "{5DEFBDBE-FF1A-4EB2-8DFB-17A26A7E6442}", "{3CC763AD-93B3-41EF-ABF8-CFE63A1DC3A6}"))
  {
    add_name_and_icon_to_button(install_dotnet6_button, "Reinstall .NET v6.0 LTS", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet6_button, "Install .NET v6.0 LTS", false);
  }

  // Check for .NET 7.0
  // TODO: capture the x86/x64 MSI product GUIDs (install once, read system.reg) to enable reinstall detection
  if (is_dotnet_runtime_installed("7", "", ""))
  {
    add_name_and_icon_to_button(install_dotnet7_button, "Reinstall .NET v7.0", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet7_button, "Install .NET v7.0", false);
  }

  // Check for .NET 8.0 LTS
  // TODO: capture the x86/x64 MSI product GUIDs (install once, read system.reg) to enable reinstall detection
  if (is_dotnet_runtime_installed("8", "", ""))
  {
    add_name_and_icon_to_button(install_dotnet8_button, "Reinstall .NET v8.0 LTS", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet8_button, "Install .NET v8.0 LTS", false);
  }

  // Check for .NET 9.0
  // TODO: capture the x86/x64 MSI product GUIDs (install once, read system.reg) to enable reinstall detection
  if (is_dotnet_runtime_installed("9", "", ""))
  {
    add_name_and_icon_to_button(install_dotnet9_button, "Reinstall .NET v9.0", true);
  }
  else
  {
    add_name_and_icon_to_button(install_dotnet9_button, "Install .NET v9.0", false);
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
 * \brief Check is Gallium Nine Standalone (Direct3D 9 via the Mesa Gallium driver) is installed
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_gallium_nine_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // ninewinecfg -e (executed by winetricks) sets the 'd3d9' DLL override (without asterisk,
      // unlike DXVK which uses '*d3d9') to 'native' load order when Gallium Nine is enabled
      is_installed = Helper::get_dll_override(wine_prefix, "d3d9");
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
 * \brief Check if MS Visual C++ 2003 libraries are installed.
 * The winetricks vcrun2003 verb only extracts the native DLLs into system32
 * (no DLL override nor uninstaller entry). Wine also ships a builtin msvcp71.dll,
 * so additionally check the file isn't the Wine builtin DLL but the native (Microsoft) one.
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_2003_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    string dll_path = Glib::build_filename(active_bottle_->wine_location(), "drive_c", "windows", "system32", "msvcp71.dll");
    is_installed = Helper::file_exists(dll_path) && !Helper::is_wine_builtin_dll(dll_path);
  }
  return is_installed;
}

/**
 * \brief Check if a MS Visual C++ Redistributable package is installed,
 * using the msvcp DLL override + the uninstaller display name prefix.
 * The display name scan works for both the 32-bit and 64-bit packages and doesn't depend
 * on version-specific MSI product GUIDs (which change with every package build).
 * \param[in] msvcp_dll_name The msvcp DLL override name (eg. "*msvcp140")
 * \param[in] uninstaller_display_name_prefix The uninstaller display name prefix (eg. "Microsoft Visual C++ 2015-2022 Redistributable")
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_visual_cpp_installed(const string& msvcp_dll_name, const string& uninstaller_display_name_prefix)
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    try
    {
      // Check if DLL is set to 'native, builtin' load order
      bool is_dll_override = Helper::get_dll_override(wine_prefix, msvcp_dll_name, DLLOverride::LoadOrder::NativeBuiltin);
      if (is_dll_override)
      {
        // Next, check if the package can be found to be uninstalled
        is_installed = Helper::has_uninstaller_display_name_prefix(wine_prefix, uninstaller_display_name_prefix);
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
 * \brief Check if a modern MS .NET runtime (v6 and up) is installed.
 * These register in the uninstaller as "Microsoft .NET Runtime - <major>.x.x", using
 * build-specific x86/x64 MSI product GUIDs. Pass an empty GUID to skip that architecture.
 * \param[in] major_version         Major .NET version, eg. "6", "8" (used for the display name match)
 * \param[in] x86_uninstaller_key   The 32-bit MSI product GUID (empty to skip)
 * \param[in] x64_uninstaller_key   The 64-bit MSI product GUID (empty to skip)
 * \return True if installed otherwise False
 */
bool BottleConfigureWindow::is_dotnet_runtime_installed(const string& major_version,
                                                        const string& x86_uninstaller_key,
                                                        const string& x64_uninstaller_key)
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    const string display_name_prefix = "Microsoft .NET Runtime - " + major_version;
    try
    {
      // Check the 32-bit package
      if (!x86_uninstaller_key.empty())
      {
        string name = Helper::get_uninstaller(wine_prefix, x86_uninstaller_key);
        // Strings has first occurrence of display name
        is_installed = (name.find(display_name_prefix) == 0);
      }
      // Try the 64-bit package (fallback)
      if (!is_installed && !x64_uninstaller_key.empty())
      {
        string name = Helper::get_uninstaller(wine_prefix, x64_uninstaller_key);
        // Strings has first occurrence of display name
        is_installed = (name.find(display_name_prefix) == 0);
      }
    }
    catch (const std::runtime_error& error)
    {
      std::cout << "Error: " << error.what() << std::endl;
    }
  }
  return is_installed;
}