/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_window.h
 * \brief   Wine runner manager window (download & manage precompiled Wine builds)
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
#pragma once

#include <functional>
#include <gtkmm.h>
#include <memory>
#include <string>
#include <vector>

#include "busy_dialog.h"
#include "dialog_window.h"
#include "wine_runner_install_task.h"
#include "wine_runner_types.h"

/**
 * \class WineRunnerWindow
 * \brief GTK Window class to download & manage precompiled Wine runner builds
 */
class WineRunnerWindow : public Gtk::Window
{
public:
  sigc::signal<void()> runners_changed; /*!< Signal when the set of installed runners changed (install or removal finished) */

  explicit WineRunnerWindow(Gtk::Window& parent);
  virtual ~WineRunnerWindow();

  void show();
  void show_for(Gtk::Window* parent);
  void set_bottle_wine_bin_paths_provider(const std::function<std::vector<std::string>()>& provider);

protected:
  // Child widgets
  Gtk::Box runner_box;        /*!< Main vertical box */
  Gtk::Label hint_label;      /*!< Hint label on top */
  Gtk::Box sidebar_stack_box; /*!< Horizontal box holding the sidebar and stack */
  Gtk::StackSidebar sidebar;  /*!< Sidebar with the categories */
  Gtk::Stack stack;           /*!< Stack with the category pages */

  // Installed runners page
  Gtk::Box installed_vbox;              /*!< Installed page vertical box */
  Gtk::ScrolledWindow installed_scroll; /*!< Scrolled window around the installed runners list */
  Gtk::ListBox installed_listbox;       /*!< List of installed runners */
  Gtk::Label installed_empty_label;     /*!< Empty state label */

private:
  /**
   * \struct SourcePage
   * \brief Widgets & state of a single downloadable-variant page
   */
  struct SourcePage
  {
    WineRunner::SourceId source_id = WineRunner::SourceId::Kron4ekWineBuilds; /*!< Runner source the page belongs to */
    std::string variant;                                                      /*!< Build variant the page shows (eg. "staging" or "GE-Proton") */
    Gtk::Box page_vbox;                                                       /*!< Page vertical box */
    Gtk::Label description_label;                                             /*!< Variant description */
    Gtk::Box selection_hbox;                                                  /*!< Horizontal box with the version selection widgets */
    Gtk::Label version_label;                                                 /*!< "Version:" label */
    Gtk::ComboBoxText version_combobox;                                       /*!< Version selection */
    Gtk::Button refresh_button;                                               /*!< Refresh the release list (also acts as retry after an error) */
    Gtk::Button install_button;                                               /*!< Install the selected version */
    Gtk::Box status_hbox;                                                     /*!< Horizontal box with the spinner and status label */
    Gtk::Spinner spinner;                                                     /*!< Fetching indicator */
    Gtk::Label status_label;                                                  /*!< Fetch status / error text */
    std::vector<WineRunner::Release> releases;                                /*!< Fetched releases of this variant (newest first) */
    bool fetched = false;                                                     /*!< True when the release list was fetched successfully */
    bool fetch_pending = false;                                               /*!< True when a fetch is queued (another operation was running) */
  };

  Gtk::Window& default_parent_;                                              /*!< Main window (default transient parent) */
  std::vector<std::unique_ptr<SourcePage>> source_pages_;                    /*!< The downloadable-variant pages */
  WineRunnerInstallTask task_;                                               /*!< Async worker for fetch/install/remove operations */
  BusyDialog busy_dialog_;                                                   /*!< Busy dialog with progress bar during install/removal */
  DialogWindow error_dialog_;                                                /*!< Error dialog */
  DialogWindow info_dialog_;                                                 /*!< Info dialog */
  std::function<std::vector<std::string>()> bottle_wine_bin_paths_provider_; /*!< Provides the wine bin paths of all bottles (for in-use checks) */
  WineRunner::InstallPhase last_phase_ = WineRunner::InstallPhase::Idle;     /*!< Last seen install phase (to limit label updates) */
  Glib::ustring installing_display_name_;                                    /*!< Display name of the runner being installed */

  // Signal handlers
  void on_page_changed();
  void on_refresh_clicked(SourcePage* page);
  void on_install_clicked(SourcePage* page);
  void on_version_selection_changed(SourcePage* page);
  void on_remove_clicked(const WineRunner::InstalledRunner& runner);
  void on_releases_fetched();
  void on_fetch_failed();
  void on_progress_changed();
  void on_install_finished();
  void on_remove_finished();

  // Member functions
  void create_layout();
  SourcePage*
  create_source_page(WineRunner::SourceId source_id, const std::string& variant, const Glib::ustring& title, const Glib::ustring& description);
  void refresh_installed_list();
  void refresh_version_comboboxes();
  void fill_version_combobox(SourcePage& page);
  void start_fetch(SourcePage& page);
  void start_pending_fetch();
  void start_fetch_for_visible_page();
  SourcePage* get_visible_source_page();
  void set_page_fetching_state(SourcePage& page, bool fetching);
  void remove_runner_confirmed(const WineRunner::InstalledRunner& runner);
  void show_error_message(const Glib::ustring& message);
  void show_info_message(const Glib::ustring& message);
  static Glib::ustring format_release_label(const WineRunner::Release& release);
};
