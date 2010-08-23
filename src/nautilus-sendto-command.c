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
#include <string.h>
#include <stdlib.h>
#include <libpeas/peas.h>
#include <girepository.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "nautilus-sendto-plugin.h"
#include "nautilus-sendto-mimetype.h"

#define NAUTILUS_SENDTO_LAST_MEDIUM	"last-medium"
#define NAUTILUS_SENDTO_STATUS_LABEL_TIMEOUT_SECONDS 10

enum {
	COLUMN_IS_SEPARATOR,
	COLUMN_ICON,
	COLUMN_ID,
	COLUMN_PAGE_NUM,
	COLUMN_DESCRIPTION,
	NUM_COLUMNS,
};

/* Options */
static char **filenames = NULL;
static gboolean run_from_build_dir = FALSE;

static PeasEngine *engine;
static PeasExtensionSet *exten_set;

GList *file_list = NULL;
char **mime_types = NULL;
guint num_dirs = 0;
GList *plugin_list = NULL;
GHashTable *hash ;
guint option = 0;

static GSettings *settings = NULL;

typedef struct _NS_ui NS_ui;

struct _NS_ui {
	GtkWidget *dialog;
	GtkWidget *scrolled_window;
	GtkWidget *options_treeview;
	GtkWidget *contacts_notebook;
	GtkWidget *send_to_label;
	GtkWidget *hbox_contacts_ws;
	GtkWidget *buttons;
	GtkWidget *cancel_button;
	GtkWidget *send_button;
};

static const GOptionEntry entries[] = {
	{ "run-from-build-dir", 'b', 0, G_OPTION_ARG_NONE, &run_from_build_dir, N_("Run from build directory"), NULL },
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, N_("Files to send"), "[FILES...]" },
	{ NULL }
};

static void
destroy_dialog (GtkWidget *widget, gpointer data )
{
        gtk_main_quit ();
}

static void
make_sensitive_for_send (NS_ui *ui,
			 gboolean sensitive)
{
	/* The plugins are responsible for making themselves
	 * unsensitive during send */
	gtk_widget_set_sensitive (ui->scrolled_window, sensitive);
	gtk_widget_set_sensitive (ui->buttons, sensitive);
}

static void
send_callback (GObject      *object,
	       GAsyncResult *res,
	       gpointer      user_data)
{
	NS_ui *ui = (NS_ui *) user_data;
	NautilusSendtoSendStatus status;

	status = nautilus_sendto_plugin_send_files_finish (NAUTILUS_SENDTO_PLUGIN (object),
							   res, NULL);

	g_message ("send_callback %d", status);

	if (status == NST_SEND_STATUS_SUCCESS_DONE) {
		destroy_dialog (NULL, NULL);
	} else if (status == NST_SEND_STATUS_SUCCESS) {
		//FIXME make the buttons into a single close button
	} else if (status == NST_SEND_STATUS_FAILED) {
		/* Do nothing, the plugin should report an error */
	} else {
		g_assert_not_reached ();
	}
}

static void
send_button_cb (GtkWidget *widget, NS_ui *ui)
{
	GtkTreeModel *model;
	GtkTreeSelection *treeselection;
	GtkTreeIter iter;
	char *id;
	PeasPluginInfo *info;
	PeasExtension *ext;

	treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->options_treeview));
	if (gtk_tree_selection_get_selected (treeselection, &model, &iter) == FALSE)
		return;

	make_sensitive_for_send (ui, FALSE);

	gtk_tree_model_get (model, &iter,
			    COLUMN_ID, &id,
			    -1);

	g_settings_set_string (settings,
			       NAUTILUS_SENDTO_LAST_MEDIUM,
			       id);

	info = peas_engine_get_plugin_info (engine, id);
	ext = peas_extension_set_get_extension (exten_set, info);

	if (peas_extension_call (ext, "send_files", file_list, send_callback, ui) == FALSE) {
		/* FIXME report the error in the UI */
		g_warning ("Failed to send files");
		make_sensitive_for_send (ui, TRUE);
		g_free (id);
		return;
	}

	g_free (id);
}

static void
option_changed (GtkTreeSelection *treeselection,
		NS_ui *ui)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	int page_num;

	if (gtk_tree_selection_get_selected (treeselection, &model, &iter) == FALSE)
		return;

	gtk_tree_model_get (model, &iter,
			    COLUMN_PAGE_NUM, &page_num,
			    -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (ui->contacts_notebook),
				       page_num);

	/* FIXME: Get a widget in the plugin to grab focus? */
}

