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
#include "edit_window.h"
#include "bottle_item.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
EditWindow::EditWindow(Gtk::Window& parent)
:
  edit_button("Edit Configuration"),
  delete_button("Delete Machine"),
  wine_config_button("WineCfg")
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

  first_row_label.set_text("WineGUI Settings");
  first_row_label.set_xalign(0);
  second_row_label.set_text("Other Tools Settings");
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

  // First row with buttons
  Gtk::Image* edit_image = Gtk::manage(new Gtk::Image());
  edit_image->set_from_icon_name("document-edit", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  edit_button.set_icon_widget(*edit_image);
  first_toolbar.insert(edit_button, 0);

  Gtk::Image* delete_image = Gtk::manage(new Gtk::Image());
  delete_image->set_from_icon_name("edit-delete", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  delete_button.set_icon_widget(*delete_image);
  delete_button.set_border_width(2);
  first_toolbar.insert(delete_button, 1);

  // Second row with buttons
  second_toolbar.insert(wine_config_button, 0);

  show_all_children();
}

/**
 * \brief Destructor
 */
EditWindow::~EditWindow() {}

/**
 * \brief Same as show() but will also update the Window title
 */
void EditWindow::Show() {
  if (activeBottle != nullptr)
    set_title("Edit Machine - " + activeBottle->name());
  else
    set_title("Edit Machine (Unknown machine)");
  // Call parent show
  Gtk::Widget::show();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void EditWindow::SetActiveBottle(BottleItem* bottle)
{
  this->activeBottle = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void EditWindow::ResetActiveBottle()
{
  this->activeBottle = nullptr;
}