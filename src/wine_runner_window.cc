/**
 * Copyright (c) 2026 WineGUI
 *
 * \file    wine_runner_window.cc
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
#include "wine_runner_window.h"

#include "wine_runner_manager.h"

#include <algorithm>
#include <glibmm/miscutils.h>

/**
 * \brief Constructor
 * \param parent Reference to parent GTK Window (main window)
 */
WineRunnerWindow::WineRunnerWindow(Gtk::Window& parent)
    : default_parent_(parent),
      busy_dialog_(*this),
      error_dialog_(*this, DialogWindow::DialogType::ERROR),
      info_dialog_(*this, DialogWindow::DialogType::INFO)
{
  set_title("Wine Runners");
  set_transient_for(parent);
  set_default_size(950, 540);
  set_modal(true);

  create_layout();

  // Signals
  stack.property_visible_child_name().signal_changed().connect(sigc::mem_fun(*this, &WineRunnerWindow::on_page_changed));
  task_.releases_fetched.connect(sigc::mem_fun(*this, &WineRunnerWindow::on_releases_fetched));
  task_.fetch_failed.connect(sigc::mem_fun(*this, &WineRunnerWindow::on_fetch_failed));
  task_.progress_changed.connect(sigc::mem_fun(*this, &WineRunnerWindow::on_progress_changed));
  task_.install_finished.connect(sigc::mem_fun(*this, &WineRunnerWindow::on_install_finished));
  task_.remove_finished.connect(sigc::mem_fun(*this, &WineRunnerWindow::on_remove_finished));
  busy_dialog_.cancel_requested.connect([this]() { task_.cancel(); });

  // Hide window instead of destroy
  signal_close_request().connect(
      [this]() -> bool
      {
        set_visible(false);
        // Reset the transient parent back to the main window (it may have been re-targeted by show_for())
        set_transient_for(default_parent_);
        return true; // stop default destroy
      },
      false);
}

/**
 * \brief Destructor
 */
WineRunnerWindow::~WineRunnerWindow()
{
}

/**
 * \brief Show the window, transient for the main window
 */
void WineRunnerWindow::show()
{
  show_for(&default_parent_);
}

/**
 * \brief Show the window, transient for the given parent window.
 * Used when opening from a modal window (eg. the edit window or the new bottle assistant),
 * otherwise this window would be unreachable behind the modal parent.
 * \param[in] parent Parent GTK Window (nullptr = main window)
 */
void WineRunnerWindow::show_for(Gtk::Window* parent)
{
  set_transient_for((parent != nullptr) ? *parent : default_parent_);
  refresh_installed_list();
  refresh_version_comboboxes();
  present();
  start_fetch_for_visible_page();
}

/**
 * \brief Set the provider of all bottle wine binary paths (used to warn when removing a runner that is still in use)
 * \param[in] provider Callable returning the wine binary paths of all bottles
 */
void WineRunnerWindow::set_bottle_wine_bin_paths_provider(const std::function<std::vector<std::string>()>& provider)
{
  bottle_wine_bin_paths_provider_ = provider;
}

/*************************************************************
 * Private member functions                                  *
 *************************************************************/

