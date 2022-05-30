/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    main_window.cc
 * \brief   Main WineGUI window
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
#include "helper.h"
#include "project_config.h"
#include "signal_dispatcher.h"
#include <locale>

/************************
 * Public methods       *
 ************************/

/**
 * \brief Contructor
 */
MainWindow::MainWindow(Menu& menu)
    : window_settings(),
      vbox(Gtk::ORIENTATION_VERTICAL),
      paned(Gtk::ORIENTATION_HORIZONTAL),
      right_box(Gtk::Orientation::ORIENTATION_VERTICAL),
      separator1(Gtk::ORIENTATION_HORIZONTAL),
      busy_dialog_(*this)
{
  // Set some Window properties
  set_title("WineGUI - WINE Manager");
  set_default_size(1100, 600);
  set_position(Gtk::WIN_POS_CENTER);

  try
  {
    set_icon_from_file(Helper::get_image_location("logo.png"));
  }
  catch (Glib::FileError& e)
  {
    cout << "Error: couldn't load our logo: " << e.what() << endl;
  }

  // Add menu to box (top), no expand/fill
  vbox.pack_start(menu, false, false);

  // Add paned to box (below menu)
  // NOTE: expand/fill = true
  vbox.pack_end(paned);

  // Create rest to vbox
  create_left_panel();
  create_right_panel();

  // Using a Vertical box container
  add(vbox);

  // Reset the right panel to default values
  reset_detailed_info();

  // Load window settings from gsettings schema file
  load_stored_window_settings();

  // By default disable the toolbar buttons
  set_sensitive_toolbar_buttons(false);

  // Left side (listbox)
  listbox.signal_row_selected().connect(sigc::mem_fun(*this, &MainWindow::on_row_clicked));
  // Disabled right-click menu for now, since it doesn't activate the right-clicked bottle as active
  // listbox.signal_button_press_event().connect(right_click_menu);

  // Right panel toolbar menu buttons
  // New button pressed signal
  new_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_new_bottle_button_clicked));
  // Apply button signal
  new_bottle_assistant_.signal_apply().connect(sigc::mem_fun(*this, &MainWindow::on_new_bottle_apply));
  // Connect the new bottle assistant signal to the mainWindow signal
  new_bottle_assistant_.new_bottle_finished.connect(finished_new_bottle);

  run_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_run_button_clicked));
  edit_button.signal_clicked().connect(show_edit_window);
  settings_button.signal_clicked().connect(show_settings_window);
  open_c_driver_button.signal_clicked().connect(open_c_drive);
  reboot_button.signal_clicked().connect(reboot_bottle);
  update_button.signal_clicked().connect(update_bottle);
  open_log_file_button.signal_clicked().connect(open_log_file);
  kill_processes_button.signal_clicked().connect(kill_running_processes);

  // Check for update (when GTK is idle)
  Glib::signal_idle().connect_once(sigc::mem_fun(*this, &MainWindow::on_startup_version_update), Glib::PRIORITY_DEFAULT_IDLE);
  // Window closed signal
  signal_delete_event().connect(sigc::mem_fun(this, &MainWindow::delete_window));

  // Show the widget children
  show_all_children();
}

/**
 * \brief Destructor
 */
MainWindow::~MainWindow()
{
}

/**
 * \brief Set a list of bottles to the left panel
 * \param[in] bottles - Wine Bottle item list
 */
void MainWindow::set_wine_bottles(std::list<BottleItem>& bottles)
{
  // Clear whole listbox
  std::vector<Gtk::Widget*> children = listbox.get_children();
  for (Gtk::Widget* el : children)
  {
    listbox.remove(*el);
  }

  for (BottleItem& bottle : bottles)
  {
    listbox.add(bottle);
  }
  // Enable/disable toolbar buttons depending on listbox
  set_sensitive_toolbar_buttons(bottles.size() > 0);
  listbox.show_all();
}

/**
 * \brief set the detailed info panel on the right
 * \param[in] bottle - Wine Bottle item object
 */
