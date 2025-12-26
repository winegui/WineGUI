/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    preferences_window.cc
 * \brief   Application preferences GTK3 window class
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
#include "preferences_window.h"
#include "general_config_file.h"
#include "gtkmm/enums.h"
#include "gtkmm/filedialog.h"
#include "gtkmm/object.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
PreferencesWindow::PreferencesWindow(Gtk::Window& parent)
    : vbox(Gtk::Orientation::VERTICAL, 4),
      hbox_buttons(Gtk::Orientation::HORIZONTAL, 4),
      header_preferences_label("Preferences"),
      default_wine_location_header("Machine folder location"),
      logging_stderr_header("Log standard error"),
      default_wine_machine_header("Default Wine machine"),
      select_folder_button("Select folder..."),
      save_button("Save"),
      cancel_button("Cancel")
{
  set_transient_for(parent);
  set_title("WineGUI Preferences");
  set_default_size(580, 420);
  set_modal(true);

  vbox.set_margin(10);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_preferences_label.set_attributes(attr_list_header_label);
  header_preferences_label.set_margin_top(5);
  header_preferences_label.set_margin_bottom(5);
  vbox.append(header_preferences_label);

  check_for_updates_switch.set_halign(Gtk::Align::END);

  select_folder_button.set_tooltip_text("Change storage location of Wine prefixes");
  default_folder_entry.set_hexpand(true);
  // If needed we can always add a separate row with big headers, like with width 3:
  // blbla_heading.set_markup("<big><b>Wow</b></big>");

  // Switch headers
  default_wine_location_header.set_halign(Gtk::Align::START);
  default_wine_location_header.set_markup("<b>Default storage location</b>");
  default_wine_machine_header.set_halign(Gtk::Align::START);
  default_wine_machine_header.set_markup("<b>Default Wine machine</b>");
  logging_stderr_header.set_halign(Gtk::Align::START);
  logging_stderr_header.set_markup("<b>Log Standard Error</b>");
  check_for_updates_header.set_halign(Gtk::Align::START);
  check_for_updates_header.set_markup("<b>Check for Updates</b>");

  // Default Wine storage location
  Gtk::Box* default_folder_vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  default_folder_vbox->set_hexpand(true);
  default_folder_vbox->set_halign(Gtk::Align::FILL);
  default_folder_vbox->append(default_wine_location_header);
  auto default_folder_label = Gtk::make_managed<Gtk::Label>("Default Windows machines (Wine prefixes) storage location on disk.");
  default_folder_label->set_halign(Gtk::Align::START);
  default_folder_vbox->append(*default_folder_label);
  Gtk::Box* default_folder_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  default_folder_box->set_hexpand(true);
  default_folder_box->append(default_folder_entry);
  default_folder_box->append(select_folder_button);
  default_folder_vbox->append(*default_folder_box);
  default_folder_vbox->set_margin_bottom(10);
  vbox.append(*default_folder_vbox);

  vbox.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));

  // Display default Wine machine
  Gtk::Box* default_wine_machine_vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  default_wine_machine_vbox->set_hexpand(true);
  default_wine_machine_vbox->set_halign(Gtk::Align::START);
  default_wine_machine_vbox->append(default_wine_machine_header);
  default_wine_machine_vbox->append(
      *Gtk::make_managed<Gtk::Label>("If enabled, the default Wine machine will be displayed in the list. Located at: ~/.wine"));
  Gtk::Box* default_wine_machine_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  default_wine_machine_box->append(*default_wine_machine_vbox);
  default_wine_machine_box->append(display_default_wine_machine_switch);
  default_wine_machine_box->set_margin_top(10);
  default_wine_machine_box->set_margin_bottom(10);
  vbox.append(*default_wine_machine_box);

  vbox.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));

  // Log standard error
  Gtk::Box* logging_stderr_vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  logging_stderr_vbox->set_hexpand(true);
  logging_stderr_vbox->set_halign(Gtk::Align::START);
  logging_stderr_vbox->append(logging_stderr_header);
  logging_stderr_vbox->append(*Gtk::make_managed<Gtk::Label>("If logging is enabled, also log standard error to log file."));
  Gtk::Box* logging_stderr_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  logging_stderr_box->append(*logging_stderr_vbox);
  logging_stderr_box->append(enable_logging_stderr_switch);
  logging_stderr_box->set_margin_top(10);
  logging_stderr_box->set_margin_bottom(10);
  vbox.append(*logging_stderr_box);

  vbox.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));

  // Check for updates
  Gtk::Box* check_updates_vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  check_updates_vbox->set_hexpand(true);
  check_updates_vbox->set_halign(Gtk::Align::START);
  check_updates_vbox->append(check_for_updates_header);
  check_updates_vbox->append(*Gtk::make_managed<Gtk::Label>("Check automatically for updates during application startup."));
  Gtk::Box* check_updates_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  check_updates_box->append(*check_updates_vbox);
  check_updates_box->append(check_for_updates_switch);
  check_updates_box->set_margin_top(10);
  check_updates_box->set_margin_bottom(10);
  vbox.append(*check_updates_box);

  vbox.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));

  // Save/cancel buttons
  hbox_buttons.set_halign(Gtk::Align::END);
  hbox_buttons.set_valign(Gtk::Align::END);
  hbox_buttons.set_expand(true);
  hbox_buttons.set_margin(6);
  hbox_buttons.append(save_button);
  hbox_buttons.append(cancel_button);
  vbox.append(hbox_buttons);

  set_child(vbox);

  // Signals
  select_folder_button.signal_clicked().connect(sigc::mem_fun(*this, &PreferencesWindow::on_select_folder));
  cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &PreferencesWindow::on_cancel_button_clicked));
  save_button.signal_clicked().connect(sigc::mem_fun(*this, &PreferencesWindow::on_save_button_clicked));
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
PreferencesWindow::~PreferencesWindow()
{
}

