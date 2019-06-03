/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    gui.cc
 * \brief   GTK+ User Interface
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
#include "gui.h"

/**
 * \brief Contructor
 */
GUI::GUI() {
}

/**
 * \brief Open the GUI
 * \return Status code
 */
int GUI::start(int argc, char **argv)
{
  GtkApplication *app;
  int status;
  
  // GTK App, default flags
  app = gtk_application_new("org.melroy.winegui", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run(G_APPLICATION (app), argc, argv);

  // Free memory
  g_object_unref(app);
  return status;
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
GtkWidget* GUI::CreateImageMenuItem(const gchar* label_text, const gchar* icon_name) {
  GtkWidget *item = gtk_menu_item_new();
  GtkWidget *helper_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
  GtkWidget *label = gtk_label_new(label_text);
  gtk_container_add(GTK_CONTAINER(helper_box), icon);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_box_pack_end(GTK_BOX(helper_box), label, TRUE, TRUE, 0);  
  gtk_container_add(GTK_CONTAINER(item), helper_box);
  return item;
}

void GUI::cbShowAbout(GtkButton *btn, gpointer parent_window) {
  GdkPixbuf *example_logo = gdk_pixbuf_new_from_file("./logo.png", NULL);
  gchar const *authors[] = {
    "Melroy van den Berg <melroy@melroy.org>",
    NULL
  };
  gtk_show_about_dialog(GTK_WINDOW(parent_window),
                       "program-name", "WineGUI",
                       "logo", example_logo,
                       "title", "About WineGUI",
                       "authors", authors,
                       "version", "v1.0",
                       "copyright", "Copyright © 2019 Melroy van den Berg",
                       "license-type", GTK_LICENSE_AGPL_3_0,
                       NULL);
}

/**
 * \brief Create the whole menu
 * \param[in] window - Pointer to the main window
 * \return GTKWidget menu pointer
 */
GtkWidget* GUI::SetupMenu(GtkWidget *window) {
  // Add menu bar (THE menu)
  GtkWidget *menu_bar = gtk_menu_bar_new();
  // Create menu item
  GtkWidget *file_menu = gtk_menu_item_new_with_mnemonic("_File");
  GtkWidget *help_menu = gtk_menu_item_new_with_mnemonic("_Help");

  // Create file sub-menu
  GtkWidget *file_sub_menu = gtk_menu_new();
  // Create Menu item with label & image, using a box
  GtkWidget *preferences = CreateImageMenuItem("Preferences", "preferences-other");
  GtkWidget *save_item = CreateImageMenuItem("Save", "document-save");
  GtkWidget *exit = CreateImageMenuItem("Exit", "application-exit");
  // Add window destroy signal to exit button
  g_signal_connect_swapped(exit, "activate", G_CALLBACK(gtk_widget_destroy), window);
  
  // Create Help sub-menu
  GtkWidget *help_sub_menu = gtk_menu_new();
  GtkWidget *about = CreateImageMenuItem("About WineGUI...", "help-about");
  g_signal_connect (about, "activate", G_CALLBACK(cbShowAbout), window);

  // Add items to sub-menu
  gtk_menu_shell_append(GTK_MENU_SHELL(file_sub_menu), preferences);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_sub_menu), gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(file_sub_menu), save_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_sub_menu), gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(file_sub_menu), exit);

  gtk_menu_shell_append(GTK_MENU_SHELL(help_sub_menu), about);
  
  // Add sub-menu's to menu's
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), file_sub_menu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu), help_sub_menu);

  // Add menu items to menu bar
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_menu);
  return menu_bar;
}

/**
 * \brief Create a GUI Foundation with a GTK Box, the menu and paned container below that
 * \param[in] window - GTK Application Window
 * \return GTK Paned pointer
 */
