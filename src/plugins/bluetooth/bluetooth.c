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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Author:  Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#include <bluetooth-marshal.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n-lib.h>
#include <gconf/gconf-client.h>

#include "nautilus-sendto-plugin.h"

#define OBEX_PUSH_SVCLASS_ID_STR "0x1105"
#define OBEX_FILETRANS_SVCLASS_ID_STR "0x1106"
#define LAST_OBEX_DEVICE "/desktop/gnome/nautilus-sendto/last_obex_device"

static GtkTreeModel *model;
static int discovered;
static GtkWidget *combobox;
static char *cmd = NULL;

DBusGConnection *conn;
DBusGProxy *object;

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
	DBusGProxy *manager;
	const char *adapter;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	/* Check whether bluetooth-sendto or gnome-obex-send are available */
	cmd = g_find_program_in_path ("bluetooth-sendto");
	if (cmd == NULL) {
		cmd = g_find_program_in_path ("gnome-obex-send");
		if (cmd == NULL)
			return FALSE;
	}

	conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &e);
	if (e != NULL) {
		g_warning ("Couldn't connect to bus: %s",
			   e->message);
		g_error_free (e);
		return FALSE;
	}

	manager = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    "/", "org.bluez.Manager");
	if (dbus_g_proxy_call (manager, "DefaultAdapter", &e,
			   G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &adapter, G_TYPE_INVALID) == FALSE) {
		g_object_unref (manager);
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
		} else {
			g_warning ("Couldn't get default bluetooth adapter: No error given");
		}
		return FALSE;
	}

	g_object_unref (manager);
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

static char *
get_device_name_from_address (const char *bdaddr)
{
	const char *device_path;
	DBusGProxy *device;
	GHashTable *props;

	if (dbus_g_proxy_call (object, "FindDevice", NULL,
			       G_TYPE_STRING, bdaddr, G_TYPE_INVALID,
			       DBUS_TYPE_G_OBJECT_PATH, &device_path, G_TYPE_INVALID) == FALSE) {
		return g_strdup (bdaddr);
	}

	device = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    device_path, "org.bluez.Device");

	if (dbus_g_proxy_call (device, "GetProperties", NULL,
						   G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
						   &props, G_TYPE_INVALID) != FALSE) {

		GValue *value;
		char *name;

		value = g_hash_table_lookup (props, "Alias");
		name = value ? g_value_dup_string (value) : g_strdup (bdaddr);

		g_hash_table_destroy (props);

		return name;
	} else {
		return g_strdup (bdaddr);
	}
}

