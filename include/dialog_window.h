/**
 * Copyright (c) 2025 WineGUI
 *
 * \file    dialog_window.h
 * \brief   Dialog window (replacement for deprecated Gtk::Dialog)
 * \author  Melroy van den Berg
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

/**
 * \class DialogWindow
 * \brief GTK window class to show a dialog with title, message, icon and an OK button.
 */
class DialogWindow : public Gtk::Window
{
public:
  enum class DialogType
  {
    INFO,
    WARNING,
    ERROR,
    QUESTION
  };

  enum class ResponseType
  {
    OK,
    YES,
    NO
  };

  sigc::signal<void(ResponseType)> signal_response;

  explicit DialogWindow(Gtk::Window& parent, DialogType type, const Glib::ustring& message = "", bool markup = false);
  virtual ~DialogWindow();

  void set_message(const Glib::ustring& message, bool markup = false);

protected:
  Gtk::Box vbox;
  Gtk::Box hbox_icon_and_text;
  Gtk::Box hbox_buttons;

  Gtk::Image icon;
  Gtk::Box text_vbox;
  Gtk::Label message_text;

  Gtk::Button ok_button;
  Gtk::Button yes_button;
  Gtk::Button no_button;

private:
  DialogType type_ = DialogType::INFO;

  // Signal handlers
  void on_ok_button_clicked();
  void on_yes_button_clicked();
  void on_no_button_clicked();

  void update_title_and_icon_and_button_();
};
