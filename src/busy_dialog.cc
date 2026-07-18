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

/**
 * \brief Constructor
 * \param parent Reference to parent GTK+ Window
 */
BusyDialog::BusyDialog(Gtk::Window& parent) : Gtk::Window(), default_parent_(parent)
{
  set_title("Applying Changes...");
  set_transient_for(parent);
  set_modal(true);
  set_resizable(false);
  set_deletable(false);
  set_default_size(400, 140);

  heading_label.set_xalign(0.0);
  message_label.set_xalign(0.0);
  message_label.set_halign(Gtk::Align::START);
  message_label.set_hexpand(true);
  loading_bar.set_pulse_step(0.3);
  loading_bar.set_hexpand(true);

  cancel_button.set_label("Cancel");
  cancel_button.set_halign(Gtk::Align::END);
  cancel_button.set_visible(false);
  cancel_button.signal_clicked().connect([this]() { cancel_requested.emit(); });

  Gtk::Box* vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  vbox->set_margin_top(10);
  vbox->set_margin_start(10);
  vbox->set_margin_bottom(10);
  vbox->set_margin_end(10);

  vbox->append(heading_label);
  vbox->append(message_label);
  vbox->append(loading_bar);
  vbox->append(cancel_button);
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
  this->message_label.set_text(message.empty() ? "Please wait..." : message + "\nPlease wait...");
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
  // Reset default parent
  set_transient_for(default_parent_);

  // Reset the progress bar back to the default indeterminate (pulsing) mode
  is_determinate_ = false;
  loading_bar.set_fraction(0.0);
  loading_bar.set_show_text(false);

  // Stop pulsing timer
  if (!timer_.empty() && timer_.connected())
  {
    timer_.disconnect();
  }

  // Dispatch the set_visible in the main thread, to avoid weird behavior when the dialog is open and closed too fast.
  // Typical GTK non sense issues.
  Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(*this, &BusyDialog::set_visible), false), Glib::PRIORITY_DEFAULT_IDLE);
}

/**
 * \brief Show a real progress fraction (stops the pulsing until set_pulsing() or hide() is called)
 * \param[in] fraction Progress fraction between 0.0 and 1.0
 * \param[in] progress_text (Optionally) Progress text shown inside the progress bar (eg. "50 of 100 MB")
 */
void BusyDialog::set_progress(double fraction, const Glib::ustring& progress_text)
{
  is_determinate_ = true;
  loading_bar.set_fraction(fraction);
  if (!progress_text.empty())
  {
    loading_bar.set_show_text(true);
    loading_bar.set_text(progress_text);
  }
}

/**
 * \brief Switch (back) to an indeterminate pulsing progress bar
 */
void BusyDialog::set_pulsing()
{
  is_determinate_ = false;
  loading_bar.set_show_text(false);
}

/**
 * \brief Show or hide the cancel button (hidden by default).
 * When clicked the cancel_requested signal is emitted, the dialog stays open (hide() it yourself).
 * \param[in] cancelable True to show the cancel button
 */
void BusyDialog::set_cancelable(bool cancelable)
{
  cancel_button.set_visible(cancelable);
}

/**
 * \brief Trigger the loading bar,
 * until timer is disconnected.
 */
bool BusyDialog::pulsing()
{
  if (!is_determinate_)
  {
    loading_bar.pulse();
  }
  return true; // Keep pulsing, util timer disconnect
}
