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

#include <iostream>

/**
 * \brief Contructor
 */
NewBottleAssistant::NewBottleAssistant()
: m_vbox(Gtk::ORIENTATION_VERTICAL, 2),
  m_hbox_name(Gtk::ORIENTATION_HORIZONTAL, 12),
  m_hbox_win(Gtk::ORIENTATION_HORIZONTAL, 12),
  intro_label("Please use a descriptive name for the machine, and select which Windows version you want to use."),
  name_label("Name:"),
  windows_version_label("Windows version:"),
  confirm_label("Confirmation page"),
  m_check("Optional extra information")
{
  set_title("New Windows Machine");
  set_border_width(8);
  set_default_size(640, 420);
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

  m_entry.signal_changed().connect(sigc::mem_fun(*this,
    &NewBottleAssistant::on_entry_changed));

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
  set_page_type(m_vbox, Gtk::ASSISTANT_PAGE_INTRO);
  set_page_title(*get_nth_page(0), "Create Windows Machine - Choose Name & Windows version");
  
  m_vbox.pack_start(intro_label);
  
  m_hbox_name.pack_start(name_label);
  m_hbox_name.pack_start(name_entry);
  m_vbox.pack_start(m_hbox_name, false, false);
  
  m_hbox_win.pack_start(windows_version_label);
  m_hbox_win.pack_start(windows_version_entry);
  m_vbox.pack_start(m_hbox_win, false, false);

  append_page(m_vbox);
}

/**
 * \brief Second page of the wizard
 */
void NewBottleAssistant::createSecondPage()
{
  set_page_title(*get_nth_page(1), "Create Windows Machine - Additional settings");
  
  set_page_complete(m_check, true);
  
  append_page(m_check);
}

/**
 * \brief Last page of the wizard
 */
void NewBottleAssistant::createThirdPage()
{
  // Confirm page
  set_page_type(confirm_label, Gtk::ASSISTANT_PAGE_CONFIRM);
  set_page_title(*get_nth_page(2), "Create Windows Machine - Confirmation/Creating *loading bar...?*");
  
  set_page_complete(confirm_label, true);

  append_page(confirm_label);
}

/**
 * \brief Retrieve the results (once wizard is finished)
 */
void NewBottleAssistant::get_result(bool& check_state, Glib::ustring& name, Glib::ustring& windows_version)
{
  check_state = m_check.get_active();
  name = name_entry.get_text();
  windows_version = windows_version_entry.get_text();
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
  std::cout
    << ", Name: \"" << name_entry.get_text()
    << ", Windows version: \"" << windows_version_entry.get_text()
    << "\", checkbutton status: " << m_check.get_active() << std::endl;
}