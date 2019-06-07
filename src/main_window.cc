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
#include "main_window.h"

#include "signal_dispatcher.h"

/**
 * \brief Contructor
 */
MainWindow::MainWindow(Menu& menu)
: vbox(Gtk::ORIENTATION_VERTICAL),
  paned(Gtk::ORIENTATION_HORIZONTAL),
  right_box(Gtk::Orientation::ORIENTATION_VERTICAL),
  separator1(Gtk::ORIENTATION_HORIZONTAL)
{
  // Set some Window properties
  set_title("WineGUI - WINE Manager");
  set_default_size(1000, 600);
  set_position(Gtk::WIN_POS_CENTER_ALWAYS);

  // Add menu to box (top), no expand/fill
  vbox.pack_start(menu, false, false);

  // Add paned to box (below menu)
  // NOTE: expand/fill = true
  vbox.pack_end(paned);

  // Create rest to vbox
  CreateLeftPanel();
  CreateRightPanel();

  // Move this code to the controller!
  std::vector<WineBottle> bottles;
  bottles.push_back(*new WineBottle("Windows 10 (32bit)", "v5.1", "~/.fadsad", "~/.sadasd", "07-07-2019 2:10AM"));
  bottles.push_back(*new WineBottle("Windows 10 (64bit)", BottleTypes::Windows10, BottleTypes::win64, "v5.1", "~/.fadsad", "~/.sadasd", "07-07-2019 2:10AM", BottleTypes::AudioDriver::pulseaudio, "Disabled"));
  bottles.push_back(*new WineBottle("Steam Bottle", BottleTypes::Windows7, BottleTypes::win32, "v5.1", "~/.fadsad", "~/.sadasd", "07-07-2019 2:10AM", BottleTypes::AudioDriver::pulseaudio, "Disabled"));
  SetWineBottles(bottles);

  // Move this code to the controller as well!
  SetDetailedInfo(*new WineBottle("Steam Bottle", BottleTypes::Windows10, BottleTypes::win64, "v4.0.1", "~/.winegui/prefixes/win7_64", "~/.winegui/prefixes/win7_64/dosdevices/c:/", "07-07-2019 2:10AM", BottleTypes::AudioDriver::pulseaudio, "Disabled"));

  // Using a Vertical box container
  add(vbox);

  // Show the widget children
  show_all_children();
}

/**
 * \brief Destructor
 */
MainWindow::~MainWindow() {
}

/**
 * \brief Set signal dispatcher
 */
void MainWindow::SetDispatcher(SignalDispatcher& signalDispatcher)
{
  // Hide signal for immidate response after pressing quit button
  signalDispatcher.hideMainWindow.connect(sigc::mem_fun(*this, &MainWindow::on_hide_window));
}

/**
 * \brief Just hide the main window
 */
void MainWindow::on_hide_window()
{
  hide();
}

/**
 * \brief Set a vector of bottles to the left panel
 * \param[in] bottles - WineBottle vector array
 */
