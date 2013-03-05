/*
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
 * Authors:  Norbert Preining <norbert@preining.info>
 *           based on the removable-devices plugin by
 *             Maxim Ermilov <ermilov.maxim@gmail.com>
 *             Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include "nst-common.h"
#include "nautilus-sendto-plugin.h"

#define SCRIPTS_GROUP "scripts"
#define NO_SCRIPTS "(no scripts defined)"

enum {
	NAME_COL,
	SCRIPT_COL,
	NUM_COLS,
};

GtkWidget *cb;

static gboolean
init (NstPlugin *plugin)
{
	g_print ("Init scripts plugin\n");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	cb = gtk_combo_box_new ();

	return TRUE;
}

int add_scripts_from_keyfile (GtkListStore *store, const gchar *pt) {
	GError *error = NULL;
	GKeyFileFlags flags;
	GKeyFile *keyfile;
	GtkTreeIter iter;

	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, pt, flags, &error)) {
		g_message ("cannot read %s: %s", pt, error->message);
		g_key_file_free(keyfile);
		return 0;
	}
	char **script_names;
	gsize num_scripts;
	script_names = g_key_file_get_keys(keyfile, SCRIPTS_GROUP, &num_scripts, &error);
	guint key;

	for (key = 0; key < num_scripts; key++) {
		gchar *name;
		gchar *script;

		name = script_names[key];
		script = g_key_file_get_value(keyfile, SCRIPTS_GROUP, name, &error);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    NAME_COL, name,
				    SCRIPT_COL, script,
				    -1);
		g_free (name);
		g_free (script);
	}
	return num_scripts;
}


static GtkWidget*
get_scripts_widget (NstPlugin *plugin)
{
	GtkListStore *store;
	GtkCellRenderer *name_renderer, *script_renderer;

	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);

	// keyfile parser 
	// http://developer.gnome.org/glib/stable/glib-Key-value-file-parser.html
	// see example code at 
	// http://www.gtkbook.com/tutorial.php?page=keyfile
	// https://gist.github.com/zdxerr/709169
	int nrscripts = 0;
	nrscripts +=
		add_scripts_from_keyfile(store, "/etc/sendto-scripts.conf");
	nrscripts += 
		add_scripts_from_keyfile(store, g_strdup_printf("%s%s",
		g_get_user_config_dir(),
		"/sendto-scripts.conf"));
	if (nrscripts == 0) {
		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    NAME_COL, NO_SCRIPTS,
				    SCRIPT_COL, NO_SCRIPTS,
				    -1);
	}

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (cb));
	gtk_combo_box_set_model (GTK_COMBO_BOX (cb), GTK_TREE_MODEL (store));

	name_renderer = gtk_cell_renderer_text_new ();
	script_renderer = gtk_cell_renderer_text_new ();

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), name_renderer, FALSE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), script_renderer, FALSE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), name_renderer, "text", 0,  NULL);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), script_renderer, "text", 1,  NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);

	return cb;
}

static gboolean
send_files (NstPlugin *plugin, GtkWidget *contact_widget,
	    GList *file_list)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *script;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (contact_widget), &iter) == FALSE)
		return TRUE;

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (cb)));
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, SCRIPT_COL, &script, -1);

	if (g_strcmp0(script, NO_SCRIPTS) == 0) {
		g_message("No scripts defined, cannot execute anything!");
		return TRUE;
	}
	gchar *cmdline, *foo;
	cmdline = strdup(script);
	foo = cmdline;
	foo = g_stpcpy(foo, cmdline);
	foo = g_stpcpy(foo, " ");
	GList *elem;
	for (elem = file_list; elem; elem = elem->next) {
		foo = g_stpcpy(foo, (char *)elem->data);
		foo = g_stpcpy(foo, " ");
	}
	GError *error = NULL;
	g_message("calling %s\n", cmdline);
	if (!g_spawn_command_line_async(cmdline, &error)) {
		g_message("Some error occurred: %s\n", error->message);
	}
	return TRUE;
}

static gboolean
destroy (NstPlugin *plugin)
{
	gtk_widget_destroy (cb);

	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"folder-documents",	/* the icon, we need a better! */
	"scripts",
	N_("User defined scripts"),
	NULL,
	NAUTILUS_CAPS_SEND_DIRECTORIES,
	init,
	get_scripts_widget,
	NULL,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

