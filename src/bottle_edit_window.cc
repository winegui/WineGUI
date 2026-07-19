/**
 * Copyright (c) 2020-2025 WineGUI
 *
 * \file    bottle_edit_window.cc
 * \brief   Wine bottle edit window
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
#include "bottle_edit_window.h"
#include "bottle_item.h"
#include "wine_defaults.h"
#include "wine_runner_manager.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
BottleEditWindow::BottleEditWindow(Gtk::Window& parent)
    : vbox(Gtk::Orientation::VERTICAL, 4),
      hbox_buttons(Gtk::Orientation::HORIZONTAL, 4),
      header_edit_label("Edit Machine"),
      name_label("Name: "),
      folder_name_label("Folder Name: "),
      wine_runner_label("Wine Runner: "),
      wine_bin_path_label("Wine Binary Path: "),
      windows_version_label("Windows Version: "),
      audio_driver_label("Audio Driver:"),
      virtual_desktop_resolution_label("Window Resolution:"),
      log_level_label("Log Level:"),
      description_label("Description:"),
      environment_variables_label("Environment Variables:"),
      hud_label("Performance Overlay:"),
      virtual_desktop_check("Enable Virtual Desktop Window"),
      enable_logging_check("Enable debug logging"),
      use_wine64_check("Prefer the wine64 binary instead of wine (advanced)"),
      hbox_hud_checks(Gtk::Orientation::HORIZONTAL, 12),
      dxvk_hud_check("DXVK HUD"),
      gallium_hud_check("Gallium HUD"),
      mangohud_check("MangoHud"),
      configure_environment_variables_button("Configure Environment Variables"),
      manage_runners_button("Manage..."),
      wine_bin_path_button("Select folder..."),
      save_button("Save"),
      cancel_button("Cancel"),
      delete_button("Delete Machine"),
      busy_dialog(*this),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_default_size(540, 540);
  set_modal(true);

  edit_grid.set_margin_top(5);
  edit_grid.set_margin_end(5);
  edit_grid.set_margin_bottom(6);
  edit_grid.set_margin_start(6);
  edit_grid.set_column_spacing(6);
  edit_grid.set_row_spacing(8);

  create_layout();

  // Signals
  wine_bin_path_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleEditWindow::on_select_wine_bin_path));
  configure_environment_variables_button.signal_clicked().connect(configure_environment_variables);
  delete_button.signal_clicked().connect(sigc::bind(remove_bottle, this));
  manage_runners_button.signal_clicked().connect(sigc::bind(manage_runners, this));
  wine_runner_combobox.signal_changed().connect(sigc::mem_fun(*this, &BottleEditWindow::on_wine_runner_changed));
  virtual_desktop_check.signal_toggled().connect(sigc::mem_fun(*this, &BottleEditWindow::on_virtual_desktop_toggle));
  enable_logging_check.signal_toggled().connect(sigc::mem_fun(*this, &BottleEditWindow::on_debug_logging_toggle));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleEditWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &BottleEditWindow::on_save_button_clicked));
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
BottleEditWindow::~BottleEditWindow()
{
}

void BottleEditWindow::create_layout()
{
  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_edit_label.set_attributes(attr_list_header_label);
  header_edit_label.set_margin_top(5);
  header_edit_label.set_margin_bottom(5);

  name_label.set_halign(Gtk::Align::END);
  folder_name_label.set_halign(Gtk::Align::END);
  wine_runner_label.set_halign(Gtk::Align::END);
  wine_bin_path_label.set_halign(Gtk::Align::END);
  windows_version_label.set_halign(Gtk::Align::END);
  audio_driver_label.set_halign(Gtk::Align::END);
  virtual_desktop_resolution_label.set_halign(Gtk::Align::END);
  log_level_label.set_halign(Gtk::Align::END);
  environment_variables_label.set_halign(Gtk::Align::END);
  hud_label.set_halign(Gtk::Align::END);
  description_label.set_halign(Gtk::Align::START);
  name_label.set_tooltip_text("Change the machine name");
  folder_name_label.set_tooltip_text("Change the folder. NOTE: This break your shortcuts!");
  wine_runner_label.set_tooltip_text("Select the Wine build used by this machine (system Wine, a downloaded Wine runner or a custom path)");
  wine_bin_path_label.set_tooltip_text("Change the path to the 'wine' binary for this machine");
  windows_version_label.set_tooltip_text("Change the Windows version");
  audio_driver_label.set_tooltip_text("Change the audio driver");
  virtual_desktop_resolution_label.set_tooltip_text("Set the emulated desktop resolution");
  log_level_label.set_tooltip_text("Change the Wine debug messages for logging");
  environment_variables_label.set_tooltip_text("Set one or more environment variables");
  hud_label.set_tooltip_text("Show a performance overlay (HUD) on top of your apps/games");
  description_label.set_tooltip_text("Add an additional description text to your machine");
  dxvk_hud_check.set_tooltip_text("Enable the DXVK HUD, showing device info and FPS (sets the DXVK_HUD environment variable).\nOnly for Direct3D "
                                  "applications running via DXVK.");
  gallium_hud_check.set_tooltip_text(
      "Enable the Mesa Gallium HUD, showing FPS, GPU load, VRAM usage and draw calls (sets the GALLIUM_HUD environment "
      "variable).\nOnly for OpenGL applications using a Mesa Gallium driver.");
  mangohud_check.set_tooltip_text(
      "Enable the MangoHud overlay for Vulkan & OpenGL (sets the MANGOHUD environment variable).\nRequires MangoHud to be "
      "installed on your system.");

  // Fill-in Audio drivers in combobox
  for (int i = BottleTypes::AudioDriverStart; i < BottleTypes::AudioDriverEnd; i++)
  {
    audio_driver_combobox.append(std::to_string(i), BottleTypes::to_string(BottleTypes::AudioDriver(i)));
  }
  virtual_desktop_check.set_active(false);
  virtual_desktop_resolution_entry.set_text("1024x768");
  enable_logging_check.set_active(false);

  log_level_combobox.append("0", "Off");
  log_level_combobox.append("1", "Error + Fixme (Default)");
  log_level_combobox.append("2", "Only Errors (Could improve performance)");
  log_level_combobox.append("3", "Also log warnings (recommended for debugging)");
  log_level_combobox.append("4", "Log Frames per second)");
  log_level_combobox.append("5", "Disable D3D/GL messages (could improve performance)");
  log_level_combobox.append("6", "Relay + Heap");
  log_level_combobox.append("7", "Relay + Message box");
  log_level_combobox.append("8", "All Except relay (too verbose)");
  log_level_combobox.append("9", "All (most likely too verbose)");
  log_level_combobox.set_tooltip_text("More info: https://wiki.winehq.org/Debug_Channels");
  name_entry.set_hexpand(true);
  folder_name_entry.set_hexpand(true);
  wine_bin_path_entry.set_hexpand(true);
  wine_runner_combobox.set_hexpand(true);
  manage_runners_button.set_tooltip_text("Download & manage Wine runners");
  windows_version_combobox.set_hexpand(true);
  audio_driver_combobox.set_hexpand(true);
  log_level_combobox.set_hexpand(true);
  description_text_view.set_hexpand(true);
  virtual_desktop_check.set_tooltip_text("Enable emulate virtual desktop resolution");
  enable_logging_check.set_tooltip_text("Enable output logging to disk");
  use_wine64_check.set_tooltip_text("Advanced: prefer the wine64 binary over the regular wine binary when it is available.\n"
                                    "Leave this off unless you know you need it. The wine64 binary can only run 64-bit "
                                    "applications, so 32-bit applications will no longer work in this machine.\n"
                                    "When no wine64 binary is present, the regular wine binary is used instead.");
  folder_name_entry.set_tooltip_text("Important: This will break your shortcuts! Consider changing the name instead, see above.");

  description_scrolled_window.set_child(description_text_view);
  description_scrolled_window.set_hexpand(true);
  description_scrolled_window.set_vexpand(true);

  int row = 0;
  edit_grid.attach(name_label, 0, row);
  edit_grid.attach(name_entry, 1, row++, 2);
  edit_grid.attach(folder_name_label, 0, row);
  edit_grid.attach(folder_name_entry, 1, row++, 2);
  edit_grid.attach(wine_runner_label, 0, row);
  edit_grid.attach(wine_runner_combobox, 1, row);
  edit_grid.attach(manage_runners_button, 2, row++);
  edit_grid.attach(wine_bin_path_label, 0, row);
  edit_grid.attach(wine_bin_path_entry, 1, row);
  edit_grid.attach(wine_bin_path_button, 2, row++);
  edit_grid.attach(windows_version_label, 0, row);
  edit_grid.attach(windows_version_combobox, 1, row++, 2);
  edit_grid.attach(audio_driver_label, 0, row);
  edit_grid.attach(audio_driver_combobox, 1, row++, 2);
  edit_grid.attach(virtual_desktop_check, 0, row++, 3);
  edit_grid.attach(virtual_desktop_resolution_label, 0, row);
  edit_grid.attach(virtual_desktop_resolution_entry, 1, row++, 2);
  edit_grid.attach(enable_logging_check, 0, row++, 3);
  edit_grid.attach(log_level_label, 0, row);
  edit_grid.attach(log_level_combobox, 1, row++, 2);
  edit_grid.attach(environment_variables_label, 0, row);
  edit_grid.attach(configure_environment_variables_button, 1, row++, 2);
  hbox_hud_checks.append(dxvk_hud_check);
  hbox_hud_checks.append(gallium_hud_check);
  hbox_hud_checks.append(mangohud_check);
  edit_grid.attach(hud_label, 0, row);
  edit_grid.attach(hbox_hud_checks, 1, row++, 2);
  // Advanced option: most users should keep the plain wine binary (it supports both 32-bit and 64-bit
  // applications). Switching to wine64 disables 32-bit application support. Placed above the description
  // (last of the settings), so it doesn't get orphaned below the expanding description text area.
  edit_grid.attach(use_wine64_check, 0, row++, 3);
  edit_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, row++, 3);
  edit_grid.attach(description_label, 0, row++, 3);
  edit_grid.attach(description_scrolled_window, 0, row++, 3);
  edit_grid.set_hexpand(true);
  edit_grid.set_vexpand(true);
  edit_grid.set_halign(Gtk::Align::FILL);
  edit_grid.set_margin_end(5);

  hbox_buttons.set_margin(6);
  delete_button.set_halign(Gtk::Align::START);
  delete_button.set_hexpand(true);
  save_button.set_halign(Gtk::Align::END);
  cancel_button.set_halign(Gtk::Align::END);
  hbox_buttons.append(delete_button);
  hbox_buttons.append(save_button);
  hbox_buttons.append(cancel_button);

  vbox.append(header_edit_label);
  vbox.append(edit_grid);
  vbox.append(hbox_buttons);
  set_child(vbox);

  // Fill the wine runner combobox (with at least: System Wine & Custom path)
  refresh_wine_runner_list();

  // Gray-out custom wine binary path, virtual desktop & log level by default
  custom_wine_bin_path_sensitive(false);
  virtual_desktop_resolution_sensitive(false);
  log_level_sensitive(false);
}

/**
 * \brief Same as show() but will also update the Window title, set name,
 * update list of windows versions, set active windows, audio driver and virtual desktop
 */