void MainWindow::set_detailed_info(BottleItem& bottle)
{
  if (!bottle.is_selected())
    this->listbox.select_row(bottle);

  // Set right side of the GUI
  name.set_text(bottle.name());
  folder_name.set_text(bottle.folder_name());
  Glib::ustring windows = BottleTypes::to_string(bottle.windows());
  windows += " (" + BottleTypes::to_string(bottle.bit()) + ')';
  window_version.set_text(windows);
  Glib::ustring wine_bitness = (bottle.is_wine64_bit()) ? "64-bit" : "32-bit";
  wine_version.set_text(bottle.wine_version() + " (" + wine_bitness + ")");
  wine_location.set_text(bottle.wine_location());
  c_drive_location.set_text(bottle.wine_c_drive());
  wine_last_changed.set_text(bottle.wine_last_changed());
  audio_driver.set_text(BottleTypes::to_string(bottle.audio_driver()));
  Glib::ustring virtual_desktop_text = (bottle.virtual_desktop().empty()) ? "Disabled" : bottle.virtual_desktop();
  virtual_desktop.set_text(virtual_desktop_text);
  Glib::ustring description_text = (bottle.description().empty()) ? "None" : bottle.description();
  description.set_text(description_text);
}

/**
 * \brief Reset the detailed info panel
 */
void MainWindow::reset_detailed_info()
{
  name.set_text("");
  folder_name.set_text("");
  window_version.set_text("");
  wine_version.set_text("");
  wine_location.set_text("");
  c_drive_location.set_text("");
  wine_last_changed.set_text("");
  audio_driver.set_text("");
  virtual_desktop.set_text("");
  description.set_text("");
  // Disable toolbar buttons
  set_sensitive_toolbar_buttons(false);
}

/**
 * \brief Show info message. User can only click 'OK'.
 * \param[in] message Show this information message
 * \param[in] markup Support markup in message text (default: false)
 */
