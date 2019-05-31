/**
 * Copyright (c) 2019 WineGUI
 *
 * \file    main.cc
 * \brief   Main, where it all starts
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
#include <gtk/gtk.h>

static void print_hello (GtkWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
static GtkWidget* create_image_menu_item(const gchar* label_text, const gchar* icon_name) {
  GtkWidget *item = gtk_menu_item_new();
  GtkWidget *helper_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
  GtkWidget *label = gtk_label_new(label_text);
  gtk_container_add (GTK_CONTAINER (helper_box), icon);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_end (GTK_BOX (helper_box), label, TRUE, TRUE, 0);  
  gtk_container_add (GTK_CONTAINER (item), helper_box);
  return item;
}

/**
 * \brief Create the whole menu
 * \param[in] Window - Pointer to the main window
 * \return GTKWidget menu pointer
 */
static GtkWidget* setup_menu(GtkWidget *window) {
  // Add menu bar (THE menu)
  GtkWidget *menu_bar = gtk_menu_bar_new();
  // Create menu item
  GtkWidget *file_menu = gtk_menu_item_new_with_mnemonic("_File");

  // Create sub-menu
  GtkWidget *submenu1 = gtk_menu_new();
  // Create Menu item with label & image, using a box
  GtkWidget *open_item = create_image_menu_item("Open", "document-open");
  GtkWidget *save_item = create_image_menu_item("Save", "document-save");
  GtkWidget *quit = create_image_menu_item("Quit", "application-exit");
  // Add destroy signal to quit button
  g_signal_connect_swapped(quit, "activate", G_CALLBACK (gtk_widget_destroy), window);
  
  // Add items to sub-menu
  gtk_menu_shell_append(GTK_MENU_SHELL (submenu1), open_item);
  gtk_menu_shell_append(GTK_MENU_SHELL (submenu1), save_item);
  gtk_menu_shell_append(GTK_MENU_SHELL (submenu1), gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL (submenu1), quit);
  

  // Add sub-menu to menu
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_menu), submenu1);
  // Add menu items to menu bar
  gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_menu);
  return menu_bar;
}

/**
 * \brief Create GUI in the activate signal trigger from the GTK app
 */
static void activate(GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *menu_bar;

  // Main Application Window + some settings
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW (window), "WineGUI");
  gtk_window_set_default_size(GTK_WINDOW (window), 1200, 750);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

  // Vertical box container
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
  gtk_container_add(GTK_CONTAINER (window), vbox);

  // Create menu
  menu_bar = setup_menu(window);  
  // Add menu to box
  gtk_box_pack_start(GTK_BOX (vbox), menu_bar, FALSE, FALSE, 0);

  // Add random button  
  GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);  
  GtkWidget *button = gtk_button_new_with_label("Hello World");
  g_signal_connect(button, "clicked", G_CALLBACK (print_hello), NULL);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_container_add(GTK_CONTAINER (button_box), button);

  // Add button to box
  gtk_box_pack_start(GTK_BOX (vbox), button_box, TRUE, TRUE, 0);

  // Show!
  gtk_widget_show_all(window);
}

/**
 * \brief The beginning
 * \return Status code
 */
int main (int argc, char **argv)
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