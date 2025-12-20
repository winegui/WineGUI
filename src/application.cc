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
// #include "menu.h"
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

  // File menu:
  add_action("quit", sigc::mem_fun(*this, &Application::on_menu_file_quit));
  // Help menu:
  add_action("about", sigc::mem_fun(*this, &Application::on_menu_help_about));

  add_action("preferences", sigc::mem_fun(*this, &Application::on_action_preferences));
  add_action("quit", sigc::mem_fun(*this, &Application::on_action_quit));
  set_accel_for_action("app.quit", "<Ctrl>Q");

  m_refBuilder = Gtk::Builder::create();

  // Layout the actions in a menubar and a menu:
  Glib::ustring ui_info = "<interface>"
                          "  <!-- menubar -->"
                          "  <menu id='menu-example'>"
                          "    <submenu>"
                          "      <attribute name='label' translatable='yes'>_File</attribute>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>New _Standard</attribute>"
                          "          <attribute name='action'>app.newstandard</attribute>"
                          "          <attribute name='accel'>&lt;Primary&gt;n</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>New _Foo</attribute>"
                          "          <attribute name='action'>app.newfoo</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>New _Goo</attribute>"
                          "          <attribute name='action'>app.newgoo</attribute>"
                          "        </item>"
                          "      </section>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>_Quit</attribute>"
                          "          <attribute name='action'>app.quit</attribute>"
                          "          <attribute name='accel'>&lt;Primary&gt;q</attribute>"
                          "        </item>"
                          "      </section>"
                          "    </submenu>"
                          "    <submenu>"
                          "      <attribute name='label' translatable='yes'>_Edit</attribute>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>_Copy</attribute>"
                          "          <attribute name='action'>win.copy</attribute>"
                          "          <attribute name='accel'>&lt;Primary&gt;c</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>_Paste</attribute>"
                          "          <attribute name='action'>win.paste</attribute>"
                          "          <attribute name='accel'>&lt;Primary&gt;v</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>_Something</attribute>"
                          "          <attribute name='action'>win.something</attribute>"
                          "        </item>"
                          "      </section>"
                          "    </submenu>"
                          "    <submenu>"
                          "      <attribute name='label' translatable='yes'>_Choices</attribute>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>Choice _A</attribute>"
                          "          <attribute name='action'>win.choice</attribute>"
                          "          <attribute name='target'>a</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>Choice _B</attribute>"
                          "          <attribute name='action'>win.choice</attribute>"
                          "          <attribute name='target'>b</attribute>"
                          "        </item>"
                          "      </section>"
                          "    </submenu>"
                          "    <submenu>"
                          "      <attribute name='label' translatable='yes'>_Other Choices</attribute>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>Choice 1</attribute>"
                          "          <attribute name='action'>win.choiceother</attribute>"
                          "          <attribute name='target' type='i'>1</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>Choice 2</attribute>"
                          "          <attribute name='action'>win.choiceother</attribute>"
                          "          <attribute name='target' type='i'>2</attribute>"
                          "        </item>"
                          "      </section>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>Some Toggle</attribute>"
                          "          <attribute name='action'>win.sometoggle</attribute>"
                          "        </item>"
                          "      </section>"
                          "    </submenu>"
                          "    <submenu>"
                          "      <attribute name='label' translatable='yes'>_Help</attribute>"
                          "      <section>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>_About Window</attribute>"
                          "          <attribute name='action'>win.about</attribute>"
                          "        </item>"
                          "        <item>"
                          "          <attribute name='label' translatable='yes'>_About App</attribute>"
                          "          <attribute name='action'>app.about</attribute>"
                          "        </item>"
                          "      </section>"
                          "    </submenu>"
                          "  </menu>"
                          "</interface>";

  try
  {
    m_refBuilder->add_from_string(ui_info);
  }
  catch (const Glib::Error& ex)
  {
    std::cerr << "Building menus failed: " << ex.what();
  }

  // Get the menubar and the app menu, and add them to the application:
  auto gmenu = m_refBuilder->get_object<Gio::Menu>("menu-example");
  if (!gmenu)
  {
    g_warning("GMenu not found");
  }
  else
  {
    set_menubar(gmenu);
  }
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

void Application::on_menu_file_quit()
{
  std::cout << G_STRFUNC << std::endl;
  quit(); // Not really necessary, when Gtk::Widget::set_visible(false) is called.

  // Gio::Application::quit() will make Gio::Application::run() return,
  // but it's a crude way of ending the program. The window is not removed
  // from the application. Neither the window's nor the application's
  // destructors will be called, because there will be remaining reference
  // counts in both of them. If we want the destructors to be called, we
  // must remove the window from the application. One way of doing this
  // is to hide the window.
  std::vector<Gtk::Window*> windows = get_windows();
  if (windows.size() > 0)
    windows[0]->set_visible(false); // In this simple case, we know there is only one window.
}

void Application::on_menu_help_about()
{
  std::cout << "Help|About App was selected." << std::endl;
}

void Application::on_action_preferences()
{
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