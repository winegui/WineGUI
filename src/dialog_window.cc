/**
 * Copyright (c) 2025 WineGUI
 *
 * \file    dialog_window.cc
 * \brief   Dialog window
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
#include "dialog_window.h"

/**
 * Constructor, creates a dialog window with a title, message, icon and an OK button.
 * Important: In case of a dialog type QUESTION the dialog will close (destroy) itself.
 *
 * \param[in] parent - The parent window
 * \param[in] type - The dialog type (if type is QUESTION, only yes/no buttons are shown)
 * \param[in] message - The message to display (default is empty string)
 * \param[in] markup - Whether to use markup (default is false)
 */
DialogWindow::DialogWindow(Gtk::Window& parent, DialogType type, const Glib::ustring& message, bool markup)
    : vbox(Gtk::Orientation::VERTICAL, 12),
      hbox_icon_and_text(Gtk::Orientation::HORIZONTAL, 18),
      hbox_buttons(Gtk::Orientation::HORIZONTAL, 6),
      text_vbox(Gtk::Orientation::VERTICAL, 6),
      ok_button("OK"),
      yes_button("Yes"),
      no_button("No"),
      type_(type)
{
  set_default_size(520, 180);

  set_message(message, markup);
  set_transient_for(parent);
  set_modal(true);

  set_resizable(false);
  set_deletable(true);

  vbox.set_margin(6);
  icon.set_pixel_size(100);
  icon.set_halign(Gtk::Align::CENTER);
  icon.set_valign(Gtk::Align::CENTER);

  message_text.set_xalign(0.0);
  message_text.set_wrap(true);
  message_text.set_wrap_mode(Pango::WrapMode::WORD_CHAR);
  message_text.set_valign(Gtk::Align::CENTER);
  message_text.set_halign(Gtk::Align::FILL);
  message_text.set_hexpand(true);
  message_text.set_vexpand(true);

  text_vbox.append(message_text);
  text_vbox.set_halign(Gtk::Align::FILL);

  hbox_icon_and_text.append(icon);
  hbox_icon_and_text.append(text_vbox);

  vbox.append(hbox_icon_and_text);
  vbox.append(hbox_buttons);

  ok_button.set_hexpand(true);
  ok_button.set_halign(Gtk::Align::FILL);
  ok_button.set_margin(2);
  yes_button.set_hexpand(true);
  yes_button.set_halign(Gtk::Align::FILL);
  yes_button.set_margin(2);
  no_button.set_hexpand(true);
  no_button.set_halign(Gtk::Align::FILL);
  no_button.set_margin(2);

  update_title_and_icon_and_button_();

  set_child(vbox);

  // Signals
  ok_button.signal_clicked().connect(sigc::mem_fun(*this, &DialogWindow::on_ok_button_clicked));
  yes_button.signal_clicked().connect(sigc::mem_fun(*this, &DialogWindow::on_yes_button_clicked));
  no_button.signal_clicked().connect(sigc::mem_fun(*this, &DialogWindow::on_no_button_clicked));

  // Hide window instead of destroy
  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        return true; // stop default destroy
      },
      false);
}

DialogWindow::~DialogWindow()
{
}

/**
 * Set the message text and markup.
 *
 * \param[in] message - The message text
 * \param[in] markup - Whether to use markup
 */
void DialogWindow::set_message(const Glib::ustring& message, bool markup)
{
  if (markup)
  {
    message_text.set_markup(message);
  }
  else
  {
    message_text.set_text(message);
  }
}

/**
 * Update the title, icon and button(s) based on the type.
 */
void DialogWindow::update_title_and_icon_and_button_()
{
  const char* icon_name = "dialog-information";
  const char* title = "Information";
  bool show_ok_button = true; // if false show yes/no buttons

  switch (type_)
  {
  case DialogType::INFO:
    icon_name = "dialog-information";
    title = "Information message";
    break;
  case DialogType::WARNING:
    icon_name = "dialog-warning";
    title = "Warning message";
    break;
  case DialogType::ERROR:
    icon_name = "dialog-error";
    title = "Error message";
    break;
  case DialogType::QUESTION:
    icon_name = "dialog-question";
    title = "Are you sure?";
    show_ok_button = false;
    break;
  default:
    break;
  }

  icon.set_from_icon_name(icon_name);

  set_title(title);

  // Set the button(s) depending on the type
  if (show_ok_button)
  {
    hbox_buttons.append(ok_button);
  }
  else
  {
    hbox_buttons.append(yes_button);
    hbox_buttons.append(no_button);
  }
}

void DialogWindow::on_ok_button_clicked()
{
  set_visible(false); // hide instead of destroy in case of OK button
  signal_response.emit(ResponseType::OK);
}

void DialogWindow::on_yes_button_clicked()
{
  close(); // Question dialog will destroy itself
  signal_response.emit(ResponseType::YES);
}

void DialogWindow::on_no_button_clicked()
{
  close(); // Question dialog will destroy itself
  signal_response.emit(ResponseType::NO);
}
