/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
 * Copyright (C) 2004 Roberto Majadas
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
 * Author:  Roberto Majadas <roberto.majadas@openshine.com>
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include "nautilus-sendto-plugin.h"

static GHashTable *contact_hash = NULL;
static gchar *blist_online = NULL;

typedef struct _ContactData {
	char *username;
	char *cname;
	char *alias;
	char *prt;
} ContactData;

static gboolean
init (NstPlugin *plugin)
{
	g_print ("Init pidgin plugin\n");

	blist_online = g_build_path ("/", g_get_home_dir(),
				     ".gnome2/nautilus-sendto/pidgin_buddies_online",
				     NULL);
	if (!g_file_test (blist_online, G_FILE_TEST_EXISTS)) {
		g_free (blist_online);
		blist_online = NULL;
		return FALSE;
	}
	return TRUE;
}

static void
add_pidgin_contacts_to_model (GtkTreeStore *store,
			      GtkTreeIter *iter,
			      GtkTreeIter *parent)
{
	GdkPixbuf *msn, *jabber, *yahoo, *aim, *icq, *bonjour;
	GdkPixbuf *icon;
	GtkIconTheme *it;
	GIOChannel *io;
	gsize terminator_pos;
	GHashTableIter hiter;
	GPtrArray *contacts_group;
	ContactData *dat;
	GValue val = {0,};

	io = g_io_channel_new_file (blist_online, "r", NULL);
	if (io == NULL)
		return;

	contact_hash = g_hash_table_new (g_str_hash, g_str_equal);

	gchar *tmp;
	g_io_channel_read_line (io, &tmp, NULL, NULL, NULL);
	g_free(tmp);

	while (1){
		dat = g_new0 (ContactData, 1);

		if (g_io_channel_read_line (io, &dat->username, NULL, &terminator_pos,
						NULL) == G_IO_STATUS_EOF)
			 break;
		dat->username[terminator_pos] = '\0';
		if (g_io_channel_read_line (io, &dat->cname, NULL, &terminator_pos,
						NULL) == G_IO_STATUS_EOF)
			 break;
		dat->cname[terminator_pos] = '\0';
		if (g_io_channel_read_line (io, &dat->alias, NULL, &terminator_pos,
						NULL) == G_IO_STATUS_EOF)
			break;
		dat->alias[terminator_pos] = '\0';
		if (g_io_channel_read_line (io, &dat->prt, NULL, &terminator_pos,
						NULL) == G_IO_STATUS_EOF)
			break;
		dat->prt[terminator_pos] = '\0';

		contacts_group = g_hash_table_lookup (contact_hash, dat->alias);
		if (contacts_group == NULL){
			GPtrArray *new_group = g_ptr_array_new ();
			g_ptr_array_add (new_group, dat);
			g_hash_table_insert (contact_hash, dat->alias, new_group);
		} else {
			g_ptr_array_add (contacts_group, dat);
		}
	}

	g_io_channel_shutdown (io, TRUE, NULL);

	it = gtk_icon_theme_get_default ();
	msn = gtk_icon_theme_load_icon (it, "im-msn", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	jabber = gtk_icon_theme_load_icon (it, "im-jabber", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	yahoo = gtk_icon_theme_load_icon (it, "im-yahoo", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	aim = gtk_icon_theme_load_icon (it, "im-aim", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	icq = gtk_icon_theme_load_icon (it, "im-icq", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	bonjour = gtk_icon_theme_load_icon (it, "network-wired", 16,
					    GTK_ICON_LOOKUP_USE_BUILTIN, NULL);

	g_hash_table_iter_init (&hiter, contact_hash);
	while (g_hash_table_iter_next (&hiter, NULL, (gpointer)&contacts_group)) {
		GString *alias_e;
		gint accounts;

		dat = g_ptr_array_index (contacts_group, 0);

		alias_e = g_string_new (dat->alias);
		if (alias_e->len > 30){
			alias_e = g_string_truncate (alias_e, 30);
			alias_e = g_string_append (alias_e, "...");
		}

		accounts = contacts_group->len;

		gtk_tree_store_append (store, parent, NULL);
		gtk_tree_store_set (store, parent, 0, NULL, 1, alias_e->str, -1);

		gint i;
		for (i = 0; i < accounts; ++i) {
			dat = g_ptr_array_index (contacts_group, i);

			if (strcmp(dat->prt, "prpl-msn")==0)
				icon = msn;
			else if (strcmp(dat->prt,"prpl-jabber")==0)
				icon = jabber;
			else if (strcmp(dat->prt,"prpl-aim")==0)
				icon = aim;
			else if (strcmp(dat->prt,"prpl-yahoo")==0)
				icon = yahoo;
			else if (strcmp(dat->prt, "prpl-icq")==0)
				icon = icq;
			else if (strcmp(dat->prt, "prpl-bonjour")==0)
				icon = bonjour;
			else
				icon = NULL;

			if (accounts == 1) {
				g_value_init(&val, GDK_TYPE_PIXBUF);
				g_value_set_object (&val, (gpointer)icon);
				gtk_tree_store_set_value (store, parent, 0, &val);
				g_value_unset (&val);
				break;
			}
			gtk_tree_store_append (store, iter, parent);
			gtk_tree_store_set (store, iter, 0, icon, 1,
					    alias_e->str, -1);
		}
		g_string_free(alias_e, TRUE);
	}
}

static void
customize (GtkCellLayout *cell_layout,
	   GtkCellRenderer *cell,
	   GtkTreeModel *tree_model,
	   GtkTreeIter *iter,
	   gpointer text)
{
	gboolean has_child;
	has_child = gtk_tree_model_iter_has_child (tree_model, iter);
	if (text) {
		if (has_child)
			g_object_set (G_OBJECT(cell), "xpad", 18, NULL);
		else
			g_object_set (G_OBJECT(cell), "xpad", 2, NULL);
	}
	g_object_set (G_OBJECT(cell), "sensitive", !has_child, NULL);
}

static GtkWidget *
get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *cb;
	GtkCellRenderer *renderer;
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeIter *iter, *iter2;

	iter = g_malloc (sizeof(GtkTreeIter));
	iter2 = g_malloc (sizeof(GtkTreeIter));
	store = gtk_tree_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	add_pidgin_contacts_to_model (store, iter, iter2);
	model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), 1,
						GTK_SORT_ASCENDING);
	cb = gtk_combo_box_new_with_model (model);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), 
					renderer,
					"pixbuf", 0,
					NULL); 
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb), renderer,
					    customize,
					    (gboolean *)FALSE, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), 
					renderer,
					"text", 1,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb), renderer,
					    customize,
					    (gboolean *)TRUE, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX(cb), iter);
	if (gtk_tree_model_iter_has_child (model, iter)) {
		GtkTreePath *path = gtk_tree_path_new_from_indices (0, 0, -1);
		gtk_tree_model_get_iter (model, iter2, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (cb), iter2);
	}

	g_free (iter);
	g_free (iter2);
	return cb;
}

