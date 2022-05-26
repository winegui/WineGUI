/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    bottle_new_assistant.cc
 * \brief   New Wine bottle assistant (Wizard with steps)
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
#include "bottle_new_assistant.h"
#include "bottle_types.h"
#include "wine_defaults.h"
#include <iostream>

#define LOADING_PAGE_INDEX 2 /*!< The loading page, 3rd page (2 when start counting from zero) */

/**
 * \brief Contructor
 */
BottleNewAssistant::BottleNewAssistant()
    : vbox(Gtk::ORIENTATION_VERTICAL, 4),
      vbox2(Gtk::ORIENTATION_VERTICAL, 4),
      vbox3(Gtk::ORIENTATION_VERTICAL, 4),
      hbox_name(Gtk::ORIENTATION_HORIZONTAL, 12),
      hbox_win(Gtk::ORIENTATION_HORIZONTAL, 12),
      name_label("Name:"),
      windows_version_label("Windows Version:"),
      audio_driver_label("Audio Driver:"),
      virtual_desktop_resolution_label("Window Resolution:"),
      confirm_label("Confirmation page"),
      virtual_desktop_check("Enable Virtual Desktop Window"),
      disable_gecko_mono_check("Disable Gecko & Mono")
{
  set_border_width(8);
  set_default_size(640, 400);
  // Only focus on assistant, disable interaction with other windows in app
  set_modal(true);

  // Create pages
  createFirstPage();
  createSecondPage();
  createThirdPage();

  // Initial set defaults
  setDefaultValues();

  signal_apply().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_apply));
  signal_cancel().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_cancel));
  signal_close().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_close));
  signal_prepare().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_assistant_prepare));

  show_all_children();

  // By default hide resolution label & entry
  hbox_virtual_desktop.hide();
}

/**
 * \brief Destructor
 */
BottleNewAssistant::~BottleNewAssistant()
{
}

/**
 * \brief Set default values of all input fields from the wizard,
 * so even after the second time, all values are correctly resetted.
 */
void BottleNewAssistant::setDefaultValues()
{
  apply_label.set_text("Please wait, changes are getting applied.");
  name_entry.set_text("");
  // TODO: Allow default to override from WineGUI settings?
  windows_version_combobox.set_active_id(std::to_string(BottleTypes::DefaultBottleIndex));
  audio_driver_combobox.set_active_id(std::to_string(BottleTypes::DefaultAudioDriverIndex));
  virtual_desktop_check.set_active(false);
  disable_gecko_mono_check.set_active(false);
  virtual_desktop_resolution_entry.set_text("960x540");
  loading_bar.set_fraction(0.0);

  if (timer_)
  {
    timer_.disconnect();
  }
}

/**
 * \brief First page of the wizard
 */
void BottleNewAssistant::createFirstPage()
{
  // Intro page
  intro_label.set_markup("<big><b>Create a New Machine</b></big>\n"
                         "Please use a descriptive name for the Windows machine, and select which Windows version you want to use.");
  intro_label.set_halign(Gtk::Align::ALIGN_START);
  intro_label.set_margin_bottom(25);
  vbox.pack_start(intro_label, false, false);

  hbox_name.pack_start(name_label, false, true);
  hbox_name.pack_start(name_entry);
  vbox.pack_start(hbox_name, false, false);

  // Fill-in Windows versions in combobox
  for (std::vector<BottleTypes::WindowsAndBit>::iterator it = BottleTypes::SupportedWindowsVersions.begin();
       it != BottleTypes::SupportedWindowsVersions.end(); ++it)
  {
    auto index = std::distance(BottleTypes::SupportedWindowsVersions.begin(), it);
    windows_version_combobox.insert(-1, std::to_string(index),
                                    BottleTypes::to_string((*it).first) + " (" + BottleTypes::to_string((*it).second) + ')');
  }

  hbox_win.pack_start(windows_version_label, false, true);
  hbox_win.pack_start(windows_version_combobox);
  vbox.pack_start(hbox_win, false, false);

  name_entry.signal_changed().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_entry_changed));

  append_page(vbox);
  set_page_type(vbox, Gtk::ASSISTANT_PAGE_INTRO);
  set_page_title(*get_nth_page(0), "Choose Name & Windows version");
}

/**
 * \brief Second page of the wizard
 */
void BottleNewAssistant::createSecondPage()
{
  // Additional page
  additional_label.set_markup("<big><b>Additional Settings</b></big>\n"
                              "There you could adapt some additional Windows settings.\n\n<b>Note:</b> If you do not "
                              "know what these settings mean, <b><i>do NOT</i></b> change the settings (keep the default values).");
  additional_label.set_halign(Gtk::Align::ALIGN_START);
  additional_label.set_margin_bottom(25);
  vbox2.pack_start(additional_label, false, false);

  // Fill-in Audio drivers in combobox
  for (int i = BottleTypes::AudioDriverStart; i < BottleTypes::AudioDriverEnd; i++)
  {
    audio_driver_combobox.insert(-1, std::to_string(i), BottleTypes::to_string(BottleTypes::AudioDriver(i)));
  }

  hbox_audio.pack_start(audio_driver_label, false, true);
  hbox_audio.pack_start(audio_driver_combobox);
  vbox2.pack_start(hbox_audio, false, false);

  vbox2.pack_start(virtual_desktop_check, false, false);
  virtual_desktop_check.signal_toggled().connect(sigc::mem_fun(*this, &BottleNewAssistant::on_virtual_desktop_toggle));

  hbox_virtual_desktop.pack_start(virtual_desktop_resolution_label, false, false);
  hbox_virtual_desktop.pack_start(virtual_desktop_resolution_entry, false, false);
  vbox2.pack_start(hbox_virtual_desktop, false, false);

  vbox2.pack_start(disable_gecko_mono_check, false, false);

  append_page(vbox2);
  set_page_complete(vbox2, true);
  set_page_type(vbox2, Gtk::ASSISTANT_PAGE_CONFIRM);
  set_page_title(*get_nth_page(1), "Additional settings");
}

