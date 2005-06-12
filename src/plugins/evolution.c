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
 * Author:  Roberto Majadas <roberto.majadas@hispalinux.es>
 */

#include <config.h>
#include "../nautilus-sendto-plugin.h"
#include <libebook/e-book.h>

static GHashTable *hash = NULL;

static 
gboolean init (NstPlugin *plugin)
{
	printf ("Init evolution plugin\n");
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
	
	hash = g_hash_table_new (g_str_hash, g_str_equal);
	return TRUE;
}

void
add_evolution_contacts_to_model (GtkWidget *entry, 
				 GtkListStore *store, GtkTreeIter *iter)
{
	GdkPixbuf *pixbuf;
	GtkIconTheme *it;
	EBook *book;
	EBookQuery *query;
	GList *cards, *c;
	gboolean status;
	gchar *hash_key;
	gchar *hash_value;
	
	it = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (it, "stock_mail", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);

	book = e_book_new_system_addressbook (NULL);
	if (!book) {
		printf ("failed to create local addressbook\n");
		exit(0);
	}
	
	status = e_book_open (book, FALSE, NULL);
	if (status == FALSE) {
		printf ("failed to open local addressbook\n");
		exit(0);
	}

	query = e_book_query_field_exists (E_CONTACT_FULL_NAME);
	status = e_book_get_contacts (book, query, &cards, NULL);
	e_book_query_unref (query);
	
	if (status == FALSE) {
		printf ("error %d getting card list\n", status);
		exit(0);
	}

	for (c = cards; c; c = c->next) {
		EContact *contact = E_CONTACT (c->data);		
		char *family_name = e_contact_get_const (contact, E_CONTACT_FAMILY_NAME);
		char *given_name = e_contact_get_const (contact, E_CONTACT_GIVEN_NAME);
		GList *emails, *e;
		
		emails = e_contact_get (contact, E_CONTACT_EMAIL);
		for (e = emails; e; e = e->next) {
			GString *str;
			char *email = e->data;
			
			hash_value = g_strdup_printf ("mailto:%s",email);
			if (strlen (family_name)==0){
				/* Output : name <emai> */
				str = g_string_new("");				
				g_string_printf (str, "%s <%s>", given_name, email);
				gtk_list_store_append (store, iter);
				gtk_list_store_set (store, iter, 0, pixbuf, 1, str->str, -1);
				hash_key = g_strdup (str->str);
				g_hash_table_insert (hash, hash_key, hash_value);
				g_string_free (str, TRUE);
				/* Output : email (name) */
				str = g_string_new("");
				g_string_printf (str, "%s (%s)", email, given_name);
				gtk_list_store_append (store, iter);
				gtk_list_store_set (store, iter, 0, pixbuf, 1, str->str, -1);
				hash_key = g_strdup (str->str);
				g_hash_table_insert (hash, hash_key, hash_value);
				g_string_free (str, TRUE);
				
			}else{
				/* Output : family_name, name <email> */
				str = g_string_new("");
				g_string_printf (str, "%s, %s <%s>", family_name, 
						 given_name, email);
				gtk_list_store_append (store, iter);
				gtk_list_store_set (store, iter, 0, pixbuf, 1, str->str, -1);
				hash_key = g_strdup (str->str);
				g_hash_table_insert (hash, hash_key, hash_value);
				g_string_free (str, TRUE);
				/* Output : name family_name <email> */
				str = g_string_new("");
				g_string_printf (str, "%s %s <%s>", given_name, 
						 family_name, email);
				gtk_list_store_append (store, iter);
				gtk_list_store_set (store, iter, 0, pixbuf, 1, str->str, -1);
				hash_key = g_strdup (str->str);
				g_hash_table_insert (hash, hash_key, hash_value);
				g_string_free (str, TRUE);
				/* Output : email (family_name, name) */
				str = g_string_new("");
				g_string_printf (str, "%s (%s, %s)", email, 
						 family_name, given_name);
				gtk_list_store_append (store, iter);
				gtk_list_store_set (store, iter, 0, pixbuf, 1, str->str, -1);
				hash_key = g_strdup (str->str);
				g_hash_table_insert (hash, hash_key, hash_value);
				g_string_free (str, TRUE);
			}			

		}
		g_list_foreach (emails, (GFunc)g_free, NULL);
		g_list_free (emails);
		g_object_unref (contact);
		
	}
	g_list_free (cards);
}

static
GtkWidget* get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *entry;
	GtkEntryCompletion *completion;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeModel *completion_model;
	GtkTreeIter *iter;
	
	entry = gtk_entry_new ();
	completion = gtk_entry_completion_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),
					renderer,
					FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), renderer,
					"pixbuf", 0, NULL);
	iter = g_malloc (sizeof(GtkTreeIter));
	store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	add_evolution_contacts_to_model (entry, store, iter);
	g_free (iter);	
	completion_model = GTK_TREE_MODEL (store);
	gtk_entry_completion_set_model (completion, completion_model);
	gtk_entry_set_completion (GTK_ENTRY (entry), completion);
	gtk_entry_completion_set_text_column (completion, 1);
	g_object_unref (completion_model);
	g_object_unref (completion);
	
	return entry;
}

static
gboolean send_files (NstPlugin *plugin, GtkWidget *contact_widget,
			GList *file_list)
{
	gchar *evo_cmd, *cmd, *send_to, *send_to_info ;
	GList *l;
	GString *mailto;
	
	send_to = (gchar *) gtk_entry_get_text (GTK_ENTRY(contact_widget));
	
	if (strlen (send_to) != 0){
		send_to_info = g_hash_table_lookup (hash, send_to);
		if (send_to_info != NULL){
			mailto = g_string_new (send_to_info);
		}else{
			mailto = g_string_new ("mailto:");
			g_string_append_printf (mailto, "%s", send_to);
		}
	}else{
		mailto = g_string_new ("mailto:\"\"");
	}
	evo_cmd = g_find_program_in_path ("evolution");
	if (evo_cmd == NULL){
		evo_cmd = g_find_program_in_path ("evolution-1.5");
		if (evo_cmd == NULL)
			evo_cmd = g_find_program_in_path ("evolution-2.0");
	}
	g_string_append_printf (mailto,"?attach=\"%s\"",file_list->data);
	for (l = file_list->next ; l; l=l->next){
		g_string_append_printf (mailto,"&attach=\"%s\"",l->data);
	}
	cmd = g_strdup_printf ("%s %s", evo_cmd, mailto->str);
	g_spawn_command_line_async (cmd, NULL);
	g_free (cmd);
	g_string_free (mailto, TRUE);
	g_free (evo_cmd);
	return TRUE;
}

static 
gboolean destroy (NstPlugin *plugin){
	return TRUE;
}

static 
NstPluginInfo plugin_info = {
	"stock_mail",
	N_("Email (Evolution)"),
	init,
	get_contacts_widget,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

