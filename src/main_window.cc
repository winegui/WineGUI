/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    main_window.cc
 * \brief   Main WineGUI window
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
#include "main_window.h"
#include "app_list_model_column.h"
#include "dialog_window.h"
#include "gtkmm/enums.h"
#include "gtkmm/object.h"
#include "helper.h"
#include "project_config.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <utility>

/************************
 * Public methods       *
 ************************/

/**
 * \brief Constructor
 */
MainWindow::MainWindow(/*Menu& menu*/)
    : window_settings(),
      vbox(Gtk::Orientation::VERTICAL),
      paned(Gtk::Orientation::HORIZONTAL),
      right_vbox(Gtk::Orientation::VERTICAL),
      app_list_vbox(Gtk::Orientation::VERTICAL),
      app_list_top_hbox(Gtk::Orientation::HORIZONTAL),
      container_paned(Gtk::Orientation::HORIZONTAL),
      separator1(Gtk::Orientation::HORIZONTAL),
      busy_dialog_(*this),
      info_dialog_(*this, DialogWindow::DialogType::INFO),
      warning_dialog_(*this, DialogWindow::DialogType::WARNING),
      error_dialog_(*this, DialogWindow::DialogType::ERROR),
      question_dialog_(*this, DialogWindow::DialogType::QUESTION),
      unknown_menu_item_name_("- Unknown menu item -"),
      unknown_desktop_item_name_("- Unknown desktop item -"),
      thread_check_version_(nullptr)
{
  // Set some Window properties
  set_title("WineGUI - WINE Manager");
  set_default_size(1120, 675);

  // TODO: No more icon via set_icon_from_file.. How to set a icon now using .res resource files?
  // try
  // {
  //   set_icon_from_file(Helper::get_image_location("logo.png"));
  // }
  // catch (Glib::FileError& e)
  // {
  //   cout << "Error: couldn't load our logo: " << e.what() << endl;
  // }

  // Add menu to box (top), no expand/fill
  // There is no GTK::MenuBar anymore ;(
  // vbox.append(menu);

  // Add paned to box (below menu)
  // NOTE: expand/fill = true
  vbox.append(paned);

  // Label alignments
  name_label.set_halign(Gtk::Align::START);
  folder_name_label.set_halign(Gtk::Align::START);
  window_version_label.set_halign(Gtk::Align::START);
  c_drive_location_label.set_halign(Gtk::Align::START);
  wine_version_label.set_halign(Gtk::Align::START);
  wine_location_label.set_halign(Gtk::Align::START);
  debug_log_level_label.set_halign(Gtk::Align::START);
  wine_last_changed_label.set_halign(Gtk::Align::START);
  audio_driver_label.set_halign(Gtk::Align::START);
  virtual_desktop_label.set_halign(Gtk::Align::START);
  description_label.set_halign(Gtk::Align::START);

  // Create rest to vbox
  create_left_panel();
  create_right_panel();

  // Using a Vertical box container
  set_child(vbox);

  // Reset the right panel to default values
  reset_detailed_info();

  // Load window settings from gsettings schema file
  load_stored_window_settings();

  // By default disable the toolbar buttons
  set_sensitive_toolbar_buttons(false);

  // Left side (listbox)
  listbox.signal_row_selected().connect(sigc::mem_fun(*this, &MainWindow::on_bottle_row_clicked));
  // Disabled right-click menu for now, since it doesn't activate the right-clicked bottle as active
  // listbox.signal_button_press_event().connect(right_click_menu);

  // Right panel toolbar menu buttons
  // New button pressed signal
  new_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_new_bottle_button_clicked));
  // Apply button signal
  new_bottle_assistant_.signal_apply().connect(sigc::mem_fun(*this, &MainWindow::on_new_bottle_apply));
  // Connect the new bottle assistant signal to the mainWindow signal
  new_bottle_assistant_.new_bottle_finished.connect(finished_new_bottle);

  // Application search
  app_list_search_entry.signal_search_changed().connect(sigc::mem_fun(*this, &MainWindow::on_app_list_search));

  // Trigger row activated signal on a single click
  app_list_list_view.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_application_row_activated));

  // Toolbar buttons
  run_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_run_button_clicked));
  edit_button.signal_clicked().connect(show_edit_window);
  clone_button.signal_clicked().connect(show_clone_window);
  configure_button.signal_clicked().connect(show_configure_window);
  open_c_driver_button.signal_clicked().connect(open_c_drive);
  reboot_button.signal_clicked().connect(reboot_bottle);
  update_button.signal_clicked().connect(update_bottle);
  open_log_file_button.signal_clicked().connect(open_log_file);
  kill_processes_button.signal_clicked().connect(kill_running_processes);

  // App list buttons
  add_app_list_button.signal_clicked().connect(show_add_app_window);
  remove_app_list_button.signal_clicked().connect(show_remove_app_window);
  refresh_app_list_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_refresh_app_list_button_clicked));

  // Dispatch signals
  error_message_check_version_dispatcher_.connect(sigc::mem_fun(*this, &MainWindow::on_error_message_check_version));
  info_message_check_version_dispatcher_.connect(sigc::mem_fun(*this, &MainWindow::on_info_message_check_version));
  new_version_available_dispatcher_.connect(sigc::mem_fun(*this, &MainWindow::on_new_version_available));
  check_version_finished_dispatcher_.connect(sigc::mem_fun(*this, &MainWindow::cleanup_check_version_thread));

  // Check for update without (error) messages, when app is idle
  Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(*this, &MainWindow::check_version_update), false), Glib::PRIORITY_DEFAULT_IDLE);
  // Window closed signal
  signal_close_request().connect(sigc::mem_fun(*this, &MainWindow::on_delete_window), false);
}

/**
 * \brief Destructor
 */
MainWindow::~MainWindow()
{
  // Avoid zombies
  this->cleanup_check_version_thread();
}

/**
 * \brief Set a list of bottles to the left panel
 * \param[in] bottles - Wine Bottle item list
 */
void MainWindow::set_wine_bottles(std::list<BottleItem>& bottles)
{
  // Clear whole listbox
  auto child = listbox.get_first_child();
  while (child != nullptr)
  {
    auto next = child->get_next_sibling();
    listbox.remove(*child);
    child = next;
  }

  for (BottleItem& bottle : bottles)
  {
    listbox.append(bottle);
  }
  // Enable/disable toolbar buttons depending on listbox
  set_sensitive_toolbar_buttons(bottles.size() > 0);
}

