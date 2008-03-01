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

#include "config.h"

#include <e-contact-entry.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include "../nautilus-sendto-plugin.h"

#define GCONF_COMPLETION "/apps/evolution/addressbook"
#define GCONF_COMPLETION_SOURCES GCONF_COMPLETION "/sources"

#define CONTACT_FORMAT "%s <%s>"

static char *evo_cmd = NULL;
static char *email = NULL;
static char *name = NULL;

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
	evo_cmd = tmp;

	return TRUE;
}

static
void contacts_selected_cb (GtkWidget *entry, EContact *contact, const char *identifier, NstPlugin *plugin)
{
	char *text;

	email = e_contact_get (contact, E_CONTACT_EMAIL_1);
	name = e_contact_get (contact, E_CONTACT_NAME_OR_ORG);

	text = g_strdup_printf (CONTACT_FORMAT, (char*)e_contact_get_const (contact, E_CONTACT_NAME_OR_ORG), email);
	gtk_entry_set_text (GTK_ENTRY (entry), text);
	g_free (text);
}

static void
state_change_cb (GtkWidget *entry, gboolean state, gpointer data)
{
	if (state == FALSE) {
		g_free (email);
		email = NULL;
		g_free (name);
		name = NULL;
	}
}

static void
add_sources (EContactEntry *entry)
{
	ESourceList *source_list;

	source_list =
		e_source_list_new_for_gconf_default (GCONF_COMPLETION_SOURCES);
	e_contact_entry_set_source_list (E_CONTACT_ENTRY (entry),
					 source_list);
	g_object_unref (source_list);
}

static void
sources_changed_cb (GConfClient *client, guint cnxn_id,
		GConfEntry *entry, EContactEntry *entry_widget)
{
	add_sources (entry_widget);
}

static void
setup_source_changes (EContactEntry *entry)
{
	GConfClient *gc;

	gc = gconf_client_get_default ();
	gconf_client_add_dir (gc, GCONF_COMPLETION,
			GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_notify_add (gc, GCONF_COMPLETION,
			(GConfClientNotifyFunc) sources_changed_cb,
			entry, NULL, NULL);
}

static
GtkWidget* get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *entry;

	entry = e_contact_entry_new ();
	g_signal_connect (G_OBJECT (entry), "contact-selected",
			  G_CALLBACK (contacts_selected_cb), plugin);
	g_signal_connect (G_OBJECT (entry), "state-change",
			  G_CALLBACK (state_change_cb), NULL);

	add_sources (E_CONTACT_ENTRY (entry));
	setup_source_changes (E_CONTACT_ENTRY (entry));

	return entry;
}

static
gboolean send_files (NstPlugin *plugin, GtkWidget *contact_widget,
			GList *file_list)
{
	gchar *cmd;
	GString *mailto;
	GList *l;

	mailto = g_string_new ("mailto:");

	if (email != NULL) {
		if (name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\"", name, email);
		else
			g_string_append_printf (mailto, "%s", email);
	}else{
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (contact_widget));
		if (text != NULL && *text != '\0')
			g_string_append_printf (mailto, "%s", text);
		else
			g_string_append (mailto, "\"\"");
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
	g_free (evo_cmd);
	evo_cmd = NULL;
	g_free (name);
	name = NULL;
	g_free (email);
	email = NULL;
	return TRUE;
}

static 
NstPluginInfo plugin_info = {
	"emblem-mail",
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