static gboolean
separator_func (GtkTreeModel *model,
		GtkTreeIter  *iter,
		gpointer      data)
{
	gboolean is_sep;

	gtk_tree_model_get (model, iter,
			    COLUMN_IS_SEPARATOR, &is_sep,
			    -1);

	return is_sep;
}

static void
set_model_for_options_treeview (NS_ui *ui)
{
	GdkPixbuf *pixbuf;
	GtkTreeIter iter;
	GtkListStore *model;
	GtkIconTheme *it;
	GtkCellRenderer *renderer;
	char *last_used = NULL;
	int i = 0;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	char **list;

	/* Disable this call if you want to debug */
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (ui->contacts_notebook), FALSE);

	it = gtk_icon_theme_get_default ();

	model = gtk_list_store_new (NUM_COLUMNS, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

	last_used = g_settings_get_string (settings,
					   NAUTILUS_SENDTO_LAST_MEDIUM);

	/* Insert the expander */
	gtk_list_store_insert_after (model, &iter, NULL);
	gtk_list_store_set (model, &iter,
			    COLUMN_IS_SEPARATOR, TRUE,
			    -1);

	/* Populate the tree model */
	list = peas_engine_get_loaded_plugins (engine);
	for (i = 0; list[i] != NULL; i++) {
		GtkWidget *label, *w;
		PeasPluginInfo *info;
		PeasExtension *ext;
		const char *id;
		int page_num;
		gboolean supported;

		info = peas_engine_get_plugin_info (engine, list[i]);
		id = peas_plugin_info_get_module_name (info);

		ext = peas_extension_set_get_extension (exten_set, info);

		/* Check if the plugin supports the mime-types of the files we have */
		if (peas_extension_call (ext, "supports_mime_types", mime_types, &supported) == FALSE ||
		    supported == FALSE) {
			g_message ("'%s' does not support mime-types", id);
			continue;
		}

		if (peas_extension_call (ext, "get_widget", file_list, &w) == FALSE || w == NULL) {
			g_warning ("Failed to get widget for %s", id);
			continue;
		}

		pixbuf = gtk_icon_theme_load_icon (it, peas_plugin_info_get_icon_name (info), 16,
						   GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
		label = gtk_label_new (id);
		page_num = gtk_notebook_append_page (GTK_NOTEBOOK (ui->contacts_notebook), w, label);

		/* XXX: do this properly */
		if (strstr (id, "evolution")) {
			gtk_list_store_insert_after (model, &iter, NULL);
		} else {
			gtk_list_store_append (model, &iter);
		}
		gtk_list_store_set (model, &iter,
				    COLUMN_IS_SEPARATOR, FALSE,
				    COLUMN_ICON, pixbuf,
				    COLUMN_ID, id,
				    COLUMN_PAGE_NUM, page_num,
				    COLUMN_DESCRIPTION, peas_plugin_info_get_name (info),
				    -1);

		gtk_widget_show (w);

		if (last_used != NULL && !strcmp(last_used, id))
			option = i;
	}
	g_free(last_used);
	g_strfreev (list);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->options_treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	gtk_tree_view_set_model (GTK_TREE_VIEW (ui->options_treeview),
				 GTK_TREE_MODEL (model));
	column = gtk_tree_view_column_new ();

	/* Pixbuf */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column),
					renderer,
					"pixbuf", COLUMN_ICON,
					NULL);

	/* Text */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column),
					renderer,
					"text", COLUMN_DESCRIPTION,
					NULL);

	/* Separator */
	gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (ui->options_treeview),
					      separator_func,
					      NULL, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (ui->options_treeview), column);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (option_changed), ui);

	/* Select the previously selected option */
	path = gtk_tree_path_new_from_indices (option, -1);
	gtk_tree_selection_select_path (selection, path);
	gtk_tree_path_free (path);
}

static void
update_button_image (GtkSettings *settings,
		     GParamSpec *spec,
		     GtkWidget *widget)
{
	gboolean show_images;

	g_object_get (settings, "gtk-button-images", &show_images, NULL);
	if (show_images == FALSE)
		gtk_widget_hide (widget);
	else
		gtk_widget_show (widget);
}