/**
 * \brief Last page of the wizard
 */
void BottleNewAssistant::createThirdPage()
{
  vbox3.set_halign(Gtk::Align::ALIGN_CENTER);
  vbox3.set_valign(Gtk::Align::ALIGN_CENTER);

  vbox3.pack_start(apply_label, false, false);
  vbox3.pack_start(loading_bar, false, false);
  append_page(vbox3);

  // Wait before we close the window
  set_page_complete(vbox3, false);
  set_page_type(vbox3, Gtk::ASSISTANT_PAGE_PROGRESS);
  set_page_title(*get_nth_page(2), "Applying changes");
}

/**
 * \brief Retrieve the results (after the wizard is finished).
 * And reset the values to default values again.
 * Idea: use one struct as in/out parameter
 * \param[inout] name                        Bottle Name
 * \param[inout] windows_version             Windows OS version
 * \param[inout] bit                         Windows Bit (32/64-bit)
 * \param[inout] virtual_desktop_resolution  Virtual desktop resolution (empty if disabled)
 * \param[inout] disable_gecko_mono          Enable/Disable Gecko/Mono during install
 * \param[inout] audio                       Audio Driver type
 */
void BottleNewAssistant::get_result(Glib::ustring& name,
                                    BottleTypes::Windows& windows_version,
                                    BottleTypes::Bit& bit,
                                    Glib::ustring& virtual_desktop_resolution,
                                    bool& disable_gecko_mono,
                                    BottleTypes::AudioDriver& audio)
{
  std::string::size_type sz;
  windows_version = BottleTypes::Windows::WindowsXP;
  bit = BottleTypes::Bit::win32;
  audio = BottleTypes::AudioDriver::pulseaudio;

  name = name_entry.get_text();

  try
  {
    size_t win_bit_index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    const auto currentWindowsBit = BottleTypes::SupportedWindowsVersions.at(win_bit_index);
    windows_version = currentWindowsBit.first;
    bit = currentWindowsBit.second;
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (std::invalid_argument& e)
  {
  }
  catch (std::out_of_range& e)
  {
  }
  // Ignore the catches

  bool isDesktopEnabled = virtual_desktop_check.get_active();
  if (isDesktopEnabled)
  {
    virtual_desktop_resolution = virtual_desktop_resolution_entry.get_text();
  }
  else
  {
    // Just empty
    virtual_desktop_resolution = "";
  }

  disable_gecko_mono = disable_gecko_mono_check.get_active();

  try
  {
    size_t audio_index = size_t(std::stoi(audio_driver_combobox.get_active_id(), &sz));
    audio = BottleTypes::AudioDriver(audio_index);
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (std::invalid_argument& e)
  {
  }
  catch (std::out_of_range& e)
  {
  }
  // Ignore the catches
}

/**
 * \brief Triggered when the bottle is fully created (signal from the bottle manager thread)
 */
void BottleNewAssistant::bottle_created()
{
  // Reset defaults (including timer_.disconnect())
  setDefaultValues();

  // Close Assistant
  this->hide();

  // Inform UI, emit signal new_bottle_finished (causes the GUI to refresh)
  new_bottle_finished.emit();
}

/**
 * \brief Triggered when the apply is pressed
 * apply_changes_gradually
 */
void BottleNewAssistant::on_assistant_apply()
{
  // Guess the time interval based on the user input
  bool is_desktop_enabled = virtual_desktop_check.get_active();
  bool is_non_default_windows = false;
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
  catch (std::invalid_argument& e)
  {
  }
  catch (std::out_of_range& e)
  {
  }
  try
  {
    size_t win_bit_index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    auto currentWindowsBit = BottleTypes::SupportedWindowsVersions.at(win_bit_index);
    is_non_default_windows = WineDefaults::WindowsOs != currentWindowsBit.first;
  }
  catch (const std::runtime_error& error)
  {
  }
  catch (std::invalid_argument& e)
  {
  }
  catch (std::out_of_range& e)
  {
  }

  int time_interval = 300;
  if (is_desktop_enabled)
  {
    time_interval += 90;
  }
  if (is_non_default_windows)
  {
    time_interval += 60;
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
  hide();
}

void BottleNewAssistant::on_assistant_close()
{
  hide();
}

/**
 * \brief Prepare handler for each page, is emitted before making the page visable.
 */
void BottleNewAssistant::on_assistant_prepare(Gtk::Widget* /* widget*/)
{
  set_title(Glib::ustring::compose("Gtk::Assistant example (Page %1 of %2)", get_current_page() + 1, get_n_pages()));

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
    hbox_virtual_desktop.hide();
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
