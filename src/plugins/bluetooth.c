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

#include "config.h"

#include <bluetooth-marshal.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n-lib.h>
#include "../nautilus-sendto-plugin.h"

#define OBEX_SERVICE_CLASS_NAME "object transfer"

static GtkTreeModel *model;
static int discovered;
static GtkWidget *combobox;

DBusGProxy *object;

enum {
	NAME_COL,
	BDADDR_COL,
	NUM_COLS
};

static gboolean
init (NstPlugin *plugin)
{
	GError *e = NULL;
	char *cmd;
	DBusGConnection *conn;
	const char *adapter;

	/* Check whether gnome-obex-send is available */
	cmd = g_find_program_in_path ("gnome-obex-send");
	if (cmd == NULL)
		return FALSE;
	g_free (cmd);

	conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &e);
	if (e != NULL) {
		g_warning ("Couldn't connect to bus: %s",
			   e->message);
		g_error_free (e);
		return FALSE;
	}

	object = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    "/org/bluez", "org.bluez.Manager");
	dbus_g_proxy_call (object, "DefaultAdapter", &e,
			   G_TYPE_INVALID, G_TYPE_STRING, &adapter, G_TYPE_INVALID);
	if (e != NULL) {

		if (e->domain == DBUS_GERROR &&
                    e->code == DBUS_GERROR_REMOTE_EXCEPTION) {
			const char *name;

			name = dbus_g_error_get_name (e);

			/* No adapter */
			if (g_str_equal (name, "org.bluez.Error.NoSuchAdapter") != FALSE) {
				g_error_free (e);
				return FALSE;
			}
		}

		g_warning ("Couldn't get default bluetooth adapter: %s",
			   e->message);
		g_error_free (e);
		return FALSE;
	}

	object = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    adapter, "org.bluez.Adapter");

	discovered = 0;

	return TRUE;
}

static gboolean
find_iter_for_address (GtkListStore *store, const char *bdaddr, GtkTreeIter *iter)
{
	int i, n_children; 
	gboolean found = FALSE;

	n_children = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL);
	for (i = 0; i < n_children; i++) {
		char *address;

		if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store),
						   iter, NULL, i) == FALSE)
			break;
		gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
				    BDADDR_COL, &address, -1);
		if (g_str_equal (address, bdaddr) != FALSE) {
			found = TRUE;
			g_free (address);
			break;
		}
		g_free (address);
	}

	return found;
}

static void
add_phone_to_list (GtkListStore *store, const char *name, const char *bdaddr)
{
	GtkTreeIter iter;
	gboolean found = FALSE;

	found = find_iter_for_address (store, bdaddr, &iter);
	if (found == FALSE) {
		gtk_list_store_append (store, &iter);
	} else {
		if (name == NULL)
			return;
	}

	if (name == NULL)
		name = bdaddr;

	gtk_list_store_set (store, &iter,
			NAME_COL, name,
			BDADDR_COL, bdaddr,
			-1);

	if (discovered == 0) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
		gtk_widget_set_sensitive (combobox, TRUE);
	}

	discovered++;
}

static void
add_devices_to_list (GtkListStore *store, const char **array)
{
	GError *e = NULL;
	while (*array) {
		const char *name;
		dbus_g_proxy_call (object, "GetRemoteName", &e,
				   G_TYPE_STRING, *array, G_TYPE_INVALID,
				   G_TYPE_STRING, &name, G_TYPE_INVALID);
		if (e == NULL) {
			add_phone_to_list (store, name, *array);
		} else {
			g_error_free (e);
			e = NULL;
		}
		array++;
	}
}

static void
add_known_devices_to_list (GtkListStore *store)
{
	GError *e = NULL;
	const char **array;

	dbus_g_proxy_call (object, "ListRemoteDevices", &e,
			   G_TYPE_INVALID, G_TYPE_STRV, &array, G_TYPE_INVALID);
	if (e == NULL) {
		add_devices_to_list (store, array);
	} else {
		/* Most likely bluez-utils < 3.8, so no ListRemoteDevices */
		const char *name;

		name = dbus_g_error_get_name (e);
		if (g_str_equal (name, "org.bluez.Error.UnknownMethod") != FALSE) {
			g_error_free (e);
			e = NULL;
			dbus_g_proxy_call (object, "ListBondings", &e,
					   G_TYPE_INVALID, G_TYPE_STRV, &array, G_TYPE_INVALID);
			if (e == NULL) {
				add_devices_to_list (store, array);
			} else {
				g_error_free (e);
			}
		} else {
			g_error_free (e);
		}
	}
}

static void
discovery_started (DBusGProxy *object, gpointer user_data)
{
	/* Discovery started! */
}

static void
remote_device_found (DBusGProxy *object,
		     const char *address, guint class, int rssi,
		     GtkListStore *store)
{
	add_phone_to_list (store, NULL, address);
}

static void
remote_name_updated (DBusGProxy *object,
		     const char *address, const char *name,
		     GtkListStore *store)
{
	add_phone_to_list (store, name, address);
}

