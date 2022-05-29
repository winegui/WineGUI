/**
 * Copyright (c) 2020-2022 WineGUI
 *
 * \file    bottle_settings_window.cc
 * \brief   Wine bottle setting window
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
#include "bottle_settings_window.h"
#include "bottle_item.h"
#include "helper.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleSettingsWindow::BottleSettingsWindow(Gtk::Window& parent)
    : wine_uninstall_button("Uninstaller"),
      open_notepad_button("Notepad"),
      open_wordpad_button("Open Wordpad"),
      open_iexplore_button("Open IE"),
      wine_task_manager_button("Task manager"),
      wine_regedit_button("Windows Registery Editor"),
      explorer_button("Explorer"),
      wine_console_button("Console"),
      wine_config_button("Wine Config"),
      winetricks_button("Winetricks"),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_default_size(900, 540);
  set_modal(true);

  add(settings_grid);
  settings_grid.set_margin_top(5);
  settings_grid.set_margin_end(5);
  settings_grid.set_margin_bottom(6);
  settings_grid.set_margin_start(6);
  settings_grid.set_column_spacing(6);
  settings_grid.set_row_spacing(8);

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

  first_row_label.set_text("Gaming packages");
  first_row_label.set_attributes(attr_list_label);
  first_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  hint_label.set_markup("<b>Hint:</b> Hover the mouse over the buttons for more info...");
  hint_label.set_margin_top(8);
  hint_label.set_margin_bottom(4);
  second_row_label.set_text("Additional packages");
  second_row_label.set_attributes(attr_list_label);
  second_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  third_row_label.set_text("Supporting Tools");
  third_row_label.set_attributes(attr_list_label);
  third_row_label.set_halign(Gtk::Align::ALIGN_CENTER);
  fourth_row_label.set_text("Fallback Tools");
  fourth_row_label.set_attributes(attr_list_label);
  fourth_row_label.set_halign(Gtk::Align::ALIGN_CENTER);

  settings_grid.attach(first_row_label, 0, 0);
  settings_grid.attach(first_toolbar, 0, 1);
  settings_grid.attach(hint_label, 0, 2);
  settings_grid.attach(second_row_label, 0, 3);
  settings_grid.attach(second_toolbar, 0, 4);
  settings_grid.attach(third_row_label, 0, 5);
  settings_grid.attach(third_toolbar, 0, 6);
  settings_grid.attach(fourth_row_label, 0, 7);
  settings_grid.attach(fourth_toolbar, 0, 8);

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

  // Third row buttons, supporting tools
  Gtk::Image* uninstaller_image = Gtk::manage(new Gtk::Image());
  uninstaller_image->set_from_icon_name("applications-system-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_uninstall_button.signal_clicked().connect(uninstaller);
  wine_uninstall_button.set_tooltip_text("Open Wine uninstaller");
  wine_uninstall_button.set_icon_widget(*uninstaller_image);
  third_toolbar.insert(wine_uninstall_button, 0);

  Gtk::Image* notepad_image = Gtk::manage(new Gtk::Image());
  notepad_image->set_from_icon_name("accessories-text-editor", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  open_notepad_button.signal_clicked().connect(notepad);
  open_notepad_button.set_tooltip_text("Open Notepad Editor");
  open_notepad_button.set_icon_widget(*notepad_image);
  third_toolbar.insert(open_notepad_button, 1);

  Gtk::Image* wordpad_image = Gtk::manage(new Gtk::Image());
  wordpad_image->set_from_icon_name("accessories-text-editor", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  open_wordpad_button.signal_clicked().connect(wordpad);
  open_wordpad_button.set_tooltip_text("Open Wordpad");
  open_wordpad_button.set_icon_widget(*wordpad_image);
  third_toolbar.insert(open_wordpad_button, 2);

  Gtk::Image* ie_explore = Gtk::manage(new Gtk::Image());
  ie_explore->set_from_icon_name("emblem-web", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  open_iexplore_button.signal_clicked().connect(iexplore);
  open_iexplore_button.set_tooltip_text("Open Internet Explorer");
  open_iexplore_button.set_icon_widget(*ie_explore);
  third_toolbar.insert(open_iexplore_button, 3);

  Gtk::Image* task_manager_image = Gtk::manage(new Gtk::Image());
  task_manager_image->set_from_icon_name("open-menu", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_task_manager_button.signal_clicked().connect(task_manager);
  wine_task_manager_button.set_tooltip_text("Open Wine task manager");
  wine_task_manager_button.set_icon_widget(*task_manager_image);
  third_toolbar.insert(wine_task_manager_button, 4);

  Gtk::Image* regedit_image = Gtk::manage(new Gtk::Image());
  regedit_image->set_from_icon_name("applications-system-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_regedit_button.signal_clicked().connect(regedit);
  wine_regedit_button.set_tooltip_text("Open Windows Registry editor (For advanced users!)");
  wine_regedit_button.set_icon_widget(*regedit_image);
  third_toolbar.insert(wine_regedit_button, 5);

  // Fourth row buttons, fallback tools
  Gtk::Image* explorer_image = Gtk::manage(new Gtk::Image());
  explorer_image->set_from_icon_name("system-file-manager", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  explorer_button.signal_clicked().connect(explorer);
  explorer_button.set_tooltip_text("Open Explorer");
  explorer_button.set_icon_widget(*explorer_image);
  fourth_toolbar.insert(explorer_button, 0);

  Gtk::Image* wine_console_image = Gtk::manage(new Gtk::Image());
  wine_console_image->set_from_icon_name("utilities-terminal", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_console_button.signal_clicked().connect(console);
  wine_console_button.set_tooltip_text("Open Windows cmd");
  wine_console_button.set_icon_widget(*wine_console_image);
  fourth_toolbar.insert(wine_console_button, 1);

  Gtk::Image* winecfg_image = Gtk::manage(new Gtk::Image());
  winecfg_image->set_from_icon_name("preferences-system", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_config_button.signal_clicked().connect(winecfg);
  wine_config_button.set_tooltip_text("FALLBACK: Open wine configuration GUI");
  wine_config_button.set_icon_widget(*winecfg_image);
  fourth_toolbar.insert(wine_config_button, 2);

  Gtk::Image* winetricks_image = Gtk::manage(new Gtk::Image());
  winetricks_image->set_from_icon_name("preferences-other-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  winetricks_button.signal_clicked().connect(winetricks);
  winetricks_button.set_tooltip_text("FALLBACK: Winetricks GUI");
  winetricks_button.set_icon_widget(*winetricks_image);
  fourth_toolbar.insert(winetricks_button, 3);

  show_all_children();
}

/**
 * \brief Destructor
 */
