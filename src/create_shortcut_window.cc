/**
 * Copyright (c) 2025 WineGUI
 *
 * \file    create_shortcut_window.cc
 * \brief   Create menu or desktop shortcuts (.desktop files) for applications Window class
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
#include "create_shortcut_window.h"
#include "bottle_item.h"
#include "helper.h"

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window
 */
CreateShortcutWindow::CreateShortcutWindow(Gtk::Window& parent)
    : vbox(Gtk::Orientation::VERTICAL, 4),
      header_label("Create menu or desktop shortcuts"),
      description_label("For each application below, add a shortcut to your applications menu or to your desktop."),
      close_button("Close"),
      active_bottle_(nullptr)
{
  set_transient_for(parent);
  set_title("Create Application shortcuts");
  set_default_size(600, 500);
  set_modal(true);

  Pango::FontDescription fd_label;
  fd_label.set_size(12 * PANGO_SCALE);
  fd_label.set_weight(Pango::Weight::BOLD);
  auto font_label = Pango::Attribute::create_attr_font_desc(fd_label);
  Pango::AttrList attr_list_header_label;
  attr_list_header_label.insert(font_label);
  header_label.set_attributes(attr_list_header_label);
  header_label.set_margin_top(5);
  header_label.set_margin_bottom(5);

  description_label.set_margin_bottom(5);
  description_label.set_wrap(true);

  app_list_box.set_selection_mode(Gtk::SelectionMode::NONE);
  app_list_box.set_can_focus(false);
  scrolled_window.set_child(app_list_box);
  scrolled_window.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
  scrolled_window.set_vexpand(true);
  scrolled_window.set_hexpand(true);
  scrolled_window.set_margin_start(6);
  scrolled_window.set_margin_end(6);

  hbox_buttons.set_orientation(Gtk::Orientation::HORIZONTAL);
  hbox_buttons.set_spacing(4);
  hbox_buttons.set_halign(Gtk::Align::END);
  hbox_buttons.set_margin(6);
  hbox_buttons.append(close_button);

  vbox.append(header_label);
  vbox.append(description_label);
  vbox.append(scrolled_window);
  vbox.append(hbox_buttons);
  set_child(vbox);

  // Signals
  close_button.signal_clicked().connect(sigc::mem_fun(*this, &CreateShortcutWindow::on_close_button_clicked));
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
CreateShortcutWindow::~CreateShortcutWindow()
{
}

/**
 * \brief Signal handler when a new bottle is set in the main window
 * \param[in] bottle Current active bottle
 */
void CreateShortcutWindow::set_active_bottle(BottleItem* bottle)
{
  active_bottle_ = bottle;
}

/**
 * \brief Signal handler for resetting the active bottle to null
 */
void CreateShortcutWindow::reset_active_bottle()
{
  active_bottle_ = nullptr;
}

/**
 * \brief Provide the list of applications that will be shown, before showing the window
 * \param[in] applications Vector of (name, description, command) tuples
 */
void CreateShortcutWindow::set_applications(const std::vector<ShortcutAppData>& applications)
{
  applications_ = applications;
}

/**
 * \brief Override show, which (re)builds the application list
 */
void CreateShortcutWindow::show()
{
  clear_list();
  populate_list();
  present();
}

/**
 * \brief Remove all rows from the application list box
 */
void CreateShortcutWindow::clear_list()
{
  auto child = app_list_box.get_first_child();
  while (child != nullptr)
  {
    auto next = child->get_next_sibling();
    app_list_box.remove(*child);
    child = next;
  }
}

/**
 * \brief Build one row per application, each with a "To Menu" and "To Desktop" button
 */
void CreateShortcutWindow::populate_list()
{
  for (const auto& [name, description, command] : applications_)
  {
    auto* row_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    row_box->set_margin_top(4);
    row_box->set_margin_bottom(4);
    row_box->set_margin_start(6);
    row_box->set_margin_end(6);

    // Name (+ description) label, expanding to push the buttons to the right
    Glib::ustring label_text = "<b>" + name + "</b>";
    if (!description.empty())
      label_text += "\n<small>" + description + "</small>";
    auto* label = Gtk::make_managed<Gtk::Label>();
    label->set_markup(label_text);
    label->set_halign(Gtk::Align::START);
    label->set_xalign(0.0);
    label->set_hexpand(true);
    row_box->append(*label);

    auto* menu_button = Gtk::make_managed<Gtk::Button>("To Menu");
    menu_button->set_tooltip_text("Add a shortcut to your applications menu");
    menu_button->set_valign(Gtk::Align::CENTER);
    auto* desktop_button = Gtk::make_managed<Gtk::Button>("To Desktop");
    desktop_button->set_tooltip_text("Add a shortcut to your desktop");
    desktop_button->set_valign(Gtk::Align::CENTER);

    // Capture the app data by value for each button
    Glib::ustring cap_name = name;
    Glib::ustring cap_description = description;
    std::string cap_command = command;
    menu_button->signal_clicked().connect(
        [this, cap_name, cap_description, cap_command, menu_button]()
        {
          on_create_clicked(cap_name, cap_description, cap_command, false);
          menu_button->set_label("Added ✓");
        });
    desktop_button->signal_clicked().connect(
        [this, cap_name, cap_description, cap_command, desktop_button]()
        {
          on_create_clicked(cap_name, cap_description, cap_command, true);
          desktop_button->set_label("Added ✓");
        });

    row_box->append(*menu_button);
    row_box->append(*desktop_button);
    app_list_box.append(*row_box);
  }
}

/**
 * \brief Triggered when close button is clicked
 */
void CreateShortcutWindow::on_close_button_clicked()
{
  set_visible(false);
}

/**
 * \brief Create a host shortcut (.desktop file) for a single application
 * \param[in] name Application name
 * \param[in] description Application description
 * \param[in] command Application command
 * \param[in] to_desktop If true add to the desktop, otherwise to the applications menu
 */
void CreateShortcutWindow::on_create_clicked(const Glib::ustring& name, const Glib::ustring& description, const std::string& command, bool to_desktop)
{
  if (active_bottle_ == nullptr)
  {
    Gtk::MessageDialog dialog(*this, "Error occurred during creating the shortcut, because there is no active Windows machine set.", false,
                              Gtk::MessageType::ERROR, Gtk::ButtonsType::OK);
    dialog.set_title("Error during shortcut creation");
    dialog.set_modal(true);
    dialog.present();
    return;
  }

  // Resolve the target directory
  std::string target_dir;
  if (to_desktop)
  {
    target_dir = Glib::get_user_special_dir(Glib::UserDirectory::DESKTOP);
    if (target_dir.empty())
      target_dir = Glib::build_filename(Glib::get_home_dir(), "Desktop");
  }
  else
  {
    target_dir = Glib::build_filename(Glib::get_user_data_dir(), "applications");
  }

  std::string icon = Helper::get_image_location("logo_big.png");
  std::string app_name = name;
  std::string comment = description;
  std::string bottle_name = active_bottle_->name();

  // Sanitized, deterministic file name so re-creating overwrites rather than duplicates
  std::string basename = "winegui-" + Helper::to_filename_part(bottle_name) + "-" + Helper::to_filename_part(app_name) + ".desktop";

  std::string exec_line = Helper::build_desktop_exec_line(active_bottle_->is_wine64_bit(), active_bottle_->wine_location(),
                                                          active_bottle_->wine_bin_path(), command, active_bottle_->env_vars());

  bool success = Helper::create_desktop_file(target_dir, basename, app_name, comment, exec_line, icon, bottle_name, to_desktop);

  if (!success)
  {
    Gtk::MessageDialog dialog(*this, "Error occurred while writing the shortcut file to disk.", false, Gtk::MessageType::ERROR, Gtk::ButtonsType::OK);
    dialog.set_title("Error during shortcut creation");
    dialog.set_modal(true);
    dialog.present();
    return;
  }

  // For the applications menu, best-effort refresh the desktop database (ignore failure, not always present)
  if (!to_desktop)
  {
    Glib::spawn_command_line_async("update-desktop-database \"" + target_dir + "\"");
  }
}
