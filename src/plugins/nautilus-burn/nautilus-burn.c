/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
 * Copyright (C) 2008 Jader Henrique da Silva
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
 * Author:  Jader Henrique da Silva <vovozito@gmail.com>
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include "nst-common.h"
#include "nautilus-sendto-plugin.h"

#define COMBOBOX_OPTION_EXISTING_DVD 0
#define COMBOBOX_OPTION_NEW_DVD 1

static GFile *burn = NULL;

static
gboolean init (NstPlugin *plugin)
{
	g_print ("Init nautilus burn plugin\n");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	burn = g_file_new_for_uri ("burn:/");

	return TRUE;
}

static
GtkWidget* get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *widget;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeModel *model;
	GdkPixbuf *icon;
	GtkIconTheme *it;
	GFileEnumerator *fenum;
	GFileInfo *file_info = NULL;


	fenum = g_file_enumerate_children (burn,
					   G_FILE_ATTRIBUTE_STANDARD_NAME,
					   G_FILE_QUERY_INFO_NONE,
					   NULL,
					   NULL);

	if (fenum != NULL) {
		file_info = g_file_enumerator_next_file (fenum, NULL, NULL);
		g_object_unref (fenum);
	}

	if (file_info == NULL) {
		widget = gtk_label_new(_("New CD/DVD"));
	} else {
		GtkTreeIter iter;

		it = gtk_icon_theme_get_default ();
		icon = gtk_icon_theme_load_icon (it,
						 "nautilus-cd-burner",
						 16,
						 GTK_ICON_LOOKUP_USE_BUILTIN,
						 NULL);

		store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, icon, 1, _("Existing CD/DVD"), -1);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, icon, 1, _("New CD/DVD"), -1);

		model = GTK_TREE_MODEL (store);
		widget = gtk_combo_box_new_with_model (model);
		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget),
					    renderer,
					    FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), 
						renderer,
						"pixbuf", 0,
						NULL);		
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget),
					    renderer,
					    TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), 
						renderer,
						"text", 1,
						NULL);

		gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
		g_object_unref (file_info);
	}

	return widget;
}

static
gboolean send_files (NstPlugin *plugin,
		     GtkWidget *burntype_widget,
		     GList *file_list)
{
	GFileEnumerator *fenum;
	GFileInfo *file_info;
	GFile *child;

	if (GTK_IS_COMBO_BOX (burntype_widget) &&
	    gtk_combo_box_get_active (GTK_COMBO_BOX (burntype_widget)) == COMBOBOX_OPTION_NEW_DVD) {
		fenum = g_file_enumerate_children (burn,
						   G_FILE_ATTRIBUTE_STANDARD_NAME,
						   G_FILE_QUERY_INFO_NONE,
						   NULL,
						   NULL);

		if (fenum != NULL) {
			while ((file_info = g_file_enumerator_next_file (fenum, NULL, NULL)) != NULL) {
				child = g_file_get_child (burn,
							  g_file_info_get_name(file_info));

				g_object_unref (file_info);
				g_file_delete (child, NULL, NULL);
				g_object_unref (child);
			}
			g_object_unref (fenum);
		}
	}

	copy_files_to (file_list, burn);

	gtk_show_uri (NULL, "burn:///", GDK_CURRENT_TIME, NULL);

	return TRUE;
}

static 
gboolean destroy (NstPlugin *plugin){

	g_object_unref (burn);
	return TRUE;

}

static 
NstPluginInfo plugin_info = {
	"nautilus-cd-burner",
	"nautilus-burn",
	N_("CD/DVD Creator"),
	FALSE,
	TRUE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