BottleSettingsWindow::~BottleSettingsWindow()
{
}

/**
 * \brief Same as show() but will also update the Window title
 */
void BottleSettingsWindow::show()
{
  this->update_installed();

  if (active_bottle_ != nullptr)
    set_title("Settings of machine - " +
              ((!active_bottle_->name().empty()) ? active_bottle_->name() : active_bottle_->folder_name())); // Fall-back to folder name
  else
    set_title("Settings for machine (Unknown machine)");
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void BottleSettingsWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleSettingsWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Update GUI state depending on the packages installed
 */
void BottleSettingsWindow::update_installed()
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
bool BottleSettingsWindow::is_d3dx9_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native' load order
    is_installed = Helper::get_dll_override(wine_prefix, "*d3dx9_43");
  }
  return is_installed;
}

/**
 * \brief Check is DXVK (Vulkan based DirectX 9/10/11) is installed
 * \return True if installed otherwise False
 */
bool BottleSettingsWindow::is_dxvk_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native' load order
    is_installed = Helper::get_dll_override(wine_prefix, "*dxgi");
  }
  return is_installed;
}

/**
 * \brief Check if Liberation fonts are installed
 * As fallback: Wine is looking for the liberation font on the local unix system (in the /usr/share/fonts/.. directory)
 * \return True if installed otherwise False
 */
bool BottleSettingsWindow::is_liberation_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    BottleTypes::Bit bit = active_bottle_->bit();
    string fontFilename = Helper::get_font_filename(wine_prefix, bit, "Liberation Mono (TrueType)");
    is_installed = (fontFilename == "liberationmono-regular.ttf");
  }
  return is_installed;
}

/**
 * \brief Check if MS Core fonts are installed
 * \return True if installed otherwise False
 */
bool BottleSettingsWindow::is_core_fonts_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    BottleTypes::Bit bit = active_bottle_->bit();
    string fontFilename = Helper::get_font_filename(wine_prefix, bit, "Comic Sans MS (TrueType)");
    is_installed = (fontFilename == "comic.ttf");
  }
  return is_installed;
}

/**
 * \brief Check if MS Visual C++ installed
 * \return True if installed otherwise False
 */
bool BottleSettingsWindow::is_visual_cpp_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native, builtin' load order
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
  return is_installed;
}

/**
 * \brief Check if MS .NET v4.0 is installed
 * \return True if installed otherwise False
 */
bool BottleSettingsWindow::is_dotnet_4_0_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native' load order
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
  return is_installed;
}

/**
 * \brief Check ife MS .NET v4.5.2 is installed
 * \return True if installed otherwise False
 */
bool BottleSettingsWindow::is_dotnet_4_5_2_installed()
{
  bool is_installed = false;
  if (active_bottle_ != nullptr)
  {
    Glib::ustring wine_prefix = active_bottle_->wine_location();
    // Check if set to 'native' load order
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
  return is_installed;
}
