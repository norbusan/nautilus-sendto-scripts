/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2008 Zeeshan Ali (Khattak)
 * Copyright (C) 2006 Peter Enseleit
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
 * Author:  Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *          Peter Enseleit <penseleit@gmail.com>
 *          Roberto Majadas <telemaco@openshine.com>
 *
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <libgupnp-av/gupnp-av.h>
#include "nautilus-sendto-plugin.h"

#define MEDIA_SERVER "urn:schemas-upnp-org:device:MediaServer:*"
#define CDS "urn:schemas-upnp-org:service:ContentDirectory"

enum {
	UDN_COL,
	NAME_COL,
	NUM_COLS
};

static GtkWidget *combobox;
static GtkTreeModel *model;
static GUPnPContext *context;
static GUPnPControlPoint *cp;

static gboolean
find_device (const gchar *udn,
	     GtkTreeIter *iter)
{
	gboolean found = FALSE;

	if (!gtk_tree_model_get_iter_first (model, iter))
		return FALSE;

	do {
		gchar *tmp;

		gtk_tree_model_get (model,
				    iter,
				    UDN_COL, &tmp,
				    -1);

		if (tmp != NULL && strcmp (tmp, udn) == 0)
			found = TRUE;

		g_free (tmp);
	} while (!found && gtk_tree_model_iter_next (model, iter));

	return found;
}

static gboolean
check_required_actions (GUPnPServiceIntrospection *introspection)
{
	if (gupnp_service_introspection_get_action (introspection,
						    "CreateObject") == NULL)
		return FALSE;
	if (gupnp_service_introspection_get_action (introspection,
						    "ImportResource") == NULL)
		return FALSE;
	return TRUE;
}

static void
get_introspection_cb (GUPnPServiceInfo *service_info,
		      GUPnPServiceIntrospection *introspection, const GError *error,
		      gpointer user_data)
{
	GUPnPDeviceInfo *device_info;
	gchar *name;
	const gchar *udn;
	GtkTreeIter iter;

	device_info = GUPNP_DEVICE_INFO (user_data);

	if (introspection != NULL) {
		/* If introspection is available, make sure required actions
		 * are implemented.
		 */
		if (!check_required_actions (introspection))
			goto error;
	}

	udn = gupnp_device_info_get_udn (device_info);
	if (G_UNLIKELY (udn == NULL))
		goto error;

	/* First check if the device is already added */
	if (find_device (udn, &iter))
		goto error;

	name = gupnp_device_info_get_friendly_name (device_info);
	if (name == NULL)
		name = g_strdup (udn);

	gtk_list_store_insert_with_values (GTK_LIST_STORE (model), NULL, -1,
					   UDN_COL, udn,
					   NAME_COL, name,
					   -1);

	g_free (name);

error:
	/* We don't need the proxy objects anymore */
	g_object_unref (service_info);
	g_object_ref (device_info);
}

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
			   GUPnPDeviceProxy  *proxy)
{
	GUPnPServiceInfo *info;
	const gchar *type;

	type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));
	if (!g_pattern_match_simple (MEDIA_SERVER, type)) {
		/* We are only interested in MediaServer */
		return;
	}

	info = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (proxy), CDS);
	if (G_UNLIKELY (info == NULL)) {
		/* No ContentDirectory implemented? Not interesting. */
		return;
	}

	gupnp_service_info_get_introspection_async (info,
						    get_introspection_cb,
						    g_object_ref (proxy));
}

static void
device_proxy_unavailable_cb (GUPnPControlPoint *cp,
			     GUPnPDeviceProxy  *proxy)
{
	GtkTreeIter iter;
	const gchar *udn;

	udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));
	if (udn == NULL)
		return;

	/* First check if the device is already added */
	if (find_device (udn, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

static gboolean
init (NstPlugin *plugin)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GError *error;
	char *upload_cmd;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	upload_cmd = g_find_program_in_path ("gupnp-upload");
	if (upload_cmd == NULL)
		return FALSE;
	g_free (upload_cmd);

	error = NULL;
	context = gupnp_context_new (NULL, NULL, 0, &error);
	if (error != NULL) {
		g_error (error->message);
		g_error_free (error);

		return FALSE;
	}

	cp = gupnp_control_point_new (context, "ssdp:all");

	g_signal_connect (cp,
			  "device-proxy-available",
			  G_CALLBACK (device_proxy_available_cb),
			  NULL);
	g_signal_connect (cp,
			  "device-proxy-unavailable",
			  G_CALLBACK (device_proxy_unavailable_cb),
			  NULL);

	gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

	combobox = gtk_combo_box_new ();

	store = gtk_list_store_new (NUM_COLS,
				    G_TYPE_STRING,   /* UDN  */
				    G_TYPE_STRING);  /* Name */
	model = GTK_TREE_MODEL (store);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combobox), model);

	renderer = gtk_cell_renderer_text_new ();

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combobox),
				       renderer,
				       "text", NAME_COL);

	return TRUE;
}

static GtkWidget*
get_contacts_widget (NstPlugin *plugin)
{
	return combobox;
}

static gboolean
send_files (NstPlugin *plugin,
	    GtkWidget *contact_widget,
	    GList *file_list)
{
	gchar *upload_cmd, *udn;
	GPtrArray *argv;
	gboolean ret;
	GList *l;
	GtkTreeIter iter;
	GError *err = NULL;

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, UDN_COL, &udn, -1);

	upload_cmd = g_find_program_in_path ("gupnp-upload");
	if (upload_cmd == NULL)
		return FALSE;

	argv = g_ptr_array_new ();
	g_ptr_array_add (argv, upload_cmd);
	g_ptr_array_add (argv, "-t");
	g_ptr_array_add (argv, "15"); /* discovery timeout (seconds) */
	g_ptr_array_add (argv, udn);
	for (l = file_list ; l; l=l->next) {
		gchar *file_path;

		file_path = g_filename_from_uri (l->data, NULL, NULL);
		g_ptr_array_add (argv, file_path);
	}
	g_ptr_array_add (argv, NULL);

	ret = g_spawn_async (NULL, (gchar **) argv->pdata,
			     NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);

	if (ret == FALSE) {
		g_warning ("Could not send files to MediaServer: %s",
			   err->message);
		g_error_free (err);
	}

	g_ptr_array_free (argv, TRUE);
	g_free (upload_cmd);
	g_free (udn);

	return ret;
}

static gboolean
destroy (NstPlugin *plugin)
{
	gtk_widget_destroy (combobox);
	g_object_unref (model);

	g_object_unref (cp);
	g_object_unref (context);

	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"folder-remote",
	"upnp",
	N_("UPnP Media Server"),
	FALSE,
	FALSE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

