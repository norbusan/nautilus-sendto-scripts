/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:  Peter Enseleit <penseleit@gmail.com>
 *          Roberto Majadas <telemaco@openshine.com>
 */

#include "../nautilus-sendto-plugin.h"


static GHashTable *hash = NULL;

static const gchar const *possible_binaries[] = 
{	"claws-mail",
	"sylpheed-claws-gtk2",
	"sylpheed-claws",
	"sylpheed"
};

static gchar *get_claws_command(void)
{
	gchar *cmd = NULL;
	gint i;

	for (i = 0; cmd == NULL && i < G_N_ELEMENTS(possible_binaries); i++) {
		cmd = g_find_program_in_path (possible_binaries[i]);
	}

	return cmd;
}

static 
gboolean init (NstPlugin *plugin)
{
	gchar *sc_cmd;
	
	printf ("Init sylpheed-claws plugin\n");
	hash = g_hash_table_new (g_str_hash, g_str_equal);

	sc_cmd = get_claws_command();

	if(sc_cmd == NULL)
		return FALSE;

	g_free(sc_cmd);

	return TRUE;
}

static
GtkWidget* get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *entry;
	
	entry = gtk_entry_new ();
	
	return entry;
}

static
gboolean send_files (NstPlugin *plugin, GtkWidget *contact_widget,
			GList *file_list)
{
	gchar *sc_cmd, *cmd, *send_to;
	GList *l;
	GString *mailto;
	gchar *tmp_str;

	send_to = (gchar *) gtk_entry_get_text (GTK_ENTRY(contact_widget));
		
	if (strlen (send_to) == 0)
	{
		mailto = g_string_new("--compose \"\"");
	}
	else
	{
		mailto = g_string_new("--compose ");
		g_string_append_printf (mailto, "%s", send_to);		
	}
	
	sc_cmd = get_claws_command();
	
	if(sc_cmd == NULL)
		return FALSE;
	/* tmp_str is used for delete file:/// at the start of the filename path */
	tmp_str = g_filename_from_uri(file_list->data, NULL, NULL);
	g_string_append_printf (mailto," --attach \"%s\"",tmp_str);
	g_free(tmp_str);
	
	for (l = file_list->next ; l; l=l->next){
		tmp_str = g_filename_from_uri(l->data, NULL, NULL);
		g_string_append_printf (mailto," \"%s\"",tmp_str);
		g_free(tmp_str);
	}
	cmd = g_strdup_printf ("%s %s", sc_cmd, mailto->str);
	g_spawn_command_line_async (cmd, NULL);
	g_free (cmd);
	g_string_free (mailto, TRUE);
	g_free (sc_cmd);
	return TRUE;
}

static 
gboolean destroy (NstPlugin *plugin){
	return TRUE;
}

static 
NstPluginInfo plugin_info = {
	"stock_mail",
	"sylpheed-claws",
	N_("Email (Claws Mail)"),
	FALSE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)
