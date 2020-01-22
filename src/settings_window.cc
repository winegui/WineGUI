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

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
SettingsWindow::SettingsWindow(Gtk::Window& parent)
:
  install_d3dx9("Install DirectX v9 (Wine Driver, no Vulkan)"),
  install_dxvk("Install DirectX v9/v10/v11 (Vulkan)"),
  install_gallium_nine("Install DirectX v9 (Mesa 3D)"),
  install_dotnet("Install .NET v4.0"),  
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
  settings_grid.set_margin_bottom(8);
  settings_grid.set_margin_start(8);
  settings_grid.set_column_spacing(8);
  settings_grid.set_row_spacing(12);

  first_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  first_toolbar.set_halign(Gtk::ALIGN_CENTER);
  first_toolbar.set_hexpand(true);
  second_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  second_toolbar.set_halign(Gtk::ALIGN_CENTER);
  second_toolbar.set_hexpand(true);
  third_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  third_toolbar.set_halign(Gtk::ALIGN_CENTER);
  third_toolbar.set_hexpand(true);

  first_row_label.set_text("Gaming packages");
  first_row_label.set_xalign(0);
  hint_label.set_markup("<b>Hint:</b> Hover mouse over the buttons for more info...");
  hint_label.set_margin_top(8);
  hint_label.set_margin_bottom(4);
  second_row_label.set_text("Additional packages");
  second_row_label.set_xalign(0);
  third_row_label.set_text("Other Tools");
  third_row_label.set_xalign(0);

  settings_grid.attach(first_row_label,  0, 0, 1, 1); 
  settings_grid.attach(first_toolbar,    0, 1, 1, 1);
  settings_grid.attach(hint_label,       0, 2, 1, 1);  
  settings_grid.attach(second_row_label, 0, 3, 1, 1);
  settings_grid.attach(second_toolbar,   0, 4, 1, 1);
  settings_grid.attach(third_row_label,  0, 5, 1, 1);
  settings_grid.attach(third_toolbar,    0, 6, 1, 1);

  // First row signals
  install_d3dx9.signal_clicked().connect(directx9);
  install_dxvk.signal_clicked().connect(vulkan);
  install_gallium_nine.signal_clicked().connect(gallium_directx9);

  // Second row signals
  install_dotnet.signal_clicked().connect(dotnet);
  
  // Third row signals
  wine_config_button.signal_clicked().connect(winecfg);
  winetricks_button.signal_clicked().connect(winetricks);

  // First row buttons, 1-button installs
  Gtk::Image* d3dx9_image = Gtk::manage(new Gtk::Image());
  d3dx9_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_d3dx9.set_tooltip_text("Installs D3DX9: Ideal for DirectX 9 games, if your videocard doesn't support Vulkan");
  install_d3dx9.set_icon_widget(*d3dx9_image);
  first_toolbar.insert(install_d3dx9, 0);

  Gtk::Image* vulkan_image = Gtk::manage(new Gtk::Image());
  vulkan_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_dxvk.set_tooltip_text("Installs DXVK: Ideal for DirectX 9/10/11 games using Vulkan");
  install_dxvk.set_icon_widget(*vulkan_image);
  first_toolbar.insert(install_dxvk, 1);

  Gtk::Image* gallium_nine_image = Gtk::manage(new Gtk::Image());
  gallium_nine_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_gallium_nine.set_tooltip_text("Installs Gallium Nine: Ideal for DirectX 9 games using Mesa 3D driver");
  install_gallium_nine.set_icon_widget(*gallium_nine_image);
  first_toolbar.insert(install_gallium_nine, 2);

  // TODO: esync wine build?
  // TODO: Inform the user to disable desktop effects of the compositor. And set CPU to performance.

  // Second row, additional packages
  Gtk::Image* dotnet_image = Gtk::manage(new Gtk::Image());
  dotnet_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_dotnet.set_icon_widget(*dotnet_image);
  second_toolbar.insert(install_dotnet, 2);

  // Third row buttons, other tools
  Gtk::Image* winecfg_image = Gtk::manage(new Gtk::Image());
  winecfg_image->set_from_icon_name("applications-system-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_config_button.set_icon_widget(*winecfg_image);
  third_toolbar.insert(wine_config_button, 0);

  Gtk::Image* winetricks_image = Gtk::manage(new Gtk::Image());
  winetricks_image->set_from_icon_name("preferences-other-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  winetricks_button.set_icon_widget(*winetricks_image);
  third_toolbar.insert(winetricks_button, 1);

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