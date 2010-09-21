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
 *          Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#include <e-contact-entry.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include "nautilus-sendto-plugin.h"
#include "nautilus-sendto-packer.h"

#define EVOLUTION_TYPE_PLUGIN         (evolution_plugin_get_type ())
#define EVOLUTION_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EVOLUTION_TYPE_PLUGIN, EvolutionPlugin))
#define EVOLUTION_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EVOLUTION_TYPE_PLUGIN, EvolutionPlugin))
#define EVOLUTION_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EVOLUTION_TYPE_PLUGIN))
#define EVOLUTION_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EVOLUTION_TYPE_PLUGIN))
#define EVOLUTION_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EVOLUTION_TYPE_PLUGIN, EvolutionPluginClass))

typedef struct _EvolutionPlugin       EvolutionPlugin;
typedef struct _EvolutionPluginClass  EvolutionPluginClass;

typedef enum {
	MAILER_UNKNOWN,
	MAILER_EVO,
	MAILER_BALSA,
	MAILER_SYLPHEED,
	MAILER_THUNDERBIRD,
} MailerType;

struct _EvolutionPlugin {
	PeasExtensionBase parent_instance;

	GtkWidget *vbox;
	GtkWidget *entry;
	GtkWidget *packer;
	gboolean has_dirs;

	char *mail_cmd;
	MailerType type;
	char *email;
	char *name;
};

struct _EvolutionPluginClass {
	PeasExtensionBaseClass parent_class;
};

NAUTILUS_PLUGIN_REGISTER(EVOLUTION_TYPE_PLUGIN, EvolutionPlugin, evolution_plugin)

#define GCONF_COMPLETION "/apps/evolution/addressbook"
#define GCONF_COMPLETION_SOURCES GCONF_COMPLETION "/sources"
#define DEFAULT_MAILTO "/desktop/gnome/url-handlers/mailto/command"

#define CONTACT_FORMAT "%s <%s>"

