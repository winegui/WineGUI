/**
 * Copyright (c) 2020-2021 WineGUI
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
    : save_button("Save"),
      delete_button("Delete Machine"),
      wine_config_button("WineCfg"),
      activeBottle(nullptr)
{
    set_transient_for(parent);
    set_default_size(750, 540);
    set_modal(true);

    show_all_children();
}

/**
 * \brief Destructor
 */
EditWindow::~EditWindow() {}

/**
 * \brief Same as show() but will also update the Window title
 */
void EditWindow::Show()
{
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
void EditWindow::SetActiveBottle(BottleItem* bottle) { this->activeBottle = bottle; }

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void EditWindow::ResetActiveBottle() { this->activeBottle = nullptr; }