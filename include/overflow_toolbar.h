/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    overflow_toolbar.h
 * \brief   Horizontal toolbar widget that moves non-fitting buttons into an overflow menu
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
#pragma once

#include <gtkmm.h>
#include <vector>

/**
 * \struct OverflowToolbarButton
 * \brief  Describes a single toolbar action, used both for the visible button and its overflow menu item
 */
struct OverflowToolbarButton
{
  Gtk::Button* button;       /*!< The visible toolbar button (owned by the caller, parented to the toolbar) */
  Glib::ustring label;       /*!< Human readable label, also used as the overflow menu item text */
  Glib::ustring icon_name;   /*!< Icon name, also used as the overflow menu item icon */
  Glib::ustring action_name; /*!< Detailed action name (e.g. "win.edit_bottle") backing the overflow menu item */
};

/**
 * \class OverflowToolbar
 * \brief A horizontal action bar that keeps every action reachable when horizontal space is tight.
 *
 * GTK4 removed GtkToolbar and its automatic overflow behaviour. This widget re-implements it:
 * during size allocation it lays out as many buttons as fit from left to right, hides the ones
 * that don't, and exposes them through a trailing "More options" (⋯) overflow menu button.
 * Because the fit computation happens inside size_allocate_vfunc() it is flicker-free and always
 * reflects the real allocated width, unlike triggering on resize signals.
 */
class OverflowToolbar : public Gtk::Widget
{
public:
  OverflowToolbar();
  ~OverflowToolbar() override;

  /**
   * \brief Register the ordered list of toolbar buttons.
   * The buttons are re-parented to this toolbar; earlier buttons get priority when space is tight.
   */
  void set_buttons(const std::vector<OverflowToolbarButton>& buttons);

protected:
  Gtk::SizeRequestMode get_request_mode_vfunc() const override;
  void
  measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural, int& minimum_baseline, int& natural_baseline) const override;
  void size_allocate_vfunc(int width, int height, int baseline) override;

private:
  std::vector<OverflowToolbarButton> buttons_; /*!< Ordered toolbar buttons (index 0 has highest priority) */
  Gtk::MenuButton menu_button_;                /*!< Trailing overflow ("More options") menu button */
  Glib::RefPtr<Gio::Menu> overflow_menu_;      /*!< Menu model populated with the hidden buttons */

  int spacing_ = 0; /*!< Horizontal spacing between toolbar children, taken from CSS margins */

  int measure_button_width(const OverflowToolbarButton& button) const;
  void populate_overflow_menu(size_t first_hidden_index);
};