/**
 * \brief Set provided bottle as current selected row (if nothing was selected yet)
 * \param[in] bottle - Wine Bottle item object
 */
void MainWindow::select_row_bottle(BottleItem& bottle)
{
  if (!bottle.is_selected())
    this->listbox.select_row(bottle);
}

/**
 * \brief Reset the detailed info panel
 */
void MainWindow::reset_detailed_info()
{
  name_label.set_text("");
  folder_name_label.set_text("");
  window_version_label.set_text("");
  c_drive_location_label.set_text("");
  wine_version_label.set_text("");
  wine_location_label.set_text("");
  debug_log_level_label.set_text("");
  wine_last_changed_label.set_text("");
  audio_driver_label.set_text("");
  virtual_desktop_label.set_text("");
  description_label.set_text("");
  app_list_search_entry.set_text("");
  // Disable toolbar buttons
  set_sensitive_toolbar_buttons(false);
}

/**
 * \brief Reset/clear application list
 */
void MainWindow::reset_application_list()
{
  app_list_store->remove_all();
  app_list_search_entry.set_text("");
}

/**
 * \brief Set the (latest) general config data
 * \param config_data The general config data
 */
void MainWindow::set_general_config(const GeneralConfigData& config_data)
{
  general_config_data_ = config_data;
}

/**
 * \brief Show info message. User can only click 'OK'.
 * \param[in] message Show this information message
 * \param[in] markup Support markup in message text (default: false)
 */
void MainWindow::show_info_message(const Glib::ustring& message, bool markup)
{
  info_dialog_.set_message(message, markup);
  // Non-blocking present
  info_dialog_.present();
}

/**
 * \brief Show warning message. User can only click 'OK'.
 * \param[in] message Show this warning message
 * \param[in] markup Support markup in message text (default: false)
 */
void MainWindow::show_warning_message(const Glib::ustring& message, bool markup)
{
  warning_dialog_.set_message(message, markup);
  // Non-blocking present
  warning_dialog_.present();
}

/**
 * \brief Show an error message with the provided text. User can only click 'OK'.
 * \param[in] message Show this error message
 * \param[in] markup Support markup in message text (default: false)
 */
void MainWindow::show_error_message(const Glib::ustring& message, bool markup)
{
  error_dialog_.set_message(message, markup);
  // Non-blocking present
  error_dialog_.present();
}

/**
 * \brief Question dialog (Yes/No message)
 * \param[in] message Show this message during confirmation
 * \param[in] markup Support markup in message text (default: false)
 * \return Pointer to a newly allocated DialogWindow object, it will close itself (destroy) when the user clicks 'Yes' or 'No'
 */
DialogWindow* MainWindow::show_question_dialog(const Glib::ustring& message, bool markup)
{
  // new gtk::manage DialogWindow wit hquestion dialog and return the pointer
  DialogWindow* dialog = Gtk::manage(new DialogWindow(*this, DialogWindow::DialogType::QUESTION, message, markup));
  // Non-blocking present
  dialog->present();
  return dialog;
}

/**
 * \brief Show busy indicator (like busy installing corefonts in Wine bottle)
 * \param[in] message Given the user more information what is going on
 */
void MainWindow::show_busy_install_dialog(const Glib::ustring& message)
{
  busy_dialog_.set_message("Installing software", message);
  busy_dialog_.present();
}

/**
 * \brief Show busy indicator, with another parent
 * \param[in] parent Parent GTK Window (set to be the GTK transient for)
 * \param[in] message Given the user more information what is going on
 */
