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

#include <e-contact-entry.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <libedataserverui/e-client-utils.h>
#include "nautilus-sendto-plugin.h"

#define CONTACT_FORMAT "%s <%s>"

typedef enum {
	MAILER_UNKNOWN,
	MAILER_EVO,
	MAILER_BALSA,
	MAILER_SYLPHEED,
	MAILER_THUNDERBIRD,
} MailerType;

static char *mail_cmd = NULL;
static MailerType type = MAILER_UNKNOWN;
static char *email = NULL;
static char *name = NULL;

static char *
get_evo_cmd (void)
{
	char *tmp = NULL;
	char *retval;
	char *cmds[] = {"evolution",
		"evolution-2.0",
		"evolution-2.2",
		"evolution-2.4",
		"evolution-2.6",
		"evolution-2.8", /* for the future */
		"evolution-3.0", /* but how far to go ? */
		NULL};
	guint i;

	
	for (i = 0; cmds[i] != NULL; i++) {
		tmp = g_find_program_in_path (cmds[i]);
		if (tmp != NULL)
			break;
	}

	if (tmp == NULL)
		return NULL;

	retval = g_strdup_printf ("%s --component=mail %%s", tmp);
	g_free (tmp);
	return retval;
}

static gboolean
init (NstPlugin *plugin)
{
	GAppInfo *app_info;

	g_print ("Init evolution plugin\n");
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	app_info = g_app_info_get_default_for_uri_scheme ("mailto");
	if (app_info) {
		mail_cmd = g_strdup (g_app_info_get_commandline (app_info));
		g_object_unref (app_info);
	} else {
		mail_cmd = NULL;
	}

	if (mail_cmd == NULL || *mail_cmd == '\0') {
		g_free (mail_cmd);
		mail_cmd = get_evo_cmd ();
		type = MAILER_EVO;
	} else {
		/* Find what the default mailer is */
		if (strstr (mail_cmd, "balsa"))
			type = MAILER_BALSA;
		else if (strstr (mail_cmd, "thunder") || strstr (mail_cmd, "seamonkey")) {
			char **strv;

			type = MAILER_THUNDERBIRD;

			/* Thunderbird sucks, see
			 * https://bugzilla.gnome.org/show_bug.cgi?id=614222 */
			strv = g_strsplit (mail_cmd, " ", -1);
			g_free (mail_cmd);
			mail_cmd = g_strdup_printf ("%s %%s", strv[0]);
			g_strfreev (strv);
		} else if (strstr (mail_cmd, "sylpheed") || strstr (mail_cmd, "claws"))
			type = MAILER_SYLPHEED;
		else if (strstr (mail_cmd, "anjal"))
			type = MAILER_EVO;
	}

	if (mail_cmd == NULL)
		return FALSE;

	return TRUE;
}

