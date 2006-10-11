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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:  Roberto Majadas <roberto.majadas@openshine.com>
 */

#include "../nautilus-sendto-plugin.h"

static GList *contact_list;
static gchar *blist_online;

static 
gboolean init (NstPlugin *plugin)
{
	g_print ("Init gaim plugin\n");
	contact_list = NULL;

	blist_online = g_build_path ("/", g_get_home_dir(),
				     ".gnome2/nautilus-sendto/buddies_online",
				     NULL);
	if (!g_file_test (blist_online, G_FILE_TEST_EXISTS))
		return FALSE;	
	return TRUE;
}

static void
add_gaim_contacts_to_model (GtkListStore *store, GtkTreeIter *iter)
{
	GdkPixbuf *msn, *jabber, *yahoo, *aim;
	GtkIconTheme *it;
	GList *list = NULL;
	GList *l;
	GString *im_str;
	gchar *contact_info;
	GIOChannel *io;	
	gint i, list_len;
		
	io = g_io_channel_new_file (blist_online, "r", NULL);

	if (io != NULL){
		GString *str;
		str = g_string_new ("");
		g_io_channel_read_line_string (io, str, NULL, NULL);
		g_string_free (str, TRUE);
		str = g_string_new ("");
		while (G_IO_STATUS_EOF != g_io_channel_read_line_string (io, str, 
								  NULL, NULL))
		{
			str = g_string_truncate (str, str->len - 1);
			list = g_list_prepend (list, str->str);
			g_string_free (str, FALSE);
			str = g_string_new ("");
		}
		g_string_free(str,TRUE);
		g_io_channel_shutdown (io, TRUE, NULL);		
		if (list != NULL)
			list = g_list_reverse(list);
		else
			return;
		
	}else
		return;
	
	it = gtk_icon_theme_get_default ();
	msn = gtk_icon_theme_load_icon (it, "im-msn", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	jabber = gtk_icon_theme_load_icon (it, "im-jabber", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	yahoo = gtk_icon_theme_load_icon (it, "im-yahoo", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	aim = gtk_icon_theme_load_icon (it, "im-aim", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
		
	l = list;
	while (l->next != NULL){
		gchar *prt, *username, *cname, *alias;
		GString *alias_e;
		
		username = (gchar *) l->data; l = l->next;
		cname = (gchar *) l->data; l = l->next;
		alias = (gchar *) l->data; l = l->next;
		prt = (gchar *) l->data; l = l->next;
		
		alias_e = g_string_new (alias);
		if (alias_e->len > 30){
			alias_e = g_string_truncate (alias_e, 30);
			alias_e = g_string_append (alias_e, "...");
		}
		
		contact_info = g_strdup_printf ("%s\n%s\n%s\n",
					      username, cname, prt);
		if (strcmp(prt ,"prpl-msn")==0){
			gtk_list_store_append (store, iter);
			gtk_list_store_set (store, iter, 0, 
					    msn, 1, alias_e->str, -1);
			contact_list = g_list_append (contact_list, contact_info);
		}else
		  if (strcmp(prt,"prpl-jabber")==0){
			  gtk_list_store_append (store, iter);
			  gtk_list_store_set (store, iter, 0, 
					      jabber, 1, alias_e->str, -1);
			  contact_list = g_list_append (contact_list, contact_info);
		  }else
		    if (strcmp(prt,"prpl-oscar")==0){
			    gtk_list_store_append (store, iter);
			    gtk_list_store_set (store, iter, 0, 
						aim, 1, alias_e->str, -1);
			    contact_list = g_list_append (contact_list, contact_info);
		    }else
		      if (strcmp(prt,"prpl-yahoo")==0){
			      gtk_list_store_append (store, iter);
			      gtk_list_store_set (store, iter, 0, 
						  yahoo, 1, alias_e->str, -1);
			      contact_list = g_list_append (contact_list, contact_info);
		      }else{
			      g_free (contact_info);
		      }
		g_string_free(alias_e, TRUE);
	}
	
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

static
GtkWidget* get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *cb;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter *iter;

	iter = g_malloc (sizeof(GtkTreeIter));
	store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	add_gaim_contacts_to_model (store, iter);
	g_free (iter);	
	model = GTK_TREE_MODEL (store);
	cb = gtk_combo_box_new_with_model (model);
	renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), 
					renderer,
                                        "pixbuf", 0,
                                        NULL);		
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), 
					renderer,
                                        "text", 1,
                                        NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);
	return cb;
}

static
gboolean send_files (NstPlugin *plugin, GtkWidget *contact_widget,
			 GList *file_list)
{
	GString *gaimto;	
	GList *l;
	gchar *spool_file, *spool_file_send, *contact_info;
	FILE *fd;
	gint t, option;

	option = gtk_combo_box_get_active (GTK_COMBO_BOX(contact_widget));
	contact_info = (gchar *) g_list_nth_data (contact_list, option);
	gaimto = g_string_new (contact_info);
	
	for (l = file_list ; l; l=l->next){
		char *path;

		path = g_filename_from_uri (l->data, NULL, NULL);
		g_string_append_printf (gaimto,"%s\n", path);
		g_free (path);
	}
	g_string_append_printf (gaimto,"\n");
	t = time (NULL);
	spool_file = g_strdup_printf ("%s/.gnome2/nautilus-sendto/spool/tmp/%i.send",
				     g_get_home_dir(), t);
	spool_file_send = g_strdup_printf ("%s/.gnome2/nautilus-sendto/spool/%i.send",
					   g_get_home_dir(), t);
	fd = fopen (spool_file,"w");
	fwrite (gaimto->str, 1, gaimto->len, fd);
	fclose (fd);
	rename (spool_file, spool_file_send);
	g_free (spool_file);
	g_free (spool_file_send);
	return TRUE;
}

static 
gboolean destroy (NstPlugin *plugin){
	g_free (blist_online);
	g_list_foreach (contact_list, (GFunc)g_free, NULL);
	return TRUE;
}

static 
NstPluginInfo plugin_info = {
	"im",
	"gaim",
	N_("Instant Message (Gaim)"),
	FALSE,
	init,
	get_contacts_widget,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)


