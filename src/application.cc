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

Application::Application() : Gtk::Application("org.melroy.winegui", Gio::Application::Flags::HANDLES_OPEN)
{
  Glib::set_application_name("WineGUI");
}

Glib::RefPtr<Application> Application::create()
{
  return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_startup()
{
  Gtk::Application::on_startup();

  // Add actions and keyboard accelerators for the menu.
  add_action("about", sigc::mem_fun(*this, &Application::on_menu_help_about));

  add_action("newstandard", [] { std::cout << "app.newstandard" << std::endl; });
  add_action("newfoo", [] { std::cout << "app.newfoo" << std::endl; });
  add_action("newgoo", [] { std::cout << "app.newgoo" << std::endl; });
  add_action("preferences", sigc::mem_fun(*this, &Application::on_action_preferences));
  add_action("quit", sigc::mem_fun(*this, &Application::on_action_quit));

  set_accel_for_action("app.newstandard", "<Ctrl>N");
  set_accel_for_action("win.copy", "<Ctrl>C");
  set_accel_for_action("win.new", "<Ctrl>V");
  set_accel_for_action("app.quit", "<Ctrl>Q");

  auto menubar = Gio::Menu::create();
  {
    auto file_menu = Gio::Menu::create();
    {
      auto section = Gio::Menu::create();
      auto item = Gio::MenuItem::create("New _Standard", "app.newstandard");
      // Create a Gicon
      auto icon = Gio::Icon::create("edit-cut");
      item->set_icon(icon);
      section->append_item(item);
      section->append_item(Gio::MenuItem::create("New _Foo", "app.newfoo"));
      section->append_item(Gio::MenuItem::create("New _Goo", "app.newgoo"));
      section->append_item(Gio::MenuItem::create("_Preferences", "app.preferences"));
      file_menu->append_section(section);
    }
    {
      auto section = Gio::Menu::create();
      section->append_item(Gio::MenuItem::create("_Quit", "app.quit"));
      file_menu->append_section(section);
    }
    menubar->append_submenu("_File", file_menu);
  }

  {
    auto edit_menu = Gio::Menu::create();
    auto section = Gio::Menu::create();
    section->append_item(Gio::MenuItem::create("_Copy", "win.copy"));
    section->append_item(Gio::MenuItem::create("_New", "win.new"));
    section->append_item(Gio::MenuItem::create("_Something", "win.something"));
    edit_menu->append_section(section);
    menubar->append_submenu("_Edit", edit_menu);
  }
  {
    auto help_menu = Gio::Menu::create();
    auto section = Gio::Menu::create();
    section->append_item(Gio::MenuItem::create("_About Window", "win.about"));
    section->append_item(Gio::MenuItem::create("_About App", "app.about"));
    help_menu->append_section(section);
    menubar->append_submenu("_Help", help_menu);
  }

  set_menubar(menubar);
}

void Application::on_activate()
{
  create_window();
}

void Application::create_window()
{
  static MainWindow main_window; // menu
  static BottleManager manager(main_window);
  static PreferencesWindow preferences_window(main_window);
  static AboutDialog about_dialog(main_window);
  static BottleEditWindow edit_window(main_window);
  static BottleCloneWindow clone_window(main_window);
  static BottleConfigureEnvVarWindow settings_env_var_window(edit_window);
  static BottleConfigureWindow settings_window(main_window);
  static AddAppWindow add_app_window(main_window);
  static RemoveAppWindow remove_app_window(main_window);
  static SignalController signal_controller(manager, /*menu,*/ preferences_window, about_dialog, edit_window, clone_window, settings_env_var_window,
                                            settings_window, add_app_window, remove_app_window);

  signal_controller.set_main_window(&main_window);
  // Do all the signal connections of the life-time of the app
  signal_controller.dispatch_signals();

  // Call the Bottle Manager prepare method,
  // it will prepare Winetricks & retrieve Wine Bottles
  manager.prepare();

  // Make sure that the application runs for as long this window is still open.
  add_window(main_window);

  main_window.set_show_menubar();
  main_window.set_visible(true);
}

void Application::on_shutdown()
{
  // TODO: Write config file to disk here

  // Call the base class's implementation.
  Gtk::Application::on_shutdown();
}

void Application::on_menu_help_about()
{
  std::cout << "Help|About App was selected." << std::endl;
}

void Application::on_action_preferences()
{
  std::cout << "Preferences was selected." << std::endl;
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