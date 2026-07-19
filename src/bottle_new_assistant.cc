/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    bottle_new_assistant.cc
 * \brief   New Wine bottle assistant (Wizard with steps)
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
#include "bottle_new_assistant.h"
#include "bottle_types.h"
#include "wine_defaults.h"
#include "wine_runner_manager.h"

#define LOADING_PAGE_INDEX 3 /*!< The loading page, 4th page (3 when start counting from zero) */

/**
 * \brief Constructor
 */
BottleNewAssistant::BottleNewAssistant()
    : vbox(Gtk::Orientation::VERTICAL, 4),
      vbox_runner(Gtk::Orientation::VERTICAL, 4),
      vbox2(Gtk::Orientation::VERTICAL, 4),
      vbox3(Gtk::Orientation::VERTICAL, 4),
      hbox_name(Gtk::Orientation::HORIZONTAL, 12),
      hbox_win(Gtk::Orientation::HORIZONTAL, 12),
      hbox_runner(Gtk::Orientation::HORIZONTAL, 12),
      name_label("Name:"),
      windows_version_label("Windows Version:"),
      runner_label("Wine Runner:"),
      audio_driver_label("Audio Driver:"),
      virtual_desktop_resolution_label("Window Resolution:"),
      confirm_label("Confirmation page"),
      virtual_desktop_check("Enable Virtual Desktop Window"),
      disable_gecko_mono_check("Disable Gecko & Mono"),
      manage_runners_button("Manage runners...")
{
  set_default_size(640, 400);
  // Only focus on assistant, disable interaction with other windows in app
  set_modal(true);

  vbox.set_margin(8);
  vbox.set_hexpand(true);
  vbox.set_vexpand(true);
  vbox_runner.set_margin(8);
  vbox_runner.set_hexpand(true);
  vbox_runner.set_vexpand(true);
  vbox2.set_margin(8);
  vbox2.set_hexpand(true);
  vbox2.set_vexpand(true);
  vbox3.set_margin(8);
  vbox3.set_hexpand(true);
  vbox3.set_vexpand(true);
  vbox3.set_halign(Gtk::Align::CENTER);
  vbox3.set_valign(Gtk::Align::CENTER);

  // Create pages (the Wine runner is chosen first, because it constrains which Windows versions are valid)
  create_first_page();
  create_second_page();
  create_third_page();
  create_fourth_page();

  // Initial set defaults
  set_default_values();

  signal_apply().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_apply));
  signal_cancel().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_cancel));
  signal_prepare().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_prepare));
  // Hide window instead of destroy
  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        set_default_values();
        // Move to first page
        set_current_page(0);
        return true; // stop default destroy
      },
      false);
}

/**
 * \brief Destructor
 */
BottleNewAssistant::~BottleNewAssistant()
{
}

/**
 * \brief Set default values of all input fields from the wizard,
 * so even after the second time, all values are correctly reset.
 */
void BottleNewAssistant::set_default_values()
{
  apply_label.set_text("Please wait, changes are getting applied.");
  name_entry.set_text("");
  // Reset to System Wine first: this re-filters the Windows version list to the full 32 & 64-bit set,
  // then select the default Windows version within that (repopulated) list.
  wine_runner_combobox.set_active_id("system");
  refresh_windows_version_list();
  windows_version_combobox.set_active_id(std::to_string(BottleTypes::DefaultBottleIndex));
  audio_driver_combobox.set_active_id(std::to_string(BottleTypes::DefaultAudioDriverIndex));
  virtual_desktop_check.set_active(false);
  disable_gecko_mono_check.set_active(false);
  virtual_desktop_resolution_entry.set_text("1024x768");
  loading_bar.set_fraction(0.0);
  // Hide resolution label & entry
  hbox_virtual_desktop.set_visible(false);

  // TODO: Unable to reset the cancel button after previous commit()?

  if (timer_)
  {
    timer_.disconnect();
  }
}

/**
 * \brief First page of the wizard: choose the Wine runner (defaults to system Wine)
 */
