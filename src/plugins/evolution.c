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

#include <config.h>
#include "../nautilus-sendto-plugin.h"
#include <libebook/e-book.h>

static GHashTable *hash = NULL;

static 
gboolean init (NstPlugin *plugin)
{
	gchar *tmp = NULL;
	gchar *cmds[] = {"evolution",
			 "evolution-2.0",
			 "evolution-2.2",
			 "evolution-2.4",
			 "evolution-2.6",
			 "evolution-2.8", /* for the future */
			 "evolution-3.0", /* but how far to go ? */
			 NULL};
	guint i;

	g_print ("Init evolution plugin\n");
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
	
	for (i = 0; cmds[i] != NULL; i++) {
		tmp = g_find_program_in_path (cmds[i]);
		if (tmp != NULL)
			break;
	}
	if (tmp == NULL)
		return FALSE;
	g_free (tmp);

	hash = g_hash_table_new (g_str_hash, g_str_equal);
	return TRUE;
}

static void
add_name_to_model (const gchar *name, GtkListStore *store, GtkTreeIter *iter, GdkPixbuf *pixbuf, gchar *hash_value)
{
	gchar *hash_key;

	gtk_list_store_append (store, iter);
	gtk_list_store_set (store, iter, 0, pixbuf, 1, name, -1);
	hash_key = g_strdup (name);
	g_hash_table_insert (hash, hash_key, hash_value);
}

static void
add_evolution_contacts_to_model (GtkWidget *entry, 
				 GtkListStore *store, GtkTreeIter *iter)
{
	GdkPixbuf *pixbuf;
	GtkIconTheme *it;
	GError *err = NULL;
	GSList *g, *s, *addr_sources = NULL;
	ESourceList *all_abooks;
	
	it = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (it, "stock_mail", 16, 
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);

	/* Collect address books marked for auto-complete */
	if (!e_book_get_addressbooks (&all_abooks, &err)) {
		g_error_free (err);
		g_error ("Unable to get addressbooks: %s", err->message);
	}
	for (g = e_source_list_peek_groups (all_abooks); g; g = g_slist_next (g)) {
		for (s = e_source_group_peek_sources ((ESourceGroup *) g->data); s; s = g_slist_next (s)) {
			ESource *source = s->data;
			const char *completion = e_source_get_property (source, "completion");
			if (completion && !g_ascii_strcasecmp (completion, "true")) {
				addr_sources = g_slist_prepend (addr_sources, source);
				g_object_ref (source);
			}
			g_object_unref (source);
		}
		g_slist_free (s);
	}
	g_slist_free (g);

	/* Extract contacts from address books */
	for (s = addr_sources; s; s = g_slist_next (s)) {
		ESource *source = s->data;
		EBook *book;
		EBookQuery *query;
		GList *contacts, *c;

		if (!(book = e_book_new (source, &err))) {
			g_warning ("Unable to create addressbook: %s", err->message);
			g_error_free (err);
			continue;
		}
		if (!e_book_open (book, TRUE, &err)) {
			g_warning ("Unable to open addressbook: %s", err->message);
			g_error_free (err);
			g_object_unref (book);
			continue;
		}
		query = e_book_query_field_exists (E_CONTACT_FULL_NAME);
		if (!e_book_get_contacts (book, query, &contacts, &err)) {
			g_warning ("Unable to get contacts: %s", err->message);
			g_error_free (err);
			g_object_unref (book);
			continue;
		}
		e_book_query_unref (query);
		g_object_unref (book);

		/* Add all contacts */
		for (c = contacts; c; c = g_list_next (c)) {
			EContact *contact = E_CONTACT (c->data);
			const char *family_name = e_contact_get_const (contact, E_CONTACT_FAMILY_NAME);
			const char *given_name = e_contact_get_const (contact, E_CONTACT_GIVEN_NAME);
			GList *emails, *e;

			/* Iterate over all email addresses for a contact */
			emails = e_contact_get (contact, E_CONTACT_EMAIL);
			for (e = emails; e; e = e->next) {
				gchar *email = e->data, *hash_value, *full_name, *str;
				gboolean both_names_set = FALSE;

				hash_value = g_strdup_printf ("mailto:%s", email);
				if (family_name != NULL && strlen (family_name) > 0 &&
				    given_name != NULL && strlen (given_name) > 0)
					both_names_set = TRUE;
				/* Create the full name as "family_name given_name" */
				/* TODO don't print given_name or family at all when they are NULL */
				full_name = g_strdup_printf ("%s%s%s", given_name, both_names_set ? " " : "", family_name);

				/* Output: email (full_name) */
				str = g_strdup_printf ("%s (%s)", email, full_name);
				add_name_to_model (str, store, iter, pixbuf, hash_value);
				g_free (str);
				/* Output: full_name <email> */
				str = g_strdup_printf ("%s <%s>", full_name, email);
				add_name_to_model (str, store, iter, pixbuf, hash_value);
				g_free (str);
				if (both_names_set) {
					/* Output: family_name, given_name <email> */
					str = g_strdup_printf ("%s, %s <%s>", family_name, given_name, email);
					add_name_to_model (str, store, iter, pixbuf, hash_value);
					g_free (str);
				}
				g_free (full_name);
				
			}
			g_object_unref (contact);
			g_list_free (e);
			g_list_free (emails);
		}
		g_object_unref (source);
		g_list_free (c);
		g_list_free (contacts);
	}
	g_object_unref (all_abooks);
	g_slist_free (s);
	g_slist_free (addr_sources);
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
		if (evo_cmd == NULL)
			return FALSE;
	}
	g_string_append_printf (mailto,"?attach=\"%s\"", (char *)file_list->data);
	for (l = file_list->next ; l; l=l->next){
		g_string_append_printf (mailto,"&attach=\"%s\"", (char *)l->data);
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
	"evolution",
	N_("Email (Evolution)"),
	FALSE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