GtkWidget* GUI::CreateFoundation(GtkWidget *window)
{
  // Using a Vertical box container
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);  
  gtk_container_add(GTK_CONTAINER(window), vbox);
  // Add CSS classes
  //add_css();
  // Create top menu
  GtkWidget *menu_bar = SetupMenu(window);  
  // Add menu to box (top)
  gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

  // Add paned container
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  // Add paned to box (below menu)
  gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
  return paned;
}

/**
 * \brief Create left side of the GUI
 * \param[in] paned - GTK Paned pointer
 */
void GUI::CreateLeftPanel(GtkWidget *paned)
{
  // Use a scrolled window
  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  // Vertical scroll only
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);  
  // Add scrolled window with listbox to paned
  gtk_paned_pack1(GTK_PANED(paned), scrolled_window, FALSE, TRUE);
  gtk_widget_set_size_request(scrolled_window, 240, -1);

  GtkWidget *listbox = gtk_list_box_new();
  // Set function that will add seperators between each item
  gtk_list_box_set_header_func(GTK_LIST_BOX(listbox), cc_list_box_update_header_func, NULL, NULL);
  for (int i=1; i<20; i++)
  {
    GtkWidget *image = gtk_image_new_from_file("../images/windows/10_64.png");
    gtk_widget_set_margin_top(image, 8);
    gtk_widget_set_margin_bottom(image, 8);
    gtk_widget_set_margin_start(image, 8);
    gtk_widget_set_margin_end(image, 8);

    GtkWidget *name = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(name), 0.0);
    // CSS text
    gtk_label_set_markup(GTK_LABEL(name), "<span size=\"medium\"><b>Windows 10 (64bit)</b></span>");
    gtk_label_set_xalign(GTK_LABEL(name), 0.0);

    GtkWidget *status_icon = gtk_image_new_from_file(READY_IMAGE);
    gtk_widget_set_size_request(status_icon, 2, -1);
    gtk_widget_set_halign(status_icon, GTK_ALIGN_START);

    gchar *status_text = g_strdup_printf("Ready");
    GtkWidget *status_label = gtk_label_new(status_text);
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);

    GtkWidget *row = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(row), 8);
    gtk_grid_set_row_spacing(GTK_GRID(row), 5);
    gtk_container_set_border_width(GTK_CONTAINER(row), 4);

    gtk_grid_attach(GTK_GRID(row), image, 0, 0, 1, 2);
    // Agh, stupid GTK! Width 2 would be enough, add 8 extra = 10
    // I can't control the gtk grid cell width
    gtk_grid_attach_next_to(GTK_GRID(row), name, image, GTK_POS_RIGHT, 10, 1);

    gtk_grid_attach(GTK_GRID(row), status_icon, 1, 1, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(row), status_label, status_icon, GTK_POS_RIGHT, 1, 1);

    gtk_widget_show(GTK_WIDGET(row));
    // Add the whole grid to the listbox
    gtk_container_add(GTK_CONTAINER(listbox), GTK_WIDGET(row));
  }
  // Add list box to scrolled window
  gtk_container_add(GTK_CONTAINER(scrolled_window), listbox);
}

/**
 * \brief Create right side of the GUI
 * \param[in] paned - GTK Paned pointer
 */
