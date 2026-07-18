/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    busy_dialog.h
 * \brief   Busy Dialog (showing a loading process bar)
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

using std::string;

/**
 * \class BusyDialog
 * \brief GTK Window class for the busy dialog, which shows a loading bar and a message
 */
class BusyDialog : public Gtk::Window
{
public:
  sigc::signal<void()> cancel_requested; /*!< Signal when the user clicked the cancel button */

  explicit BusyDialog(Gtk::Window& parent);
  virtual ~BusyDialog();

  void present();
  void hide(); /**< Hide the busy dialog (stop the timer and calls parent set_visible()) */

  void set_message(const Glib::ustring& heading_text, const Glib::ustring& message);
  void set_progress(double fraction, const Glib::ustring& progress_text = ""); /**< Switch to a determinate progress bar */
  void set_pulsing();                                                          /**< Switch (back) to an indeterminate pulsing progress bar */
  void set_cancelable(bool cancelable);                                        /**< Show or hide the cancel button (hidden by default) */

protected:
  Gtk::Label heading_label;     /*!< Heading label */
  Gtk::Label message_label;     /*!< Message box label */
  Gtk::ProgressBar loading_bar; /*!< Loading bar */
  Gtk::Button cancel_button;    /*!< Optional cancel button (see set_cancelable()) */

private:
  sigc::connection timer_; /*!< Timer connection */
  Gtk::Window& default_parent_;
  bool is_determinate_ = false; /*!< True when a real progress fraction is shown (no pulsing) */

  virtual bool pulsing();
};
