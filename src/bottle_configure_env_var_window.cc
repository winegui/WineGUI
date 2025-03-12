/**
 * Copyright (c) 2024-2025 WineGUI
 *
 * \file    bottle_configure_env_var_window.cc
 * \brief   Configure bottle environment variables GTK3 Window class
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
#include "bottle_configure_env_var_window.h"
#include "bottle_config_file.h"
#include "bottle_item.h"
#include <iostream>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleConfigureEnvVarWindow::BottleConfigureEnvVarWindow(Gtk::Window& parent)
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      hbox_2_buttons(Gtk::ORIENTATION_HORIZONTAL, 4),
      header_configure_env_var_label("Configure Environment Variables"),
      environment_variables_label("Current environment variables set for this machine:"),
      add_button("Add"),
      remove_button("Remove"),
      save_button("Save"),
      cancel_button("Cancel"),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_title("Configure Environment Variables");
  set_default_size(500, 500);
  set_modal(true);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::WEIGHT_BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_configure_env_var_label.set_attributes(attr_list_header_label);
  header_configure_env_var_label.set_margin_top(5);
  header_configure_env_var_label.set_margin_bottom(5);

  environment_variables_label.set_halign(Gtk::Align::ALIGN_START);
  environment_variables_label.set_margin_start(6);

  // Horizontal buttons
  hbox_buttons.set_homogeneous(true);
  hbox_buttons.pack_end(remove_button, false, true, 4);
  hbox_buttons.pack_end(add_button, false, true, 4);
  hbox_buttons.set_margin_bottom(12);
  hbox_2_buttons.pack_end(save_button, false, false, 4);
  hbox_2_buttons.pack_end(cancel_button, false, false, 4);

  // Add treeview to a scrolled window
  m_ScrolledWindow.add(m_TreeView);
  m_ScrolledWindow.set_margin_start(6);
  m_ScrolledWindow.set_margin_end(6);

  vbox.pack_start(header_configure_env_var_label, false, false, 4);
  vbox.pack_start(environment_variables_label, false, false, 4);
  vbox.pack_start(m_ScrolledWindow, true, true, 4);
  vbox.pack_start(hbox_buttons, false, true, 4);
  vbox.pack_start(hbox_2_buttons, false, false, 4);

  add(vbox);

  // Create the Tree model
  m_refTreeModel = Gtk::ListStore::create(m_Columns);
  m_TreeView.set_model(m_refTreeModel);

  // Add the TreeView's view columns:
  m_TreeView.append_column_editable("Name", m_Columns.m_col_name);
  m_TreeView.append_column_editable("Value", m_Columns.m_col_value);
  m_TreeView.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_SINGLE);

  m_TreeView.get_column(0)->set_min_width(200);
  m_TreeView.set_resize_mode(Gtk::ResizeMode::RESIZE_IMMEDIATE);

  // Signals
  add_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_add_button_clicked));
  remove_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_remove_button_clicked));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_save_button_clicked));
  // On show signal, load the environment variables from the config file
  signal_show().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::load_environment_variables_from_config));

  show_all_children();
}

/**
 * \brief Destructor
 */
BottleConfigureEnvVarWindow::~BottleConfigureEnvVarWindow()
{
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle Current active bottle
 */
void BottleConfigureEnvVarWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleConfigureEnvVarWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

void BottleConfigureEnvVarWindow::load_environment_variables_from_config()
{
  // Clear the treeview
  m_refTreeModel->clear();

  if (active_bottle_ != nullptr)
  {
    std::string prefix_path = active_bottle_->wine_location();
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> app_list;
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);

    for (const auto& [name, value] : bottle_config.env_vars)
    {
      Gtk::TreeModel::Row row = *(m_refTreeModel->append());
      row[m_Columns.m_col_name] = name;
      row[m_Columns.m_col_value] = value;
    }
  }
}

void BottleConfigureEnvVarWindow::on_add_button_clicked()
{
  Gtk::TreeModel::Row row = *(m_refTreeModel->append());
  row[m_Columns.m_col_name] = ""; // Empty placeholder
  row[m_Columns.m_col_value] = "";

  // Move cursor to the new row
  Gtk::TreeModel::Path path = m_refTreeModel->get_path(row);
  Gtk::TreeViewColumn* column = m_TreeView.get_column(0);
  m_TreeView.scroll_to_row(path, 0);
  m_TreeView.set_cursor(path, *column, true);
}

void BottleConfigureEnvVarWindow::on_remove_button_clicked()
{
  Glib::RefPtr<Gtk::TreeSelection> refSelection = m_TreeView.get_selection();

  // Get the selected row iterator
  Gtk::TreeModel::iterator iter = refSelection->get_selected();

  // Check if a row is selected
  if (iter)
  {
    // Remove the selected row from the TreeModel
    m_refTreeModel->erase(iter);
  }
}

/**
 * \brief Triggered when cancel button is clicked
 */
void BottleConfigureEnvVarWindow::on_cancel_button_clicked()
{
  hide();
}

/**
 * \brief Triggered when save button is clicked
 */
void BottleConfigureEnvVarWindow::on_save_button_clicked()
{
  if (active_bottle_ != nullptr)
  {
    std::string prefix_path = active_bottle_->wine_location();
    // Read existing config data
    BottleConfigData bottle_config;
    std::vector<std::pair<std::string, std::string>> env_vars;
    env_vars.reserve(2);
    std::map<int, ApplicationData> app_list;
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);

    // Get all items from the treeview
    Gtk::TreeModel::Children children = m_refTreeModel->children();
    for (const auto& child : children)
    {
      std::string name = child.get_value(m_Columns.m_col_name);
      std::string value = child.get_value(m_Columns.m_col_value);
      if (name.empty() || value.empty())
      {
        continue; // Skip empty rows
      }
      env_vars.emplace_back(std::make_pair(name, value));
    }

    // Set (override existing) env vars
    bottle_config.env_vars = env_vars;

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
      hide();

      // Trigger update config signal (so the bottle config file will be re-read)
      config_saved.emit();
    }
  }
  else
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving, because there is no active Windows machine set.", false, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
    dialog.set_title("Error during new application saving");
    dialog.set_modal(true);
    dialog.run();
    std::cout << "Error: No current Windows machine is set. Change won't be saved." << std::endl;
  }
}
