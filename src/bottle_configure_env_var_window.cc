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
    : vbox(Gtk::Orientation::VERTICAL, 4),
      hbox_buttons(Gtk::Orientation::HORIZONTAL, 4),
      hbox_2_buttons(Gtk::Orientation::HORIZONTAL, 4),
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
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_configure_env_var_label.set_attributes(attr_list_header_label);
  header_configure_env_var_label.set_margin_top(5);
  header_configure_env_var_label.set_margin_bottom(5);

  environment_variables_label.set_halign(Gtk::Align::START);
  environment_variables_label.set_margin_start(6);

  remove_button.set_halign(Gtk::Align::FILL);
  remove_button.set_margin_bottom(5);
  add_button.set_halign(Gtk::Align::FILL);
  add_button.set_margin_bottom(5);
  // Add / remove buttons
  hbox_buttons.set_homogeneous(true);
  hbox_buttons.set_margin(6);
  hbox_buttons.set_margin_bottom(2);
  hbox_buttons.set_halign(Gtk::Align::FILL);
  hbox_buttons.append(add_button);
  hbox_buttons.append(remove_button);
  // Save / Cancel buttons
  hbox_2_buttons.append(save_button);
  hbox_2_buttons.append(cancel_button);
  hbox_2_buttons.set_halign(Gtk::Align::END);
  hbox_2_buttons.set_margin(6);

  // Add ColumnView to a scrolled window
  m_ScrolledWindow.set_child(m_ColumnView);
  m_ScrolledWindow.set_margin_start(6);
  m_ScrolledWindow.set_margin_end(6);
  m_ScrolledWindow.set_margin_bottom(6);
  m_ScrolledWindow.set_vexpand(true);
  m_ScrolledWindow.set_hexpand(true);
  m_ScrolledWindow.set_halign(Gtk::Align::FILL);
  m_ScrolledWindow.set_valign(Gtk::Align::FILL);

  vbox.append(header_configure_env_var_label);
  vbox.append(environment_variables_label);
  vbox.append(m_ScrolledWindow);
  vbox.append(hbox_buttons);
  vbox.append(hbox_2_buttons);

  set_child(vbox);

  // Create list model
  env_var_store_ = Gio::ListStore<EnvVarModelRow>::create();
  // Set list model and selection model
  env_var_selection_model_ = Gtk::SingleSelection::create(env_var_store_);
  env_var_selection_model_->set_autoselect(false);
  env_var_selection_model_->set_can_unselect(true);
  m_ColumnView.set_model(env_var_selection_model_);

  // Name column
  auto factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect(sigc::bind(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_setup_env_var_cell), true));
  factory->signal_bind().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::bind_name_cell));
  auto column = Gtk::ColumnViewColumn::create("Name", factory);
  column->set_expand(true);
  m_ColumnView.append_column(column);

  // Value column
  factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect(sigc::bind(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_setup_env_var_cell), false));
  factory->signal_bind().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::bind_value_cell));
  column = Gtk::ColumnViewColumn::create("Value", factory);
  column->set_expand(true);
  m_ColumnView.append_column(column);

  // Signals
  add_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_add_button_clicked));
  remove_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_remove_button_clicked));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::on_save_button_clicked));
  // On show signal, load the environment variables from the config file
  signal_show().connect(sigc::mem_fun(*this, &BottleConfigureEnvVarWindow::load_environment_variables_from_config));
  // Hide window instead of destroy
  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        return true; // stop default destroy
      },
      false);
}

void BottleConfigureEnvVarWindow::on_setup_env_var_cell(const Glib::RefPtr<Gtk::ListItem>& list_item, bool is_name)
{
  auto entry = Gtk::make_managed<Gtk::Entry>();
  entry->set_hexpand(true);
  entry->signal_changed().connect(
      [list_item, entry, is_name]()
      {
        const auto item = list_item->get_item();
        const auto row = std::dynamic_pointer_cast<EnvVarModelRow>(item);
        if (!row)
          return;
        if (is_name)
        {
          row->name = entry->get_text();
        }
        else
        {
          row->value = entry->get_text();
        }
      });
  list_item->set_child(*entry);
}

void BottleConfigureEnvVarWindow::bind_name_cell(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
  const auto item = list_item->get_item();
  const auto row = std::dynamic_pointer_cast<EnvVarModelRow>(item);
  auto* entry = dynamic_cast<Gtk::Entry*>(list_item->get_child());
  if (!entry)
    return;
  entry->set_text(row ? row->name : "");
}

void BottleConfigureEnvVarWindow::bind_value_cell(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
  const auto item = list_item->get_item();
  const auto row = std::dynamic_pointer_cast<EnvVarModelRow>(item);
  auto* entry = dynamic_cast<Gtk::Entry*>(list_item->get_child());
  if (!entry)
    return;
  entry->set_text(row ? row->value : "");
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
  // Clear list model
  env_var_store_->remove_all();

  if (active_bottle_ != nullptr)
  {
    std::string prefix_path = active_bottle_->wine_location();
    BottleConfigData bottle_config;
    std::map<int, ApplicationData> app_list;
    std::tie(bottle_config, app_list) = BottleConfigFile::read_config_file(prefix_path);

    for (const auto& [name, value] : bottle_config.env_vars)
    {
      env_var_store_->append(EnvVarModelRow::create(name, value));
    }
  }
}

void BottleConfigureEnvVarWindow::on_add_button_clicked()
{
  env_var_store_->append(EnvVarModelRow::create("", ""));
  const auto pos = env_var_store_->get_n_items();
  if (pos > 0)
  {
    env_var_selection_model_->set_selected(pos - 1);
  }
}

void BottleConfigureEnvVarWindow::on_remove_button_clicked()
{
  const auto selected = env_var_selection_model_->get_selected();
  if (selected != static_cast<guint>(-1))
  {
    env_var_store_->remove(selected);
  }
}

/**
 * \brief Triggered when cancel button is clicked
 */
void BottleConfigureEnvVarWindow::on_cancel_button_clicked()
{
  set_visible(false);
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

    // Get all items from the model
    const auto n_items = env_var_store_->get_n_items();
    for (guint i = 0; i < n_items; i++)
    {
      auto row = env_var_store_->get_item(i);
      if (!row)
        continue;

      const std::string name = row->name;
      const std::string value = row->value;
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
      Gtk::MessageDialog dialog(*this, "Error occurred during saving bottle config file.", false, Gtk::MessageType::ERROR, Gtk::ButtonsType::OK);
      dialog.set_title("An error has occurred!");
      dialog.set_modal(true);
      dialog.present();
    }
    else
    {
      set_visible(false); // Hide the window

      // Trigger update config signal (so the bottle config file will be re-read)
      config_saved.emit();
    }
  }
  else
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving, because there is no active Windows machine set.", false, Gtk::MessageType::ERROR,
                              Gtk::ButtonsType::OK);
    dialog.set_title("Error during new application saving");
    dialog.set_modal(true);
    dialog.present();
    std::cout << "Error: No current Windows machine is set. Change won't be saved." << std::endl;
  }
}