void GUI::CreateRightPanel(GtkWidget *paned)
{
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *toolbar = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

  // Buttons in toolbar
  GtkWidget *add_image = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
  GtkToolItem *add_button = gtk_tool_button_new(add_image, "New");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), add_button, 0);

  GtkWidget *run_image = gtk_image_new_from_icon_name("system-run", GTK_ICON_SIZE_LARGE_TOOLBAR);
  GtkToolItem *run_button = gtk_tool_button_new(run_image, "Run Program...");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), run_button, 1);

  GtkWidget *perf_image = gtk_image_new_from_icon_name("preferences-other", GTK_ICON_SIZE_LARGE_TOOLBAR);
  GtkToolItem *per_button = gtk_tool_button_new(perf_image, "Settings");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), per_button, 2);

  GtkWidget *install_image = gtk_image_new_from_icon_name("system-software-install", GTK_ICON_SIZE_LARGE_TOOLBAR);
  GtkToolItem *install_button = gtk_tool_button_new(install_image, "Configure");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), install_button, 3);

  GtkWidget *reboot_image = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_LARGE_TOOLBAR);
  GtkToolItem *reboot_button = gtk_tool_button_new(reboot_image, "Reboot");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), reboot_button, 4);

  // Add toolbar to box
  gtk_container_add(GTK_CONTAINER(box), toolbar);
  gtk_container_add(GTK_CONTAINER(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  // Add detail section below toolbar
  GtkWidget *detail_grid = gtk_grid_new();
  gtk_widget_set_margin_top(detail_grid, 5);
  gtk_widget_set_margin_bottom(detail_grid, 8);
  gtk_widget_set_margin_start(detail_grid, 8);
  gtk_widget_set_margin_end(detail_grid, 5);
  gtk_grid_set_column_spacing(GTK_GRID(detail_grid), 8);
  gtk_grid_set_row_spacing(GTK_GRID(detail_grid), 12);

  // General heading
  GtkWidget *general_icon = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_MENU);
  GtkWidget *general_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(general_label), "<b>General</b>");
  gtk_grid_attach(GTK_GRID(detail_grid), general_icon, 0, 0, 1, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), general_label, general_icon, GTK_POS_RIGHT, 1, 1);

  // Windows version + bit os
  GtkWidget *window_version_label = gtk_label_new("Windows:");
  gchar windows[30], bit[10];
  g_stpcpy(windows, "Windows 7");
  g_snprintf(bit, sizeof(bit), " (%s)", "64-bit");
  g_strlcat(windows, bit, sizeof(windows));
  GtkWidget *window_version = gtk_label_new(windows);
  gtk_label_set_xalign(GTK_LABEL(window_version_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(window_version), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), window_version_label, 0, 1, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), window_version, window_version_label, GTK_POS_RIGHT, 1, 1);

  // Wine version
  GtkWidget *wine_version_label = gtk_label_new("Wine version:");
  GtkWidget *wine_version = gtk_label_new("v4.0.1");
  gtk_label_set_xalign(GTK_LABEL(wine_version_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(wine_version), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), wine_version_label, 0, 2, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), wine_version, wine_version_label, GTK_POS_RIGHT, 1, 1);

  // Wine location
  GtkWidget *wine_location_label = gtk_label_new("Wine location:");
  GtkWidget *wine_location = gtk_label_new("~/.winegui/prefixes/win7_64");
  gtk_label_set_xalign(GTK_LABEL(wine_location_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(wine_location), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), wine_location_label, 0, 3, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), wine_location, wine_location_label, GTK_POS_RIGHT, 1, 1);

  // Wine C drive location
  GtkWidget *windows_c_drive_label = gtk_label_new("C: drive location:");
  GtkWidget *windows_c_drive = gtk_label_new("~/.winegui/prefixes/win7_64/dosdevices/c:/");
  gtk_label_set_xalign(GTK_LABEL(windows_c_drive_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(windows_c_drive), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), windows_c_drive_label, 0, 4, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), windows_c_drive, windows_c_drive_label, GTK_POS_RIGHT, 1, 1);

  // Wine last changed
  GtkWidget *wine_last_updated_label = gtk_label_new("Wine last changed:");
  GtkWidget *wine_last_updated = gtk_label_new("07-07-2019 23:11AM");
  gtk_label_set_xalign(GTK_LABEL(wine_last_updated_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(wine_last_updated), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), wine_last_updated_label, 0, 5, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), wine_last_updated, wine_last_updated_label, GTK_POS_RIGHT, 1, 1);
  // End General
  gtk_grid_attach(GTK_GRID(detail_grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 6, 3, 1);

  // Audio heading
  GtkWidget *audio_icon = gtk_image_new_from_icon_name("audio-speakers", GTK_ICON_SIZE_MENU);
  GtkWidget *audio_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(audio_label), "<b>Audio</b>");
  gtk_grid_attach(GTK_GRID(detail_grid), audio_icon, 0, 7, 1, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), audio_label, audio_icon, GTK_POS_RIGHT, 1, 1);

  // Audio driver
  GtkWidget *audio_driver_label = gtk_label_new("Audio driver:");
  GtkWidget *audio_driver = gtk_label_new("Pulseaudio");
  gtk_label_set_xalign(GTK_LABEL(audio_driver_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(audio_driver), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), audio_driver_label, 0, 8, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), audio_driver, audio_driver_label, GTK_POS_RIGHT, 1, 1);
  // End Audio driver
  gtk_grid_attach(GTK_GRID(detail_grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 9, 3, 1);

  // Display heading
  GtkWidget *display_icon = gtk_image_new_from_icon_name("video-display", GTK_ICON_SIZE_MENU);
  GtkWidget *display_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(display_label), "<b>Display</b>");
  gtk_grid_attach(GTK_GRID(detail_grid), display_icon, 0, 10, 1, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), display_label, display_icon, GTK_POS_RIGHT, 1, 1);

  // Virtual Desktop
  GtkWidget *virtual_desktop_label = gtk_label_new("Virtual desktop\n(Window Mode):");
  GtkWidget *virtual_desktop = gtk_label_new("Disabled");
  gtk_label_set_xalign(GTK_LABEL(virtual_desktop_label), 0.0);
  gtk_label_set_xalign(GTK_LABEL(virtual_desktop), 0.0);
  // Label consumes 2 columns
  gtk_grid_attach(GTK_GRID(detail_grid), virtual_desktop_label, 0,11, 2, 1);
  gtk_grid_attach_next_to(GTK_GRID(detail_grid), virtual_desktop, virtual_desktop_label, GTK_POS_RIGHT, 1, 1);
  // End Display
  gtk_grid_attach(GTK_GRID(detail_grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 12, 3, 1);


  // Add detail grid to box
  gtk_box_pack_start(GTK_BOX(box), detail_grid, FALSE, FALSE, 0);

  // Add box to paned
  gtk_paned_add2(GTK_PANED(paned), box);
}