static void
nautilus_sendto_create_ui (void)
{
	GtkBuilder *app;
	GError* error = NULL;
	NS_ui *ui;
	GtkSettings *gtk_settings;
	GtkWidget *button_image;
	const char *ui_file;
	char *title;

	app = gtk_builder_new ();
	if (run_from_build_dir) {
		ui_file = "../data/nautilus-sendto.ui";
		g_setenv ("NST_RUN_FROM_BUILDDIR", "1", TRUE);
	} else {
		ui_file = UIDIR "/" "nautilus-sendto.ui";
	}

	if (!gtk_builder_add_from_file (app, ui_file, &error)) {
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ui = g_new0 (NS_ui, 1);

	ui->options_treeview = GTK_WIDGET (gtk_builder_get_object (app, "options_treeview"));
	ui->contacts_notebook = GTK_WIDGET (gtk_builder_get_object (app, "contacts_notebook"));
	ui->scrolled_window = GTK_WIDGET (gtk_builder_get_object (app, "scrolledwindow1"));
	ui->hbox_contacts_ws = GTK_WIDGET (gtk_builder_get_object (app, "hbox_contacts_widgets"));
	ui->send_to_label = GTK_WIDGET (gtk_builder_get_object (app, "send_to_label"));
	ui->dialog = GTK_WIDGET (gtk_builder_get_object (app, "nautilus_sendto_dialog"));
	ui->buttons = GTK_WIDGET (gtk_builder_get_object (app, "dialog-action_area2"));
	ui->cancel_button = GTK_WIDGET (gtk_builder_get_object (app, "cancel_button"));
	ui->send_button = GTK_WIDGET (gtk_builder_get_object (app, "send_button"));

	gtk_settings = gtk_settings_get_default ();
	button_image = GTK_WIDGET (gtk_builder_get_object (app, "image1"));
	g_signal_connect (G_OBJECT (gtk_settings), "notify::gtk-button-images",
			  G_CALLBACK (update_button_image), button_image);
	update_button_image (gtk_settings, NULL, button_image);

	/* Set a title depending on the number of files to
	 * share, and their types */
	title = nst_title_from_mime_types ((const char **) mime_types,
					   g_list_length (file_list) - num_dirs,
					   num_dirs);
	gtk_window_set_title (GTK_WINDOW (gtk_builder_get_object (app, "nautilus_sendto_dialog")),
			     title);
	g_free (title);

	set_model_for_options_treeview (ui);
	g_signal_connect (G_OBJECT (ui->dialog), "destroy",
                          G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->cancel_button), "clicked",
			  G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->send_button), "clicked",
			  G_CALLBACK (send_button_cb), ui);

	gtk_widget_show (ui->dialog);
}

static void
nautilus_sendto_plugin_load_all (void)
{
	const GList *list, *l;
	GPtrArray *activate;

	activate = g_ptr_array_new ();
	list = peas_engine_get_plugin_list (PEAS_ENGINE (engine));
	for (l = list; l != NULL; l = l->next) {
		PeasPluginInfo *info = l->data;
		g_ptr_array_add (activate, (gpointer) peas_plugin_info_get_module_name (info));
	}
	g_ptr_array_add (activate, NULL);

	peas_engine_set_loaded_plugins (PEAS_ENGINE (engine), (const char **) activate->pdata);
	g_ptr_array_free (activate, TRUE);
}

static gboolean
nautilus_sendto_plugin_init (void)
{
	GPtrArray *search_paths;
	char **paths, *user_dir;

	if (g_irepository_require (g_irepository_get_default (), "Peas", "1.0", 0, NULL) == NULL) {
		g_warning ("Failed to load Peas bindings");
		return FALSE;
	}
	if (run_from_build_dir)
		g_irepository_prepend_search_path ("plugins/");
	if (g_irepository_require (g_irepository_get_default (), "NautilusSendto", "1.0", 0, NULL) == NULL) {
		g_warning ("Failed to load NautilusSendto bindings");
		return FALSE;
	}

	search_paths = g_ptr_array_new ();

	/* Add uninstalled plugins */
	if (run_from_build_dir) {
		g_ptr_array_add (search_paths, "plugins/");
		g_ptr_array_add (search_paths, "plugins/");
	}

	/* Add user plugins */
	user_dir = g_build_filename (g_get_user_config_dir (), "nautilus-sendto", "plugins", NULL);
	g_ptr_array_add (search_paths, user_dir);
	g_ptr_array_add (search_paths, user_dir);

	/* Add system-wide plugins */
	g_ptr_array_add (search_paths, PLUGINDIR);
	g_ptr_array_add (search_paths, PLUGINDIR);

	/* Terminate array */
	g_ptr_array_add (search_paths, NULL);

	/* Init engine */
	paths = (char **) g_ptr_array_free (search_paths, FALSE);
	engine = peas_engine_new ("Nst",
				  (const gchar **) paths);
	g_free (user_dir);

	/* Create the extension set */
	exten_set = peas_extension_set_new (PEAS_ENGINE (engine),
					    NAUTILUS_SENDTO_TYPE_PLUGIN,
					    NULL);

	/* Load all the plugins now */
	nautilus_sendto_plugin_load_all ();

	return TRUE;
}