void WineRunnerWindow::create_layout()
{
  hint_label.set_markup("<big><b>Tip:</b> Downloaded Wine runners can be selected in the Edit window and when creating a new machine.</big>");
  hint_label.set_margin_top(4);
  hint_label.set_margin_bottom(4);
  hint_label.set_halign(Gtk::Align::CENTER);

  // Sidebar (categories) on the left, the stack with the pages on the right
  sidebar_stack_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  sidebar_stack_box.set_spacing(6);
  sidebar_stack_box.set_expand();
  stack.set_expand();
  stack.set_transition_type(Gtk::StackTransitionType::CROSSFADE);
  sidebar.set_stack(stack);

  // Installed runners page
  installed_vbox.set_orientation(Gtk::Orientation::VERTICAL);
  installed_vbox.set_spacing(6);
  installed_vbox.set_margin(10);
  installed_empty_label.set_markup("<i>No Wine runners installed yet.\n\nPick a Wine build on the left, choose a version and press install.</i>");
  installed_empty_label.set_halign(Gtk::Align::CENTER);
  installed_empty_label.set_valign(Gtk::Align::CENTER);
  installed_empty_label.set_expand();
  installed_empty_label.set_justify(Gtk::Justification::CENTER);
  installed_listbox.set_selection_mode(Gtk::SelectionMode::NONE);
  installed_listbox.add_css_class("boxed-list");
  installed_scroll.set_child(installed_listbox);
  installed_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
  installed_scroll.set_expand();
  installed_vbox.append(installed_empty_label);
  installed_vbox.append(installed_scroll);
  stack.add(installed_vbox, "installed", "Installed Runners");

  // Downloadable Wine build variant pages
  create_source_page(WineRunner::SourceId::Kron4ekWineBuilds, "vanilla", "Wine Vanilla",
                     "Vanilla Wine builds, compiled from the official WineHQ sources by Kron4ek.");
  create_source_page(WineRunner::SourceId::Kron4ekWineBuilds, "staging", "Wine Staging",
                     "Wine builds with the Wine Staging patchset (testing patches not yet in regular Wine), compiled by Kron4ek.");
  create_source_page(WineRunner::SourceId::Kron4ekWineBuilds, "staging-tkg", "Wine Staging-TkG",
                     "Wine builds with the Staging patchset and many additional gaming patches, built with the Wine-TkG build system by Kron4ek.");
  create_source_page(WineRunner::SourceId::Kron4ekWineBuilds, "proton", "Wine Proton (Kron4ek)",
                     "Wine builds with Valve's Proton patches (primarily for a better gaming experience) as a regular Wine build, compiled by "
                     "Kron4ek.");
  create_source_page(WineRunner::SourceId::GEProton, "GE-Proton", "GE-Proton",
                     "Proton builds by GloriousEggroll with extra patches and media codecs, the successor of Wine-GE. Note: these are large "
                     "downloads (about 500 MB) and primarily made for the Steam runtime.");

  sidebar_stack_box.append(sidebar);
  sidebar_stack_box.append(stack);

  runner_box.set_orientation(Gtk::Orientation::VERTICAL);
  runner_box.set_margin(5);
  runner_box.set_spacing(8);
  runner_box.append(hint_label);
  runner_box.append(sidebar_stack_box);
  set_child(runner_box);
}

/**
 * \brief Create one downloadable-variant page and add it to the stack
 * \param[in] source_id Runner source
 * \param[in] variant Build variant the page shows (used as stack page name)
 * \param[in] title Page title (shown in the sidebar)
 * \param[in] description Variant description
 * \return Pointer to the created page
 */
WineRunnerWindow::SourcePage* WineRunnerWindow::create_source_page(WineRunner::SourceId source_id,
                                                                   const std::string& variant,
                                                                   const Glib::ustring& title,
                                                                   const Glib::ustring& description)
{
  auto page = std::make_unique<SourcePage>();
  page->source_id = source_id;
  page->variant = variant;

  page->description_label.set_markup(description);
  page->description_label.set_wrap(true);
  page->description_label.set_xalign(0.0);

  page->version_label.set_text("Version: ");
  page->version_combobox.set_hexpand(true);
  page->version_combobox.set_sensitive(false);
  page->refresh_button.set_label("Refresh");
  page->refresh_button.set_tooltip_text("Fetch the release list again");
  page->install_button.set_label("Install");
  page->install_button.set_sensitive(false);
  page->install_button.add_css_class("suggested-action");
  page->selection_hbox.set_orientation(Gtk::Orientation::HORIZONTAL);
  page->selection_hbox.set_spacing(6);
  page->selection_hbox.append(page->version_label);
  page->selection_hbox.append(page->version_combobox);
  page->selection_hbox.append(page->refresh_button);
  page->selection_hbox.append(page->install_button);

  page->status_hbox.set_orientation(Gtk::Orientation::HORIZONTAL);
  page->status_hbox.set_spacing(6);
  page->status_label.set_xalign(0.0);
  page->status_label.set_wrap(true);
  page->status_hbox.append(page->spinner);
  page->status_hbox.append(page->status_label);

  page->page_vbox.set_orientation(Gtk::Orientation::VERTICAL);
  page->page_vbox.set_spacing(10);
  page->page_vbox.set_margin(10);
  page->page_vbox.append(page->description_label);
  page->page_vbox.append(page->selection_hbox);
  page->page_vbox.append(page->status_hbox);

  SourcePage* page_ptr = page.get();
  page->refresh_button.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &WineRunnerWindow::on_refresh_clicked), page_ptr));
  page->install_button.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &WineRunnerWindow::on_install_clicked), page_ptr));
  page->version_combobox.signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &WineRunnerWindow::on_version_selection_changed), page_ptr));

  stack.add(page->page_vbox, variant, title);
  source_pages_.emplace_back(std::move(page));
  return page_ptr;
}