static void
add_phone_to_list (GtkListStore *store,
		   const char *name,
		   const char *bdaddr,
		   const char *icon)
{
	GtkTreeIter iter;
	gboolean found = FALSE;

	found = find_iter_for_address (store, bdaddr, &iter);
	if (found == FALSE) {
		gtk_list_store_append (store, &iter);
	}

	gtk_list_store_set (store, &iter,
			    ICON_COL, icon,
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
add_device_to_list (GtkListStore *store, const char *device_path)
{
	DBusGProxy *device;
	GHashTable *props;

	device = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    device_path, "org.bluez.Device");
	if (dbus_g_proxy_call (device, "GetProperties", NULL,
			       G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       &props, G_TYPE_INVALID) != FALSE) {
		GValue *value;
		const char *name, *address, *icon;

		value = g_hash_table_lookup (props, "Address");
		address = g_value_get_string (value);
		value = g_hash_table_lookup (props, "Alias");
		name = g_value_get_string (value);
		value = g_hash_table_lookup (props, "Icon");
		icon = value ? g_value_get_string (value) : NULL;

		//FIXME double check the obexftp support?
		add_phone_to_list (store, name, address, icon);
	}
	g_object_unref (device);
}

static void
add_last_used_device_to_list (GtkListStore *store)
{
	char *bdaddr, *name;
	GConfClient *gconfclient;

	gconfclient = gconf_client_get_default ();
	bdaddr = gconf_client_get_string (gconfclient, LAST_OBEX_DEVICE, NULL);
	g_object_unref (gconfclient);

	if (bdaddr != NULL && *bdaddr != '\0') {
		name = get_device_name_from_address (bdaddr);
		add_phone_to_list (store, name, bdaddr, NULL);
		g_free (name);
	}

	g_free (bdaddr);
}

static void
add_known_devices_to_list (GtkListStore *store)
{
	GError *e = NULL;
	GPtrArray *array;

	if (dbus_g_proxy_call (object, "ListDevices", &e,
			       G_TYPE_INVALID, dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &array, G_TYPE_INVALID) != FALSE) {
		guint i;
		for (i = 0; i < array->len ; i++)
			add_device_to_list (store, g_ptr_array_index (array, i));
		g_ptr_array_free (array, TRUE);
	}
}

static void
device_found (DBusGProxy *object,
	      const char *address, GHashTable *props,
	      GtkListStore *store)
{
	GValue *value;
	const char *name, *icon;

	value = g_hash_table_lookup (props, "Alias");
	name = value ? g_value_get_string (value) : NULL;
	value = g_hash_table_lookup (props, "Icon");
	icon = value ? g_value_get_string (value) : NULL;

	add_phone_to_list (store, name, address, icon);
}

static void
start_device_scanning (GtkListStore *store)
{
	GError *e = NULL;

	dbus_g_object_register_marshaller (nst_bluetooth_marshal_VOID__STRING_BOXED,
					   G_TYPE_NONE, G_TYPE_STRING,
					   G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_add_signal (object, "DeviceFound",
				 G_TYPE_STRING, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (object, "DeviceFound",
				     G_CALLBACK (device_found), store, NULL);

	dbus_g_proxy_call (object, "StartDiscovery", &e,
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
	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL (store);

	/* The widget itself */
	combobox = gtk_combo_box_new_with_model (model);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), 
					renderer,
					"icon-name", ICON_COL,
					NULL);
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

	add_last_used_device_to_list (store);
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

static void
save_last_used_obex_device (const char *bdaddr)
{
	GConfClient *client;

	client = gconf_client_get_default ();
	gconf_client_set_string (client,
				 LAST_OBEX_DEVICE,
				 bdaddr,
				 NULL);

	g_object_unref (client);
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
	g_ptr_array_add (argv, cmd);
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
	} else {
		save_last_used_obex_device (bdaddr);
	}
	return ret;
}

static gboolean
validate_destination (NstPlugin *plugin,
		      GtkWidget *contact_widget,
		      char **error)
{
	GError *e = NULL;
	char *bdaddr, *device_path;
	DBusGProxy *device;
	GHashTable *props;
	GValue *value;
	gboolean found = FALSE;
	char **array;
	gboolean first_time = TRUE;

	g_return_val_if_fail (error != NULL, FALSE);

	if (get_select_device (NULL, &bdaddr) == FALSE) {
		*error = g_strdup (_("Programming error, could not find the device in the list"));
		return FALSE;
	}

	if (dbus_g_proxy_call (object, "FindDevice", NULL,
			       G_TYPE_STRING, bdaddr, G_TYPE_INVALID,
			       DBUS_TYPE_G_OBJECT_PATH, &device_path, G_TYPE_INVALID) == FALSE) {
		g_free (bdaddr);
		return TRUE;
	}

	device = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    device_path, "org.bluez.Device");

again:
	if (dbus_g_proxy_call (device, "GetProperties", NULL,
			       G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       &props, G_TYPE_INVALID) == FALSE) {
		goto bail;
	}

	value = g_hash_table_lookup (props, "UUIDs");
	array = g_value_get_boxed (value);
	if (array != NULL) {
		char *uuid;
		guint i;

		for (i = 0; array[i] != NULL; i++) {
			if (g_str_has_suffix (array[i], "-0000-1000-8000-00805f9b34fb") != FALSE) {
				if (g_str_has_prefix (array[i], "0000") != FALSE) {
					char *tmp;
					tmp = g_strndup (array[i] + 4, 4);
					uuid = g_strdup_printf ("0x%s", tmp);
					g_free (tmp);
				} else {
					char *tmp;
					tmp = g_strndup (array[i], 8);
					uuid = g_strdup_printf ("0x%s", tmp);
				}
			} else {
				uuid = g_strdup (array[i]);
			}

			if (strcmp (uuid, OBEX_FILETRANS_SVCLASS_ID_STR) == 0 ||
			    strcmp (uuid, OBEX_PUSH_SVCLASS_ID_STR) == 0      ){
				found = TRUE;
				g_free (uuid);
				break;
			}

			g_free (uuid);
		}
	} else {
		/* No array, can't really check now, can we */
		found = TRUE;
	}

	g_hash_table_destroy (props);
	if (found == TRUE || first_time == FALSE)
		goto bail;

	first_time = FALSE;

	/* If no valid service found the first time around, then request services refresh */
	if (! dbus_g_proxy_call (device, "DiscoverServices", &e, G_TYPE_STRING, NULL,
				 G_TYPE_INVALID, dbus_g_type_get_map("GHashTable", G_TYPE_UINT, G_TYPE_STRING),
				 &props, G_TYPE_INVALID)) {
		goto bail;
	}
	goto again;

bail:
	g_object_unref (device);

	if (found == FALSE)
		*error = g_strdup_printf (_("Obex Push file transfer unsupported"));

	return found;
}

static gboolean
destroy (NstPlugin *plugin)
{
	if (object != NULL) {
		dbus_g_proxy_call (object, "StopDiscovery", NULL,
				   G_TYPE_INVALID, G_TYPE_INVALID);
		g_object_unref (object);
	}
	g_object_unref (conn);
	g_object_unref (model);
	gtk_widget_destroy (combobox);
	g_free (cmd);
	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"bluetooth",
	"bluetooth",
	N_("Bluetooth (OBEX Push)"),
	NULL,
	NAUTILUS_CAPS_NONE,
	init,
	get_contacts_widget,
	validate_destination,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

