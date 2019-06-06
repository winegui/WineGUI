/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    window.cc
 * \brief   GTK+ Window class
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
#include "window.h"

/**
 * \brief Contructor
 */
Window::Window()
: vbox(Gtk::ORIENTATION_VERTICAL),
  paned(Gtk::ORIENTATION_HORIZONTAL)
{
  // Set some Window properties
  this->set_title("WineGUI - WINE Manager");
  this->set_default_size(1000, 600);
  this->set_position(Gtk::WIN_POS_CENTER_ALWAYS);

  // Using a Vertical box container
  this->add(vbox);

  // Create GTK menu
  Menu menu;

  // Add menu to box (top), no expand/fill
  vbox.pack_start(menu, false, false);

  // Add paned to box (below menu)
  // NOTE: expand/fill = true
  vbox.pack_end(paned);

  // Create rest
  CreateLeftPanel();
  CreateRightPanel();

  // Finally, show!
  this->show_all();
}

/**
 * \brief Destructor
 */
Window::~Window() {
}

/**
 * \brief Create left side of the GUI
 */
void Window::CreateLeftPanel()
{
  // Use a scrolled window
  Gtk::ScrolledWindow scrolled_window;
  // Vertical scroll only
  scrolled_window.set_policy(Gtk::PolicyType::POLICY_NEVER, Gtk::PolicyType::POLICY_AUTOMATIC);

  // Add scrolled window with listbox to paned
  paned.pack1(scrolled_window, false, true);
  scrolled_window.set_size_request(240, -1);

  Gtk::ListBox listbox;
  // Set function that will add seperators between each item
  listbox.set_header_func(sigc::ptr_fun(&Window::cc_list_box_update_header_func));
  for (int i=1; i<20; i++)
  {
    Gtk::Image image;
    image.set("../images/windows/10_64.png");
    image.set_margin_top(8);
    image.set_margin_end(8);
    image.set_margin_bottom(8);
    image.set_margin_start(8);

    Gtk::Label name;
    name.set_xalign(0.0);
    name.set_markup("<span size=\"medium\"><b>Windows 10 (64bit)</b></span>");
   
    Gtk::Image status_icon;
    status_icon.set(READY_IMAGE);
    status_icon.set_size_request(2, -1);
    status_icon.set_halign(Gtk::Align::ALIGN_START);

    Gtk::Label status_label("Ready");
    status_label.set_xalign(0.0);

    Gtk::Grid row;
    row.set_column_spacing(8);
    row.set_row_spacing(5);
    row.set_border_width(4);

    row.attach(image, 0, 0, 1, 2);
    // Agh, stupid GTK! Width 2 would be enough, add 8 extra = 10
    // I can't control the gtk grid cell width
    row.attach_next_to(name, image, Gtk::PositionType::POS_RIGHT, 10, 1);

    row.attach(status_icon, 1, 1, 1, 1);
    row.attach_next_to(status_label, status_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

    row.show();
    // Add the whole grid to the listbox
    listbox.add(row);
  }
  // Add list box to scrolled window
  scrolled_window.add(listbox);
}

/**
 * \brief Create right side of the GUI
 */
void Window::CreateRightPanel()
{
  Gtk::Box box(Gtk::Orientation::ORIENTATION_VERTICAL);
  Gtk::Toolbar toolbar;
  toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);

  // Buttons in toolbar
  Gtk::Image new_image;
  new_image.set_from_icon_name("list-add", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::MenuToolButton new_button(new_image, "New");
  toolbar.insert(new_button, 0);

  Gtk::Image run_image;
  run_image.set_from_icon_name("system-run", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::MenuToolButton run_button(run_image, "Run Program...");
  toolbar.insert(run_button, 1);

  Gtk::Image settings_image;
  settings_image.set_from_icon_name("preferences-other", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::MenuToolButton settings_button(settings_image, "Settings");
  toolbar.insert(settings_button, 2);

  Gtk::Image manage_image;
  manage_image.set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::MenuToolButton manage_button(manage_image, "Manage");
  toolbar.insert(manage_button, 3);

  Gtk::Image reboot_image;
  reboot_image.set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::MenuToolButton reboot_button(reboot_image, "Reboot");
  toolbar.insert(reboot_button, 4);

  // Add toolbar to box
  box.add(toolbar);
  box.add(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));

  // Add detail section below toolbar
  Gtk::Grid detail_grid;
  detail_grid.set_margin_top(5);
  detail_grid.set_margin_end(5);
  detail_grid.set_margin_bottom(8);
  detail_grid.set_margin_start(8);
  detail_grid.set_column_spacing(8);
  detail_grid.set_row_spacing(12);

  // General heading
  Gtk::Image general_icon;
  general_icon.set_from_icon_name("computer", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label general_label;
  general_label.set_markup("<b>General</b>");
  detail_grid.attach(general_icon, 0, 0, 1, 1);
  detail_grid.attach_next_to(general_label, general_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Windows version + bit os
  Gtk::Label window_version_label("Windows:");
  window_version_label.set_xalign(0.0);

  Glib::ustring windows, bit;
  windows = "Windows 7";
  bit = "64-bit";
  Glib::ustring windows_text = windows + " (" + bit + ")";
  Gtk::Label window_version(windows_text);
  window_version.set_xalign(0.0);
  // Label consumes 2 columns
  detail_grid.attach(window_version_label, 0, 1, 2, 1);
  detail_grid.attach_next_to(window_version, window_version_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine version
  Gtk::Label wine_version_label("Wine version:");
  wine_version_label.set_xalign(0.0);
  Gtk::Label wine_version("v4.0.1");
  wine_version.set_xalign(0.0);
  detail_grid.attach(wine_version_label, 0, 2, 2, 1);
  detail_grid.attach_next_to(wine_version, wine_version_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine location
  Gtk::Label wine_location_label("Wine location:");
  wine_location_label.set_xalign(0.0);
  Gtk::Label wine_location("~/.winegui/prefixes/win7_64");
  wine_location.set_xalign(0.0);
  detail_grid.attach(wine_location_label, 0, 3, 2, 1);
  detail_grid.attach_next_to(wine_location, wine_location_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine C drive location
  Gtk::Label c_drive_location_label("C: drive location:");
  c_drive_location_label.set_xalign(0.0);
  Gtk::Label c_drive_location("~/.winegui/prefixes/win7_64/dosdevices/c:/");
  c_drive_location.set_xalign(0.0);
  detail_grid.attach(c_drive_location_label, 0, 4, 2, 1);
  detail_grid.attach_next_to(c_drive_location, c_drive_location_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine last changed
  Gtk::Label wine_last_changed_label("Wine last changed:");
  wine_last_changed_label.set_xalign(0.0);
  Gtk::Label wine_last_changed("07-07-2019 23:11AM");
  wine_last_changed.set_xalign(0.0);
  detail_grid.attach(wine_last_changed_label, 0, 5, 2, 1);
  detail_grid.attach_next_to(wine_last_changed, wine_last_changed_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End General
  detail_grid.attach(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL), 0, 6, 3, 1);

  // Audio heading
  Gtk::Image audio_icon;
  audio_icon.set_from_icon_name("audio-speakers", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label audio_label;
  audio_label.set_markup("<b>Audio</b>");
  detail_grid.attach(audio_icon, 0, 7, 1, 1);
  detail_grid.attach_next_to(audio_label, audio_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Audio driver
  Gtk::Label audio_driver_label("Audio driver:");
  audio_driver_label.set_xalign(0.0);
  Gtk::Label audio_driver("Pulseaudio");
  audio_driver.set_xalign(0.0);
  detail_grid.attach(audio_driver_label, 0, 8, 2, 1);
  detail_grid.attach_next_to(audio_driver, audio_driver_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End Audio driver
  detail_grid.attach(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL), 0, 9, 3, 1);

  // Display heading
  Gtk::Image display_icon;
  display_icon.set_from_icon_name("video-display", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label display_label;
  display_label.set_markup("<b>Display</b>");
  detail_grid.attach(display_icon, 0, 10, 1, 1);
  detail_grid.attach_next_to(display_label, display_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Virtual Desktop
  Gtk::Label virtual_desktop_label("Virtual desktop\n(Window Mode):");
  virtual_desktop_label.set_xalign(0.0);
  Gtk::Label virtual_desktop("Disabled");
  virtual_desktop.set_xalign(0.0);
  detail_grid.attach(virtual_desktop_label, 0, 11, 2, 1);
  detail_grid.attach_next_to(virtual_desktop, virtual_desktop_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End Display
  detail_grid.attach(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL), 0, 12, 3, 1);

  // Add detail grid to box
  box.pack_start(detail_grid, false, false);

  // Add box to paned
  paned.add2(box);
}

/**
 * \brief Override update header function of GTK Listbox with custom layout
 * \param[in] row
 * \param[in] before
 */
void Window::cc_list_box_update_header_func(Gtk::ListBoxRow* m_row, Gtk::ListBoxRow* before)
{
  GtkWidget *current;
  GtkListBoxRow *row = m_row->gobj();
  if (before == NULL) {
    gtk_list_box_row_set_header(row, NULL);
    return;
  }
  current = gtk_list_box_row_get_header(row);
  if (current == NULL){
    current = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show(current);
    gtk_list_box_row_set_header(row, current);
  }
}