static char *
get_evo_cmd (void)
{
	char *tmp = NULL;
	char *retval;
	char *cmds[] = {
		"evolution",
		"evolution-2.0",
		"evolution-2.2",
		"evolution-2.4",
		"evolution-2.6",
		"evolution-2.8", /* for the future */
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

static void
evolution_plugin_init (EvolutionPlugin *p)
{
	GConfClient *client;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	p->type = MAILER_UNKNOWN;
	p->email = NULL;
	p->name = NULL;

	client = gconf_client_get_default ();
	p->mail_cmd = gconf_client_get_string (client, DEFAULT_MAILTO, NULL);
	g_object_unref (client);

	if (p->mail_cmd == NULL || *p->mail_cmd == '\0') {
		g_free (p->mail_cmd);
		p->mail_cmd = get_evo_cmd ();
		p->type = MAILER_EVO;
	} else {
		/* Find what the default mailer is */
		if (strstr (p->mail_cmd, "balsa"))
			p->type = MAILER_BALSA;
		else if (strstr (p->mail_cmd, "thunder") || strstr (p->mail_cmd, "seamonkey")) {
			char **strv;

			p->type = MAILER_THUNDERBIRD;

			/* Thunderbird sucks, see
			 * https://bugzilla.gnome.org/show_bug.cgi?id=614222 */
			strv = g_strsplit (p->mail_cmd, " ", -1);
			g_free (p->mail_cmd);
			p->mail_cmd = g_strdup_printf ("%s %%s", strv[0]);
			g_strfreev (strv);
		} else if (strstr (p->mail_cmd, "sylpheed") || strstr (p->mail_cmd, "claws"))
			p->type = MAILER_SYLPHEED;
		else if (strstr (p->mail_cmd, "anjal"))
			p->type = MAILER_EVO;
	}
}

static void
contacts_selected_cb (GtkWidget       *entry,
		      EContact        *contact,
		      const char      *identifier,
		      EvolutionPlugin *p)
{
	char *text;

	g_free (p->email);
	p->email = NULL;

	if (identifier != NULL)
		p->email = g_strdup (identifier);
	else
		p->email = e_contact_get (contact, E_CONTACT_EMAIL_1);

	g_free (p->name);
	p->name = NULL;

	p->name = e_contact_get (contact, E_CONTACT_FULL_NAME);
	if (p->name == NULL) {
		p->name = e_contact_get (contact, E_CONTACT_NICKNAME);
		if (p->name == NULL)
			p->name = e_contact_get (contact, E_CONTACT_ORG);
	}
	if (p->name != NULL) {
		text = g_strdup_printf (CONTACT_FORMAT, (char*) p->name, p->email);
		gtk_entry_set_text (GTK_ENTRY (entry), text);
		g_free (text);
	} else
		gtk_entry_set_text (GTK_ENTRY (entry), p->email);
}

static void
state_change_cb (GtkWidget       *entry,
		 gboolean         state,
		 EvolutionPlugin *p)
{
	if (state == FALSE) {
		g_free (p->email);
		p->email = NULL;
		g_free (p->name);
		p->name = NULL;
	}
}

static void
error_cb (EContactEntry   *entry_widget,
	  const char      *error,
	  EvolutionPlugin *plugin)
{
	g_warning ("An error occurred: %s", error);
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
sources_changed_cb (GConfClient   *client,
		    guint          cnxn_id,
		    GConfEntry    *entry,
		    EContactEntry *entry_widget)
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

static void
can_send_changed (GObject         *gobject,
		  GParamSpec      *pspec,
		  EvolutionPlugin *p)
{
	gboolean can_send;

	g_object_get (gobject, "can-send", &can_send, NULL);

	/* FIXME, can we validate whatever was already in the entry? */
	g_signal_emit_by_name (G_OBJECT (p),
			       "can-send",
			       "evolution",
			       can_send);
}

static GtkWidget *
evolution_plugin_get_widget (NautilusSendtoPlugin *plugin,
			     GList                *file_list)
{
	EvolutionPlugin *p;
	GtkWidget *alignment;

	p = EVOLUTION_PLUGIN (plugin);
	p->entry = e_contact_entry_new ();
	g_signal_connect (G_OBJECT (p->entry), "contact-selected",
			  G_CALLBACK (contacts_selected_cb), plugin);
	g_signal_connect (G_OBJECT (p->entry), "state-change",
			  G_CALLBACK (state_change_cb), plugin);
	g_signal_connect (G_OBJECT (p->entry), "error",
			  G_CALLBACK (error_cb), plugin);

	add_sources (E_CONTACT_ENTRY (p->entry));
	setup_source_changes (E_CONTACT_ENTRY (p->entry));

	p->vbox = gtk_vbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (p->vbox), p->entry, FALSE, FALSE, 0);

	alignment = gtk_alignment_new (0.0, 1.0, 1.0, 0.0);
	p->packer = nst_pack_widget_new ();
	nst_pack_widget_set_from_names (NST_PACK_WIDGET (p->packer), file_list);
	if (p->has_dirs != FALSE)
		nst_pack_widget_set_force_enabled (NST_PACK_WIDGET (p->packer), TRUE);
	g_signal_connect (G_OBJECT (p->packer), "notify::can-send",
			  G_CALLBACK (can_send_changed), p);
	gtk_container_add (GTK_CONTAINER (alignment), p->packer);
	gtk_box_pack_start (GTK_BOX (p->vbox), alignment, TRUE, TRUE, 0);

	gtk_widget_show_all (p->vbox);

	return p->vbox;
}

static void
evolution_plugin_create_widgets (NautilusSendtoPlugin *plugin,
				 GList                *file_list,
				 const char          **mime_types)
{
	EvolutionPlugin *p;
	guint i;

	p = EVOLUTION_PLUGIN (plugin);

	if (p->mail_cmd == NULL)
		return;

	for (i = 0; mime_types[i] != NULL; i++) {
		if (g_str_equal (mime_types[i], "inode/directory"))
			p->has_dirs = TRUE;
	}

	g_signal_emit_by_name (G_OBJECT (plugin),
			       "add-widget",
			       "evolution",
			       _("Mail"),
			       "emblem-mail",
			       evolution_plugin_get_widget (plugin, file_list));

	g_signal_emit_by_name (G_OBJECT (p),
			       "can-send",
			       "evolution",
			       TRUE);
}

static void
get_evo_mailto (EvolutionPlugin *p,
		GString         *mailto,
		GList           *file_list)
{
	GList *l;

	g_string_append (mailto, "mailto:");
	if (p->email != NULL) {
		if (p->name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\"", p->name, p->email);
		else
			g_string_append_printf (mailto, "%s", p->email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (p->entry));
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
get_balsa_mailto (EvolutionPlugin *p,
		  GString         *mailto,
		  GList           *file_list)
{
	GList *l;

	if (strstr (p->mail_cmd, " -m ") == NULL && strstr (p->mail_cmd, " --compose=") == NULL)
		g_string_append (mailto, " --compose=");
	if (p->email != NULL) {
		if (p->name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\"", p->name, p->email);
		else
			g_string_append_printf (mailto, "%s", p->email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (p->entry));
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
get_thunderbird_mailto (EvolutionPlugin *p,
			GString         *mailto,
			GList           *file_list)
{
	GList *l;

	g_string_append (mailto, "-compose \"");
	if (p->email != NULL) {
		if (p->name != NULL)
			g_string_append_printf (mailto, "to='"CONTACT_FORMAT"',", p->name, p->email);
		else
			g_string_append_printf (mailto, "to='%s',", p->email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (p->entry));
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
get_sylpheed_mailto (EvolutionPlugin *p,
		     GString         *mailto,
		     GList           *file_list)
{
	GList *l;

	g_string_append (mailto, "--compose ");
	if (p->email != NULL) {
		if (p->name != NULL)
			g_string_append_printf (mailto, "\""CONTACT_FORMAT"\" ", p->name, p->email);
		else
			g_string_append_printf (mailto, "%s ", p->email);
	} else {
		const char *text;

		text = gtk_entry_get_text (GTK_ENTRY (p->entry));
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

static void
evolution_plugin_send_files (NautilusSendtoPlugin *plugin,
			     const char           *id,
			     GList                *file_list,
			     GAsyncReadyCallback   callback,
			     gpointer              user_data)
{
	EvolutionPlugin *p;
	gchar *cmd;
	GString *mailto;
	GList *packed;
	GSimpleAsyncResult *simple;

	p = EVOLUTION_PLUGIN (plugin);

	packed = NULL;
	if (nst_pack_widget_get_enabled (NST_PACK_WIDGET (p->packer))) {
		char *filename;

		/* FIXME: this should be async */
		filename = nst_pack_widget_pack_files (NST_PACK_WIDGET (p->packer), file_list);
		packed = g_list_append (packed, filename);
	}

	simple = g_simple_async_result_new (G_OBJECT (plugin),
					    callback,
					    user_data,
					    nautilus_sendto_plugin_send_files);

	mailto = g_string_new ("");
	switch (p->type) {
	case MAILER_BALSA:
		get_balsa_mailto (p, mailto, packed ? packed : file_list);
		break;
	case MAILER_SYLPHEED:
		get_sylpheed_mailto (p, mailto, packed ? packed : file_list);
		break;
	case MAILER_THUNDERBIRD:
		get_thunderbird_mailto (p, mailto, packed ? packed : file_list);
		break;
	case MAILER_EVO:
	default:
		get_evo_mailto (p, mailto, packed ? packed : file_list);
	}

	if (packed != NULL) {
		g_free (packed->data);
		g_list_free (packed);
	}

	cmd = g_strdup_printf (p->mail_cmd, mailto->str);
	g_string_free (mailto, TRUE);

	g_message ("Mailer type: %d", p->type);
	g_message ("Command: %s", cmd);

	/* FIXME: collect errors from this call */
	g_spawn_command_line_async (cmd, NULL);
	g_free (cmd);

	g_simple_async_result_set_op_res_gpointer (simple,
						   GINT_TO_POINTER (NST_SEND_STATUS_SUCCESS_DONE),
						   NULL);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);
}

static void
evolution_plugin_finalize (GObject *object)
{
	EvolutionPlugin *p;

	p = EVOLUTION_PLUGIN (object);

	g_free (p->mail_cmd);
	p->mail_cmd = NULL;

	g_free (p->name);
	p->name = NULL;

	g_free (p->email);
	p->email = NULL;

	gtk_widget_destroy (p->entry);
	p->entry = NULL;

}

