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

/**
 * \brief Helper method for creating a menu with an image
 * \return GTKWidget menu item pointer
 */
static GtkWidget* CreateImageMenuItem(const gchar* label_text, const gchar* icon_name) {
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

void showDialog(GtkWindow *parent, const gchar *message)
{
  GtkWidget *dialog, *label, *content_area;
  GtkDialogFlags flags;

  // Create the widgets
  flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  dialog = gtk_dialog_new_with_buttons("Message",
                                        parent,
                                        flags,
                                        "_CLOSE",
                                        GTK_RESPONSE_NONE,
                                        NULL);
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  label = gtk_label_new(message);

  // Ensure that the dialog box is destroyed when the user responds
  g_signal_connect_swapped (dialog,
                            "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);
  // Add the label, and show everything we’ve added
  gtk_container_add(GTK_CONTAINER(content_area), label);
  gtk_widget_show_all(dialog);
}

static void cbShowAbout(GtkButton *btn, gpointer parent_window) {
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
static GtkWidget* SetupMenu(GtkWidget *window) {
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
  g_signal_connect_swapped(exit, "activate", G_CALLBACK (gtk_widget_destroy), window);
  
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


static void print_hello (GtkWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

/**
 * \brief Create GUI in the activate signal trigger from the GTK app
 */
static void activate(GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *vbox;

  // Main Application Window + some settings
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "WineGUI - WINE Manager");
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

  // Vertical box container
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
  gtk_container_add(GTK_CONTAINER(window), vbox);

  // Create top menu
  GtkWidget *menu_bar = SetupMenu(window);  
  // Add menu to box (top)
  gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

  // Add paned container
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  // Vertical scroll only
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  // Add scrolled window with listbox to paned
  gtk_paned_add1(GTK_PANED(paned), scrolled_window);
  GtkWidget *listbox = gtk_list_box_new();
  for (int i=1; i<100; i++)
  {
    gchar *name = g_strdup_printf("Label %i", i);
    GtkWidget *label = gtk_label_new(name);
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(listbox), label);
  }
  // Add list box to scrolled window
  gtk_container_add(GTK_CONTAINER(scrolled_window), listbox);
  
  GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);  
  GtkWidget *button = gtk_button_new_with_label("Hello World");
  g_signal_connect(button, "clicked", G_CALLBACK (print_hello), NULL);
  g_signal_connect_swapped(button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_container_add(GTK_CONTAINER(button_box), button);
  // Add random button to paned
  gtk_paned_add2(GTK_PANED(paned), button_box);

  // Add paned to box (below menu)
  gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

  // Show!
  gtk_widget_show_all(window);
}

/**
 * \brief The beginning
 * \return Status code
 */
int main(int argc, char **argv)
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