/**
 * \brief Same as show() but will also load the WineGUI preferences from disk
 */
void PreferencesWindow::show()
{
  GeneralConfigData general_config = GeneralConfigFile::read_config_file();
  default_folder_entry.set_text(general_config.default_folder);
  display_default_wine_machine_switch.set_active(general_config.display_default_wine_machine);
  enable_logging_stderr_switch.set_active(general_config.enable_logging_stderr);
  check_for_updates_switch.set_active(general_config.check_for_updates_startup);
  // Call parent present
  present();
}

/**
 * \brief Triggered when select folder button is clicked
 */
void PreferencesWindow::on_select_folder()
{
  #ifndef OLD_GTK
  // New GTK4 version, using FileDialog
  auto dialog = Gtk::FileDialog::create();
  dialog->set_title("Choose a folder");
  dialog->set_modal(true);
  {
    // Grep the default gnome application configuration folder (like the .local/share/winegui)
    static const std::vector<std::string> wineGuiDataDirs{Glib::get_user_data_dir(), "winegui"};
    static const std::string WineGuiDataDir = Glib::build_path(G_DIR_SEPARATOR_S, wineGuiDataDirs);
    auto folder = Gio::File::create_for_path(WineGuiDataDir);
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
                            default_folder_entry.set_text(folder->get_path());
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
  #else
  auto* folder_chooser =
      new Gtk::FileChooserDialog(*this, "Choose a folder", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER, Gtk::DialogFlags::DIALOG_MODAL);
  folder_chooser->set_modal(true);
  folder_chooser->set_transient_for(*this);
  folder_chooser->signal_response().connect(
    [this, folder_chooser](int response_id)
    {
      switch (response_id)
      {
      case Gtk::ResponseType::RESPONSE_OK:
      {
        // Get current older and update folder entry
        auto folder = folder_chooser->get_current_folder();
        default_folder_entry.set_text(folder);
        break;
      }
      case Gtk::ResponseType::RESPONSE_CANCEL:
      {
        break; // ignore
      }
      default:
      {
        std::cout << "Error: Unexpected button clicked." << std::endl;
        break;
      }
      }
      delete folder_chooser;
    });
  folder_chooser->add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
  folder_chooser->add_button("_Select folder", Gtk::ResponseType::RESPONSE_OK);
  folder_chooser->set_current_folder(default_folder_entry.get_text());
  folder_chooser->show();
  #endif
}

/**
 * \brief Triggered when cancel button is clicked
 */
void PreferencesWindow::on_cancel_button_clicked()
{
  set_visible(false);
}

/**
 * \brief Triggered when save button is clicked
 */
void PreferencesWindow::on_save_button_clicked()
{
  // Save preferences to disk
  GeneralConfigData general_config;
  general_config.default_folder = default_folder_entry.get_text();
  general_config.display_default_wine_machine = display_default_wine_machine_switch.get_active();
  general_config.enable_logging_stderr = enable_logging_stderr_switch.get_active();
  general_config.check_for_updates_startup = check_for_updates_switch.get_active();
  if (!GeneralConfigFile::write_config_file(general_config))
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during saving generic config file.", false, Gtk::MessageType::ERROR, Gtk::ButtonsType::OK);
    dialog.set_title("An error has occurred!");
    dialog.set_modal(true);
    dialog.present();
  }
  else
  {
    // Hide preferences window
    set_visible(false);
    // Trigger manager update & UI update
    config_saved.emit();
  }
}
