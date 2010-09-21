/*
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
 * Authors:  Maxim Ermilov <ermilov.maxim@gmail.com>
 *           Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include "nst-common.h"
#include "nautilus-sendto-plugin.h"

#define REMOVABLE_TYPE_DEVICES_PLUGIN         (removable_devices_plugin_get_type ())
#define REMOVABLE_DEVICES_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REMOVABLE_TYPE_DEVICES_PLUGIN, RemovableDevicesPlugin))
#define REMOVABLE_DEVICES_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REMOVABLE_TYPE_DEVICES_PLUGIN, RemovableDevicesPlugin))
#define REMOVABLE_IS_DEVICES_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REMOVABLE_TYPE_DEVICES_PLUGIN))
#define REMOVABLE_IS_DEVICES_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REMOVABLE_TYPE_DEVICES_PLUGIN))
#define REMOVABLE_DEVICES_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REMOVABLE_TYPE_DEVICES_PLUGIN, RemovableDevicesPluginClass))

typedef struct _RemovableDevicesPlugin       RemovableDevicesPlugin;
typedef struct _RemovableDevicesPluginClass  RemovableDevicesPluginClass;

struct _RemovableDevicesPlugin {
	PeasExtensionBase parent_instance;

	GVolumeMonitor *vol_monitor;
	GtkWidget *cb;
};

struct _RemovableDevicesPluginClass {
	PeasExtensionBaseClass parent_class;
};

NAUTILUS_PLUGIN_REGISTER(REMOVABLE_TYPE_DEVICES_PLUGIN, RemovableDevicesPlugin, removable_devices_plugin)

enum {
	NAME_COL,
	ICON_COL,
	MOUNT_COL,
	NUM_COLS,
};

static void
cb_mount_removed (GVolumeMonitor         *volume_monitor,
		  GMount                 *mount,
		  RemovableDevicesPlugin *p)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean b, found;

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (p->cb)));
	b = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
	found = FALSE;

	while (b) {
		GMount *m;
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &m, -1);
		if (m == mount) {
			gtk_list_store_remove (store, &iter);
			g_object_unref (m);
			found = TRUE;
			break;
		}
		g_object_unref (m);
		b = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
	}

	/* If a mount was removed */
	if (found != FALSE) {
		/* And it was the selected one */
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (p->cb)) == -1) {
			/* Select the first item in the list */
			gtk_combo_box_set_active (GTK_COMBO_BOX (p->cb), 0);
		}
	}
}

static void
cb_mount_changed (GVolumeMonitor         *volume_monitor,
		  GMount                 *mount,
		  RemovableDevicesPlugin *p)
{
	GtkTreeIter iter;
	gboolean b;
	GtkListStore *store;

	if (g_mount_is_shadowed (mount) != FALSE) {
		cb_mount_removed (volume_monitor, mount, p);
		return;
	}

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (p->cb)));
	b = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

	while (b) {
		GMount *m;
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &m, -1);

		if (m == mount) {
			char *name;

			name = g_mount_get_name (mount);
			gtk_list_store_set (store, &iter,
					    NAME_COL, name,
					    ICON_COL, g_mount_get_icon (mount),
					    -1);
			g_free (name);
			g_object_unref (m);
			break;
		}
		g_object_unref (m);
		b = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
	}
}

static void
cb_mount_added (GVolumeMonitor         *volume_monitor,
		GMount                 *mount,
		RemovableDevicesPlugin *p)
{
	char *name;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean select_added;

	if (g_mount_is_shadowed (mount) != FALSE)
		return;

	name = g_mount_get_name (mount);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (p->cb));

	select_added = gtk_tree_model_iter_n_children (model, NULL) == 0;

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    NAME_COL, name,
			    ICON_COL, g_mount_get_icon (mount),
			    MOUNT_COL, mount,
			    -1);

	g_free (name);

	if (select_added != FALSE)
		gtk_combo_box_set_active (GTK_COMBO_BOX (p->cb), 0);

}