void BottleNewAssistant::create_first_page()
{
  runner_intro_label.set_markup("<big><b>Create a New Machine</b></big>\n"
                                "First, select which Wine build this machine will use. Keep <b>System Wine</b> unless you want "
                                "a specific Wine build.\nUse the 'Manage runners...' button to download additional Wine builds "
                                "(like Wine Staging, Wine Staging-TkG or GE-Proton).\n\n"
                                "<i>Note:</i> a WoW64 build only supports 64-bit Windows versions.");
  runner_intro_label.set_halign(Gtk::Align::START);
  runner_intro_label.set_margin_bottom(25);
  vbox_runner.append(runner_intro_label);

  wine_runner_combobox.set_hexpand(true);
  hbox_runner.append(runner_label);
  hbox_runner.append(wine_runner_combobox);
  hbox_runner.append(manage_runners_button);
  vbox_runner.append(hbox_runner);

  // Fill the runner combobox initially
  refresh_wine_runner_list();

  // Re-filter the Windows version list whenever the runner selection changes
  wine_runner_combobox.signal_changed().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_wine_runner_changed));

  manage_runners_button.signal_clicked().connect(sigc::bind(manage_runners, this));

  append_page(vbox_runner);
  // System Wine is a valid default, so the page is always complete
  set_page_complete(vbox_runner, true);
  set_page_type(vbox_runner, Gtk::AssistantPage::Type::INTRO);
  set_page_title(*get_nth_page(0), "Choose Wine Runner");
}

/**
 * \brief Second page of the wizard
 */
void BottleNewAssistant::create_second_page()
{
  // Name & Windows version page
  intro_label.set_markup("<big><b>Name &amp; Windows Version</b></big>\n"
                         "Please use a descriptive name for the Windows machine, "
                         "and select which Windows version you want to use.");
  intro_label.set_halign(Gtk::Align::START);
  intro_label.set_margin_bottom(25);
  vbox.append(intro_label);

  hbox_name.append(name_label);
  hbox_name.append(name_entry);
  vbox.append(hbox_name);

  // Fill-in Windows versions in combobox (filtered by the selected Wine runner)
  refresh_windows_version_list();

  hbox_win.append(windows_version_label);
  hbox_win.append(windows_version_combobox);
  vbox.append(hbox_win);

  name_entry.signal_changed().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_entry_changed));

  append_page(vbox);
  set_page_type(vbox, Gtk::AssistantPage::Type::CONTENT);
  set_page_title(*get_nth_page(1), "Choose Name & Windows version");
}

/**
 * \brief Third page of the wizard: additional settings
 */
void BottleNewAssistant::create_third_page()
{
  // Additional page
  additional_label.set_markup("<big><b>Additional Settings</b></big>\n"
                              "There you could adapt some additional Windows settings.\n\n<b>Note:</b> If you do not "
                              "know what these settings mean, <b><i>do NOT</i></b> change the settings (keep the default values).");
  additional_label.set_halign(Gtk::Align::START);
  additional_label.set_margin_bottom(25);
  vbox2.append(additional_label);

  // Fill-in Audio drivers in combobox
  for (int i = BottleTypes::AudioDriverStart; i < BottleTypes::AudioDriverEnd; i++)
  {
    audio_driver_combobox.append(std::to_string(i), BottleTypes::to_string(BottleTypes::AudioDriver(i)));
  }

  audio_driver_label.set_margin_end(6);
  hbox_audio.append(audio_driver_label);
  hbox_audio.append(audio_driver_combobox);
  vbox2.append(hbox_audio);

  vbox2.append(virtual_desktop_check);
  virtual_desktop_check.signal_toggled().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_virtual_desktop_toggle));

  hbox_virtual_desktop.append(virtual_desktop_resolution_label);
  hbox_virtual_desktop.append(virtual_desktop_resolution_entry);
  vbox2.append(hbox_virtual_desktop);

  vbox2.append(disable_gecko_mono_check);

  append_page(vbox2);
  set_page_complete(vbox2, true);
  set_page_type(vbox2, Gtk::AssistantPage::Type::CONFIRM);
  set_page_title(*get_nth_page(2), "Additional settings");
}

/**
 * \brief Last page of the wizard
 */
void BottleNewAssistant::create_fourth_page()
{
  vbox3.append(apply_label);
  vbox3.append(loading_bar);
  append_page(vbox3);

  // Wait before we close the window
  set_page_complete(vbox3, false);
  set_page_type(vbox3, Gtk::AssistantPage::Type::PROGRESS);
  set_page_title(*get_nth_page(3), "Applying changes");
}