static char *
escape_ampersands_and_commas (const char *url)
{
	int i;
	char *str, *ptr;

	/* Count the number of ampersands & commas */
	i = 0;
	ptr = (char *) url;
	while ((ptr = strchr (ptr, '&')) != NULL) {
		i++;
		ptr++;
	}
	ptr = (char *) url;
	while ((ptr = strchr (ptr, ',')) != NULL) {
		i++;
		ptr++;
	}

	/* No ampersands or commas ? */
	if (i == 0)
		return NULL;

	/* Replace the '&' */
	str = g_malloc0 (strlen (url) - i + 3 * i + 1);
	ptr = str;
	for (i = 0; url[i] != '\0'; i++) {
		if (url[i] == '&') {
			*ptr++ = '%';
			*ptr++ = '2';
			*ptr++ = '6';
		} else if (url[i] == ',') {
			*ptr++ = '%';
			*ptr++ = '2';
			*ptr++ = 'C';
		} else {
			*ptr++ = url[i];
		}
	}

	return str;
}

static void
collate_mimetypes (const char *key,
		   gpointer    value,
		   GPtrArray  *array)
{
	g_ptr_array_add (array, g_strdup (key));
}

static void
nautilus_sendto_init (void)
{
	GHashTable *ht;
	GPtrArray *array;
	int i;

	ht = g_hash_table_new_full (g_str_hash, g_direct_equal,
				    g_free, NULL);

	/* Clean up the URIs passed, and collect the mime-types of
	 * the files */
	for (i = 0; filenames != NULL && filenames[i] != NULL; i++) {
		GFile *file;
		char *filename, *escaped, *uri;
		GFileInfo *info;
		const char *mimetype;

		/* We need a filename */
		file = g_file_new_for_commandline_arg (filenames[i]);
		filename = g_file_get_path (file);
		if (filename == NULL) {
			g_object_unref (file);
			continue;
		}

		/* Get the mime-type, and whether the file is readable */
		info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE","G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
					  G_FILE_QUERY_INFO_NONE,
					  NULL,
					  NULL);
		g_object_unref (file);

		if (info == NULL)
			continue;

		if (g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ) == FALSE) {
			g_message ("Foobar is not readable");
			g_object_unref (info);
			continue;
		}
		mimetype = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
		g_hash_table_insert (ht, g_strdup (mimetype), GINT_TO_POINTER (1));
		if (g_str_equal (mimetype, "inode/directory"))
			num_dirs++;

		g_object_unref (info);

		uri = g_filename_to_uri (filename, NULL, NULL);
		g_free (filename);
		escaped = escape_ampersands_and_commas (uri);

		if (escaped == NULL) {
			file_list = g_list_prepend (file_list, uri);
		} else {
			file_list = g_list_prepend (file_list, escaped);
			g_free (uri);
		}
	}

	if (file_list == NULL) {
		/* FIXME, this needs to be done in UI now */
		g_print (_("Expects URIs or filenames to be passed as options\n"));
		exit (1);
	}

	file_list = g_list_reverse (file_list);

	/* Collate the mime-types */
	array = g_ptr_array_new ();
	g_hash_table_foreach (ht, (GHFunc) collate_mimetypes, array);
	g_hash_table_destroy (ht);

	g_ptr_array_add (array, NULL);
	mime_types = (char **) g_ptr_array_free (array, FALSE);
}

int main (int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);
	context = g_option_context_new ("nautilus-sendto");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print (_("Could not parse command-line options: %s\n"), error->message);
		g_error_free (error);
		return 1;
	}

	settings = g_settings_new ("org.gnome.Nautilus.Sendto");
	nautilus_sendto_init ();
	if (nautilus_sendto_plugin_init () == FALSE)
		return 1;
	nautilus_sendto_create_ui ();

	gtk_main ();
	g_object_unref(settings);
	g_strfreev (mime_types);

	return 0;
}