/**
 * \brief Rebuild the installed runners list
 */
void WineRunnerWindow::refresh_installed_list()
{
  // Remove all existing rows
  while (Gtk::ListBoxRow* row = installed_listbox.get_row_at_index(0))
  {
    installed_listbox.remove(*row);
  }

  std::vector<WineRunner::InstalledRunner> runners = WineRunnerManager::get_installed_runners();
  for (const WineRunner::InstalledRunner& runner : runners)
  {
    auto* name_label = Gtk::make_managed<Gtk::Label>();
    name_label->set_markup("<b>" + Glib::Markup::escape_text(runner.display_name) + "</b>");
    name_label->set_xalign(0.0);
    auto* sub_label = Gtk::make_managed<Gtk::Label>();
    Glib::ustring wine_version = runner.wine_version.empty() ? "unknown" : runner.wine_version;
    sub_label->set_markup("<small>Wine version: " + Glib::Markup::escape_text(wine_version) + " — " + Glib::Markup::escape_text(runner.name) +
                          "</small>");
    sub_label->set_xalign(0.0);
    sub_label->add_css_class("dim-label");

    auto* label_vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    label_vbox->append(*name_label);
    label_vbox->append(*sub_label);
    label_vbox->set_hexpand(true);

    auto* remove_button = Gtk::make_managed<Gtk::Button>("Remove");
    remove_button->set_valign(Gtk::Align::CENTER);
    remove_button->add_css_class("destructive-action");
    remove_button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &WineRunnerWindow::on_remove_clicked), runner));

    auto* row_hbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    row_hbox->set_margin(6);
    row_hbox->append(*label_vbox);
    row_hbox->append(*remove_button);

    auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
    row->set_child(*row_hbox);
    row->set_activatable(false);
    row->set_tooltip_text(runner.runner_dir);
    installed_listbox.append(*row);
  }

  installed_empty_label.set_visible(runners.empty());
  installed_scroll.set_visible(!runners.empty());
}

/**
 * \brief Refill all fetched version comboboxes (eg. to update the installed markers), keeping the selection
 */
void WineRunnerWindow::refresh_version_comboboxes()
{
  for (const auto& page : source_pages_)
  {
    if (page->fetched)
      fill_version_combobox(*page);
  }
}

/**
 * \brief Fill the version combobox of a page from its release list, keeping the previous selection when possible
 * \param[in] page Page to fill
 */
void WineRunnerWindow::fill_version_combobox(SourcePage& page)
{
  Glib::ustring previous_selection = page.version_combobox.get_active_id();
  page.version_combobox.remove_all();
  for (std::size_t i = 0; i < page.releases.size(); ++i)
  {
    page.version_combobox.append(std::to_string(i), format_release_label(page.releases.at(i)));
  }
  if (!page.releases.empty())
  {
    if (previous_selection.empty() || !page.version_combobox.set_active_id(previous_selection))
      page.version_combobox.set_active(0);
  }
  page.version_combobox.set_sensitive(!page.releases.empty());
  on_version_selection_changed(&page);
}

/**
 * \brief Display label for a release in the version combobox
 * \param[in] release Release
 * \return Label, eg. "11.13 (2026-07-11, 102.0 MB) — installed"
 */
