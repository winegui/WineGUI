/**
 * Copyright (c) 2020-2021 WineGUI
 *
 * \file    busy_dialog.cc
 * \brief   GTK+ Busy dialog
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
#include "busy_dialog.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
BusyDialog::BusyDialog(Gtk::Window& parent) : Gtk::Dialog("Applying Changes"), defaultParent(parent)
{
  set_transient_for(parent);
  set_default_size(400, 120);
  set_modal(true);
  set_deletable(false);

  heading_label.set_markup("<big><b>Installing software</b></big>");
  heading_label.set_alignment(0.0);
  message_label.set_alignment(0.0);
  loading_bar.set_pulse_step(0.3);

  Gtk::Box* box = get_vbox();
  box->set_margin_top(10);
  box->set_margin_right(10);
  box->set_margin_bottom(10);
  box->set_margin_left(10);

  box->pack_start(heading_label, false, false);
  box->pack_start(message_label, true, false);
  box->pack_start(loading_bar, true, false);

  show_all_children();
}

/**
 * \brief Destructor
 */
BusyDialog::~BusyDialog()
{
}

/**
 * \brief Set busy message
 * \param[in] message - Message
 */
void BusyDialog::SetMessage(const Glib::ustring& message)
{
  this->message_label.set_text(message + " Please wait...");
}

/**
 * \brief Show the busy dialog (override the show(), calls parent show())
 */
void BusyDialog::show()
{
  if (!timer.empty() && timer.connected())
  {
    timer.disconnect();
  }

  int time_interval = 200;
  timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &BusyDialog::Pulsing), time_interval);
  Gtk::Dialog::show();
}

/**
 * \brief Close the busy dialog (override the close(), calls parent close())
 */
void BusyDialog::close()
{
  // Reset default parent
  set_transient_for(defaultParent);

  // Stop pulsing timer
  if (!timer.empty() && timer.connected())
  {
    timer.disconnect();
  }
  Gtk::Dialog::close();
}

/**
 * \brief Trigger the loading bar, until timer is disconnected
 */
bool BusyDialog::Pulsing()
{
  loading_bar.pulse();
  return true;
}