static gboolean
send_files (NstPlugin *plugin,
	    GtkWidget *contact_widget,
	    GList *file_list)
{
	GString *pidginto;
	GList *l;
	gchar *spool_file, *spool_file_send, *contact_info;
	FILE *fd;
	gint t, depth;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint *indices;
	const gchar *alias;
	GPtrArray *contacts_group;
	ContactData *dat;
	GValue val = {0,};

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (contact_widget), &iter);
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (
					gtk_combo_box_get_model (GTK_COMBO_BOX(
					contact_widget))), &iter);
	depth = gtk_tree_path_get_depth(path);
	indices = gtk_tree_path_get_indices(path);
	gtk_tree_path_free (path);
	gtk_tree_model_get_value (GTK_TREE_MODEL (gtk_combo_box_get_model (
					GTK_COMBO_BOX(contact_widget))), 
					&iter, 1, &val);
	alias = g_value_get_string (&val);
	contacts_group = g_hash_table_lookup (contact_hash, alias);
	g_value_unset (&val);
	dat = g_ptr_array_index (contacts_group, (depth == 2)?indices[1]:0);
	contact_info = g_strdup_printf ("%s\n%s\n%s\n",	dat->username, 
					dat->cname, dat->prt);
	pidginto = g_string_new (contact_info);
	g_free (contact_info);

	for (l = file_list ; l; l=l->next) {
		char *path;

		path = g_filename_from_uri (l->data, NULL, NULL);
		g_string_append_printf (pidginto,"%s\n", path);
		g_free (path);
	}
	g_string_append_printf (pidginto,"\n");
	t = time (NULL);
	spool_file = g_strdup_printf ("%s/.gnome2/nautilus-sendto/spool/tmp/%i.send",
				      g_get_home_dir(), t);
	spool_file_send = g_strdup_printf ("%s/.gnome2/nautilus-sendto/spool/%i.send",
					   g_get_home_dir(), t);
	fd = fopen (spool_file,"w");
	fwrite (pidginto->str, 1, pidginto->len, fd);
	fclose (fd);
	rename (spool_file, spool_file_send);
	g_free (spool_file);
	g_free (spool_file_send);
	g_string_free (pidginto, TRUE);
	return TRUE;
}

static void
free_contact (ContactData *dat)
{
	g_free(dat->username);
	g_free(dat->cname);
	g_free(dat->alias);
	g_free(dat->prt);
	g_free(dat);
}

static gboolean
destroy (NstPlugin *plugin)
{
	GHashTableIter iter;
	GPtrArray *contacts_group;
	ContactData *dat;

	g_free (blist_online);

	g_hash_table_iter_init (&iter, contact_hash);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer)&contacts_group)) {
		gint accounts;
		accounts = contacts_group->len;

		gint i;
		for (i = 0; i < accounts; ++i) {
			dat = g_ptr_array_index (contacts_group, i);
			free_contact (dat);
		}
		g_ptr_array_free (contacts_group, TRUE);
	}
	g_hash_table_destroy (contact_hash);
	return TRUE;
}

static 
NstPluginInfo plugin_info = {
	"im",
	"pidgin",
	N_("Instant Message (Pidgin)"),
	FALSE,
	FALSE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