static void
removable_devices_plugin_send_files (NautilusSendtoPlugin *plugin,
				     const char           *id,
				     GList                *file_list,
				     GAsyncReadyCallback   callback,
				     gpointer              user_data)
{
	RemovableDevicesPlugin *p = REMOVABLE_DEVICES_PLUGIN (plugin);
	GtkListStore *store;
	GtkTreeIter iter;
	GMount *dest_mount;
	GFile *mount_root;
	GSimpleAsyncResult *simple;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (p->cb), &iter) == FALSE) {
		/* FIXME: This should not happen */
		g_assert_not_reached ();
		return;
	}

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (p->cb)));
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &dest_mount, -1);
	mount_root = g_mount_get_root (dest_mount);

	simple = g_simple_async_result_new (G_OBJECT (plugin),
					    callback,
					    user_data,
					    nautilus_sendto_plugin_send_files);

	copy_files_to (file_list, mount_root);

	g_object_unref (mount_root);

	/* FIXME: Report errors properly */
	g_simple_async_result_set_op_res_gpointer (simple,
						   GINT_TO_POINTER (NST_SEND_STATUS_SUCCESS_DONE),
						   NULL);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);
}

static GtkWidget *
removable_devices_plugin_get_widget (NautilusSendtoPlugin *plugin,
				     GList                *file_list)
{
	RemovableDevicesPlugin *p = REMOVABLE_DEVICES_PLUGIN (plugin);
	GtkListStore *store;
	GList *l, *mounts;
	GtkTreeIter iter;
	GtkWidget *box;
	GtkCellRenderer *text_renderer, *icon_renderer;

	mounts = g_volume_monitor_get_mounts (p->vol_monitor);

	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_ICON, G_TYPE_OBJECT);

	for (l = mounts; l != NULL; l = l->next) {
		char *name;

		if (g_mount_is_shadowed (l->data) != FALSE) {
			g_object_unref (l->data);
			continue;
		}

		name = g_mount_get_name (l->data);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    NAME_COL, name,
				    ICON_COL, g_mount_get_icon (l->data),
				    MOUNT_COL, l->data,
				    -1);
		g_free (name);

		g_object_unref (l->data);
	}
	g_list_free (mounts);

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (p->cb));
	gtk_combo_box_set_model (GTK_COMBO_BOX (p->cb), GTK_TREE_MODEL (store));

	text_renderer = gtk_cell_renderer_text_new ();
	icon_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (p->cb), icon_renderer, FALSE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (p->cb), text_renderer, TRUE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (p->cb), text_renderer, "text", 0,  NULL);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (p->cb), icon_renderer, "gicon", 1,  NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (p->cb), 0);

	g_signal_connect (G_OBJECT (p->vol_monitor), "mount-removed", G_CALLBACK (cb_mount_removed), plugin);
	g_signal_connect (G_OBJECT (p->vol_monitor), "mount-added", G_CALLBACK (cb_mount_added), plugin);
	g_signal_connect (G_OBJECT (p->vol_monitor), "mount-changed", G_CALLBACK (cb_mount_changed), plugin);

	box = gtk_vbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (box), p->cb, TRUE, FALSE, 0);
	gtk_widget_show_all (box);

	return box;
}

static void
removable_devices_plugin_create_widgets (NautilusSendtoPlugin *plugin,
					 GList                *file_list,
					 const char          **mime_types)
{
	/* All the mime-types are supported */
	g_signal_emit_by_name (G_OBJECT (plugin),
			       "add-widget",
			       "removable-devices",
			       _("Removable disks and shares"),
			       "folder-remote",
			       removable_devices_plugin_get_widget (plugin, file_list));
}

static void
removable_devices_plugin_init (RemovableDevicesPlugin *p)
{
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	p->vol_monitor = g_volume_monitor_get ();
	p->cb = gtk_combo_box_new ();
}

static void
removable_devices_plugin_finalize (GObject *object)
{
	RemovableDevicesPlugin *p = REMOVABLE_DEVICES_PLUGIN (object);

	if (p->cb != NULL) {
		gtk_widget_destroy (p->cb);
		p->cb = NULL;
	}

	if (p->vol_monitor != NULL) {
		g_object_unref (p->vol_monitor);
		p->vol_monitor = NULL;
	}

	G_OBJECT_CLASS (removable_devices_plugin_parent_class)->finalize (object);
}