void MainWindow::SetWineBottles(std::vector<WineBottle> bottles)
{
  for (const WineBottle& bottle : bottles)
  {
    Glib::ustring name = bottle.name();
    Glib::ustring bit = BottleTypes::toString(bottle.bit());

    Gtk::Image* image = Gtk::manage(new Gtk::Image());
    image->set("../images/windows/10_" + bit + ".png");
    image->set_margin_top(8);
    image->set_margin_end(8);
    image->set_margin_bottom(8);
    image->set_margin_start(8);

    Gtk::Label* name_label = Gtk::manage(new Gtk::Label());
    name_label->set_xalign(0.0);
    name_label->set_markup("<span size=\"medium\"><b>" + name + "</b></span>");
   
    Gtk::Image* status_icon = Gtk::manage(new Gtk::Image());
    status_icon->set(READY_IMAGE);
    status_icon->set_size_request(2, -1);
    status_icon->set_halign(Gtk::Align::ALIGN_START);

    Gtk::Label* status_label = Gtk::manage(new Gtk::Label("Ready"));
    status_label->set_xalign(0.0);

    Gtk::Grid* row = Gtk::manage(new Gtk::Grid());
    row->set_column_spacing(8);
    row->set_row_spacing(5);
    row->set_border_width(4);

    row->attach(*image, 0, 0, 1, 2);
    // Agh, stupid GTK! Width 2 would be enough, add 8 extra = 10
    // I can't control the gtk grid cell width
    row->attach_next_to(*name_label, *image, Gtk::PositionType::POS_RIGHT, 10, 1);

    row->attach(*status_icon, 1, 1, 1, 1);
    row->attach_next_to(*status_label, *status_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

    // Add the whole grid to the listbox
    listbox.add(*row);
    row->show();
  }
}

/**
 * \brief set the detailed info panel on the right
 * \param[in] bottle - WineBottle object
 */
void MainWindow::SetDetailedInfo(WineBottle bottle)
{
  name.set_text(bottle.name());
  Glib::ustring windows = BottleTypes::toString(bottle.windows());
  windows += " (" + BottleTypes::toString(bottle.bit()) + "-bit)";
  window_version.set_text(windows);
  wine_version.set_text(bottle.wine_version());
  wine_location.set_text(bottle.wine_location());
  c_drive_location.set_text(bottle.wine_c_drive());
  wine_last_changed.set_text(bottle.wine_last_changed());
  audio_driver.set_text(BottleTypes::toString(bottle.audio_driver()));
  virtual_desktop.set_text(bottle.virtual_desktop());
}

/**
 * \brief Just show an error message
 * TODO: add custom string as input
 */
void MainWindow::ShowErrorMessage()
{
  Gtk::MessageDialog* dialog = Gtk::manage(new Gtk::MessageDialog(*this, "An error has occurred!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK));
  dialog->set_modal(Gtk::DialogFlags::DIALOG_DESTROY_WITH_PARENT);
  dialog->run();
}

/**
 * \brief Create left side of the GUI
 */
void MainWindow::CreateLeftPanel()
{
  // Vertical scroll only
  scrolled_window.set_policy(Gtk::PolicyType::POLICY_NEVER, Gtk::PolicyType::POLICY_AUTOMATIC);

  // Add scrolled window with listbox to paned
  paned.pack1(scrolled_window, false, true);
  scrolled_window.set_size_request(240, -1);

  // Set function that will add seperators between each item
  listbox.set_header_func(sigc::ptr_fun(&MainWindow::cc_list_box_update_header_func));

  // Add list box to scrolled window
  scrolled_window.add(listbox);
}

/**
 * \brief Create right side of the GUI
 */
void MainWindow::CreateRightPanel()
{
  toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);

  // Buttons in toolbar
  Gtk::Image* new_image = Gtk::manage(new Gtk::Image());
  new_image->set_from_icon_name("list-add", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::ToolButton* new_button = Gtk::manage(new Gtk::ToolButton(*new_image, "New"));
  toolbar.insert(*new_button, 0);

  Gtk::Image* run_image = Gtk::manage(new Gtk::Image());
  run_image->set_from_icon_name("system-run", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::ToolButton* run_button = Gtk::manage(new Gtk::ToolButton(*run_image, "Run Program..."));
  toolbar.insert(*run_button, 1);

  Gtk::Image* settings_image = Gtk::manage(new Gtk::Image());
  settings_image->set_from_icon_name("preferences-other", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::ToolButton* settings_button = Gtk::manage(new Gtk::ToolButton(*settings_image, "Settings"));
  toolbar.insert(*settings_button, 2);

  Gtk::Image* manage_image = Gtk::manage(new Gtk::Image());
  manage_image->set_from_icon_name("system-software-install", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::ToolButton* manage_button = Gtk::manage(new Gtk::ToolButton(*manage_image, "Manage"));
  toolbar.insert(*manage_button, 3);

  Gtk::Image* reboot_image = Gtk::manage(new Gtk::Image());
  reboot_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  Gtk::ToolButton* reboot_button = Gtk::manage(new Gtk::ToolButton(*reboot_image, "Reboot"));
  toolbar.insert(*reboot_button, 4);

  // Add toolbar to right box
  right_box.add(toolbar);
  right_box.add(separator1);

  // Add detail section below toolbar
  detail_grid.set_margin_top(5);
  detail_grid.set_margin_end(5);
  detail_grid.set_margin_bottom(8);
  detail_grid.set_margin_start(8);
  detail_grid.set_column_spacing(8);
  detail_grid.set_row_spacing(12);

  // General heading
  Gtk::Image* general_icon = Gtk::manage(new Gtk::Image());
  general_icon->set_from_icon_name("computer", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label* general_label = Gtk::manage(new Gtk::Label());
  general_label->set_markup("<b>General</b>");
  detail_grid.attach(*general_icon, 0, 0, 1, 1);
  detail_grid.attach_next_to(*general_label, *general_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Name
  Gtk::Label* name_label = Gtk::manage(new Gtk::Label("Name:", 0.0, -1));
  name.set_xalign(0.0);
  detail_grid.attach(*name_label, 0, 1, 2, 1);
  detail_grid.attach_next_to(name, *name_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Windows version + bit os
  Gtk::Label* window_version_label = Gtk::manage(new Gtk::Label("Windows:", 0.0, -1));
  window_version.set_xalign(0.0);
  // Label consumes 2 columns
  detail_grid.attach(*window_version_label, 0, 2, 2, 1);
  detail_grid.attach_next_to(window_version, *window_version_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine version
  Gtk::Label* wine_version_label = Gtk::manage(new Gtk::Label("Wine Version:", 0.0, -1));
  wine_version.set_xalign(0.0);
  detail_grid.attach(*wine_version_label, 0, 3, 2, 1);
  detail_grid.attach_next_to(wine_version, *wine_version_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine location
  Gtk::Label* wine_location_label = Gtk::manage(new Gtk::Label("Wine Location:", 0.0, -1));
  wine_location.set_xalign(0.0);
  detail_grid.attach(*wine_location_label, 0, 4, 2, 1);
  detail_grid.attach_next_to(wine_location, *wine_location_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine C drive location
  Gtk::Label* c_drive_location_label = Gtk::manage(new Gtk::Label("C: Drive Location:", 0.0, -1));
  c_drive_location.set_xalign(0.0);
  detail_grid.attach(*c_drive_location_label, 0, 5, 2, 1);
  detail_grid.attach_next_to(c_drive_location, *c_drive_location_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine last changed
  Gtk::Label* wine_last_changed_label = Gtk::manage(new Gtk::Label("Wine Last Changed:", 0.0, -1));
  wine_last_changed.set_xalign(0.0);
  detail_grid.attach(*wine_last_changed_label, 0, 6, 2, 1);
  detail_grid.attach_next_to(wine_last_changed, *wine_last_changed_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End General
  detail_grid.attach(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL), 0, 7, 3, 1);

  // Audio heading
  Gtk::Image* audio_icon = Gtk::manage(new Gtk::Image());
  audio_icon->set_from_icon_name("audio-speakers", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label* audio_label = Gtk::manage(new Gtk::Label());
  audio_label->set_markup("<b>Audio</b>");
  detail_grid.attach(*audio_icon, 0, 8, 1, 1);
  detail_grid.attach_next_to(*audio_label, *audio_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Audio driver
  Gtk::Label* audio_driver_label = Gtk::manage(new Gtk::Label("Audio Driver:", 0.0, -1));
  audio_driver.set_xalign(0.0);
  detail_grid.attach(*audio_driver_label, 0, 9, 2, 1);
  detail_grid.attach_next_to(audio_driver, *audio_driver_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End Audio driver
  detail_grid.attach(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL), 0, 10, 3, 1);

  // Display heading
  Gtk::Image* display_icon = Gtk::manage(new Gtk::Image());
  display_icon->set_from_icon_name("video-display", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label* display_label = Gtk::manage(new Gtk::Label());
  display_label->set_markup("<b>Display</b>");
  detail_grid.attach(*display_icon, 0, 11, 1, 1);
  detail_grid.attach_next_to(*display_label, *display_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Virtual Desktop
  Gtk::Label* virtual_desktop_label = Gtk::manage(new Gtk::Label("Virtual Desktop\n(Window Mode):", 0.0, -1));
  virtual_desktop.set_xalign(0.0);
  detail_grid.attach(*virtual_desktop_label, 0, 12, 2, 1);
  detail_grid.attach_next_to(virtual_desktop, *virtual_desktop_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End Display
  detail_grid.attach(*new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL), 0, 13, 3, 1);

  // Add detail grid to box
  right_box.pack_start(detail_grid, false, false);

  // Add box to paned
  paned.add2(right_box);
}

/**
 * \brief Override update header function of GTK Listbox with custom layout
 * \param[in] row
 * \param[in] before
 */
void MainWindow::cc_list_box_update_header_func(Gtk::ListBoxRow* m_row, Gtk::ListBoxRow* before)
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


