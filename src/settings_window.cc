/**
 * Copyright (c) 2020 WineGUI
 *
 * \file    settings_window.cc
 * \brief   Setting GTK+ Window class
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
#include "settings_window.h"
#include "bottle_item.h"
#include "helper.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
SettingsWindow::SettingsWindow(Gtk::Window& parent)
:
  install_d3dx9_button("Install DirectX v9 (OpenGL)"),
  install_dxvk_button("Install DirectX v9/v10/v11 (Vulkan)"),
  install_liberation_fonts_button("Install Liberation (open-source fonts)"),
  install_core_fonts_button("Install Core Fonts"),
  install_visual_cpp_button("Install Visual C++ 2013"),
  install_dotnet4_button("Install .NET v4"),
  install_dotnet452_button("Install .NET v4.5.2"),
  wine_uninstall_button("Uninstaller"),
  open_notepad_button("Notepad"),
  wine_task_manager_button("Task manager"),
  wine_regedit_button("Windows Registery Editor"),
  wine_config_button("WineCfg"),
  winetricks_button("Winetricks"),
  activeBottle(nullptr)
{
  set_transient_for(parent);
  set_default_size(850, 540);
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

  first_row_label.set_text("Gaming packages");
  first_row_label.set_xalign(0);
  hint_label.set_markup("<b>Hint:</b> Hover the mouse over the buttons for more info...");
  hint_label.set_margin_top(8);
  hint_label.set_margin_bottom(4);
  second_row_label.set_text("Additional packages");
  second_row_label.set_xalign(0);
  third_row_label.set_text("Supporting Tools");
  third_row_label.set_xalign(0);
  fourth_row_label.set_text("Fallback Tools");
  fourth_row_label.set_xalign(0);

  settings_grid.attach(first_row_label,  0, 0, 1, 1); 
  settings_grid.attach(first_toolbar,    0, 1, 1, 1);
  settings_grid.attach(hint_label,       0, 2, 1, 1);  
  settings_grid.attach(second_row_label, 0, 3, 1, 1);
  settings_grid.attach(second_toolbar,   0, 4, 1, 1);
  settings_grid.attach(third_row_label,  0, 5, 1, 1);
  settings_grid.attach(third_toolbar,    0, 6, 1, 1);
  settings_grid.attach(fourth_row_label, 0, 7, 1, 1);
  settings_grid.attach(fourth_toolbar,   0, 8, 1, 1);

  // TODO: Inform the user to disable desktop effects of the compositor. And set CPU to performance.

  // First row buttons, 1-button installs
  Gtk::Image* d3dx9_image = Gtk::manage(new Gtk::Image());
  d3dx9_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_d3dx9_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(directx9, *this, ""));
  install_d3dx9_button.set_tooltip_text("Installs MS D3DX9: Ideal for DirectX 9 games, by using OpenGL");
  install_d3dx9_button.set_icon_widget(*d3dx9_image);
  first_toolbar.insert(install_d3dx9_button, 0);

  Gtk::Image* vulkan_image = Gtk::manage(new Gtk::Image());
  vulkan_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_dxvk_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(vulkan, *this, "latest"));
  install_dxvk_button.set_tooltip_text("Installs DXVK: Ideal for DirectX 9/10/11 games, by using Vulkan");
  install_dxvk_button.set_icon_widget(*vulkan_image);
  first_toolbar.insert(install_dxvk_button, 1);
  
  // Second row, additional packages
  Gtk::Image* liberation_image = Gtk::manage(new Gtk::Image());
  liberation_image->set_from_icon_name("font-x-generic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_liberation_fonts_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(corefonts, *this));
  install_liberation_fonts_button.set_tooltip_text("Installs Liberation open-source Fonts, replaces Core fonts");
  install_liberation_fonts_button.set_icon_widget(*liberation_image);
  second_toolbar.insert(install_liberation_fonts_button, 0);

  Gtk::Image* corefonts_image = Gtk::manage(new Gtk::Image());
  corefonts_image->set_from_icon_name("font-x-generic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_core_fonts_button.signal_clicked().connect(sigc::bind<Gtk::Window&>(corefonts, *this));
  install_core_fonts_button.set_tooltip_text("Installs Microsoft Core Fonts");
  install_core_fonts_button.set_icon_widget(*corefonts_image);
  second_toolbar.insert(install_core_fonts_button, 1);

  Gtk::Image* visual_cpp_image = Gtk::manage(new Gtk::Image());
  visual_cpp_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_visual_cpp_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(visual_cpp_package, *this, "2013"));
  install_visual_cpp_button.set_tooltip_text("Installs Visual C++ 2013 package");
  install_visual_cpp_button.set_icon_widget(*visual_cpp_image);
  second_toolbar.insert(install_visual_cpp_button, 2);

  Gtk::Image* dotnet4_image = Gtk::manage(new Gtk::Image());
  dotnet4_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_dotnet4_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "40"));
  install_dotnet4_button.set_tooltip_text("Installs .NET 4.0");
  install_dotnet4_button.set_icon_widget(*dotnet4_image);
  second_toolbar.insert(install_dotnet4_button, 4);

    Gtk::Image* dotnet452_image = Gtk::manage(new Gtk::Image());
  dotnet452_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_dotnet452_button.signal_clicked().connect(sigc::bind<Gtk::Window&, Glib::ustring>(dotnet, *this, "452"));
  install_dotnet452_button.set_tooltip_text("Installs .NET 4.0 and .NET 4.5.2");
  install_dotnet452_button.set_icon_widget(*dotnet452_image);
  second_toolbar.insert(install_dotnet452_button, 5);

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

  Gtk::Image* task_manager_image = Gtk::manage(new Gtk::Image());
  task_manager_image->set_from_icon_name("open-menu", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_task_manager_button.signal_clicked().connect(task_manager);
  wine_task_manager_button.set_tooltip_text("Open Wine task manager");
  wine_task_manager_button.set_icon_widget(*task_manager_image);
  third_toolbar.insert(wine_task_manager_button, 2);

  Gtk::Image* regedit_image = Gtk::manage(new Gtk::Image());
  regedit_image->set_from_icon_name("applications-system-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_regedit_button.signal_clicked().connect(regedit);
  wine_regedit_button.set_tooltip_text("Open Windows Registry editor (For advanced users!)");
  wine_regedit_button.set_icon_widget(*regedit_image);
  third_toolbar.insert(wine_regedit_button, 3);

  // Fourth row buttons, fallback tools
  Gtk::Image* winecfg_image = Gtk::manage(new Gtk::Image());
  winecfg_image->set_from_icon_name("preferences-system", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_config_button.signal_clicked().connect(winecfg);
  wine_config_button.set_tooltip_text("FALLBACK: Open winecfg GUI");
  wine_config_button.set_icon_widget(*winecfg_image);
  fourth_toolbar.insert(wine_config_button, 0);

  Gtk::Image* winetricks_image = Gtk::manage(new Gtk::Image());
  winetricks_image->set_from_icon_name("preferences-other-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  winetricks_button.signal_clicked().connect(winetricks);
  winetricks_button.set_tooltip_text("FALLBACK: Winetricks GUI");
  winetricks_button.set_icon_widget(*winetricks_image);
  fourth_toolbar.insert(winetricks_button, 1);

  show_all_children();
}

/**
 * \brief Destructor
 */
