/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    new_bottle_assistant.cc
 * \brief   New Bottle Assistant (Wizard)
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
#include "new_bottle_assistant.h"
#include "bottle_types.h"
#include <iostream>

/**
 * \brief Contructor
 */
NewBottleAssistant::NewBottleAssistant()
: m_vbox(Gtk::ORIENTATION_VERTICAL, 2),
  m_hbox_name(Gtk::ORIENTATION_HORIZONTAL, 12),
  m_hbox_win(Gtk::ORIENTATION_HORIZONTAL, 12),
  name_label("Name:"),
  windows_version_label("Windows version:"),
  confirm_label("Confirmation page"),
  m_check("Optional extra information")
{
  set_title("New Windows Machine");
  set_border_width(8);
  set_default_size(640, 400);
  // Only focus on assistant, disable interaction with other windows in app
  set_modal(true);

  // Create pages
  createFirstPage();
  createSecondPage();
  createThirdPage();

  signal_apply().connect(sigc::mem_fun(*this,
    &NewBottleAssistant::on_assistant_apply));
  signal_cancel().connect(sigc::mem_fun(*this,
    &NewBottleAssistant::on_assistant_cancel));
  signal_close().connect(sigc::mem_fun(*this,
    &NewBottleAssistant::on_assistant_close));
  signal_prepare().connect(sigc::mem_fun(*this,
    &NewBottleAssistant::on_assistant_prepare));

  show_all_children();
}

/**
 * \brief Destructor
 */
NewBottleAssistant::~NewBottleAssistant()
{
}


/**
 * \brief First page of the wizard
 */
void NewBottleAssistant::createFirstPage()
{
  // Intro page
  intro_label.set_markup("<big><b>Create a New Machine</b></big>\n"
    "Please use a descriptive name for the Windows machine, and select which Windows version you want to use.");
  intro_label.set_margin_bottom(25);
  m_vbox.pack_start(intro_label, false, false);
  
  m_hbox_name.pack_start( name_label, false, true);
  m_hbox_name.pack_start(name_entry);
  m_vbox.pack_start(m_hbox_name, false, false);
  
  // Fill-in Windows versions in combobox
  for(std::vector<BottleTypes::WindowsAndBit>::iterator it = BottleTypes::SupportedWindowsVersions.begin(); it != BottleTypes::SupportedWindowsVersions.end(); ++it)
  {
    auto index = std::distance(BottleTypes::SupportedWindowsVersions.begin(), it);
    windows_version_combobox.insert(-1, std::to_string(index), BottleTypes::toString((*it).first) + " (" + BottleTypes::toString((*it).second) + ')');
  }
  // Set default Windows version selection
  windows_version_combobox.set_active_id(std::to_string(BottleTypes::DefaultBottleIndex));

  m_hbox_win.pack_start(windows_version_label, false, true);
  m_hbox_win.pack_start(windows_version_combobox);
  m_vbox.pack_start(m_hbox_win, false, false);

  name_entry.signal_changed().connect(sigc::mem_fun(*this,
    &NewBottleAssistant::on_entry_changed));

  append_page(m_vbox);
  set_page_type(m_vbox, Gtk::ASSISTANT_PAGE_INTRO);
  set_page_title(*get_nth_page(0), "Choose Name & Windows version");
}

/**
 * \brief Second page of the wizard
 */
void NewBottleAssistant::createSecondPage()
{ 
  append_page(m_check);
  set_page_complete(m_check, true);
  set_page_title(*get_nth_page(1), "Additional settings");
}

/**
 * \brief Last page of the wizard
 */
void NewBottleAssistant::createThirdPage()
{
  // Confirm page
  append_page(confirm_label);
  set_page_complete(confirm_label, true);
  set_page_type(confirm_label, Gtk::ASSISTANT_PAGE_CONFIRM);
  set_page_title(*get_nth_page(2), "Confirmation/Creating *loading bar...?*");
}

/**
 * \brief Retrieve the results (once wizard is finished)
 */
void NewBottleAssistant::get_result(bool& check_state, Glib::ustring& name, Glib::ustring& windows_version)
{
  check_state = m_check.get_active();
  name = name_entry.get_text();
  windows_version = windows_version_combobox.get_active_text();
}

void NewBottleAssistant::on_assistant_apply()
{
  std::cout << "Apply was clicked";
  print_status();
}

void NewBottleAssistant::on_assistant_cancel()
{
  std::cout << "Cancel was clicked";
  print_status();
  hide();
}

void NewBottleAssistant::on_assistant_close()
{
  std::cout << "Assistant was closed";
  print_status();
  hide();
}

void NewBottleAssistant::on_assistant_prepare(Gtk::Widget* /* widget */)
{
  set_title(Glib::ustring::compose("Gtk::Assistant example (Page %1 of %2)",
    get_current_page() + 1, get_n_pages()));
}

void NewBottleAssistant::on_entry_changed()
{
  // The page is only complete if the name entry contains text.
  if(name_entry.get_text_length())
    set_page_complete(m_vbox, true);
  else
    set_page_complete(m_vbox, false);
}

void NewBottleAssistant::print_status()
{
  std::string::size_type sz;
  try {
    // stoi could throw issues when empty string or invalid string is returned
    size_t index = size_t(std::stoi(windows_version_combobox.get_active_id(), &sz));
    const auto currentWindowsBit = BottleTypes::SupportedWindowsVersions.at(index);
    std::cout
    << ", Name: \"" << name_entry.get_text()
    << ", Windows name: \"" << BottleTypes::toString(currentWindowsBit.first)
    << ", Windows bit: \"" << BottleTypes::toString(currentWindowsBit.second)
    << "\", checkbutton status: " << m_check.get_active() << std::endl;
  } catch(const std::exception& ex) {
    std::cout << " No windows version is selected..." << std::endl;
  }
}