/**
 * \brief (Re)fill the wine runner combobox with: System Wine & the installed Wine runners.
 * Keeps the current selection when possible. Also called when the set of installed runners changed.
 */
void BottleNewAssistant::refresh_wine_runner_list()
{
  Glib::ustring previous_selection = wine_runner_combobox.get_active_id();
  wine_runner_combobox.remove_all();
  runner_is_wow64_.clear();
  wine_runner_combobox.append("system", "System Wine (default)");
  for (const WineRunner::InstalledRunner& runner : WineRunnerManager::get_installed_runners())
  {
    Glib::ustring text = runner.display_name;
    if (!runner.wine_version.empty())
      text += " — Wine " + runner.wine_version;
    // The absolute wine binary directory doubles as unique combobox ID (it can never collide with "system")
    wine_runner_combobox.append(runner.bin_dir, text);
    // Remember the WoW64 (64-bit only) capability, used to filter the Windows version list
    runner_is_wow64_[runner.bin_dir] = runner.wow64;
  }
  if (previous_selection.empty() || !wine_runner_combobox.set_active_id(previous_selection))
  {
    wine_runner_combobox.set_active_id("system");
  }
}

/**
 * \brief (Re)fill the Windows version combobox, filtered by the currently selected Wine runner.
 * A WoW64 runner cannot create a 32-bit (WINEARCH=win32) prefix, so only 64-bit Windows versions are offered
 * for it. System Wine and regular (non-WoW64) runners offer the full 32 & 64-bit list.
 */
void BottleNewAssistant::refresh_windows_version_list()
{
  const Glib::ustring runner_id = wine_runner_combobox.get_active_id();
  bool only_64bit = false;
  if (runner_id != "system" && !runner_id.empty())
  {
    auto it = runner_is_wow64_.find(runner_id);
    only_64bit = (it != runner_is_wow64_.end()) && it->second;
  }

  const Glib::ustring previous_selection = windows_version_combobox.get_active_id();
  windows_version_combobox.remove_all();
  for (std::vector<BottleTypes::WindowsAndBit>::iterator it = BottleTypes::SupportedWindowsVersions.begin();
       it != BottleTypes::SupportedWindowsVersions.end(); ++it)
  {
    if (only_64bit && (*it).second != BottleTypes::Bit::win64)
      continue; // Skip 32-bit Windows versions, a WoW64 build cannot host them
    auto index = std::distance(BottleTypes::SupportedWindowsVersions.begin(), it);
    windows_version_combobox.append(std::to_string(index), BottleTypes::to_string((*it).first) + " (" + BottleTypes::to_string((*it).second) + ')');
  }
  // Keep the previous selection when it survives the filter, otherwise fall back to a sensible 64-bit default
  if (previous_selection.empty() || !windows_version_combobox.set_active_id(previous_selection))
  {
    windows_version_combobox.set_active_id(std::to_string(BottleTypes::DefaultBottleIndex));
  }
}

/**
 * \brief Signal handler when the Wine runner selection changed: re-filter the Windows version list
 */
void BottleNewAssistant::on_wine_runner_changed()
{
  refresh_windows_version_list();
}

/**
 * \brief Retrieve the results (after the wizard is finished).
 * And reset the values to default values again.
 */