void MainWindow::show_busy_install_dialog(Gtk::Window* parent, const Glib::ustring& message)
{
  busy_dialog_.set_message("Installing software", message);
  busy_dialog_.set_transient_for(*parent);
  busy_dialog_.present();
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
  new_bottle_assistant_.present();
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
  auto dialog = Gtk::FileDialog::create();
  dialog->set_title("Please choose a file");
  dialog->set_modal(true);
  {
    auto folder = Gio::File::create_for_path(c_drive_location_label.get_text());
    if (!folder->get_path().empty())
    {
      dialog->set_initial_folder(folder);
    }
  }

  // Filters
  const auto filters = Gio::ListStore<Gtk::FileFilter>::create();
  auto filter_win = Gtk::FileFilter::create();
  filter_win->set_name("Windows Executable/MSI Installer");
  filter_win->add_mime_type("application/x-ms-dos-executable");
  filter_win->add_mime_type("application/x-msi");
  filters->append(filter_win);
  auto filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any file");
  filter_any->add_pattern("*");
  filters->append(filter_any);
  // Set the filters
  dialog->set_filters(filters);

  dialog->open(*this,
               [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result)
               {
                 try
                 {
                   const auto file = dialog->open_finish(result);
                   string path = file->get_path();
                   // Just guess based on extension
                   string ext = path.substr(path.find_last_of(".") + 1);
                   // To lower case
                   std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
                   if (ext == "exe")
                   {
                     run_executable.emit(path, false);
                   }
                   else if (ext == "msi")
                   {
                     // Run as MSI (true=MSI)
                     run_executable.emit(path, true);
                   }
                   else
                   {
                     // fall-back: try run as Exe
                     run_executable.emit(path, false);
                   }
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
 * \brief Triggered when the user pressed the application list refresh button
 */
void MainWindow::on_refresh_app_list_button_clicked()
{
  Gtk::ListBoxRow* selected_row = listbox.get_selected_row();
  if (selected_row)
  {
    // Refresh the current app list
    const auto* current_bottle = dynamic_cast<BottleItem*>(selected_row);
    set_application_list(current_bottle->wine_location(), current_bottle->app_list());
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
  if (!Gio::AppInfo::launch_default_for_uri("https://github.com/winegui/WineGUI/issues/new"))
  {
    this->show_error_message("Could not open browser.");
  }
}

/**
 * \brief Open issue tickets on GitLab
 */
void MainWindow::on_issue_tickets()
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
  Gtk::MessageDialog dialog(*this, "\nExecuting the selected Windows application on Wine went wrong.\n", false, Gtk::MessageType::INFO,
                            Gtk::ButtonsType::OK);
  dialog.set_title("An error has occurred during Wine application execution!");
  dialog.set_modal(false);
  dialog.present();
}

/************************
 * Private methods      *
 ************************/

/**
 * \brief Change detailed window on listbox row clicked event
 * \param row Row that is selected/clicked
 */
void MainWindow::on_bottle_row_clicked(Gtk::ListBoxRow* row)
{
  if (row != nullptr)
  {
    auto current_bottle = dynamic_cast<BottleItem*>(row);
    // Set bottle details
    set_detailed_info(*current_bottle);
    // Set application list
    set_application_list(current_bottle->wine_location(), current_bottle->app_list());
    // Clear the application filter
    app_list_search_entry.set_text("");

    // Signal activate Bottle with current BottleItem as parameter to the dispatcher
    // Which updates the connected modules accordingly.
    active_bottle.emit(current_bottle);
  }
}

void MainWindow::on_app_list_search()
{
  string search_text = app_list_search_entry.get_text();
  if (!search_text.empty())
  {
    app_list_filter->set_search(search_text);
    app_list_selection_model->set_model(app_list_filter_list_model);
  }
  else
  {
    // Show all
    app_list_selection_model->set_model(app_list_store);
  }
}

void MainWindow::on_application_row_activated(unsigned int position)
{
  auto col = app_list_store->get_item(position);
  if (!col)
    return;

  // Run the command
  run_program.emit(col->command);
}

/**
 * \brief Signal when the new assistant/wizard is finished and applied
 * Retrieve the results from the assistant and send it to the manager (via the dispatcher)
 */
void MainWindow::on_new_bottle_apply()
{
  // Retrieve assistant results
  auto [name, windows_version, bit, vd_res, disable_gecko_mono, audio] = new_bottle_assistant_.get_result();

  // Emit new bottle signal (see dispatcher)
  new_bottle.emit(name, windows_version, bit, vd_res, disable_gecko_mono, audio);
}

/**
 * \brief Show error messages from the version check thread
 */
void MainWindow::on_error_message_check_version()
{
  this->cleanup_check_version_thread();

  {
    std::lock_guard<std::mutex> lock(error_message_mutex_);
    show_error_message(error_message_);
  }
}

/**
 * \brief Show info messages from the version check thread
 */
void MainWindow::on_info_message_check_version()
{
  this->cleanup_check_version_thread();

  {
    std::lock_guard<std::mutex> lock(info_message_mutex_);
    show_info_message(info_message_);
  }
}

/**
 * \brief Show new version available dialog from the version check thread
 */
void MainWindow::on_new_version_available()
{
  this->cleanup_check_version_thread();

  // Show dialog
  {
    std::lock_guard<std::mutex> lock(new_version_mutex_);
    string message = "<b>New WineGUI release is out.</b> Please, <i>update</i> WineGUI to the latest release.\n"
                     "You are using: v" +
                     std::string(PROJECT_VER) + ". Latest version: v" + new_version_ + ".";
    Gtk::MessageDialog dialog(*this, message, true, Gtk::MessageType::WARNING, Gtk::ButtonsType::OK);
    dialog.set_secondary_text("<big><a href=\"https://gitlab.melroy.org/melroy/winegui/-/releases\">Download the latest release now!</a></big>",
                              true);
    dialog.set_title("New WineGUI Release!");
    dialog.set_modal(true);
    dialog.present();
  }
}

/**
 * \brief Called when Window is closed/exited
 */
bool MainWindow::on_delete_window()
{
  if (window_settings)
  {
    // Save the schema settings
    window_settings->set_int("width", get_width());
    window_settings->set_int("height", get_height());
    window_settings->set_boolean("maximized", is_maximized());
    // window_settings->set_boolean("fullscreen", is_fullscreen());
    if (paned.get_position() > 0)
      window_settings->set_int("position-divider-paned", paned.get_position());
    if (container_paned.get_position() > 0)
      window_settings->set_int("position-divider-container-paned", container_paned.get_position());
  }
  return false;
}

/**
 * \brief set the detailed info panel on the right
 * \param[in] bottle - Wine Bottle item object
 */
void MainWindow::set_detailed_info(const BottleItem& bottle)
{
  // Set right side of the GUI
  name_label.set_text(bottle.name());
  folder_name_label.set_text(bottle.folder_name());
  Glib::ustring windows = BottleTypes::to_string(bottle.windows());
  windows += " (" + BottleTypes::to_string(bottle.bit()) + ')';
  window_version_label.set_text(windows);
  c_drive_location_label.set_text(bottle.wine_c_drive());
  // Hide which wine bitness is used, since users think they need to run 64-bit,
  // while they shouldn't.
  // Glib::ustring wine_bitness = (bottle.is_wine64_bit()) ? "64-bit" : "32-bit";
  wine_version_label.set_text(bottle.wine_version());
  if (Helper::is_default_wine_bottle(bottle.wine_location()))
  {
    wine_location_label.set_text(bottle.wine_location() + " - ⚠ Default Wine prefix");
  }
  else
  {
    wine_location_label.set_text(bottle.wine_location());
  }

  Glib::ustring debug_log_level_str = BottleTypes::debug_log_level_to_string(bottle.debug_log_level());
  if (!bottle.is_debug_logging())
    debug_log_level_str = "<s>" + debug_log_level_str + "</s>"; // Strikethrough when logging is disabled

  Glib::ustring log_level_prefix_str = (!bottle.is_debug_logging()) ? "Logging is disabled - " : "";
  debug_log_level_label.set_markup(log_level_prefix_str + debug_log_level_str);
  wine_last_changed_label.set_text(bottle.wine_last_changed());
  audio_driver_label.set_text(BottleTypes::to_string(bottle.audio_driver()));
  Glib::ustring virtual_desktop_text = (bottle.virtual_desktop().empty()) ? "Disabled" : bottle.virtual_desktop();
  virtual_desktop_label.set_text(virtual_desktop_text);
  Glib::ustring description_text = (bottle.description().empty()) ? "None" : bottle.description();
  description_label.set_text(description_text);
}

/**
 * \brief Set application list
 * \param prefix_path Wine bottle prefix
 * \param app_List Custom application list for this bottle
 */
void MainWindow::set_application_list(const string& prefix_path, const std::map<int, ApplicationData>& app_list)
{
  // First clear list + clear search entry
  reset_application_list();

  // First add the custom application items
  for (const auto& [_, app_data] : app_list)
  {
    string command = app_data.command;
    string icon = Helper::string_to_icon(command);
    add_application(app_data.name, app_data.description, command, icon);
  }

  // Temporally store the list of menu item names,
  // used for checking for duplicates when adding desktop items
  std::set<std::string> menu_item_names;
  // Fill the application list (list view)
  // First the start menu apps/games (if present)
  try
  {
    auto menu_items = Helper::get_menu_items(prefix_path);
    for (const string& item : menu_items)
    {
      string name = unknown_menu_item_name_;
      bool is_icon_full_path = false;
      string icon, comment;
      // Only continue further if the item is not empty
      if (!item.empty())
      {
        size_t found = item.find_last_of('\\');
        size_t subtract = found + 5; // Remove the .lnk part as well using substr
        if (found != string::npos && item.length() >= subtract)
        {
          // Get the name only
          name = item.substr(found + 1, item.length() - subtract);
        }
        try
        {
          std::tie(icon, comment) = Helper::get_menu_program_icon_path_and_comment(item);
          is_icon_full_path = true;
        }
        catch (const Glib::FileError& error)
        {
          std::cerr << "WARN: Linux desktop file couldn't be found for menu item: " << item << std::endl;
        }
        catch (const std::runtime_error& error)
        {
          std::cerr << "WARN: Could not retrieve menu icon: " << error.what() << std::endl;
        }
        catch (const std::exception& error)
        {
          std::cerr << "ERROR: Something really went wrong trying to get the menu icon: " << error.what() << std::endl;
        }
        if (icon.empty())
        {
          // If desktop file could not be found; use the Windows shortcut (lnk) file to retrieve the target path
          // For example: "C:\Program Files\Game\game.exe (which will get an icon for the .exe file extension)
          try
          {
            icon = Helper::get_program_icon_from_shortcut_file(prefix_path, item);
            is_icon_full_path = false;
          }
          catch (const Glib::FileError& error)
          {
            std::cerr << "WARN: Windows shortcut file couldn't be found for menu item: " << item << std::endl;
          }
          catch (const std::runtime_error& error)
          {
            // Ignore if Windows target path could not be found
          }
          catch (const std::exception& error)
          {
            std::cerr << "ERROR: Something really went wrong trying to get the menu icon from the shortcut file: " << error.what() << std::endl;
          }
        }
      }
      else
      {
        std::cerr << "WARN: Menu item is empty, so expect an unknown menu item." << std::endl;
      }
      // Fall-back (keep in mind, menu item has almost always a .lnk file extension)
      if (icon.empty())
      {
        icon = Helper::string_to_icon(item);
        is_icon_full_path = false;
      }
      add_application(name, comment, item, icon, is_icon_full_path);
      // Also add the name to your list, used for finding duplicates when adding desktop files
      if (name != unknown_menu_item_name_)
        menu_item_names.insert(name);
    }
  }
  catch (const std::runtime_error& error)
  {
    cout << "Error: " << error.what() << std::endl;
  }

  // Secondly, the desktop items
  try
  {
    auto desktop_items = Helper::get_desktop_items(prefix_path);
    for (const auto& [value_name, value_data] : desktop_items)
    {
      string name = unknown_desktop_item_name_;
      if (!value_data.empty())
      {
        size_t found = value_data.find_last_of('\\');
        size_t subtract = found + 5; // Remove the .lnk part as well using substr
        if (found != string::npos && value_data.length() >= subtract)
        {
          // Get the name only
          name = value_data.substr(found + 1, value_data.length() - subtract);
        }
      }
      else
      {
        std::cerr << "ERROR: Desktop value data is empty, so expect the desktop item to not work." << std::endl;
      }

      // Only add the desktop item if the item is not found in the list of menu items
      if (menu_item_names.find(name) == menu_item_names.end())
      {
        string icon;
        bool is_icon_full_path = false;
        // Only continue further if the value name is not empty
        if (!value_name.empty())
        {
          try
          {
            icon = Helper::get_desktop_program_icon_path(prefix_path, value_name);
            is_icon_full_path = true;
          }
          catch (const Glib::FileError& error)
          {
            std::cerr << "WARN: Linux desktop file couldn't be found for desktop item: " << value_name << std::endl;
          }
          if (icon.empty())
          {
            // If desktop file could not be found; use the Windows shortcut (lnk) file to retrieve the target path
            // For example: "C:\Program Files\Game\game.exe (which will get an icon for the .exe file extension)
            try
            {
              icon = Helper::get_program_icon_from_shortcut_file(prefix_path, value_name);
              is_icon_full_path = false; // just to be sure
            }
            catch (const Glib::FileError& error)
            {
              std::cerr << "WARN: Windows shortcut file couldn't be found for desktop item: " << value_name << std::endl;
            }
            catch (const std::runtime_error& error)
            {
              // Ignore if Windows target path could not be found
            }
          }
        }
        else
        {
          std::cerr << "WARN: Desktop value name is empty, expect a fallback desktop icon." << std::endl;
        }
        // Fall-back (keep in mind, desktop item are almost always a .desktop file extension)
        if (icon.empty())
        {
          icon = Helper::string_to_icon(value_name);
          is_icon_full_path = false;
        }
        add_application(name, "", value_data, icon, is_icon_full_path);
      }
    }
  }
  catch (const std::runtime_error& error)
  {
    cout << "Error: " << error.what() << std::endl;
  }

  // Lastly, the additional programs
  add_application("Wine Config", "Wine configuration program", "winecfg", "winecfg");
  add_application("Uninstaller", "Remove programs", "uninstaller", "uninstaller");
  add_application("Control Panel", "Wine control panel", "control", "winecontrol");
  add_application("WineMine", "Wine Minesweeper single-player game", "winemine", "minesweeper");
  add_application("Winetricks", "Wine helper script to download and install various libraries", Helper::get_winetricks_location() + " --gui -q",
                  "winetricks");
  add_application("Notepad", "Text editor", "notepad", "notepad");
  add_application("File Manager", "Wine File manager", "winefile", "winefile");
  add_application("Internet Explorer", "Wine Internet Explorer", "iexplore", "internet_explorer");
  add_application("Task Manager", "Task Manager", "taskmgr", "task_manager");
  add_application("File Explorer", "File explorer", "explorer", "file_explorer");
  add_application("Command Prompt", "Command-line interpreter", "wineconsole", "command_prompt");
  add_application("Registry editor", "Windows registry editor", "regedit", "regedit");
  add_application("Wine OLE View", "Windows OLE object viewer", "oleview", "oleview");
}

/**
 * \brief Add application to tree model list
 * \param name Application name
 * \param description Application description
 * \param command Application command
 * \param icon Application icon (icon name or full path to icon)
 * \param is_icon_full_path (Optionally) Use icon as full path (default: false, meaning icon is only the icon file name)
 */
void MainWindow::add_application(const string& name, const string& description, const string& command, const string& icon, bool is_icon_full_path)
{
  auto pixbuf = Gdk::Pixbuf::create_from_file(Helper::get_image_location("apps/unknown_file.png"));
  try
  {
    if (!is_icon_full_path)
      pixbuf = Gdk::Pixbuf::create_from_file(Helper::get_image_location("apps/" + icon + ".png"));
    else
      pixbuf = Gdk::Pixbuf::create_from_file(icon); // Use icon as full path
  }
  catch (const Glib::Error& error)
  {
    std::cerr << "ERROR: Could not find icon (" << icon << ") for app " << name << ": " << error.what() << std::endl;
  }

  auto item =
      AppListModelColumns::create(Helper::encode_text(name), Helper::encode_text(description), Gdk::Texture::create_for_pixbuf(pixbuf), command);
  app_list_store->append(item);
}

/**
 * \brief Helper method for cleaning the manage thread.
 */
void MainWindow::cleanup_check_version_thread()
{
  if (thread_check_version_)
  {
    if (thread_check_version_->joinable())
      thread_check_version_->join();
    delete thread_check_version_;
    thread_check_version_ = nullptr;
  }
}

/**
 * \brief Check for WineGUI version, is there an update?
 * \param show_equal_or_error Also show message when the versions matches or an error occurs.
 */
void MainWindow::check_version_update(bool show_equal_or_error)
{
  if (thread_check_version_)
  {
    if (show_equal_or_error)
    {
      show_error_message("WineGUI version check is stilling running. Please try again later.");
    }
  }
  else
  {
    // Start the check version thread
    thread_check_version_ = new std::thread([this, show_equal_or_error] { check_version(show_equal_or_error); });
  }
}

/**
 * \brief Check WineGUI version (runs in thread)
 * \param[in] show_equal_or_error Also show message when the versions matches or an error occurs.
 */
void MainWindow::check_version(bool show_equal_or_error)
{
  string version = Helper::open_file_from_uri("https://winegui.melroy.org/latest_release.txt");
  // Remove new lines
  version.erase(std::remove(version.begin(), version.end(), '\n'), version.end());
  if (!version.empty())
  {
    // Is there a different version? Signal a new version available.
    if (version.compare(PROJECT_VER) != 0)
    {
      {
        std::lock_guard<std::mutex> lock(new_version_mutex_);
        new_version_ = version;
      }
      this->new_version_available_dispatcher_.emit(); // Will eventually show a dialog
      return;
    }
    else
    {
      if (show_equal_or_error)
      {

        {
          std::lock_guard<std::mutex> lock(info_message_mutex_);
          info_message_ = "WineGUI release is up-to-date. Well done!";
        }
        this->info_message_check_version_dispatcher_.emit();
        return;
      }
    }
  }
  else
  {
    if (show_equal_or_error)
    {
      {
        std::lock_guard<std::mutex> lock(error_message_mutex_);
        error_message_ = "We could not determine the latest WineGUI version. Try again later.";
      }
      this->error_message_check_version_dispatcher_.emit();
      return;
    }
  }
  this->check_version_finished_dispatcher_.emit(); // Clean-up the thread pointer
}

/**
 * \brief Load window settings from schema file
 */
void MainWindow::load_stored_window_settings()
{
  // Load schema settings file
  auto schema_source = Gio::SettingsSchemaSource::get_default()->lookup("org.melroy.winegui", true);
  // Can we find it?
  if (schema_source)
  {
    window_settings = Gio::Settings::create("org.melroy.winegui");

    // Apply global settings
    set_default_size(window_settings->get_int("width"), window_settings->get_int("height"));
    if (window_settings->get_boolean("maximized"))
      maximize();
    // if (window_settings->get_boolean("fullscreen"))
    //   fullscreen();
    int position_divider_paned = window_settings->get_int("position-divider-paned");
    paned.set_position(position_divider_paned);
    int position_divider_container_paned = window_settings->get_int("position-divider-container-paned");
    container_paned.set_position(position_divider_container_paned);
  }
  else
  {
    std::cerr << "Error: Gsettings schema file could not be found." << std::endl;
    // Fallback values
    paned.set_position(320);
    container_paned.set_position(480);
  }
}

/**
 * \brief Create left side of the GUI
 */
void MainWindow::create_left_panel()
{
  // Add scrolled window with listbox to paned
  paned.set_start_child(scrolled_window_listbox);

  // Set function that will add separators between each item
  listbox.set_header_func(sigc::ptr_fun(&MainWindow::cc_list_box_update_header_func));

  // Add list box to scrolled window
  scrolled_window_listbox.set_child(listbox);
}

/**
 * \brief Create right side of the GUI
 */
void MainWindow::create_right_panel()
{
  /***
   * Toolbar section
   * TODO: Make it configurable to only show icons, text or both using preferences
   */
  toolbar.set_margin_start(16);
  toolbar.set_margin_top(6);
  toolbar.set_margin_bottom(6);
  toolbar.set_spacing(6);
  toolbar.set_orientation(Gtk::Orientation::HORIZONTAL);

  // Buttons in toolbar
  Gtk::Image* new_image = Gtk::manage(new Gtk::Image());
  new_image->set_from_icon_name("list-add");
  Gtk::Label* new_label = Gtk::manage(new Gtk::Label("New"));
  Gtk::Box* new_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  new_box->append(*new_image);
  new_box->append(*new_label);

  new_button.set_tooltip_text("Create a new machine!");
  new_button.set_child(*new_box);
  toolbar.append(new_button);

  Gtk::Image* edit_image = Gtk::manage(new Gtk::Image());
  edit_image->set_from_icon_name("document-edit");
  Gtk::Label* edit_label = Gtk::manage(new Gtk::Label("Edit"));
  Gtk::Box* edit_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  edit_box->append(*edit_image);
  edit_box->append(*edit_label);

  edit_button.set_child(*edit_box);
  edit_button.set_tooltip_text("Edit Wine Machine");
  toolbar.append(edit_button);

  Gtk::Image* clone_image = Gtk::manage(new Gtk::Image());
  clone_image->set_from_icon_name("edit-copy");
  Gtk::Label* clone_label = Gtk::manage(new Gtk::Label("Clone"));
  Gtk::Box* clone_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  clone_box->append(*clone_image);
  clone_box->append(*clone_label);

  clone_button.set_child(*clone_box);
  clone_button.set_tooltip_text("Clone Wine Machine");
  toolbar.append(clone_button);

  Gtk::Image* manage_image = Gtk::manage(new Gtk::Image());
  manage_image->set_from_icon_name("preferences-other");
  Gtk::Label* manage_label = Gtk::manage(new Gtk::Label("Configure"));
  Gtk::Box* manage_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  manage_box->append(*manage_image);
  manage_box->append(*manage_label);

  configure_button.set_child(*manage_box);
  configure_button.set_tooltip_text("Install additional packages");
  toolbar.append(configure_button);

  Gtk::Image* run_image = Gtk::manage(new Gtk::Image());
  run_image->set_from_icon_name("media-playback-start");
  Gtk::Label* run_label = Gtk::manage(new Gtk::Label("Run Program..."));
  Gtk::Box* run_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  run_box->append(*run_image);
  run_box->append(*run_label);

  run_button.set_child(*run_box);
  run_button.set_tooltip_text("Run exe or msi in Wine Machine");
  toolbar.append(run_button);

  Gtk::Image* open_c_drive_image = Gtk::manage(new Gtk::Image());
  open_c_drive_image->set_from_icon_name("drive-harddisk");
  Gtk::Label* open_c_drive_label = Gtk::manage(new Gtk::Label("Open C: Drive"));
  Gtk::Box* open_c_drive_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  open_c_drive_box->append(*open_c_drive_image);
  open_c_drive_box->append(*open_c_drive_label);

  open_c_driver_button.set_child(*open_c_drive_box);
  open_c_driver_button.set_tooltip_text("Open the C: drive location in file manager");
  toolbar.append(open_c_driver_button);

  Gtk::Image* reboot_image = Gtk::manage(new Gtk::Image());
  reboot_image->set_from_icon_name("view-refresh");
  Gtk::Label* reboot_label = Gtk::manage(new Gtk::Label("Reboot"));
  Gtk::Box* reboot_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  reboot_box->append(*reboot_image);
  reboot_box->append(*reboot_label);

  reboot_button.set_child(*reboot_box);
  reboot_button.set_tooltip_text("Simulate Machine Reboot");
  toolbar.append(reboot_button);

  Gtk::Image* update_image = Gtk::manage(new Gtk::Image());
  update_image->set_from_icon_name("system-software-update");
  Gtk::Label* update_label = Gtk::manage(new Gtk::Label("Update Config"));
  Gtk::Box* update_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  update_box->append(*update_image);
  update_box->append(*update_label);

  update_button.set_child(*update_box);
  update_button.set_tooltip_text("Update the Wine Machine configuration");
  toolbar.append(update_button);

  Gtk::Image* open_log_file_image = Gtk::manage(new Gtk::Image());
  open_log_file_image->set_from_icon_name("text-x-generic");
  Gtk::Label* open_log_file_label = Gtk::manage(new Gtk::Label("Open Log"));
  Gtk::Box* open_log_file_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  open_log_file_box->append(*open_log_file_image);
  open_log_file_box->append(*open_log_file_label);

  open_log_file_button.set_child(*open_log_file_box);
  open_log_file_button.set_tooltip_text("Open debug logging file");
  toolbar.append(open_log_file_button);

  Gtk::Image* kill_processes_image = Gtk::manage(new Gtk::Image());
  kill_processes_image->set_from_icon_name("process-stop");
  Gtk::Label* kill_processes_label = Gtk::manage(new Gtk::Label("Kill Processes"));
  Gtk::Box* kill_processes_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
  kill_processes_box->append(*kill_processes_image);
  kill_processes_box->append(*kill_processes_label);

  kill_processes_button.set_child(*kill_processes_box);
  kill_processes_button.set_tooltip_text("Kill all running processes in Wine Machine");
  toolbar.append(kill_processes_button);

  // Add toolbar to right vbox
  right_vbox.append(toolbar);
  right_vbox.append(separator1);

  /**
   * Detail section (below toolbar)
   */
  detail_grid.set_margin_top(5);
  detail_grid.set_margin_end(5);
  detail_grid.set_margin_bottom(8);
  detail_grid.set_margin_start(8);
  detail_grid.set_column_spacing(8);
  detail_grid.set_row_spacing(12);
  detail_grid.set_hexpand(true);
  detail_grid.set_vexpand(false);

  // General heading
  Gtk::Image* general_icon = Gtk::manage(new Gtk::Image());
  // TODO:  Gtk::IconSize(Gtk::ICON_SIZE_MENU) is just removed from set_from_icon_name in gtkmm-4.0
  general_icon->set_from_icon_name("dialog-information");
  general_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* general_label = Gtk::manage(new Gtk::Label());
  general_label->set_markup("<b>General</b>");
  detail_grid.attach(*general_icon, 0, 0, 1, 1);
  detail_grid.attach_next_to(*general_label, *general_icon, Gtk::PositionType::RIGHT, 1, 1);

  // Bottle Name
  Gtk::Label* name_text_label = Gtk::manage(new Gtk::Label("Name:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*name_text_label, 0, 1, 2, 1);
  detail_grid.attach_next_to(name_label, *name_text_label, Gtk::PositionType::RIGHT, 1, 1);

  // Folder Name
  Gtk::Label* folder_name_text_label = Gtk::manage(new Gtk::Label("Folder Name:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*folder_name_text_label, 0, 2, 2, 1);
  detail_grid.attach_next_to(folder_name_label, *folder_name_text_label, Gtk::PositionType::RIGHT, 1, 1);
  // End General
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, 3, 3, 1);

  // System heading
  Gtk::Image* system_icon = Gtk::manage(new Gtk::Image());
  // TODO:  Gtk::IconSize(Gtk::ICON_SIZE_MENU) is just removed from set_from_icon_name in gtkmm-4.0
  system_icon->set_from_icon_name("computer");
  system_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* system_label = Gtk::manage(new Gtk::Label());
  system_label->set_markup("<b>System</b>");
  detail_grid.attach(*system_icon, 0, 4, 1, 1);
  detail_grid.attach_next_to(*system_label, *system_icon, Gtk::PositionType::RIGHT, 1, 1);

  // Windows version + bit os
  Gtk::Label* window_version_text_label = Gtk::manage(new Gtk::Label("Windows:", Gtk::Align::START, Gtk::Align::CENTER));
  // Label consumes 2 columns
  detail_grid.attach(*window_version_text_label, 0, 5, 2, 1);
  detail_grid.attach_next_to(window_version_label, *window_version_text_label, Gtk::PositionType::RIGHT, 1, 1);

  // C:\ drive location
  Gtk::Label* c_drive_location_text_label = Gtk::manage(new Gtk::Label("C: Drive Location:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*c_drive_location_text_label, 0, 6, 2, 1);
  detail_grid.attach_next_to(c_drive_location_label, *c_drive_location_text_label, Gtk::PositionType::RIGHT, 1, 1);
  // End system
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, 7, 3, 1);

  // Wine heading
  Gtk::Image* wine_icon = Gtk::manage(new Gtk::Image());
  wine_icon->set_from_icon_name("dialog-information");
  wine_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* wine_label = Gtk::manage(new Gtk::Label());
  wine_label->set_markup("<b>Wine details</b>");
  detail_grid.attach(*wine_icon, 0, 8, 1, 1);
  detail_grid.attach_next_to(*wine_label, *wine_icon, Gtk::PositionType::RIGHT, 1, 1);

  // Wine version
  Gtk::Label* wine_version_text_label = Gtk::manage(new Gtk::Label("Wine Version:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*wine_version_text_label, 0, 9, 2, 1);
  detail_grid.attach_next_to(wine_version_label, *wine_version_text_label, Gtk::PositionType::RIGHT, 1, 1);

  // Wine debug log level
  Gtk::Label* wine_log_level_text_label = Gtk::manage(new Gtk::Label("Log level:", Gtk::Align::START, Gtk::Align::CENTER));
  debug_log_level_label.set_tooltip_text("Enable debug logging in Edit Window");
  detail_grid.attach(*wine_log_level_text_label, 0, 10, 2, 1);
  detail_grid.attach_next_to(debug_log_level_label, *wine_log_level_text_label, Gtk::PositionType::RIGHT, 1, 1);

  // Wine location
  Gtk::Label* wine_location_text_label = Gtk::manage(new Gtk::Label("Wine Location:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*wine_location_text_label, 0, 11, 2, 1);
  detail_grid.attach_next_to(wine_location_label, *wine_location_text_label, Gtk::PositionType::RIGHT, 1, 1);

  // Wine last changed
  Gtk::Label* wine_last_changed_text_label = Gtk::manage(new Gtk::Label("Wine Last Changed:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*wine_last_changed_text_label, 0, 12, 2, 1);
  detail_grid.attach_next_to(wine_last_changed_label, *wine_last_changed_text_label, Gtk::PositionType::RIGHT, 1, 1);
  // End Wine
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, 13, 3, 1);

  // Audio heading
  Gtk::Image* audio_icon = Gtk::manage(new Gtk::Image());
  audio_icon->set_from_icon_name("audio-speakers");
  audio_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* audio_text_label = Gtk::manage(new Gtk::Label());
  audio_text_label->set_markup("<b>Audio</b>");
  detail_grid.attach(*audio_icon, 0, 14, 1, 1);
  detail_grid.attach_next_to(*audio_text_label, *audio_icon, Gtk::PositionType::RIGHT, 1, 1);

  // Audio driver
  Gtk::Label* audio_driver_text_label = Gtk::manage(new Gtk::Label("Audio Driver:", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*audio_driver_text_label, 0, 15, 2, 1);
  detail_grid.attach_next_to(audio_driver_label, *audio_driver_text_label, Gtk::PositionType::RIGHT, 1, 1);
  // End Audio driver
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, 16, 3, 1);

  // Display heading
  Gtk::Image* display_icon = Gtk::manage(new Gtk::Image());
  display_icon->set_from_icon_name("view-fullscreen");
  display_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* display_text_label = Gtk::manage(new Gtk::Label());
  display_text_label->set_markup("<b>Display</b>");
  detail_grid.attach(*display_icon, 0, 17, 1, 1);
  detail_grid.attach_next_to(*display_text_label, *display_icon, Gtk::PositionType::RIGHT, 1, 1);

  // Virtual Desktop
  Gtk::Label* virtual_desktop_text_label = Gtk::manage(new Gtk::Label("Virtual Desktop\n(Windowed Mode):", Gtk::Align::START, Gtk::Align::CENTER));
  detail_grid.attach(*virtual_desktop_text_label, 0, 18, 2, 1);
  detail_grid.attach_next_to(virtual_desktop_label, *virtual_desktop_text_label, Gtk::PositionType::RIGHT, 1, 1);
  // End Display
  detail_grid.attach(*Gtk::manage(new Gtk::Separator(Gtk::Orientation::HORIZONTAL)), 0, 19, 3, 1);

  // Description heading
  Gtk::Image* description_icon = Gtk::manage(new Gtk::Image());
  description_icon->set_from_icon_name("user-available");
  description_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* description_text_label = Gtk::manage(new Gtk::Label());
  description_text_label->set_markup("<b>Description</b>");
  detail_grid.attach(*description_icon, 0, 20, 1, 1);
  detail_grid.attach_next_to(*description_text_label, *description_icon, Gtk::PositionType::RIGHT, 1, 1);

  // Description text
  detail_grid.attach(description_label, 0, 21, 3, 1);
  // End Description

  // Place inside a scrolled window
  detail_grid_scrolled_window_detail.set_child(detail_grid);

  // Add to container
  container_paned.set_start_child(detail_grid_scrolled_window_detail);

  /**
   * Application list section
   */
  // Create list model
  app_list_store = Gio::ListStore<AppListModelColumns>::create();

  // Create the filter model
  auto expression = Gtk::ClosureExpression<Glib::ustring>::create(
      [](const Glib::RefPtr<Glib::ObjectBase>& item) -> Glib::ustring
      {
        const auto col = std::dynamic_pointer_cast<AppListModelColumns>(item);
        return col ? col->name : "";
      });
  app_list_filter = Gtk::StringFilter::create(expression);
  app_list_filter->set_ignore_case(true);
  app_list_filter->set_match_mode(Gtk::StringFilter::MatchMode::SUBSTRING);
  app_list_filter_list_model = Gtk::FilterListModel::create(app_list_store, app_list_filter);

  // Set selection model
  app_list_selection_model = Gtk::SingleSelection::create(app_list_store);
  app_list_selection_model->set_autoselect(false);
  app_list_selection_model->set_can_unselect(true);

  // Create factory
  app_list_factory = Gtk::SignalListItemFactory::create();
  app_list_factory->signal_setup().connect(sigc::mem_fun(*this, &MainWindow::on_setup_label));
  app_list_factory->signal_bind().connect(sigc::mem_fun(*this, &MainWindow::on_bind_icon_and_name));

  // Set list model and factory
  app_list_list_view.set_model(app_list_selection_model);
  app_list_list_view.set_factory(app_list_factory);

  // Set scrolled window properties
  app_list_scrolled_window.set_margin_top(6);
  app_list_scrolled_window.set_margin_start(6);
  app_list_scrolled_window.set_margin_end(6);
  app_list_scrolled_window.set_margin_bottom(6);
  app_list_scrolled_window.set_halign(Gtk::Align::FILL);
  app_list_scrolled_window.set_valign(Gtk::Align::FILL);
  app_list_scrolled_window.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
  app_list_scrolled_window.set_expand();
  app_list_scrolled_window.set_child(app_list_list_view);

  // Add application header text
  Gtk::Image* application_icon = Gtk::manage(new Gtk::Image());
  application_icon->set_from_icon_name("application-x-executable");
  application_icon->set_icon_size(Gtk::IconSize::NORMAL);
  Gtk::Label* application_label = Gtk::manage(new Gtk::Label());
  application_label->set_markup("<b>Applications</b>");
  application_label->set_margin_start(10);
  Gtk::Box* application_box = Gtk::manage(new Gtk::Box());
  application_box->append(*application_icon);
  application_box->append(*application_label);
  application_box->set_margin_start(6);
  application_box->set_margin_end(6);
  application_box->set_margin_top(6);
  application_box->set_halign(Gtk::Align::FILL);

  // Search entry
  app_list_search_entry.set_margin_start(6);
  app_list_search_entry.set_margin_end(6);
  app_list_search_entry.set_margin_top(6);
  app_list_search_entry.set_hexpand(true);
  app_list_search_entry.set_halign(Gtk::Align::FILL);

  // App list add shortcut button
  add_app_list_button.set_tooltip_text("Add shortcut to application list");
  add_app_list_button.set_label("Add");
  add_app_list_button.set_icon_name("list-add");
  add_app_list_button.set_margin_top(6);
  add_app_list_button.set_margin_end(6);

  // App list remove shortcut button
  remove_app_list_button.set_tooltip_text("Remove shortcut from application list");
  remove_app_list_button.set_label("Remove");
  remove_app_list_button.set_icon_name("list-remove");
  remove_app_list_button.set_margin_top(6);
  remove_app_list_button.set_margin_end(6);

  // App list refresh button
  refresh_app_list_button.set_tooltip_text("Refresh application list");
  refresh_app_list_button.set_label("Refresh");
  refresh_app_list_button.set_icon_name("view-refresh");
  refresh_app_list_button.set_margin_top(6);
  refresh_app_list_button.set_margin_end(6);

  // Preparing the horizontal box above the app list (containing the search entry & refresh button)
  app_list_top_hbox.append(app_list_search_entry);
  app_list_top_hbox.append(add_app_list_button);
  app_list_top_hbox.append(remove_app_list_button);
  app_list_top_hbox.append(refresh_app_list_button);
  app_list_top_hbox.set_halign(Gtk::Align::FILL);

  // Add heading (label + icon)
  app_list_vbox.append(*application_box);
  // Add horizontal box (search entry + refresh button)
  app_list_vbox.append(app_list_top_hbox);
  // Add application list (in scrolled window)
  app_list_vbox.append(app_list_scrolled_window);
  // Add to container
  container_paned.set_end_child(app_list_vbox);
  container_paned.set_halign(Gtk::Align::FILL);
  container_paned.set_valign(Gtk::Align::FILL);
  container_paned.set_vexpand(true);
  container_paned.set_hexpand(true);

  // Add container to right box
  right_vbox.append(container_paned);

  // Add right box to paned
  paned.set_end_child(right_vbox);
}

void MainWindow::on_setup_label(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
  Gtk::Box* hbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
  Gtk::Box* vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
  Gtk::Picture* icon = Gtk::make_managed<Gtk::Picture>();
  icon->set_can_shrink(false);
  icon->set_halign(Gtk::Align::CENTER);
  icon->set_valign(Gtk::Align::CENTER);
  icon->set_margin_end(8);
  hbox->append(*icon);

  Gtk::Label* name = Gtk::make_managed<Gtk::Label>("", Gtk::Align::START);
  Gtk::Label* description = Gtk::make_managed<Gtk::Label>("", Gtk::Align::START);
  vbox->append(*name);
  vbox->append(*description);
  hbox->append(*vbox);

  list_item->set_child(*hbox);
}

void MainWindow::on_bind_icon_and_name(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
  auto col = std::dynamic_pointer_cast<AppListModelColumns>(list_item->get_item());
  if (!col)
    return;
  Gtk::Box* hbox = dynamic_cast<Gtk::Box*>(list_item->get_child());
  if (!hbox)
    return;
  auto icon = dynamic_cast<Gtk::Picture*>(hbox->get_first_child());
  if (!icon)
    return;
  Gtk::Box* vbox = dynamic_cast<Gtk::Box*>(icon->get_next_sibling());
  if (!vbox)
    return;
  Gtk::Label* name = dynamic_cast<Gtk::Label*>(vbox->get_first_child());
  if (!name)
    return;
  Gtk::Label* description = dynamic_cast<Gtk::Label*>(name->get_next_sibling());
  if (!description)
    return;

  // Set all fields
  icon->set_paintable(col->icon);
  name->set_markup("<b>" + col->name + "</b>");
  description->set_markup(col->description);
}

/**
 * \brief set sensitive toolbar buttons (eg. when a bottle is active)
 * \param sensitive Set toolbar buttons sensitivity (true is enabled, false is disabled)
 */
void MainWindow::set_sensitive_toolbar_buttons(bool sensitive)
{
  edit_button.set_sensitive(sensitive);
  clone_button.set_sensitive(sensitive);
  configure_button.set_sensitive(sensitive);
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
void MainWindow::cc_list_box_update_header_func(Gtk::ListBoxRow* list_box_row, Gtk::ListBoxRow* before)
{
  GtkWidget* current;
  GtkListBoxRow* row = list_box_row->gobj();
  if (before == NULL)
  {
    gtk_list_box_row_set_header(row, NULL);
    return;
  }
  current = gtk_list_box_row_get_header(row);
  if (current == NULL)
  {
    current = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    // TODO: It shows automatically in gtk4?
    // gtk_widget_show(current);
    gtk_list_box_row_set_header(row, current);
  }
}