void BottleEditWindow::show()
{
  if (active_bottle_ != nullptr)
  {
    set_title("Edit Machine - " +
              ((!active_bottle_->name().empty()) ? active_bottle_->name() : active_bottle_->folder_name())); // Fallback to folder name
    // Enable save button (again)
    save_button.set_sensitive(true);

    // Set name
    name_entry.set_text(active_bottle_->name());
    // Set folder name
    folder_name_entry.set_text(active_bottle_->folder_name());
    // Set description
    description_text_view.get_buffer()->set_text(active_bottle_->description());

    // Set wine runner selection & binary path (refresh the runner list first, it may have changed)
    refresh_wine_runner_list();
    Glib::ustring wine_bin_path = active_bottle_->wine_bin_path();
    wine_bin_path_entry.set_text(wine_bin_path);
    if (wine_bin_path.empty())
    {
      wine_runner_combobox.set_active_id("system");
    }
    else if (std::optional<WineRunner::InstalledRunner> runner = WineRunnerManager::find_runner_by_bin_dir(wine_bin_path))
    {
      wine_runner_combobox.set_active_id(runner->bin_dir);
    }
    else
    {
      wine_runner_combobox.set_active_id("custom");
    }

    // Clear list
    windows_version_combobox.remove_all();
    // Fill-in Windows versions in combobox
    for (std::vector<BottleTypes::WindowsAndBit>::iterator it = BottleTypes::SupportedWindowsVersions.begin();
         it != BottleTypes::SupportedWindowsVersions.end(); ++it)
    {
      // Only show the same bitness Windows versions
      if (active_bottle_->bit() == (*it).second)
      {
        auto index = std::distance(BottleTypes::SupportedWindowsVersions.begin(), it);
        windows_version_combobox.append(std::to_string(index),
                                        BottleTypes::to_string((*it).first) + " (" + BottleTypes::to_string((*it).second) + ')');
      }
    }
    windows_version_combobox.set_active_text(BottleTypes::to_string(active_bottle_->windows()) + " (" +
                                             BottleTypes::to_string(active_bottle_->bit()) + ")");
    audio_driver_combobox.set_active_id(std::to_string((int)active_bottle_->audio_driver()));
    if (!active_bottle_->virtual_desktop().empty())
    {
      virtual_desktop_resolution_entry.set_text(active_bottle_->virtual_desktop());
      virtual_desktop_check.set_active(true);
    }
    else
    {
      virtual_desktop_check.set_active(false);
    }

    enable_logging_check.set_active(active_bottle_->is_debug_logging());
    log_level_combobox.set_active_id(std::to_string((int)active_bottle_->debug_log_level()));
    use_wine64_check.set_active(active_bottle_->use_wine64());

    // Reflect the current HUD environment variables in the checkboxes
    bool has_dxvk_hud = false, has_gallium_hud = false, has_mangohud = false;
    for (const auto& [key, value] : active_bottle_->env_vars())
    {
      if (key == "DXVK_HUD")
        has_dxvk_hud = true;
      else if (key == "GALLIUM_HUD")
        has_gallium_hud = true;
      else if (key == "MANGOHUD")
        has_mangohud = true;
    }
    dxvk_hud_check.set_active(has_dxvk_hud);
    gallium_hud_check.set_active(has_gallium_hud);
    mangohud_check.set_active(has_mangohud);
  }
  else
  {
    set_title("Edit Machine (Unknown machine)");
  }
  // Call parent present
  present();
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle - New bottle
 */
void BottleEditWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void BottleEditWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Triggered when bottle is actually confirmed to be removed
 */
void BottleEditWindow::bottle_removed()
{
  set_visible(false); // Hide the edit window
}

/**
 * \brief (Re)fill the wine runner combobox with: System Wine, the installed Wine runners & Custom path.
 * Keeps the current selection when possible. Also called when the set of installed runners changed.
 */
void BottleEditWindow::refresh_wine_runner_list()
{
  Glib::ustring previous_selection = wine_runner_combobox.get_active_id();
  wine_runner_combobox.remove_all();
  wine_runner_combobox.append("system", "System Wine (default)");
  for (const WineRunner::InstalledRunner& runner : WineRunnerManager::get_installed_runners())
  {
    Glib::ustring text = runner.display_name;
    if (!runner.wine_version.empty())
      text += " — Wine " + runner.wine_version;
    // The absolute wine binary directory doubles as unique combobox ID (it can never collide with "system"/"custom")
    wine_runner_combobox.append(runner.bin_dir, text);
  }
  wine_runner_combobox.append("custom", "Custom path...");
  if (previous_selection.empty())
  {
    wine_runner_combobox.set_active_id("system");
  }
  else if (!wine_runner_combobox.set_active_id(previous_selection))
  {
    // The previously selected runner was removed, fall back to a custom path (keeping the path as text)
    wine_bin_path_entry.set_text(previous_selection);
    wine_runner_combobox.set_active_id("custom");
  }
}

/**
 * \brief Handler when the bottle is updated.
 */
void BottleEditWindow::on_bottle_updated()
{
  busy_dialog.hide();
  set_visible(false); // Hide the edit Window
}

/**
 * \brief Enable/disable the custom Wine binary path fields.
 * \param sensitive Set true to enable, false for disable
 */
void BottleEditWindow::custom_wine_bin_path_sensitive(bool sensitive)
{
  wine_bin_path_label.set_sensitive(sensitive);
  wine_bin_path_entry.set_sensitive(sensitive);
  wine_bin_path_button.set_sensitive(sensitive);
}

/**
 * \brief Enable/disable desktop resolution fields.
 * \param sensitive Set true to enable, false for disable
 */
void BottleEditWindow::virtual_desktop_resolution_sensitive(bool sensitive)
{
  virtual_desktop_resolution_label.set_sensitive(sensitive);
  virtual_desktop_resolution_entry.set_sensitive(sensitive);
}

/**
 * \brief Enable/disable debug log level
 * \param sensitive Set true to enable, false for disable
 */
void BottleEditWindow::log_level_sensitive(bool sensitive)
{
  log_level_label.set_sensitive(sensitive);
  log_level_combobox.set_sensitive(sensitive);
}

/**
 * \brief Signal handler when the virtual desktop checkbox is checked.
 * It will show the additional resolution input field.
 */
void BottleEditWindow::on_virtual_desktop_toggle()
{
  virtual_desktop_resolution_sensitive(virtual_desktop_check.get_active());
}

/**
 * \brief Signal handler when the debug logging checkbox is checked.
 * It will show the additional log level input field.
 */
void BottleEditWindow::on_debug_logging_toggle()
{
  log_level_sensitive(enable_logging_check.get_active());
}

/**
 * \brief Signal handler when the Wine runner selection changed.
 * The 'Wine Binary Path' text and button widgets are only enabled for the 'Custom path...' option.
 */
void BottleEditWindow::on_wine_runner_changed()
{
  custom_wine_bin_path_sensitive(wine_runner_combobox.get_active_id() == "custom");
}

/**
 * \brief Triggered when select folder button (next to 'Wine Binary Path') is clicked.
 */
void BottleEditWindow::on_select_wine_bin_path()
{
  auto dialog = Gtk::FileDialog::create();
  dialog->set_title("Choose a folder");
  dialog->set_modal(true);
  {
    auto folder = Gio::File::create_for_path(wine_bin_path_entry.get_text());
    if (!folder->get_path().empty())
    {
      dialog->set_initial_folder(folder);
    }
  }

  dialog->select_folder(*this,
                        [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result)
                        {
                          try
                          {
                            auto folder = dialog->select_folder_finish(result);
                            wine_bin_path_entry.set_text(folder->get_path());
                          }
                          catch (const Gtk::DialogError& err)
                          {
                            // Do nothing
                          }
                          catch (const Glib::Error& err)
                          {
                            // Do nothing
                          }
                        });
}

/**
 * \brief Triggered when cancel button is clicked
 */
void BottleEditWindow::on_cancel_button_clicked()
{
  set_visible(false);
}

/**
 * \brief Triggered when save button is clicked
 */
void BottleEditWindow::on_save_button_clicked()
{
  // First disable save button (avoid multiple presses)
  save_button.set_sensitive(false);

  // Show busy dialog
  busy_dialog.set_message("Updating Windows Machine", "Busy applying all your changes currently.");
  busy_dialog.present();

  std::string::size_type sz;

  UpdateBottleStruct update_bottle_struct;
  update_bottle_struct.windows_version = WineDefaults::WindowsOs; // Fallback
  update_bottle_struct.audio = WineDefaults::AudioDriver;         // Fallback
  update_bottle_struct.virtual_desktop_resolution = "";           // Empty string default (= disabled windowed mode)
  update_bottle_struct.wine_bin_path = "";                        // Empty string default (= use system wine path)
  update_bottle_struct.debug_log_level = 1;                       // // 1 = Default wine debug logging

  update_bottle_struct.name = name_entry.get_text();
  update_bottle_struct.folder_name = folder_name_entry.get_text();
  const Glib::ustring runner_id = wine_runner_combobox.get_active_id();
  if (runner_id == "custom")
  {
    // Custom Wine binary path (whatever the user entered)
    update_bottle_struct.wine_bin_path = wine_bin_path_entry.get_text();
  }
  else if (runner_id != "system" && !runner_id.empty())
  {
    // An installed Wine runner is selected, its ID is the wine binary directory
    update_bottle_struct.wine_bin_path = runner_id;
  }
  // Otherwise: system Wine (keep the default empty string)

  update_bottle_struct.description = description_text_view.get_buffer()->get_text();
  bool is_desktop_enabled = virtual_desktop_check.get_active();
  if (is_desktop_enabled)
  {
    update_bottle_struct.virtual_desktop_resolution = virtual_desktop_resolution_entry.get_text();
  }
  update_bottle_struct.is_debug_logging = enable_logging_check.get_active();
  update_bottle_struct.enable_dxvk_hud = dxvk_hud_check.get_active();
  update_bottle_struct.enable_gallium_hud = gallium_hud_check.get_active();
  update_bottle_struct.enable_mangohud = mangohud_check.get_active();
  update_bottle_struct.use_wine64 = use_wine64_check.get_active();
  try
  {
    update_bottle_struct.debug_log_level = std::stoi(log_level_combobox.get_active_id(), &sz);
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  // Ignore the catches
  try
  {
    size_t win_bit_index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    const auto& [currentWindowsVersionName, _] = BottleTypes::SupportedWindowsVersions.at(win_bit_index);
    update_bottle_struct.windows_version = currentWindowsVersionName;
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  // Ignore the catches
  try
  {
    size_t audio_index = size_t(std::stoi(audio_driver_combobox.get_active_id(), &sz));
    update_bottle_struct.audio = BottleTypes::AudioDriver(audio_index);
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  // Ignore the catches

  update_bottle.emit(update_bottle_struct);
}