NewBottleStruct BottleNewAssistant::get_result()
{
  std::string::size_type sz;
  auto windows_version = WineDefaults::WindowsOs;
  auto bit = BottleTypes::Bit::win32;
  auto audio = WineDefaults::AudioDriver;
  auto name = name_entry.get_text();
  auto vd_res = Glib::ustring("");
  auto disable_gecko_mono = false;
  auto wine_bin_path = Glib::ustring("");

  // An installed Wine runner selection carries the wine binary directory as ID ("system" = system Wine)
  const Glib::ustring runner_id = wine_runner_combobox.get_active_id();
  if (runner_id != "system" && !runner_id.empty())
  {
    wine_bin_path = runner_id;
  }

  try
  {
    size_t win_bit_index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    const auto& [windows_version_value, bit_value] = BottleTypes::SupportedWindowsVersions.at(win_bit_index);
    windows_version = windows_version_value;
    bit = bit_value;
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

  bool isDesktopEnabled = virtual_desktop_check.get_active();
  if (isDesktopEnabled)
  {
    vd_res = virtual_desktop_resolution_entry.get_text();
  }
  else
  {
    // Just empty
    vd_res = "";
  }

  disable_gecko_mono = disable_gecko_mono_check.get_active();

  try
  {
    size_t audio_index = size_t(std::stoi(audio_driver_combobox.get_active_id(), &sz));
    audio = BottleTypes::AudioDriver(audio_index);
  }
  // Ignore the catches
  catch (const std::runtime_error& error)
  {
  }
  catch (const std::invalid_argument& e)
  {
  }
  catch (const std::out_of_range& e)
  {
  }
  return {name, windows_version, bit, vd_res, disable_gecko_mono, audio, wine_bin_path};
}

/**
 * \brief Triggered when the bottle is fully created (signal from the bottle manager thread)
 */
void BottleNewAssistant::bottle_created()
{
  // Grap a copy of the bottle name
  const Glib::ustring created_bottle_name = name_entry.get_text();

  // Reset defaults (including timer_.disconnect())
  set_default_values();

  // Close Assistant
  this->set_visible(false);

  // Inform UI, emit signal new_bottle_finished (causes the GUI to refresh)
  // pass along the name of the created bottle
  new_bottle_finished.emit(created_bottle_name);
}

/**
 * \brief Triggered when the apply is pressed
 * apply_changes_gradually
 */
void BottleNewAssistant::on_assistant_apply()
{
  // Guess the time interval based on the user input
  bool is_desktop_enabled = virtual_desktop_check.get_active();
  bool is_non_default_audio_driver = false;

  std::string::size_type sz;
  try
  {
    size_t audio_index = size_t(std::stoi(audio_driver_combobox.get_active_id(), &sz));
    auto audio = BottleTypes::AudioDriver(audio_index);
    is_non_default_audio_driver = WineDefaults::AudioDriver != audio;
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

  int time_interval = 360;
  if (is_desktop_enabled)
  {
    time_interval += 90;
  }
  if (is_non_default_audio_driver)
  {
    time_interval += 90;
  }

  /* Start a timer to give the user feedback about the changes taking a few seconds to apply. */
  timer_ = Glib::signal_timeout().connect(sigc::mem_fun(*this, &BottleNewAssistant::apply_changes_gradually), time_interval);
}

void BottleNewAssistant::on_assistant_cancel()
{
  set_visible(false);
}

/**
 * \brief Prepare handler for each page, is emitted before making the page visible.
 */
void BottleNewAssistant::on_assistant_prepare(Gtk::Widget* /* widget*/)
{
  set_title(Glib::ustring::compose("Create a New Machine (Step %1 of %2)", get_current_page() + 1, get_n_pages()));

  /* The last page is the progress page.  The
   * user clicked Apply to get here so we tell the assistant to commit,
   * which means the changes up to this point are permanent and cannot
   * be cancelled or revisited. */
  if (get_current_page() == LOADING_PAGE_INDEX)
    this->commit();
}

/**
 * \brief Only complete the first page, when the name input field is *not* empty
 */
void BottleNewAssistant::on_entry_changed()
{
  // The page is only complete if the name entry contains text.
  if (name_entry.get_text_length() != 0)
    set_page_complete(vbox, true);
  else
    set_page_complete(vbox, false);
}

/**
 * \brief Signal handler when the virtual desktop checkbox is checked.
 * It will show the additional resolution input field.
 */
void BottleNewAssistant::on_virtual_desktop_toggle()
{
  if (virtual_desktop_check.get_active())
    // Show resolution label & input field
    hbox_virtual_desktop.show();
  else
    hbox_virtual_desktop.set_visible(false);
}

/**
 * \brief Smooth loading bar and pulse loading bar when
 * it takes longer than expected
 */
bool BottleNewAssistant::apply_changes_gradually()
{
  double fraction = (loading_bar.get_fraction() + 0.02);
  if (fraction <= 1.0)
  {
    loading_bar.set_fraction(fraction);
  }
  else
  {
    loading_bar.set_pulse_step(0.3);
    loading_bar.pulse();
    // Say it takes a bit longer
    apply_label.set_text("Almost done creating the new machine...");
  }
  // Never stop the timer (only when disconnect() is called)
  return true;
}