SettingsWindow::~SettingsWindow() {}

/**
 * \brief Same as show() but will also update the Window title
 */
void SettingsWindow::Show() {
  this->UpdateInstalled();

  if (activeBottle != nullptr)
    set_title("Settings of machine - " + activeBottle->name());
  else
    set_title("Settings for machine (Unknown machine)");
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void SettingsWindow::SetActiveBottle(BottleItem* bottle)
{
  this->activeBottle = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void SettingsWindow::ResetActiveBottle()
{
  this->activeBottle = nullptr;
}

/**
 * \brief Update GUI state depending on the packages installed
 */
void SettingsWindow::UpdateInstalled()
{
  if (IsD3DX9Installed()) {
    Gtk::Image* reinstall_d3dx9_image = Gtk::manage(new Gtk::Image());
    reinstall_d3dx9_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_d3dx9_button.set_label("Reinstall DirectX v9 (OpenGL)");
    install_d3dx9_button.set_icon_widget(*reinstall_d3dx9_image);
  } else {
    Gtk::Image* install_d3dx9_image = Gtk::manage(new Gtk::Image());
    install_d3dx9_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_d3dx9_button.set_label("Install DirectX v9 (OpenGL)");
    install_d3dx9_button.set_icon_widget(*install_d3dx9_image);
  }
  
  if (IsDXVKInstalled()) {
    Gtk::Image* reinstall_d3dx9_image = Gtk::manage(new Gtk::Image());
    reinstall_d3dx9_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dxvk_button.set_label("Reinstall DirectX v9/v10/v11 (Vulkan)");
    install_dxvk_button.set_icon_widget(*reinstall_d3dx9_image);
  } else {
    Gtk::Image* install_d3dx9_image = Gtk::manage(new Gtk::Image());
    install_d3dx9_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dxvk_button.set_label("Install DirectX v9/v10/v11 (Vulkan)");
    install_dxvk_button.set_icon_widget(*install_d3dx9_image);
  }

  if (isDotnet4Installed()) {
    Gtk::Image* reinstall_dotnet4_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet4_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_button.set_label("Reinstall .NET v4");
    install_dotnet4_button.set_icon_widget(*reinstall_dotnet4_image);
  } else {
    Gtk::Image* install_dotnet4_image = Gtk::manage(new Gtk::Image());
    install_dotnet4_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet4_button.set_label("Install .NET v4");
    install_dotnet4_button.set_icon_widget(*install_dotnet4_image);
  }

  if (isDotnet452Installed()) {
    Gtk::Image* reinstall_dotnet452_image = Gtk::manage(new Gtk::Image());
    reinstall_dotnet452_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet452_button.set_label("Reinstall .NET v4.5.2");
    install_dotnet452_button.set_icon_widget(*reinstall_dotnet452_image);
  } else {
    Gtk::Image* install_dotnet452_image = Gtk::manage(new Gtk::Image());
    install_dotnet452_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
    install_dotnet452_button.set_label("Install .NET v4.5.2");
    install_dotnet452_button.set_icon_widget(*install_dotnet452_image);
  }

  show_all_children();
}


/**
 * \brief Check is D3DX9 (DirectX 9 OpenGL) is installed
 * \return True if installed otherwise False
 */
bool SettingsWindow::IsD3DX9Installed()
{
  bool isInstalled = false;
  if (this->activeBottle != nullptr) {
    Glib::ustring wine_prefix = activeBottle->wine_location();
    // Check if set to 'native' load order
    isInstalled = Helper::GetDLLOverride(wine_prefix, "*d3dx9_43");
  }
  return isInstalled;
}

/**
 * \brief Check is DXVK (Vulkan based DirectX 9/10/11) is installed
 * \return True if installed otherwise False
 */
bool SettingsWindow::IsDXVKInstalled()
{
  bool isInstalled = false;
  if (this->activeBottle != nullptr) {
    Glib::ustring wine_prefix = activeBottle->wine_location();
    // Check if set to 'native' load order
    isInstalled = Helper::GetDLLOverride(wine_prefix, "*dxgi");
  }
  return isInstalled;
}

/**
 * \brief Check is Dotnet v4.0 is installed
 * \return True if installed otherwise False
 */
bool SettingsWindow::isDotnet4Installed()
{
  bool isInstalled = false;
  if (this->activeBottle != nullptr) {
    Glib::ustring wine_prefix = activeBottle->wine_location();
    // Check if set to 'native' load order
    bool isDllOverride = Helper::GetDLLOverride(wine_prefix, "*mscoree");

    if (isDllOverride) {
      // Next, check if package can be found to be uninstalled      
      string name = Helper::GetUninstaller(wine_prefix, "Microsoft .NET Framework 4 Extended");
      isInstalled = (name == "Microsoft .NET Framework 4 Extended");
    } else {
      isInstalled = false;
    }
  }
  return isInstalled;
}

/**
 * \brief Check is Dotnet v4.5.2 is installed
 * \return True if installed otherwise False
 */
bool SettingsWindow::isDotnet452Installed()
{
  bool isInstalled = false;
  if (this->activeBottle != nullptr) {
    Glib::ustring wine_prefix = activeBottle->wine_location();
    // Check if set to 'native' load order
    bool isDllOverride = Helper::GetDLLOverride(wine_prefix, "*mscoree");

    if (isDllOverride) {
      // Next, check if package can be found to be uninstalled      
      string name = Helper::GetUninstaller(wine_prefix, "{92FB6C44-E685-45AD-9B20-CADF4CABA132}");
      isInstalled = (name == "Microsoft .NET Framework 4.5.2");
    } else {
      isInstalled = false;
    }
  }
  return isInstalled;
}

