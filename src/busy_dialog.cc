/**
 * Copyright (c) 2020-2025 WineGUI
 *
 * \file    busy_dialog.cc
 * \brief   Busy Dialog (showing a loading process bar)
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
#include "busy_dialog.h"
#include "gtkmm/window.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
BusyDialog::BusyDialog(Gtk::Window& parent) : Gtk::Window(), default_parent_(parent)
{
  set_title("Applying Changes...");
  set_transient_for(parent);
  set_default_size(400, 120);
  set_modal(true);
  set_deletable(false);

  heading_label.set_xalign(0.0);
  message_label.set_xalign(0.0);
  message_label.set_hexpand(true);
  loading_bar.set_pulse_step(0.3);
  loading_bar.set_hexpand(true);

  Gtk::Box* vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  vbox->set_margin_top(10);
  vbox->set_margin_start(10);
  vbox->set_margin_bottom(10);
  vbox->set_margin_end(10);

  vbox->append(heading_label);
  vbox->append(message_label);
  vbox->append(loading_bar);
  set_child(*vbox);

  // Hide window instead of destroy (although set deletable is set to false)
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
BusyDialog::~BusyDialog()
{
}

/**
 * \brief Set busy message
 * \param[in] heading_text Heading text (displayed above the message)
 * \param[in] message Message
 */
void BusyDialog::set_message(const Glib::ustring& heading_text, const Glib::ustring& message)
{
  this->heading_label.set_markup("<big><b>" + Glib::Markup::escape_text(heading_text) + "</b></big>");
  this->message_label.set_text(message + " Please wait...");
}

/**
 * \brief Present the busy dialog (override the present(), calls parent present())
 */
void BusyDialog::present()
{
  if (!timer_.empty() && timer_.connected())
  {
    timer_.disconnect();
  }

  int time_interval = 200;
  timer_ = Glib::signal_timeout().connect(sigc::mem_fun(*this, &BusyDialog::pulsing), time_interval);
  Gtk::Window::present();
}

/**
 * \brief Hide the busy dialog (stop the timer and calls parent set_visible())
 */
void BusyDialog::hide()
{
  std::cout << "Hiding busy dialog" << std::endl;
  // Reset default parent
  set_transient_for(default_parent_);

  // Stop pulsing timer
  if (!timer_.empty() && timer_.connected())
  {
    timer_.disconnect();
  }
  set_visible(false);
}

/**
 * \brief Trigger the loading bar,
 * until timer is disconnected.
 */
bool BusyDialog::pulsing()
{
  loading_bar.pulse();
  return true; // Keep pulsing, util timer disconnect
}