/**
 * \brief Create GUI in the activate signal trigger from the GTK app
 */
void GUI::activate(GtkApplication *app, gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *paned;

  // General Main Application Window + some settings
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "WineGUI - WINE Manager");
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

  // Create the rest
  paned = CreateFoundation(window);
  CreateLeftPanel(paned);
  CreateRightPanel(paned);
  // Finally, show!
  gtk_widget_show_all(window);
}

/**
 * \brief Override update header function of GTK Listbox with custom layout
 * \param[in] row
 * \param[in] before
 * \param[in] user_data
 */
void GUI::cc_list_box_update_header_func(GtkListBoxRow *row,
                                GtkListBoxRow *before,
                                gpointer user_data)
{
  GtkWidget *current;
  if (before == NULL) {
    gtk_list_box_row_set_header(row, NULL);
    return;
  }
  current = gtk_list_box_row_get_header(row);
  if (current == NULL){
    current = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (current);
    gtk_list_box_row_set_header(row, current);
  }
}

void GUI::print_hello (GtkWidget *widget, gpointer data)
{
  g_print ("Hello World\n");
}

void GUI::add_css()
{
  char const *css =
    ".box { border-style: solid; border-width: 4px; }\n"
    ".border_solid    { border-style: solid; }\n"
    ;
  /* CSS */
  GError *error = NULL;
  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, strlen (css), &error);
  if (error != NULL)
  {
    fprintf (stderr, "CSS: %s\n", error->message);
  }

  gtk_style_context_add_provider_for_screen(
    gdk_screen_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_USER);
}

