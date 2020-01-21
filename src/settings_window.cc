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
  install_directx("Install DirectX v9"),
  install_vulkan("Install Vulkan"),
  install_dotnet("Install .NET v4.0"),
  wine_config_button("WineCfg"),
  winetricks_button("Winetricks"),
  activeBottle(nullptr)
{
  set_transient_for(parent);
  set_default_size(750, 540);
  set_modal(true);

  add(settings_grid);
  settings_grid.set_margin_top(5);
  settings_grid.set_margin_end(5);
  settings_grid.set_margin_bottom(8);
  settings_grid.set_margin_start(8);
  settings_grid.set_column_spacing(8);
  settings_grid.set_row_spacing(12);

  first_row_label.set_text("Install additional packages");
  first_row_label.set_xalign(0);
  second_row_label.set_text("Other Tools");
  second_row_label.set_xalign(0);

  first_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  first_toolbar.set_halign(Gtk::ALIGN_CENTER);
  first_toolbar.set_hexpand(true);
  second_toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);
  second_toolbar.set_halign(Gtk::ALIGN_CENTER);
  second_toolbar.set_hexpand(true);

  settings_grid.attach(first_row_label, 0, 0, 1, 1); 
  settings_grid.attach(first_toolbar, 0, 1, 1, 1);
  settings_grid.attach(second_row_label, 0, 2, 1, 1);
  settings_grid.attach(second_toolbar, 0, 3, 1, 1);

  install_directx.signal_clicked().connect(directx);
  install_vulkan.signal_clicked().connect(vulkan);
  install_dotnet.signal_clicked().connect(dotnet);
  
  wine_config_button.signal_clicked().connect(winecfg);
  winetricks_button.signal_clicked().connect(winetricks);

  // First row buttons, 1-button installs
  Gtk::Image* directx_image = Gtk::manage(new Gtk::Image());
  directx_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_directx.set_icon_widget(*directx_image);
  first_toolbar.insert(install_directx, 0);

  Gtk::Image* vulkan_image = Gtk::manage(new Gtk::Image());
  vulkan_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_vulkan.set_icon_widget(*vulkan_image);
  first_toolbar.insert(install_vulkan, 1);

  Gtk::Image* dotnet_image = Gtk::manage(new Gtk::Image());
  dotnet_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  install_dotnet.set_icon_widget(*dotnet_image);
  first_toolbar.insert(install_dotnet, 2);

  // Second row buttons/other tools
  Gtk::Image* winecfg_image = Gtk::manage(new Gtk::Image());
  winecfg_image->set_from_icon_name("applications-system-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  wine_config_button.set_icon_widget(*winecfg_image);
  second_toolbar.insert(wine_config_button, 0);

  Gtk::Image* winetricks_image = Gtk::manage(new Gtk::Image());
  winetricks_image->set_from_icon_name("preferences-other-symbolic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  winetricks_button.set_icon_widget(*winetricks_image);
  second_toolbar.insert(winetricks_button, 1);

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