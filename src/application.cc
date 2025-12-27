#include "application.h"
#include "about_dialog.h"
#include "add_app_window.h"
#include "bottle_clone_window.h"
#include "bottle_configure_env_var_window.h"
#include "bottle_configure_window.h"
#include "bottle_edit_window.h"
#include "bottle_manager.h"
#include "main_window.h"
#include "preferences_window.h"
#include "remove_app_window.h"
#include "signal_controller.h"
#include <iostream>

Application::Application() : Gtk::Application("org.melroy.winegui", Gio::Application::Flags::DEFAULT_FLAGS)
{
  Glib::set_application_name("WineGUI");

  // Create all objects
  main_window_ = Gtk::make_managed<MainWindow>();
  if (main_window_)
  {
    preferences_window_ = Gtk::make_managed<PreferencesWindow>(*main_window_);
    about_dialog_ = Gtk::make_managed<AboutDialog>(*main_window_);
    edit_window_ = Gtk::make_managed<BottleEditWindow>(*main_window_);
    clone_window_ = Gtk::make_managed<BottleCloneWindow>(*main_window_);
    configure_env_var_window_ = Gtk::make_managed<BottleConfigureEnvVarWindow>(*edit_window_);
    configure_window_ = Gtk::make_managed<BottleConfigureWindow>(*main_window_);
    add_app_window_ = Gtk::make_managed<AddAppWindow>(*main_window_);
    remove_app_window_ = Gtk::make_managed<RemoveAppWindow>(*main_window_);
    manager_ = std::make_shared<BottleManager>(*main_window_);
    signal_controller_ = std::make_shared<SignalController>(main_window_, *manager_, *preferences_window_, *edit_window_, *clone_window_,
                                                            *configure_env_var_window_, *configure_window_, *add_app_window_, *remove_app_window_);
  }
  else
  {
    g_error("Something really strange is going on with creating the main window!");
  }
}

Glib::RefPtr<Application> Application::create()
{
  return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_startup()
{
  Gtk::Application::on_startup();

  // Add actions and keyboard accelerators for the menu.
  add_action("preferences", sigc::mem_fun(*preferences_window_, &PreferencesWindow::show));
  add_action("quit", sigc::mem_fun(*this, &Application::on_action_quit));
  add_action("refresh_view", sigc::bind(sigc::mem_fun(*manager_, &BottleManager::update_config_and_bottles), "", false));
  add_action("remove_bottle", sigc::bind(sigc::mem_fun(*manager_, &BottleManager::delete_bottle), main_window_));
  add_action("open_c_drive", sigc::mem_fun(*manager_, &BottleManager::open_c_drive));
  add_action("open_log_file", sigc::mem_fun(*manager_, &BottleManager::open_log_file));
  add_action("edit_bottle", sigc::mem_fun(*edit_window_, &BottleEditWindow::show));
  add_action("clone_bottle", sigc::mem_fun(*clone_window_, &BottleCloneWindow::show));
  add_action("configure_bottle", sigc::mem_fun(*configure_window_, &BottleConfigureWindow::show));
  add_action("about", sigc::mem_fun(*about_dialog_, &AboutDialog::run_dialog));

  // Add accelerators
  set_accel_for_action("app.preferences", "<Ctrl>P");
  set_accel_for_action("app.quit", "<Ctrl>Q");
  set_accel_for_action("app.refresh_view", "<Ctrl><Alt>R");
  set_accel_for_action("app.remove_bottle", "<Ctrl>Delete");
  set_accel_for_action("app.open_c_drive", "<Ctrl>O");
  set_accel_for_action("app.open_log_file", "<Ctrl>L");
  set_accel_for_action("app.edit_bottle", "<Ctrl>E");
  set_accel_for_action("app.clone_bottle", "<Ctrl><Alt>C");
  set_accel_for_action("app.configure_bottle", "<Ctrl>U");

  set_accel_for_action("win.new_bottle", "<Ctrl>N");
  set_accel_for_action("win.run", "<Ctrl>R");
  set_accel_for_action("win.check_version", "<Ctrl><Alt>V");

  auto menubar = Gio::Menu::create();
  {
    auto file_menu = Gio::Menu::create();
    {
      auto section = Gio::Menu::create();
      auto item = Gio::MenuItem::create("Preferences", "app.preferences");
      auto icon = Gio::Icon::create("edit-cut");
      item->set_icon(icon); // This is not working ;(
      section->append_item(item);
      file_menu->append_section(section);
    }
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("Exit", "app.quit")); // Icon: application-exit
      file_menu->append_section(section);
    }
    menubar->append_submenu("File", file_menu);
  }
  {
    auto view_menu = Gio::Menu::create();
    view_menu->append_item(Gio::MenuItem::create("Refresh", "app.refresh_view"));
    menubar->append_submenu("View", view_menu);
  }
  {
    auto machine_menu = Gio::Menu::create();
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("New", "win.new_bottle")); // Icon: list-add
      machine_menu->append_section(section);
    }
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("Edit", "app.edit_bottle"));           // Icon: document-edit
      section->append_item(Gio::MenuItem::create("Run...", "win.run"));                 // Icon: media-playback-start
      section->append_item(Gio::MenuItem::create("Remove", "app.remove_bottle"));       // Icon: edit-delete
      section->append_item(Gio::MenuItem::create("Clone", "app.clone_bottle"));         // Icon: edit-copy
      section->append_item(Gio::MenuItem::create("Configure", "app.configure_bottle")); // Icon: preferences-other
      machine_menu->append_section(section);
    }
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("Open C Drive", "app.open_c_drive"));   // Icon: drive-harddisk
      section->append_item(Gio::MenuItem::create("Open Log File", "app.open_log_file")); // Icon: text-x-generic
      machine_menu->append_section(section);
    }
    menubar->append_submenu("Machine", machine_menu);
  }
  {
    auto help_menu = Gio::Menu::create();
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("Issue List", "win.list_issues"));          // Icon: emblem-package
      section->append_item(Gio::MenuItem::create("Report an Issue", "win.report_issue"));    // Icon: emblem-package
      section->append_item(Gio::MenuItem::create("Check for Updates", "win.check_version")); // Icon: system-software-update
      help_menu->append_section(section);
    }
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("About WineGUI", "app.about")); // Icon: help-about
      help_menu->append_section(section);
    }
    menubar->append_submenu("Help", help_menu);
  }

  set_menubar(menubar);
}

void Application::on_activate()
{
  // Configure the signal controller signals
  signal_controller_->dispatch_signals();

  // Call the Bottle Manager prepare method,
  // it will prepare Winetricks & retrieve Wine Bottles from disk
  manager_->prepare();

  // Make sure that the application runs for as long this window is still open.
  add_window(*main_window_);

  // Show the main window
  main_window_->set_show_menubar();
  main_window_->present();
}

void Application::on_shutdown()
{
  Gtk::Application::on_shutdown();
}

void Application::on_action_quit()
{
  // Gio::Application::quit() will make Gio::Application::run() return,
  // but it's a crude way of ending the program. The window is not removed
  // from the application. Neither the window's nor the application's
  // destructors will be called, because there will be remaining reference
  // counts in both of them. If we want the destructors to be called, we
  // must remove the window from the application. See Application's constructor.
  // A window can be removed from an application with Gtk::Application::remove_window()
  // or Gtk::Window::unset_application().
  auto windows = get_windows();
  for (auto window : windows)
    remove_window(*window);

  // Not really necessary, when Gtk::Application::remove_window() is called,
  // unless Gio::Application::hold() has been called without a corresponding
  // call to Gio::Application::release().
  quit();
}