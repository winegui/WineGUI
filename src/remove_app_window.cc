/**
 * Copyright (c) 2023-2025 WineGUI
 *
 * \file    remove_app_window.cc
 * \brief   Remove application shortcut for app list GTK3 window class
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
#include "remove_app_window.h"
#include "bottle_config_file.h"
#include "bottle_item.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
RemoveAppWindow::RemoveAppWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_remove_app_label("Remove Application shortcut(s)"),
      header_remove_description_label("Select one or more shortcuts you want to remove,\n then press the \"Remove selected\" button."),
      select_all_button("Select all"),
      unselect_all_button("Unselect all"),
      remove_selected_button("Removed selected"),
      cancel_button("Cancel"),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_title("Remove Application shortcut(s)");
  set_default_size(400, 450);
  set_modal(true);
  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_remove_app_label.set_attributes(attr_list_header_label);
  header_remove_app_label.set_margin_top(5);
  header_remove_app_label.set_margin_bottom(5);

  // TODO: Use treeview with ApplicationData as model objects?
  // So we can also use the map key as index?
  app_list_box.set_margin_top(5);
  app_list_box.set_margin_end(5);
  app_list_box.set_margin_bottom(6);
  app_list_box.set_margin_start(6);
  app_list_box.set_can_focus(false);
  app_list_box.set_selection_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);

  select_all_button.set_margin_start(5);
  select_all_button.set_margin_end(5);
  unselect_all_button.set_margin_start(5);
  unselect_all_button.set_margin_end(5);

  hbox_buttons.pack_end(remove_selected_button, false, false, 4);
  hbox_buttons.pack_end(cancel_button, false, false, 4);

  vbox.pack_start(header_remove_app_label, false, false, 4);
  vbox.pack_start(header_remove_description_label, false, true, 4);
  vbox.pack_start(app_list_box, true, true, 4);
  vbox.pack_start(select_all_button, false, true, 0);
  vbox.pack_start(unselect_all_button, false, true, 0);
  vbox.pack_start(hbox_buttons, false, false, 4);
  add(vbox);

  // Signals
  select_all_button.signal_clicked().connect(sigc::mem_fun(app_list_box, &Gtk::ListBox::select_all));
  unselect_all_button.signal_clicked().connect(sigc::mem_fun(app_list_box, &Gtk::ListBox::unselect_all));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &RemoveAppWindow::on_cancel_button_clicked));
  remove_selected_button.signal_clicked().connect(sigc::mem_fun(*this, &RemoveAppWindow::on_remove_selected_button_clicked));

  show_all_children();
}

/**
 * \brief Destructor
 */
RemoveAppWindow::~RemoveAppWindow()
{
}

/**
 * \brief Override show, which will load the app list
 */
void RemoveAppWindow::show()
{
  // Clean-up
  for (const auto& app_item : app_list_box.get_children())
  {
    delete app_item;
  }

  if (active_bottle_ != nullptr)
  {
    std::string prefix_path = active_bottle_->wine_location();
    // Read existing config data
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> app_list;
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);
    for (const auto& [_, app_data] : app_list)
    {
      std::string label_name = (!app_data.description.empty()) ? app_data.name + " - " + app_data.description : app_data.name;
      Gtk::Label* label = Gtk::manage(new Gtk::Label(label_name));
      app_list_box.add(*label);
    }
    show_all_children();
  }
  // Call parent show
  Gtk::Widget::show();
  // Unselect items by default
  app_list_box.unselect_all();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle Current active bottle
 */
void RemoveAppWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void RemoveAppWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Triggered when cancel button is clicked
 */
void RemoveAppWindow::on_cancel_button_clicked()
{
  hide();
}

/**
 * \brief Triggered when remove selected button is clicked
 */
void RemoveAppWindow::on_remove_selected_button_clicked()
{
  if (active_bottle_ != nullptr)
  {
    std::string prefix_path = active_bottle_->wine_location();
    // Read existing config data
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> app_list;
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);
    std::vector<Gtk::ListBoxRow*> selected_for_removal = app_list_box.get_selected_rows();
    if (selected_for_removal.size() > 0)
    {
      for (const auto& app : selected_for_removal)
      {
        app_list.erase(app->get_index());
      }

      // Save application to bottle config
      if (!BottleConfigFile::write_config_file(prefix_path, bottle_config, app_list))
      {
        Gtk::MessageDialog dialog(*this, "Error occurred during saving bottle config file.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        dialog.set_title("An error has occurred!");
        dialog.set_modal(true);
        dialog.run();
      }
      else
      {
        // Hide remove application window
        hide();
        // Trigger manager update & UI update
        config_saved.emit();
      }
    }
    else
    {
      Gtk::MessageDialog dialog(*this, "You have not selected anything to remove. Select one or more shortcuts or press the cancel button.", false,
                                Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
      dialog.set_title("Nothing selected?");
      dialog.set_modal(true);
      dialog.run();
    }
  }
  else
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving, because there is no active Windows machine set.", false, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
    dialog.set_title("Error during remove application saving");
    dialog.set_modal(true);
    dialog.run();
    std::cout << "Error: No current Windows machine is set. Change won't be saved." << std::endl;
  }
}