Glib::ustring WineRunnerWindow::format_release_label(const WineRunner::Release& release)
{
  Glib::ustring label = release.version;
  Glib::ustring details;
  if (release.published_at.size() >= 10)
    details = release.published_at.substr(0, 10);
  if (release.size_bytes > 0)
    details += (details.empty() ? "" : ", ") + Glib::format_size(release.size_bytes);
  if (!details.empty())
    label += " (" + details + ")";
  if (WineRunnerManager::is_installed(release))
    label += " — installed";
  return label;
}

/**
 * \brief Start fetching the release list for a page (or queue it when another operation is running)
 * \param[in] page Page to fetch for
 */
void WineRunnerWindow::start_fetch(SourcePage& page)
{
  if (page.fetched)
    return;
  if (task_.is_busy())
  {
    page.fetch_pending = true;
    return;
  }
  // A single fetch serves all pages of the same source (the variants share one release list)
  for (const auto& source_page : source_pages_)
  {
    if (source_page->source_id == page.source_id)
      set_page_fetching_state(*source_page, true);
  }
  task_.fetch_releases_async(page.source_id);
}

/**
 * \brief Start the next queued fetch (if any)
 */
void WineRunnerWindow::start_pending_fetch()
{
  auto it = std::find_if(source_pages_.begin(), source_pages_.end(),
                         [](const std::unique_ptr<SourcePage>& page) { return page->fetch_pending && !page->fetched; });
  if (it != source_pages_.end())
  {
    (*it)->fetch_pending = false;
    start_fetch(*(*it));
  }
}

/**
 * \brief Start fetching the release list for the currently visible page (when not fetched yet)
 */
void WineRunnerWindow::start_fetch_for_visible_page()
{
  SourcePage* page = get_visible_source_page();
  if (page != nullptr && !page->fetched)
    start_fetch(*page);
}

/**
 * \brief Get the currently visible downloadable-variant page (or nullptr for the installed page)
 * \return Visible page or nullptr
 */
WineRunnerWindow::SourcePage* WineRunnerWindow::get_visible_source_page()
{
  std::string visible_name = stack.get_visible_child_name();
  auto it = std::find_if(source_pages_.begin(), source_pages_.end(),
                         [&visible_name](const std::unique_ptr<SourcePage>& page) { return page->variant == visible_name; });
  return (it != source_pages_.end()) ? it->get() : nullptr;
}

/**
 * \brief Update the widgets of a page to reflect the fetching state
 * \param[in] page Page to update
 * \param[in] fetching True while the release list is being fetched
 */
void WineRunnerWindow::set_page_fetching_state(SourcePage& page, bool fetching)
{
  if (fetching)
  {
    page.spinner.start();
    page.status_label.set_text("Fetching the release list from GitHub...");
    page.version_combobox.set_sensitive(false);
    page.install_button.set_sensitive(false);
    page.refresh_button.set_sensitive(false);
  }
  else
  {
    page.spinner.stop();
    page.refresh_button.set_sensitive(true);
  }
}

/*************************************************************
 * Signal handlers                                           *
 *************************************************************/

/**
 * \brief Signal handler when another page is selected in the sidebar (lazy fetch of the release list)
 */
void WineRunnerWindow::on_page_changed()
{
  if (is_visible())
    start_fetch_for_visible_page();
}

/**
 * \brief Signal handler when the refresh button of a page is clicked (fetch a fresh release list)
 * \param[in] page Page the button belongs to
 */
void WineRunnerWindow::on_refresh_clicked(SourcePage* page)
{
  if (task_.is_busy())
    return;
  WineRunnerManager::invalidate_release_cache();
  for (const auto& source_page : source_pages_)
  {
    if (source_page->source_id == page->source_id)
      source_page->fetched = false;
  }
  start_fetch(*page);
}

/**
 * \brief Signal handler when the version selection of a page changed (update the install button sensitivity)
 * \param[in] page Page the combobox belongs to
 */