static void
contacts_selected_cb (GtkWidget *entry, EContact *contact, const char *identifier, NstPlugin *plugin)
{
	char *text;

	g_free (email);
	email = NULL;

	if (identifier != NULL)
		email = g_strdup (identifier);
	else
		email = e_contact_get (contact, E_CONTACT_EMAIL_1);

	g_free (name);
	name = NULL;

	name = e_contact_get (contact, E_CONTACT_FULL_NAME);
	if (name == NULL) {
		name = e_contact_get (contact, E_CONTACT_NICKNAME);
		if (name == NULL)
			name = e_contact_get (contact, E_CONTACT_ORG);
	}
	if (name != NULL) {
		text = g_strdup_printf (CONTACT_FORMAT, (char*) name, email);
		gtk_entry_set_text (GTK_ENTRY (entry), text);
		g_free (text);
	} else
		gtk_entry_set_text (GTK_ENTRY (entry), email);
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
error_cb (EContactEntry *entry_widget, const char *error, NstPlugin *plugin)
{
	g_warning ("An error occurred: %s", error);
}

static void
add_sources (EContactEntry *entry)
{
	ESourceList *source_list;

	if (e_client_utils_get_sources (&source_list,
					E_CLIENT_SOURCE_TYPE_CONTACTS,
					NULL)) {
		e_contact_entry_set_source_list (E_CONTACT_ENTRY (entry),
						 source_list);
		g_object_unref (source_list);
	}
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
	g_signal_connect (G_OBJECT (entry), "error",
			  G_CALLBACK (error_cb), plugin);

	add_sources (E_CONTACT_ENTRY (entry));

	return entry;
}

static void
get_evo_mailto (GtkWidget *contact_widget, GString *mailto, GList *file_list)
{
	GList *l;

	g_string_append (mailto, "mailto:");
	if (email != NULL) {
		if (name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\"", name, email);
		else
			g_string_append_printf (mailto, "%s", email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (contact_widget));
		if (text != NULL && *text != '\0')
			g_string_append_printf (mailto, "\"%s\"", text);
		else
			g_string_append (mailto, "\"\"");
	}
	g_string_append_printf (mailto,"?attach=\"%s\"", (char *)file_list->data);
	for (l = file_list->next ; l; l=l->next){
		g_string_append_printf (mailto,"&attach=\"%s\"", (char *)l->data);
	}
}

static void
get_balsa_mailto (GtkWidget *contact_widget, GString *mailto, GList *file_list)
{
	GList *l;

	if (strstr (mail_cmd, " -m ") == NULL && strstr (mail_cmd, " --compose=") == NULL)
		g_string_append (mailto, " --compose=");
	if (email != NULL) {
		if (name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\"", name, email);
		else
			g_string_append_printf (mailto, "%s", email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (contact_widget));
		if (text != NULL && *text != '\0')
			g_string_append_printf (mailto, "\"%s\"", text);
		else
			g_string_append (mailto, "\"\"");
	}
	g_string_append_printf (mailto," --attach=\"%s\"", (char *)file_list->data);
	for (l = file_list->next ; l; l=l->next){
		g_string_append_printf (mailto," --attach=\"%s\"", (char *)l->data);
	}
}

static void
get_thunderbird_mailto (GtkWidget *contact_widget, GString *mailto, GList *file_list)
{
	GList *l;

	g_string_append (mailto, "-compose \"");
	if (email != NULL) {
		if (name != NULL)
			g_string_append_printf (mailto, "to='"CONTACT_FORMAT"',", name, email);
		else
			g_string_append_printf (mailto, "to='%s',", email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (contact_widget));
		if (text != NULL && *text != '\0')
			g_string_append_printf (mailto, "to='%s',", text);
	}
	g_string_append_printf (mailto,"attachment='%s", (char *)file_list->data);
	for (l = file_list->next ; l; l=l->next){
		g_string_append_printf (mailto,",%s", (char *)l->data);
	}
	g_string_append (mailto, "'\"");
}

static void
get_sylpheed_mailto (GtkWidget *contact_widget, GString *mailto, GList *file_list)
{
	GList *l;

	g_string_append (mailto, "--compose ");
	if (email != NULL) {
		if (name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\" ", name, email);
		else
			g_string_append_printf (mailto, "%s ", email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (contact_widget));
		if (text != NULL && *text != '\0')
			g_string_append_printf (mailto, "\"%s\" ", text);
		else
			g_string_append (mailto, "\"\"");
	}
	g_string_append_printf (mailto,"--attach \"%s\"", (char *)file_list->data);
	for (l = file_list->next ; l; l=l->next){
		g_string_append_printf (mailto," \"%s\"", (char *)l->data);
	}
}

static gboolean
send_files (NstPlugin *plugin,
	    GtkWidget *contact_widget,
	    GList *file_list)
{
	gchar *cmd;
	GString *mailto;

	mailto = g_string_new ("");
	switch (type) {
	case MAILER_BALSA:
		get_balsa_mailto (contact_widget, mailto, file_list);
		break;
	case MAILER_SYLPHEED:
		get_sylpheed_mailto (contact_widget, mailto, file_list);
		break;
	case MAILER_THUNDERBIRD:
		get_thunderbird_mailto (contact_widget, mailto, file_list);
		break;
	case MAILER_EVO:
	default:
		get_evo_mailto (contact_widget, mailto, file_list);
	}

	cmd = g_strdup_printf (mail_cmd, mailto->str);
	g_string_free (mailto, TRUE);

	g_message ("Mailer type: %d", type);
	g_message ("Command: %s", cmd);

	g_spawn_command_line_async (cmd, NULL);
	g_free (cmd);

	return TRUE;
}

static 
gboolean destroy (NstPlugin *plugin){
	g_free (mail_cmd);
	mail_cmd = NULL;
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
	N_("Email"),
	NULL,
	NAUTILUS_CAPS_NONE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)

