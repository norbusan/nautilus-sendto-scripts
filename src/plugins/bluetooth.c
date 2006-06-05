/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
 * Copyright (C) 2004 Roberto Majadas
 * Copyright (C) 2005 Bastien Nocera
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more av.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:  Bastien Nocera <hadess@hadess.net>
 */

#include "../nautilus-sendto-plugin.h"
#include <gnomebt-controller.h>
#include <gnomebt-spinner.h>

static GtkTreeModel *model;
static GnomebtController *btctl;
static GdkPixbuf *phone_pix;
static int discovered;
static GtkWidget *combobox;
static GnomebtSpinner *spinner;
static guint id;

enum {
	ICON_COL,
	NAME_COL,
	BDADDR_COL,
	NUM_COLS
};

static gboolean
init (NstPlugin *plugin)
{
	GError *e = NULL;
	char *cmd;

	/* Check whether gnome-obex-send is available */
	cmd = g_find_program_in_path ("gnome-obex-send");
	if (cmd == NULL)
		return FALSE;
	g_free (cmd);

	btctl = gnomebt_controller_new ();

	if (btctl_controller_is_initialised (BTCTL_CONTROLLER (btctl), &e) == FALSE) {
		g_object_unref (btctl);
		g_print ("Couldn't init bluetooth plugin: %s\n", e ? e->message : "No reason");
		if (e)
			g_error_free (e);
		return FALSE;
	}

	discovered = 0;
	id = -1;

	//phone_pix = gtk_icon_theme_load_icon (it, "

	return TRUE;
}

static void
add_phone_to_list (GtkListStore *store, const gchar *name,
		const gchar *bdaddr)
{
	GtkTreeIter iter;

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			ICON_COL, phone_pix,
			NAME_COL, name,
			BDADDR_COL, bdaddr,
			-1);

	if (discovered == 0) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
		gtk_widget_set_sensitive (combobox, TRUE);
	}

	discovered++;
}

static gboolean
add_known_devices_to_list (GtkListStore *store)
{
	GSList *list, *item;

	list = gnomebt_controller_known_devices (btctl);
	if (list == NULL)
		return FALSE;
	for (item = list; item != NULL; item = item->next) {
		GnomebtDeviceDesc *dd= (GnomebtDeviceDesc *) item->data;

		/* FIXME use the class to have a nice icon thing */
		add_phone_to_list (store, dd->name, dd->bdaddr);
	}
	gnomebt_device_desc_list_free (list);

	return TRUE;
}

static void
on_device_name_cb (GnomebtController *btctl,
		gchar* device, gchar* name, gpointer data)
{
	GtkListStore *store = (GtkListStore *) data;

	/* We don't get new discovery for already known devices */
	add_phone_to_list (store, name, device);
}

static void
on_status_change_cb (GnomebtController *btctl, int status, gpointer data)
{
	if (status != BTCTL_STATUS_COMPLETE)
		return;

	g_source_remove (id);
	gnomebt_spinner_reset (spinner);
}

static gboolean
spin_spinner (gpointer data)
{
	gnomebt_spinner_spin (spinner);
	return TRUE;
}

static void
start_device_scanning (GtkListStore *store)
{
	g_signal_connect (G_OBJECT (btctl), "device_name",
			G_CALLBACK (on_device_name_cb), store);
	g_signal_connect (G_OBJECT (btctl), "status_change",
			G_CALLBACK (on_status_change_cb), NULL);
	btctl_controller_discover_async (BTCTL_CONTROLLER (btctl));
	id = g_timeout_add (200, (GSourceFunc) spin_spinner, NULL);
}

static GtkWidget*
get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *hbox;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeIter iter;

	/* The model */
	store = gtk_list_store_new (NUM_COLS,
			GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL (store);

	/* The widget itself */
	combobox = gtk_combo_box_new_with_model (model);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox),
			renderer,
			FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), 
			renderer,
			"pixbuf", 0,
			NULL);		
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox),
			renderer,
			TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), 
			renderer,
			"text", 1,
			NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
	gtk_widget_set_sensitive (combobox, FALSE);

	/* The spinner */
	spinner = gnomebt_spinner_new ();

	/* The box */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (spinner),
			FALSE, FALSE, 0);

	add_known_devices_to_list (store);
	start_device_scanning (store);
	gtk_widget_show_all (hbox);

	return hbox;
}

static gboolean
send_files (NstPlugin *plugin, GtkWidget *contact_widget,
		GList *file_list)
{
	GPtrArray *argv;
	GList *list;
	gboolean ret;
	GtkTreeIter iter;
	gchar *path, *bdaddr;
	int option;
	guint i;
	GError *err = NULL;

	option = gtk_combo_box_get_active (GTK_COMBO_BOX (combobox));
	if (option == -1) {
		g_warning ("Couldn't find an active device");
		return FALSE;
	}

	path = g_strdup_printf ("%d", option);
	ret = gtk_tree_model_get_iter_from_string (model, &iter, path);
	g_free (path);
	if (ret == FALSE) {
		g_warning ("Couldn't get bluetooth address of the device");
		return FALSE;
	}
	gtk_tree_model_get (model, &iter, BDADDR_COL, &bdaddr, -1);

	argv = g_ptr_array_new ();
	g_ptr_array_add (argv, "gnome-obex-send");
	g_ptr_array_add (argv, "--dest");
	g_ptr_array_add (argv, bdaddr);

	for (list = file_list; list != NULL; list = list->next) {
		g_ptr_array_add (argv, (gchar *) list->data);
	}
	g_ptr_array_add (argv, NULL);

#if 0
	g_print ("launching command: ");
	for (i = 0; i < argv->len - 1; i++) {
		g_print ("%s ", (gchar *) g_ptr_array_index (argv, i));
	}
	g_print ("\n");
#endif
	ret = g_spawn_async (NULL, (gchar **) argv->pdata,
			NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
	g_ptr_array_free (argv, TRUE);

	if (ret == FALSE) {
		g_warning ("Couldn't send files via bluetooth: %s", err->message);
		g_error_free (err);
	}
	return ret;
}

static gboolean
destroy (NstPlugin *plugin){
	//FIXME
	return TRUE;
}

static 
NstPluginInfo plugin_info = {
	"stock_bluetooth",
	N_("Bluetooth (OBEX)"),
	TRUE,
	init,
	get_contacts_widget,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