void WineRunnerWindow::on_version_selection_changed(SourcePage* page)
{
  Glib::ustring active_id = page->version_combobox.get_active_id();
  bool can_install = false;
  if (!active_id.empty())
  {
    std::size_t index = std::stoul(active_id.raw());
    if (index < page->releases.size())
      can_install = !WineRunnerManager::is_installed(page->releases.at(index));
  }
  page->install_button.set_sensitive(can_install);
}

/**
 * \brief Signal handler when the install button of a page is clicked (download & install the selected version)
 * \param[in] page Page the button belongs to
 */
void WineRunnerWindow::on_install_clicked(SourcePage* page)
{
  if (task_.is_busy())
  {
    show_info_message("Another Wine runner operation is still running. Please, try again later.");
    return;
  }
  Glib::ustring active_id = page->version_combobox.get_active_id();
  if (active_id.empty())
    return;
  std::size_t index = std::stoul(active_id.raw());
  if (index >= page->releases.size())
    return;
  const WineRunner::Release& release = page->releases.at(index);

  installing_display_name_ = WineRunnerManager::derive_display_name(WineRunnerManager::expected_install_dir_name(release));
  last_phase_ = WineRunner::InstallPhase::Idle;
  busy_dialog_.set_message("Installing " + installing_display_name_, "Downloading " + Glib::ustring(release.asset_name) + "...");
  busy_dialog_.set_cancelable(true);
  busy_dialog_.set_progress(0.0);
  busy_dialog_.present();
  task_.install_async(release);
}

/**
 * \brief Signal handler when the remove button of an installed runner is clicked (ask for confirmation first)
 * \param[in] runner Runner to remove
 */
void WineRunnerWindow::on_remove_clicked(const WineRunner::InstalledRunner& runner)
{
  if (task_.is_busy())
  {
    show_info_message("Another Wine runner operation is still running. Please, try again later.");
    return;
  }
  Glib::ustring message;
  std::vector<std::string> bottle_paths = bottle_wine_bin_paths_provider_ ? bottle_wine_bin_paths_provider_() : std::vector<std::string>();
  if (WineRunnerManager::is_runner_used_by_bottle(runner, bottle_paths))
  {
    message = "<b>Warning:</b> One or more Windows machines still use <b>" + Glib::Markup::escape_text(runner.display_name) +
              "</b>!\n\nThose machines will stop working until you select another Wine runner for them (via the Edit window).\n\nAre you sure you "
              "want to remove this Wine runner?";
  }
  else
  {
    message = "Are you sure you want to remove the Wine runner <b>" + Glib::Markup::escape_text(runner.display_name) + "</b>?";
  }
  DialogWindow* dialog = Gtk::manage(new DialogWindow(*this, DialogWindow::DialogType::QUESTION, message, true));
  dialog->signal_response.connect(
      [this, runner](DialogWindow::ResponseType response)
      {
        if (response == DialogWindow::ResponseType::YES)
          remove_runner_confirmed(runner);
      });
  // Defer present, ensuring the dialog is centered correctly relative to this window
  Glib::signal_idle().connect_once([dialog]() { dialog->present(); }, Glib::PRIORITY_DEFAULT_IDLE);
}

/**
 * \brief Remove an installed runner (after the user confirmed)
 * \param[in] runner Runner to remove
 */
void WineRunnerWindow::remove_runner_confirmed(const WineRunner::InstalledRunner& runner)
{
  busy_dialog_.set_message("Removing " + Glib::ustring(runner.display_name), "Removing the Wine runner from disk.");
  busy_dialog_.set_cancelable(false);
  busy_dialog_.present();
  task_.remove_async(runner);
}

/**
 * \brief Signal handler when a release list fetch finished successfully (fill the pages of that source)
 */
void WineRunnerWindow::on_releases_fetched()
{
  WineRunner::SourceId source_id = task_.get_fetched_source_id();
  std::vector<WineRunner::Release> releases = task_.get_fetched_releases();
  for (const auto& page : source_pages_)
  {
    if (page->source_id != source_id)
      continue;
    page->releases.clear();
    for (const WineRunner::Release& release : releases)
    {
      if (release.variant == page->variant)
        page->releases.emplace_back(release);
    }
    page->fetched = true;
    page->fetch_pending = false;
    set_page_fetching_state(*page, false);
    fill_version_combobox(*page);
    if (page->releases.empty())
      page->status_label.set_text("No downloadable releases found for this variant.");
    else
      page->status_label.set_text(std::to_string(page->releases.size()) + " releases available.");
  }
  start_pending_fetch();
}