void MainWindow::show_info_message(const Glib::ustring& message, bool markup)
{
  Gtk::MessageDialog dialog(*this, message, markup, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
  dialog.set_title("Information message");
  dialog.set_modal(true);
  dialog.run();
}

/**
 * \brief Show warning message. User can only click 'OK'.
 * \param[in] message Show this warning message
 * \param[in] markup Support markup in message text (default: false)
 */
void MainWindow::show_warning_message(const Glib::ustring& message, bool markup)
{
  Gtk::MessageDialog dialog(*this, message, markup, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
  dialog.set_title("Warning message");
  dialog.set_modal(true);
  dialog.run();
}

/**
 * \brief Show an error message with the provided text. User can only click 'OK'.
 * \param[in] message Show this error message
 * \param[in] markup Support markup in message text (default: false)
 */
void MainWindow::show_error_message(const Glib::ustring& message, bool markup)
{
  Gtk::MessageDialog dialog(*this, message, markup, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
  dialog.set_title("An error has occurred!");
  dialog.set_modal(true);
  dialog.run();
}

/**
 * \brief Confirm dialog (Yes/No message)
 * \param[in] message Show this message during confirmation
 * \param[in] markup Support markup in message text (default: false)
 * \return True if user pressed confirm (yes), otherwise False
 */
bool MainWindow::show_confirm_dialog(const Glib::ustring& message, bool markup)
{
  Gtk::MessageDialog dialog(*this, message, markup, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
  dialog.set_title("Are you sure?");
  dialog.set_modal(true);
  int result = dialog.run();
  bool return_value = false;
  if (result == Gtk::RESPONSE_YES)
  {
    return_value = true;
  }
  return return_value;
}

/**
 * \brief Show busy indicator (like busy installing corefonts in Wine bottle)
 * \param[in] message Given the user more information what is going on
 */
void MainWindow::show_busy_install_dialog(const Glib::ustring& message)
{
  busy_dialog_.set_message("Installing software", message);
  busy_dialog_.show();
}

/**
 * \brief Show busy indicator, with another parent
 * \param[in] parent Parent GTK Window (set to be the GTK transient for)
 * \param[in] message Given the user more information what is going on
 */
void MainWindow::show_busy_install_dialog(Gtk::Window& parent, const Glib::ustring& message)
{
  busy_dialog_.set_message("Installing software", message);
  busy_dialog_.set_transient_for(parent);
  busy_dialog_.show();
}

/**
 * \brief Close the busy dialog again
 */
void MainWindow::close_busy_dialog()
{
  busy_dialog_.hide();
}

/**
 * \brief Signal when the new button is clicked in the top toolbar/menu
 */
void MainWindow::on_new_bottle_button_clicked()
{
  new_bottle_assistant_.set_transient_for(*this);
  new_bottle_assistant_.show();
}

/**
 * \brief Handler when the bottle is created, notify the new bottle assistant.
 * Pass through the signal from the dispatcher to the 'new bottle assistant'.
 */
void MainWindow::on_new_bottle_created()
{
  new_bottle_assistant_.bottle_created();
}

/**
 * \brief Signal when the Run Program... button is clicked in top toolbar/menu
 */
void MainWindow::on_run_button_clicked()
{
  Gtk::FileChooserDialog dialog("Please choose a file", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);

  // Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  auto filter_win = Gtk::FileFilter::create();
  filter_win->set_name("Windows Executable/MSI Installer");
  filter_win->add_mime_type("application/x-ms-dos-executable");
  filter_win->add_mime_type("application/x-msi");
  dialog.add_filter(filter_win);

  auto filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any file");
  filter_any->add_pattern("*");
  dialog.add_filter(filter_any);
  dialog.set_filename(c_drive_location.get_text().c_str());

  // Show the dialog and wait for a user response:
  int result = dialog.run();

  // Handle the response:
  switch (result)
  {
  case (Gtk::RESPONSE_OK):
  {
    string filename = dialog.get_filename();
    // Just guess based on extenstion
    string ext = filename.substr(filename.find_last_of(".") + 1);
    // To lower case
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == "exe")
    {
      run_program.emit(filename, false);
    }
    else if (ext == "msi")
    {
      // Run as MSI (true=MSI)
      run_program.emit(filename, true);
    }
    else
    {
      // fall-back: try run as Exe
      run_program.emit(filename, false);
    }
    break;
  }
  case (Gtk::RESPONSE_CANCEL):
  {
    // Cancelled, do nothing
    break;
  }
  default:
  {
    // Unexpected button, ignore
    break;
  }
  }
}

/**
 * \brief Just hide the main window
 */
void MainWindow::on_hide_window()
{
  hide();
}

/**
 * \brief When the feedback button is pressed
 */
void MainWindow::on_give_feedback()
{
  if (!Gio::AppInfo::launch_default_for_uri("https://gitlab.melroy.org/melroy/winegui/-/issues"))
  {
    this->show_error_message("Could not open browser.");
  }
}

/**
 * \brief When the check version button is pressed
 */
void MainWindow::on_check_version()
{
  check_version_update(true); // Also show message when versions matches
}

/**
 * \brief Not implemented feature
 */
void MainWindow::on_exec_failure()
{
  Gtk::MessageDialog dialog(*this, "\nExecuting the selected Windows application on Wine went wrong.\n", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
  dialog.set_title("An error has occurred during Wine application execution!");
  dialog.set_modal(false);
  dialog.run();
}

/************************
 * Private methods      *
 ************************/

/**
 * \brief Change detailed window on listbox row clicked event
 */
void MainWindow::on_row_clicked(Gtk::ListBoxRow* row)
{
  if (row != nullptr)
  {
    set_detailed_info(*dynamic_cast<BottleItem*>(row));
    // Signal activate Bottle with current BottleItem as parameter to the dispatcher
    // Which updates the connected modules accordingly.
    active_bottle.emit(dynamic_cast<BottleItem*>(row));
  }
}

/**
 * \brief Signal when the new assistant/wizard is finished and applied
 * Retrieve the results from the assistant and send it to the manager (via the dispatcher)
 */
void MainWindow::on_new_bottle_apply()
{
  Glib::ustring name;
  BottleTypes::Windows windows_version;
  BottleTypes::Bit bit;
  Glib::ustring virtual_desktop_resolution;
  bool disable_gecko_mono;
  BottleTypes::AudioDriver audio;

  // Retrieve assistant results
  new_bottle_assistant_.get_result(name, windows_version, bit, virtual_desktop_resolution, disable_gecko_mono, audio);

  // Emit new bottle signal (see dispatcher)
  new_bottle.emit(name, windows_version, bit, virtual_desktop_resolution, disable_gecko_mono, audio);
}

/**
 * \brief Check for WineGUI version, is there an update?
 */
void MainWindow::on_startup_version_update()
{
  check_version_update(); // Without message when versions are equal
}

/**
 * \brief Called when Window is closed/exited
 */
bool MainWindow::delete_window(GdkEventAny* any_event __attribute__((unused)))
{
  if (window_settings)
  {
    // Save the schema settings
    window_settings->set_int("width", get_width());
    window_settings->set_int("height", get_height());
    window_settings->set_boolean("maximized", is_maximized());
  }
  return false;
}

/**
 * \brief Check for WineGUI version, is there an update?
 * \param show_equal Also show user message when the versions matches.
 */
void MainWindow::check_version_update(bool show_equal)
{
  string version = Helper::open_file_from_uri("https://winegui.melroy.org/latest_release.txt");
  // Remove new lines
  version.erase(std::remove(version.begin(), version.end(), '\n'), version.end());
  // Is there a different version? Show the user the message to update to the latest release.
  if (version.compare(PROJECT_VER) != 0)
  {
    string message = "<b>New WineGUI release is out.</b> Please, <i>update</i> WineGUI to the latest release.\n"
                     "You are using: v" +
                     std::string(PROJECT_VER) + ". Latest version: v" + version + ".";
    Gtk::MessageDialog dialog(*this, message, true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
    dialog.set_secondary_text("<big><a href=\"https://gitlab.melroy.org/melroy/winegui/-/releases\">Download the latest release now!</a></big>",
                              true);
    dialog.set_title("New WineGUI Release!");
    dialog.set_modal(true);
    dialog.run();
  }
  else
  {
    if (show_equal)
    {
      show_info_message("WineGUI release is up-to-date. Well done!");
    }
  }
}

/**
 * \brief Load window settings from schema file
 */
void MainWindow::load_stored_window_settings()
{
  // Load schema settings file
  auto schemaSource = Gio::SettingsSchemaSource::get_default()->lookup("org.melroy.winegui", true);
  if (schemaSource)
  {
    window_settings = Gio::Settings::create("org.melroy.winegui");
    // Apply global settings
    set_default_size(window_settings->get_int("width"), window_settings->get_int("height"));
    if (window_settings->get_boolean("maximized"))
      maximize();
  }
  else
  {
    std::cerr << "Error: Gsettings schema file could not be found." << std::endl;
  }
}

/**
 * \brief Create left side of the GUI
 */
void MainWindow::create_left_panel()
{
  // Vertical scroll only
  scrolled_window_listbox.set_policy(Gtk::PolicyType::POLICY_NEVER, Gtk::PolicyType::POLICY_AUTOMATIC);

  // Add scrolled window with listbox to paned
  paned.pack1(scrolled_window_listbox, false, true);
  scrolled_window_listbox.set_size_request(240, -1);

  // Set function that will add seperators between each item
  listbox.set_header_func(sigc::ptr_fun(&MainWindow::cc_list_box_update_header_func));

  // Add list box to scrolled window
  scrolled_window_listbox.add(listbox);
}

/**
 * \brief Create right side of the GUI
 */
void MainWindow::create_right_panel()
{
  // TODO: Make it configurable to only show icons, text or both using preferences
  toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_BOTH);

  // Buttons in toolbar
  Gtk::Image* new_image = Gtk::manage(new Gtk::Image());
  new_image->set_from_icon_name("list-add", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  new_button.set_label("New");
  new_button.set_tooltip_text("Create a new machine!");
  new_button.set_icon_widget(*new_image);
  new_button.set_homogeneous(false);
  toolbar.insert(new_button, 0);

  Gtk::Image* edit_image = Gtk::manage(new Gtk::Image());
  edit_image->set_from_icon_name("document-edit", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  edit_button.set_label("Edit");
  edit_button.set_tooltip_text("Edit Wine Machine");
  edit_button.set_icon_widget(*edit_image);
  edit_button.set_homogeneous(false);
  toolbar.insert(edit_button, 1);

  // Idea: Extra button for the configurations? And call settings just 'install packages'..?

  Gtk::Image* manage_image = Gtk::manage(new Gtk::Image());
  manage_image->set_from_icon_name("preferences-other", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  settings_button.set_label("Settings");
  settings_button.set_tooltip_text("Install additional packages");
  settings_button.set_icon_widget(*manage_image);
  settings_button.set_homogeneous(false);
  toolbar.insert(settings_button, 2);

  Gtk::Image* run_image = Gtk::manage(new Gtk::Image());
  run_image->set_from_icon_name("media-playback-start", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  run_button.set_label("Run Program...");
  run_button.set_tooltip_text("Run exe or msi in Wine Machine");
  run_button.set_icon_widget(*run_image);
  run_button.set_homogeneous(false);
  toolbar.insert(run_button, 3);

  Gtk::Image* open_c_drive_image = Gtk::manage(new Gtk::Image());
  open_c_drive_image->set_from_icon_name("drive-harddisk", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  open_c_driver_button.set_label("Open C: Drive");
  open_c_driver_button.set_tooltip_text("Open the C: drive location in file manager");
  open_c_driver_button.set_icon_widget(*open_c_drive_image);
  open_c_driver_button.set_homogeneous(false);
  toolbar.insert(open_c_driver_button, 4);

  Gtk::Image* reboot_image = Gtk::manage(new Gtk::Image());
  reboot_image->set_from_icon_name("view-refresh", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  reboot_button.set_label("Reboot");
  reboot_button.set_tooltip_text("Simulate Machine Reboot");
  reboot_button.set_icon_widget(*reboot_image);
  reboot_button.set_homogeneous(false);
  toolbar.insert(reboot_button, 5);

  Gtk::Image* update_image = Gtk::manage(new Gtk::Image());
  update_image->set_from_icon_name("system-software-update", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  update_button.set_label("Update Config");
  update_button.set_tooltip_text("Update the Wine Machine configuration");
  update_button.set_icon_widget(*update_image);
  update_button.set_homogeneous(false);
  toolbar.insert(update_button, 6);

  Gtk::Image* open_log_file_image = Gtk::manage(new Gtk::Image());
  open_log_file_image->set_from_icon_name("text-x-generic", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  open_log_file_button.set_label("Open Log");
  open_log_file_button.set_tooltip_text("Open debug logging file");
  open_log_file_button.set_icon_widget(*open_log_file_image);
  open_log_file_button.set_homogeneous(false);
  toolbar.insert(open_log_file_button, 7);

  Gtk::Image* kill_processes_image = Gtk::manage(new Gtk::Image());
  kill_processes_image->set_from_icon_name("process-stop", Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
  kill_processes_button.set_label("Kill Processes");
  kill_processes_button.set_tooltip_text("Kill all running processes in Wine Machine");
  kill_processes_button.set_icon_widget(*kill_processes_image);
  kill_processes_button.set_homogeneous(false);
  toolbar.insert(kill_processes_button, 8);

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

  // Bottle Name
  Gtk::Label* name_label = Gtk::manage(new Gtk::Label("Name:", 0.0, -1));
  name.set_xalign(0.0);
  detail_grid.attach(*name_label, 0, 1, 2, 1);
  detail_grid.attach_next_to(name, *name_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Folder Name
  Gtk::Label* folder_name_label = Gtk::manage(new Gtk::Label("Folder Name:", 0.0, -1));
  folder_name.set_xalign(0.0);
  detail_grid.attach(*folder_name_label, 0, 2, 2, 1);
  detail_grid.attach_next_to(folder_name, *folder_name_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Windows version + bit os
  Gtk::Label* window_version_label = Gtk::manage(new Gtk::Label("Windows:", 0.0, -1));
  window_version.set_xalign(0.0);
  // Label consumes 2 columns
  detail_grid.attach(*window_version_label, 0, 3, 2, 1);
  detail_grid.attach_next_to(window_version, *window_version_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine version
  Gtk::Label* wine_version_label = Gtk::manage(new Gtk::Label("Wine Version:", 0.0, -1));
  wine_version.set_xalign(0.0);
  detail_grid.attach(*wine_version_label, 0, 4, 2, 1);
  detail_grid.attach_next_to(wine_version, *wine_version_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine location
  Gtk::Label* wine_location_label = Gtk::manage(new Gtk::Label("Wine Location:", 0.0, -1));
  wine_location.set_xalign(0.0);
  detail_grid.attach(*wine_location_label, 0, 5, 2, 1);
  detail_grid.attach_next_to(wine_location, *wine_location_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine C drive location
  Gtk::Label* c_drive_location_label = Gtk::manage(new Gtk::Label("C:\\ Drive Location:", 0.0, -1));
  c_drive_location.set_xalign(0.0);
  detail_grid.attach(*c_drive_location_label, 0, 6, 2, 1);
  detail_grid.attach_next_to(c_drive_location, *c_drive_location_label, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Wine last changed
  Gtk::Label* wine_last_changed_label = Gtk::manage(new Gtk::Label("Wine Last Changed:", 0.0, -1));
  wine_last_changed.set_xalign(0.0);
  detail_grid.attach(*wine_last_changed_label, 0, 7, 2, 1);
  detail_grid.attach_next_to(wine_last_changed, *wine_last_changed_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End General
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 8, 3, 1);

  // Audio heading
  Gtk::Image* audio_icon = Gtk::manage(new Gtk::Image());
  audio_icon->set_from_icon_name("audio-speakers", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label* audio_label = Gtk::manage(new Gtk::Label());
  audio_label->set_markup("<b>Audio</b>");
  detail_grid.attach(*audio_icon, 0, 9, 1, 1);
  detail_grid.attach_next_to(*audio_label, *audio_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Audio driver
  Gtk::Label* audio_driver_label = Gtk::manage(new Gtk::Label("Audio Driver:", 0.0, -1));
  audio_driver.set_xalign(0.0);
  detail_grid.attach(*audio_driver_label, 0, 10, 2, 1);
  detail_grid.attach_next_to(audio_driver, *audio_driver_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End Audio driver
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 11, 3, 1);

  // Display heading
  Gtk::Image* display_icon = Gtk::manage(new Gtk::Image());
  display_icon->set_from_icon_name("video-display", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label* display_label = Gtk::manage(new Gtk::Label());
  display_label->set_markup("<b>Display</b>");
  detail_grid.attach(*display_icon, 0, 12, 1, 1);
  detail_grid.attach_next_to(*display_label, *display_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Virtual Desktop
  Gtk::Label* virtual_desktop_label = Gtk::manage(new Gtk::Label("Virtual Desktop\n(Window Mode):", 0.0, -1));
  virtual_desktop.set_xalign(0.0);
  detail_grid.attach(*virtual_desktop_label, 0, 13, 2, 1);
  detail_grid.attach_next_to(virtual_desktop, *virtual_desktop_label, Gtk::PositionType::POS_RIGHT, 1, 1);
  // End Display
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 14, 3, 1);

  // Description heading
  Gtk::Image* description_icon = Gtk::manage(new Gtk::Image());
  description_icon->set_from_icon_name("user-available", Gtk::IconSize(Gtk::ICON_SIZE_MENU));
  Gtk::Label* description_label = Gtk::manage(new Gtk::Label());
  description_label->set_markup("<b>Description</b>");
  detail_grid.attach(*description_icon, 0, 15, 1, 1);
  detail_grid.attach_next_to(*description_label, *description_icon, Gtk::PositionType::POS_RIGHT, 1, 1);

  // Description text
  description.set_xalign(0.0);
  detail_grid.attach(description, 0, 16, 3, 1);
  // End Description
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, 17, 3, 1);

  scrolled_window_grid.add(detail_grid);

  // Add detail grid window to box
  right_box.pack_start(scrolled_window_grid, true, true);

  // Add box to paned
  paned.add2(right_box);
}

/**
 * \brief set sensitive toolbar buttons (eg. when a bottle is active)
 * \param sensitive Set toolbar buttons sensitivity (true is enabled, false is disabled)
 */
void MainWindow::set_sensitive_toolbar_buttons(bool sensitive)
{
  edit_button.set_sensitive(sensitive);
  settings_button.set_sensitive(sensitive);
  run_button.set_sensitive(sensitive);
  open_c_driver_button.set_sensitive(sensitive);
  reboot_button.set_sensitive(sensitive);
  update_button.set_sensitive(sensitive);
  open_log_file_button.set_sensitive(sensitive);
  kill_processes_button.set_sensitive(sensitive);
}

/**
 * \brief Override update header function of GTK Listbox with custom layout
 * \param[in] row
 * \param[in] before
 */
void MainWindow::cc_list_box_update_header_func(Gtk::ListBoxRow* m_row, Gtk::ListBoxRow* before)
{
  GtkWidget* current;
  GtkListBoxRow* row = m_row->gobj();
  if (before == NULL)
  {
    gtk_list_box_row_set_header(row, NULL);
    return;
  }
  current = gtk_list_box_row_get_header(row);
  if (current == NULL)
  {
    current = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show(current);
    gtk_list_box_row_set_header(row, current);
  }
}
