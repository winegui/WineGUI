/**
 * Copyright (c) 2020-2025 WineGUI
 *
 * \file    bottle_clone_window.cc
 * \brief   Wine bottle clone window
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
#include "bottle_clone_window.h"
#include "bottle_item.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleCloneWindow::BottleCloneWindow(Gtk::Window& parent)
    : vbox(Gtk::Orientation::VERTICAL, 4),
      hbox_buttons(Gtk::Orientation::HORIZONTAL, 4),
      header_clone_label("Clone Existing Machine"),
      name_label("New Name: "),
      folder_name_label("New Folder Name: "),
      description_label("New Description:"),
      clone_button("Clone"),
      cancel_button("Cancel"),
      busy_dialog(*this),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_default_size(400, 250);
  set_modal(true);

  clone_grid.set_margin_top(5);
  clone_grid.set_margin_end(5);
  clone_grid.set_margin_bottom(6);
  clone_grid.set_margin_start(6);
  clone_grid.set_column_spacing(6);
  clone_grid.set_row_spacing(8);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_clone_label.set_attributes(attr_list_header_label);
  header_clone_label.set_margin_top(5);
  header_clone_label.set_margin_bottom(5);

  name_label.set_halign(Gtk::Align::END);
  folder_name_label.set_halign(Gtk::Align::END);
  name_label.set_tooltip_text("New name of the machine");
  folder_name_label.set_tooltip_text("Do NOT keep this the same as the original machine folder (or a copy will not work)");

  description_label.set_halign(Gtk::Align::START);
  name_entry.set_hexpand(true);
  folder_name_entry.set_hexpand(true);

  description_text_view.set_hexpand(true);
  description_text_view.set_vexpand(true);
  description_text_view.set_wrap_mode(Gtk::WrapMode::WORD_CHAR);
  description_text_view.set_halign(Gtk::Align::FILL);
  description_label.set_tooltip_text("Optional new description text to your machine");

  description_scrolled_window.set_child(description_text_view);
  description_scrolled_window.set_hexpand(true);
  description_scrolled_window.set_vexpand(true);

  clone_grid.attach(name_label, 0, 0);
  clone_grid.attach(name_entry, 1, 0);
  clone_grid.attach(folder_name_label, 0, 1);
  clone_grid.attach(folder_name_entry, 1, 1);
  clone_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, 8, 2);
  clone_grid.attach(description_label, 0, 9, 2);
  clone_grid.attach(description_scrolled_window, 0, 10, 2);
  clone_grid.set_hexpand(true);
  clone_grid.set_vexpand(true);
  clone_grid.set_halign(Gtk::Align::FILL);
  clone_grid.set_valign(Gtk::Align::FILL);
  clone_grid.set_margin_bottom(5);

  hbox_buttons.set_halign(Gtk::Align::END);
  hbox_buttons.set_margin(6);
  hbox_buttons.append(clone_button);
  hbox_buttons.append(cancel_button);

  vbox.append(header_clone_label);
  vbox.append(clone_grid);
  vbox.append(hbox_buttons);
  set_child(vbox);

  // Signals
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleCloneWindow::on_cancel_button_clicked));
  clone_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleCloneWindow::on_clone_button_clicked));
  // Hide window instead of destroy
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
BottleCloneWindow::~BottleCloneWindow()
{
}

/**
 * \brief Same as show() but will also update the Window title, set name, folder name and description
 */
void BottleCloneWindow::show()
{
  if (active_bottle_ != nullptr)
  {
    set_title("Clone Machine - " +
              ((!active_bottle_->name().empty()) ? active_bottle_->name() : active_bottle_->folder_name())); // Fallback to folder name
    // Enable save button (again)
    clone_button.set_sensitive(true);

    // Set name
    name_entry.set_text(active_bottle_->name() + " (copy)");
    // Set folder name
    folder_name_entry.set_text(active_bottle_->folder_name() + "_copy");
    // Set description
    description_text_view.get_buffer()->set_text(active_bottle_->description());
  }
  else
  {
    set_title("Clone Machine (Unknown machine)");
  }
  // Call parent present
  present();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void BottleCloneWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleCloneWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Handler when the bottle is cloned.
 * \return The name of the cloned bottle
 */
Glib::ustring BottleCloneWindow::on_bottle_cloned()
{
  busy_dialog.hide();
  set_visible(false); // Hide the clone Window
  return name_entry.get_text();
}

/**
 * \brief Triggered when cancel button is clicked
 */
void BottleCloneWindow::on_cancel_button_clicked()
{
  set_visible(false);
}

/**
 * \brief Triggered when clone button is clicked
 */
void BottleCloneWindow::on_clone_button_clicked()
{
  // First disable save button (avoid multiple presses)
  clone_button.set_sensitive(false);

  CloneBottleStruct clone_bottle_struct;

  // Show busy dialog
  busy_dialog.set_message("Clone Windows Machine",
                          "Currently cloning the Windows Machine.\nThis can take a while, depending on the size of the machine.");
  busy_dialog.present();

  // Set the new bottle configuration data for the clone
  clone_bottle_struct.name = name_entry.get_text();
  clone_bottle_struct.folder_name = folder_name_entry.get_text();
  clone_bottle_struct.description = description_text_view.get_buffer()->get_text();
  clone_bottle.emit(clone_bottle_struct);
}
