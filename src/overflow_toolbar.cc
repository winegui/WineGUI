/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    overflow_toolbar.cc
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
#include "overflow_toolbar.h"
#include <numeric>

OverflowToolbar::OverflowToolbar() : Gtk::Widget(), overflow_menu_(Gio::Menu::create())
{
  set_layout_manager(Glib::RefPtr<Gtk::LayoutManager>()); // We manage the layout manually via the *_vfunc overrides
  set_overflow(Gtk::Overflow::HIDDEN);

  // Trailing "More options" overflow button, only shown when something doesn't fit
  menu_button_.set_icon_name("view-more-symbolic");
  menu_button_.set_tooltip_text("More options");
  menu_button_.set_menu_model(overflow_menu_);
  // Keep the button "visible" so it always measures to a real size; whether it actually
  // participates in the layout is controlled per-allocation via set_child_visible().
  menu_button_.set_child_visible(false);
  menu_button_.set_parent(*this);
}

OverflowToolbar::~OverflowToolbar()
{
  // Unparent every child we own, otherwise gtkmm warns on destruction
  for (auto& toolbar_button : buttons_)
  {
    if (toolbar_button.button->get_parent() == this)
      toolbar_button.button->unparent();
  }
  menu_button_.unparent();
}

void OverflowToolbar::set_buttons(const std::vector<OverflowToolbarButton>& buttons)
{
  // Unparent any previously registered buttons
  for (auto& toolbar_button : buttons_)
  {
    if (toolbar_button.button->get_parent() == this)
      toolbar_button.button->unparent();
  }

  buttons_ = buttons;

  // Parent the new buttons before the trailing menu button so the menu button stays last in the child list
  for (auto& toolbar_button : buttons_)
  {
    toolbar_button.button->unparent();
    toolbar_button.button->insert_before(*this, menu_button_);
  }

  queue_resize();
}

Gtk::SizeRequestMode OverflowToolbar::get_request_mode_vfunc() const
{
  return Gtk::SizeRequestMode::CONSTANT_SIZE;
}

int OverflowToolbar::measure_button_width(const OverflowToolbarButton& button) const
{
  int child_min = 0, child_nat = 0, ignore_min = 0, ignore_nat = 0;
  button.button->measure(Gtk::Orientation::HORIZONTAL, -1, child_min, child_nat, ignore_min, ignore_nat);
  return child_nat;
}

void OverflowToolbar::measure_vfunc(
    Gtk::Orientation orientation, int /*for_size*/, int& minimum, int& natural, int& minimum_baseline, int& natural_baseline) const
{
  minimum_baseline = -1;
  natural_baseline = -1;

  if (orientation == Gtk::Orientation::HORIZONTAL)
  {
    // Natural width: all buttons laid out side by side. Minimum width: just the overflow menu button,
    // so the toolbar (and therefore the whole window) can shrink far and rely on overflow.
    int menu_min = 0, menu_nat = 0, ignore_min = 0, ignore_nat = 0;
    menu_button_.measure(Gtk::Orientation::HORIZONTAL, -1, menu_min, menu_nat, ignore_min, ignore_nat);

    int total_natural = std::accumulate(buttons_.begin(), buttons_.end(), 0, [this](int sum, const OverflowToolbarButton& toolbar_button)
                                        { return sum + measure_button_width(toolbar_button) + spacing_; });

    minimum = menu_min;
    natural = total_natural > 0 ? total_natural : menu_nat;
  }
  else
  {
    // Height: tallest child (buttons and the menu button share a single row)
    int max_min = 0, max_nat = 0;
    for (const auto& toolbar_button : buttons_)
    {
      int child_min = 0, child_nat = 0, ignore_min = 0, ignore_nat = 0;
      toolbar_button.button->measure(Gtk::Orientation::VERTICAL, -1, child_min, child_nat, ignore_min, ignore_nat);
      max_min = std::max(max_min, child_min);
      max_nat = std::max(max_nat, child_nat);
    }
    int menu_min = 0, menu_nat = 0, ignore_min = 0, ignore_nat = 0;
    menu_button_.measure(Gtk::Orientation::VERTICAL, -1, menu_min, menu_nat, ignore_min, ignore_nat);
    minimum = std::max(max_min, menu_min);
    natural = std::max(max_nat, menu_nat);
  }
}

void OverflowToolbar::populate_overflow_menu(size_t first_hidden_index)
{
  overflow_menu_->remove_all();
  for (size_t i = first_hidden_index; i < buttons_.size(); ++i)
  {
    const auto& toolbar_button = buttons_[i];
    if (toolbar_button.action_name.empty())
      continue;

    // The menu item is driven by its action, which the caller keeps in sync with the
    // button sensitivity, so a disabled action shows up greyed out here automatically.
    auto menu_item = Gio::MenuItem::create(toolbar_button.label, toolbar_button.action_name);
    if (!toolbar_button.icon_name.empty())
      menu_item->set_attribute_value("icon", Glib::Variant<Glib::ustring>::create(toolbar_button.icon_name));
    overflow_menu_->append_item(menu_item);
  }
}

void OverflowToolbar::size_allocate_vfunc(int width, int height, int baseline)
{
  // Measure the overflow menu button once; we may need to reserve room for it
  int menu_width = 0, menu_nat = 0, ignore_min = 0, ignore_nat = 0;
  menu_button_.measure(Gtk::Orientation::HORIZONTAL, -1, menu_width, menu_nat, ignore_min, ignore_nat);
  menu_width = menu_nat;

  // Pre-measure every button so we can greedily decide which ones fit
  std::vector<int> button_widths(buttons_.size(), 0);
  int total_natural = 0;
  for (size_t i = 0; i < buttons_.size(); ++i)
  {
    button_widths[i] = measure_button_width(buttons_[i]);
    total_natural += button_widths[i] + spacing_;
  }

  // Determine how many leading buttons fit. If everything fits we don't need the menu button at all.
  size_t visible_count = buttons_.size();
  bool needs_overflow = total_natural > width;
  if (needs_overflow)
  {
    int budget = width - menu_width - spacing_; // reserve room for the trailing menu button
    int used = 0;
    visible_count = 0;
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
      int next = used + button_widths[i] + spacing_;
      if (next > budget && visible_count > 0)
        break;
      used = next;
      visible_count++;
    }
  }

  // Show/hide buttons and fill the overflow menu with the remainder
  for (size_t i = 0; i < buttons_.size(); ++i)
    buttons_[i].button->set_child_visible(i < visible_count);
  menu_button_.set_child_visible(needs_overflow);
  if (needs_overflow)
    populate_overflow_menu(visible_count);

  // Allocate the visible buttons left to right on a single row
  int x = 0;
  for (size_t i = 0; i < visible_count; ++i)
  {
    Gtk::Allocation allocation(x, 0, button_widths[i], height);
    buttons_[i].button->size_allocate(allocation, baseline);
    x += button_widths[i] + spacing_;
  }

  // Allocate the overflow menu button flush to the right edge
  if (needs_overflow)
  {
    int menu_x = std::max(x, width - menu_width);
    Gtk::Allocation allocation(menu_x, 0, menu_width, height);
    menu_button_.size_allocate(allocation, baseline);
  }
}