static void
discovery_completed (DBusGProxy *object, gpointer user_data)
{
	GError *e = NULL;

	/* Discovery finished, launch a periodic discovery */

	dbus_g_proxy_call (object, "StartPeriodicDiscovery", &e,
			   G_TYPE_INVALID, G_TYPE_INVALID);
	if (e != NULL) {
		g_warning ("Couldn't start periodic discovery: %s",
			   e->message);
		g_error_free (e);
	}
}

static void
remote_device_disappeared (DBusGProxy *object,
			   const char *address,
			   gpointer user_data)
{
	GtkListStore *store = (GtkListStore *) user_data;
	GtkTreeIter iter;

	if (find_iter_for_address (store, address, &iter) == FALSE)
		return;
	gtk_list_store_remove (store, &iter);
}

static void
start_device_scanning (GtkListStore *store)
{
	GError *e = NULL;

	dbus_g_proxy_add_signal (object, "DiscoveryStarted", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (object, "DiscoveryStarted",
				     G_CALLBACK (discovery_started), NULL, NULL);

	dbus_g_object_register_marshaller(nst_bluetooth_marshal_VOID__STRING_UINT_INT,
					  G_TYPE_NONE, G_TYPE_STRING, G_TYPE_UINT,
					  G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_proxy_add_signal (object, "RemoteDeviceFound",
				 G_TYPE_STRING, G_TYPE_UINT, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (object, "RemoteDeviceFound",
				     G_CALLBACK (remote_device_found), store, NULL);

	dbus_g_object_register_marshaller(nst_bluetooth_marshal_VOID__STRING_STRING,
					  G_TYPE_NONE, G_TYPE_STRING,
					  G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_add_signal (object, "RemoteNameUpdated",
				 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (object, "RemoteNameUpdated",
				     G_CALLBACK (remote_name_updated), store, NULL);

	dbus_g_proxy_add_signal (object, "RemoteDeviceDisappeared",
				 G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (object, "RemoteDeviceDisappeared",
				     G_CALLBACK (remote_device_disappeared), store, NULL);

	dbus_g_proxy_add_signal (object, "DiscoveryCompleted", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (object, "DiscoveryCompleted",
				     G_CALLBACK (discovery_completed), NULL, NULL);

	dbus_g_proxy_call (object, "DiscoverDevices", &e,
			   G_TYPE_INVALID, G_TYPE_INVALID);
	if (e != NULL) {
		g_warning ("Couldn't start discovery: %s: %s",
			   dbus_g_error_get_name (e), e->message);
		g_error_free (e);
	}
}

static GtkWidget*
get_contacts_widget (NstPlugin *plugin)
{
	GtkCellRenderer *renderer;
	GtkListStore *store;

	/* The model */
	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL (store);

	/* The widget itself */
	combobox = gtk_combo_box_new_with_model (model);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox),
			renderer,
			TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), 
			renderer,
			"text", NAME_COL,
			NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
	gtk_widget_set_sensitive (combobox, FALSE);

	add_known_devices_to_list (store);
	start_device_scanning (store);

	gtk_widget_show_all (combobox);

	return combobox;
}

static gboolean
get_select_device (char **name, char **bdaddr)
{
	int option;
	GtkTreeIter iter;
	char *path, *_bdaddr, *_name;
	gboolean ret;

	g_return_val_if_fail (bdaddr != NULL, FALSE);

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
	gtk_tree_model_get (model, &iter,
			    BDADDR_COL, &_bdaddr,
			    NAME_COL, &_name,
			    -1);
	if (name)
		*name = _name;
	*bdaddr = _bdaddr;

	return ret;
}

static gboolean
send_files (NstPlugin *plugin, GtkWidget *contact_widget,
		GList *file_list)
{
	GPtrArray *argv;
	GList *list;
	gboolean ret;
	char *bdaddr;
	GError *err = NULL;

	if (get_select_device (NULL, &bdaddr) == FALSE)
		return FALSE;

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
validate_destination (NstPlugin *plugin,
		      GtkWidget *contact_widget,
		      char **error)
{
	GError *e = NULL;
	char *bdaddr, *name, **array;
	gboolean found = TRUE;

	g_return_val_if_fail (error != NULL, FALSE);

	//FIXME shouldn't error if there's no selected device

	if (get_select_device (&name, &bdaddr) == FALSE) {
		*error = g_strdup (_("Programming error, could not find the device in the list"));
		return FALSE;
	}

	dbus_g_proxy_call (object, "GetRemoteServiceClasses", &e,
			   G_TYPE_STRING, bdaddr, G_TYPE_INVALID,
			   G_TYPE_STRV, &array, G_TYPE_INVALID);
	if (e == NULL) {
		found = FALSE;
		while (*array) {
			if (g_str_equal (*array, OBEX_SERVICE_CLASS_NAME) != FALSE) {
				found = TRUE;
				break;
			}
			array++;
		}
	} else {
		g_error_free (e);
	}

	if (found == FALSE)
		*error = g_strdup_printf (_("Device does not support Obex File Transfer"));

	return found;
}

static gboolean
destroy (NstPlugin *plugin){
	//FIXME
	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"stock_bluetooth",
	"bluetooth",
	N_("Bluetooth (OBEX Push)"),
	TRUE,
	init,
	get_contacts_widget,
	validate_destination,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