/**
 * \brief Signal handler when a release list fetch failed (show the error on the pages of that source)
 */
void WineRunnerWindow::on_fetch_failed()
{
  WineRunner::SourceId source_id = task_.get_fetched_source_id();
  Glib::ustring error_message = task_.get_error_message();
  for (const auto& page : source_pages_)
  {
    if (page->source_id != source_id)
      continue;
    page->fetch_pending = false;
    set_page_fetching_state(*page, false);
    page->status_label.set_text(error_message);
  }
  start_pending_fetch();
}

/**
 * \brief Signal handler when the install progress or phase changed (update the busy dialog)
 */
void WineRunnerWindow::on_progress_changed()
{
  WineRunner::InstallPhase phase = task_.get_phase();
  if (phase != last_phase_)
  {
    last_phase_ = phase;
    switch (phase)
    {
    case WineRunner::InstallPhase::Downloading:
      busy_dialog_.set_message("Installing " + installing_display_name_, "Downloading the archive...");
      break;
    case WineRunner::InstallPhase::Verifying:
      busy_dialog_.set_message("Installing " + installing_display_name_, "Verifying the archive checksum...");
      busy_dialog_.set_pulsing();
      break;
    case WineRunner::InstallPhase::Extracting:
      busy_dialog_.set_message("Installing " + installing_display_name_, "Extracting the archive...");
      busy_dialog_.set_pulsing();
      break;
    case WineRunner::InstallPhase::Idle:
      break;
    }
  }
  if (phase == WineRunner::InstallPhase::Downloading)
  {
    const auto& [bytes_done, bytes_total] = task_.get_progress();
    if (bytes_total > 0)
    {
      double fraction = static_cast<double>(bytes_done) / static_cast<double>(bytes_total);
      busy_dialog_.set_progress(fraction, Glib::format_size(bytes_done) + " of " + Glib::format_size(bytes_total));
    }
  }
}

/**
 * \brief Signal handler when the install finished (hide the busy dialog & refresh the lists)
 */
void WineRunnerWindow::on_install_finished()
{
  busy_dialog_.hide();
  switch (task_.get_install_status())
  {
  case WineRunner::InstallStatus::Success:
    refresh_installed_list();
    refresh_version_comboboxes();
    runners_changed.emit();
    break;
  case WineRunner::InstallStatus::Cancelled:
    break;
  case WineRunner::InstallStatus::Error:
    show_error_message(task_.get_error_message());
    break;
  }
  start_pending_fetch();
}

/**
 * \brief Signal handler when the removal finished (hide the busy dialog & refresh the lists)
 */
void WineRunnerWindow::on_remove_finished()
{
  busy_dialog_.hide();
  Glib::ustring error_message = task_.get_error_message();
  if (!error_message.empty())
  {
    show_error_message(error_message);
  }
  else
  {
    refresh_installed_list();
    refresh_version_comboboxes();
    runners_changed.emit();
  }
  start_pending_fetch();
}

/**
 * \brief Show an error message. User can only click 'OK'.
 * \param[in] message Show this error message
 */
void WineRunnerWindow::show_error_message(const Glib::ustring& message)
{
  error_dialog_.set_message(message);
  Glib::signal_idle().connect_once([this]() { error_dialog_.present(); }, Glib::PRIORITY_DEFAULT_IDLE);
}

/**
 * \brief Show an info message. User can only click 'OK'.
 * \param[in] message Show this info message
 */
void WineRunnerWindow::show_info_message(const Glib::ustring& message)
{
  info_dialog_.set_message(message);
  Glib::signal_idle().connect_once([this]() { info_dialog_.present(); }, Glib::PRIORITY_DEFAULT_IDLE);
}
