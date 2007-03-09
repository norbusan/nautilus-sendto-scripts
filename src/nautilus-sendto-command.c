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
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include "nautilus-sendto-plugin.h"

#define NAUTILUS_SENDTO_GCONF		"/desktop/gnome/nautilus-sendto"
#define NAUTILUS_SENDTO_LAST_MEDIUM	NAUTILUS_SENDTO_GCONF"/last_medium"
#define NAUTILUS_SENDTO_LAST_COMPRESS	NAUTILUS_SENDTO_GCONF"/last_compress"
#define NAUTILUS_SENDTO_STATUS_LABEL_TIMEOUT 10000

/* Options */
static gchar *default_url = NULL;
static char **filenames = NULL;

gboolean force_user_to_compress = FALSE;
GList *file_list = NULL;
GList *plugin_list = NULL;
GHashTable *hash ;
guint option = 0;

static GConfClient *gconf_client = NULL;

typedef struct _NS_ui NS_ui;

struct _NS_ui {
	GtkWidget *dialog;
	GtkWidget *options_combobox;
	GtkWidget *hbox_contacts_ws;
	GtkWidget *cancel_button;
	GtkWidget *send_button;
	GtkWidget *pack_combobox;
	GtkWidget *pack_checkbutton;
	GtkWidget *pack_entry;
	GList *contact_widgets;

	GtkWidget *status_image;
	GtkWidget *status_label;
	guint status_timeoutid;
};

static const GOptionEntry entries[] = {
	{ "default-dir", 'd', 0, G_OPTION_ARG_FILENAME, &default_url, N_("Default folder to use"), NULL },
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, "Movies to index", NULL },
	{ NULL }
};

static void 
destroy_dialog (GtkWidget *widget, gpointer data )
{
        gtk_main_quit ();
}

static char *
get_filename_from_list (void)
{
	GList *l;
	GString *common_part = NULL;
	gboolean matches = TRUE;
	guint offset = 0;
	const char *encoding;
	gboolean use_utf8 = TRUE;

	encoding = g_getenv ("G_FILENAME_ENCODING");

	if (encoding != NULL && strcasecmp(encoding, "UTF-8") != 0)
		use_utf8 = FALSE;

	if (file_list == NULL)
		return NULL;

	common_part = g_string_new("");

	while (TRUE) {
		gunichar cur_char = '\0';
		for (l = file_list; l ; l = l->next) {
			char *path = NULL, *name = NULL;
			char *offset_name = NULL;

			path = g_filename_from_uri ((char *) l->data,
					NULL, NULL);
			if (!path)
				break;

			name = g_path_get_basename (path);

			if (!use_utf8) {
				char *tmp;

				tmp = g_filename_to_utf8 (name, -1,
						NULL, NULL, NULL);
				g_free (name);
				name = tmp;
			}

			if (!name) {
				g_free (path);
				break;
			}

			if (offset >= g_utf8_strlen (name, -1)) {
				g_free(name);
				g_free(path);
				matches = FALSE;
				break;
			}

			offset_name = g_utf8_offset_to_pointer (name, offset);

			if (offset_name == g_utf8_strrchr (name, -1, '.')) {
				g_free (name);
				g_free (path);
				matches = FALSE;
				break;
			}
			if (cur_char == '\0') {
				cur_char = g_utf8_get_char (offset_name);
			} else if (cur_char != g_utf8_get_char (offset_name)) {
				g_free (name);
				g_free (path);
				matches = FALSE;
				break;
			}
			g_free (name);
			g_free (path);
		}
		if (matches == TRUE && cur_char != '\0') {
			offset++;
			common_part = g_string_append_unichar (common_part,
					cur_char);
		} else {
			break;
		}
	}

	if (g_utf8_strlen (common_part->str, -1) < 4) {
		g_string_free (common_part, TRUE);
		return NULL;
	}

	return g_string_free (common_part, FALSE);
}

static char *
pack_files (NS_ui *ui)
{
	char *file_roller_cmd;
	GtkWidget *error_dialog;

	file_roller_cmd = g_find_program_in_path ("file-roller");
	
	if (strlen (gtk_entry_get_text(GTK_ENTRY(ui->pack_entry))) != 0)
	{
		GList *l;
		GString *cmd, *tmp;
		char *pack_type, *tmp_dir, *tmp_work_dir, *packed_file;

		tmp_dir = g_strdup_printf ("%s/nautilus-sendto-%s", 
				   g_get_tmp_dir(), g_get_user_name());	
		g_mkdir (tmp_dir, 0700);
		tmp_work_dir = g_strdup_printf ("%s/nautilus-sendto-%s/%li",
						g_get_tmp_dir(), g_get_user_name(),
						time(NULL));
		g_mkdir (tmp_work_dir, 0700);
		g_free (tmp_dir);

		switch (gtk_combo_box_get_active (GTK_COMBO_BOX(ui->pack_combobox)))
		{
		case 0:
			pack_type = g_strdup (".zip");
			break;
		case 1:
			pack_type = g_strdup (".tar.gz");
			break;
		case 2: 
			pack_type = g_strdup (".tar.bz2");
			break;
		default:
			pack_type = NULL;
			g_assert_not_reached ();
		}

		gconf_client_set_int(gconf_client, 
				NAUTILUS_SENDTO_LAST_COMPRESS, 
				gtk_combo_box_get_active(GTK_COMBO_BOX(ui->pack_combobox)), 
				NULL);

		cmd = g_string_new ("");
		g_string_printf (cmd, "%s --add-to=\"%s/%s%s\"",
				 file_roller_cmd, tmp_work_dir,
				 gtk_entry_get_text (GTK_ENTRY(ui->pack_entry)),
				 pack_type);

		/* file-roller doesn't understand URIs */
		for (l = file_list ; l; l=l->next){
			char *file;

			file = g_filename_from_uri (l->data, NULL, NULL);
			g_string_append_printf (cmd," \"%s\"", file);
			g_free (file);
		}

		g_spawn_command_line_sync (cmd->str, NULL, NULL, NULL, NULL);
		g_string_free (cmd, TRUE);
		tmp = g_string_new("");
		g_string_printf (tmp,"%s/%s%s", tmp_work_dir,
 				 gtk_entry_get_text (GTK_ENTRY(ui->pack_entry)),
				 pack_type);
		g_free (tmp_work_dir);
		packed_file = g_filename_to_uri (tmp->str, NULL, NULL);
		g_string_free(tmp, TRUE);
		return packed_file;
	}else{
		error_dialog = gtk_message_dialog_new (GTK_WINDOW(ui->dialog),
						       GTK_DIALOG_MODAL,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_CLOSE,
						       _("You don't insert the package name"));
		gtk_dialog_run (GTK_DIALOG (error_dialog));
		gtk_widget_destroy (error_dialog);
		return NULL;
	}
	
}

static gboolean
status_label_clear (gpointer data)
{
	NS_ui *ui = (NS_ui *) data;
	gtk_label_set_label (GTK_LABEL (ui->status_label), " ");
	gtk_widget_hide (ui->status_image);

	return FALSE;
}

static void
send_button_cb (GtkWidget *widget, gpointer data)
{
	NS_ui *ui = (NS_ui *) data;
	gchar *f, *error;
	NstPlugin *p;
	GtkWidget *w;

	gtk_widget_set_sensitive (ui->dialog, FALSE);
	while (gtk_events_pending ())
		gtk_main_iteration ();

	p = (NstPlugin *) g_list_nth_data (plugin_list, option);
	w = (GtkWidget *) g_list_nth_data (ui->contact_widgets, option);

	if (ui->status_timeoutid != 0) {
		g_source_remove (ui->status_timeoutid);
		ui->status_timeoutid = 0;
		status_label_clear (ui->status_label);
	}

	if (p == NULL)
		return;

	if (p->info->validate_destination != NULL) {
		error = NULL;
		if (p->info->validate_destination (p, w, &error) == FALSE) {
			char *message;

			message = g_strdup_printf ("<b>%s</b>", error);
			g_free (error);
			gtk_label_set_markup (GTK_LABEL (ui->status_label), message);
			g_free (message);
			ui->status_timeoutid = g_timeout_add (NAUTILUS_SENDTO_STATUS_LABEL_TIMEOUT,
							      status_label_clear,
							      ui);
			gtk_widget_show (ui->status_image);
			gtk_widget_set_sensitive (ui->dialog, TRUE);
			return;
		}
	}

	gconf_client_set_string (gconf_client, 
				NAUTILUS_SENDTO_LAST_MEDIUM, p->info->id, NULL);

	if (force_user_to_compress){
		f = pack_files (ui);
		if (f != NULL){
			GList *packed_file = NULL;		
			packed_file = g_list_append (packed_file, f);
			if (!p->info->send_files (p, w, packed_file)) {
				g_list_free (packed_file);
				return;
			}
			g_list_free (packed_file);
		}else{
			return;
		}
	}else{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->pack_checkbutton))){
			f = pack_files (ui);
			if (f != NULL){
				GList *packed_file = NULL;
				packed_file = g_list_append (packed_file, f);
				if (!p->info->send_files (p, w, packed_file)) {
					g_list_free (packed_file);
					return;
				}
				g_list_free (packed_file);
			}else{
				return;
			}
		}else{
			if (!p->info->send_files (p, w, file_list)) {
				g_list_free (file_list);
				file_list = NULL;
				return;
			}
			g_list_free (file_list);
			file_list = NULL;
		}
	}
	destroy_dialog (NULL,NULL);
}

static void
send_if_no_pack_cb (GtkWidget *widget, gpointer data)
{
	NS_ui *ui = (NS_ui *) data;

	if (force_user_to_compress || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton))) {
		if (force_user_to_compress) {
			gtk_widget_grab_focus (ui->pack_entry);
		} else {
			gtk_widget_grab_focus (ui->pack_checkbutton);
		}
	} else {
		send_button_cb (widget, data);
	}
}

static void
toggle_pack_check (GtkWidget *widget, gpointer data )
{
	GtkToggleButton *t = GTK_TOGGLE_BUTTON (widget);
	NS_ui *ui_x = (NS_ui *) data ;
	gint toogle ;

	toogle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t));
	gtk_widget_set_sensitive (ui_x->pack_combobox, toogle);
	gtk_widget_set_sensitive (ui_x->pack_entry, toogle);
}

static void
option_changed (GtkComboBox *cb, gpointer data){
	NS_ui *ui = (NS_ui *) data ;
	GList *aux;	

	aux = g_list_nth (ui->contact_widgets, option);
	option = gtk_combo_box_get_active (GTK_COMBO_BOX(cb));
	gtk_widget_hide ((GtkWidget *) aux->data);
	aux = g_list_nth (ui->contact_widgets, option);
	gtk_widget_show ((GtkWidget *) aux->data);
}

static void
set_contact_widgets (NS_ui *ui){
	GList *aux ;
	GtkWidget *w;
	NstPlugin *p;

	ui->contact_widgets = NULL;

	for (aux = plugin_list; aux; aux = aux->next){
		p = (NstPlugin *) aux->data;
		w = p->info->get_contacts_widget(p);
		gtk_box_pack_end_defaults (GTK_BOX(ui->hbox_contacts_ws),w);
		gtk_widget_hide (GTK_WIDGET(w));
		ui->contact_widgets = g_list_append (ui->contact_widgets, w);
		if (GTK_IS_ENTRY (w)) {
			g_signal_connect (G_OBJECT (w), "activate",
					G_CALLBACK (send_if_no_pack_cb), ui);
		}
	}
}

static void
set_model_for_options_combobox (NS_ui *ui){
	GdkPixbuf *pixbuf;
        GtkTreeIter iter;
        GtkTreeStore *model;
	GtkIconTheme *it;
	GtkCellRenderer *renderer;
	GList *aux;
	NstPlugin *p;
	gchar *last_used = NULL;
	int i = 0;

	it = gtk_icon_theme_get_default ();

	model = gtk_tree_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	last_used = gconf_client_get_string (gconf_client,
			NAUTILUS_SENDTO_LAST_MEDIUM, NULL);

	for (aux = plugin_list; aux; aux = aux->next){
		p = (NstPlugin *) aux->data;
		pixbuf = gtk_icon_theme_load_icon (it, p->info->icon, 16, 
						   GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
		gtk_tree_store_append (model, &iter, NULL);
		gtk_tree_store_set (model, &iter,
					0, pixbuf,
					1, p->info->description,
					-1);
		if (last_used != NULL && !strcmp(last_used, p->info->id))
			option = i;
		i++;
	}
	g_free(last_used);

	gtk_combo_box_set_model (GTK_COMBO_BOX(ui->options_combobox),
				GTK_TREE_MODEL (model));
	renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ui->options_combobox),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ui->options_combobox), 
					renderer,
                                        "pixbuf", 0,
                                        NULL);		
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ui->options_combobox),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ui->options_combobox), 
					renderer,
                                        "text", 1,
                                        NULL);

	g_signal_connect (G_OBJECT (ui->options_combobox), "changed",
			  G_CALLBACK (option_changed), ui);

	gtk_combo_box_set_active (GTK_COMBO_BOX (ui->options_combobox), option);
}

static void
nautilus_sendto_create_ui (void)
{
	GladeXML *app;	
	gint toggle;
	NS_ui *ui;
	gboolean one_file = FALSE;

	app = glade_xml_new (GLADEDIR "/" "nautilus-sendto.glade", NULL, NULL);

	ui = g_new0 (NS_ui, 1);

	ui->hbox_contacts_ws = glade_xml_get_widget (app, "hbox_contacts_widgets");
	ui->options_combobox = glade_xml_get_widget (app, "options_combobox");
	ui->dialog = glade_xml_get_widget (app, "nautilus_sendto_dialog");
	ui->cancel_button = glade_xml_get_widget (app, "cancel_button");
	ui->send_button = glade_xml_get_widget (app, "send_button");
	ui->pack_combobox = glade_xml_get_widget (app, "pack_combobox");	
	ui->pack_entry = glade_xml_get_widget (app, "pack_entry");
	ui->pack_checkbutton = glade_xml_get_widget (app, "pack_checkbutton");
	ui->status_label = glade_xml_get_widget (app, "status_label");
	ui->status_image = glade_xml_get_widget (app, "status_image");

 	gtk_combo_box_set_active (GTK_COMBO_BOX(ui->pack_combobox), 
 		gconf_client_get_int(gconf_client, 
 				NAUTILUS_SENDTO_LAST_COMPRESS, NULL));
 

	if (file_list != NULL && file_list->next != NULL)
		one_file = FALSE;
	else if (file_list != NULL)
		one_file = TRUE;
	
	gtk_entry_set_text (GTK_ENTRY (ui->pack_entry), _("Files"));

	if (one_file) {
		char *filepath = NULL, *filename = NULL;

		filepath = g_filename_from_uri ((gchar *)file_list->data,
				NULL, NULL);

		if (filepath != NULL)
			filename = g_path_get_basename (filepath);
		if (filename != NULL && filename[0] != '\0')
			gtk_entry_set_text (GTK_ENTRY (ui->pack_entry), filename);

		g_free (filename);
		g_free (filepath);
	} else {
		char *filename = get_filename_from_list ();
		if (filename != NULL && filename[0] != '\0') {
			gtk_entry_set_text (GTK_ENTRY (ui->pack_entry),
					filename);
		}
		g_free (filename);
	}

	set_contact_widgets (ui);
	set_model_for_options_combobox (ui);
	g_signal_connect (G_OBJECT (ui->dialog), "destroy",
                          G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->cancel_button), "clicked",
			  G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->send_button), "clicked",
			  G_CALLBACK (send_button_cb), ui);
	g_signal_connect (G_OBJECT (ui->pack_entry), "activate",
			  G_CALLBACK (send_button_cb), ui);

	if (force_user_to_compress == FALSE){
		toggle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton));
		gtk_widget_set_sensitive (ui->pack_combobox, toggle);
		gtk_widget_set_sensitive (ui->pack_entry, toggle);
		g_signal_connect (G_OBJECT (ui->pack_checkbutton), "toggled",
				  G_CALLBACK (toggle_pack_check), ui);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton), TRUE);
		gtk_widget_set_sensitive (ui->pack_checkbutton, FALSE);
	}

	gtk_widget_show (ui->dialog);

}

static gboolean
nautilus_sendto_plugin_init (void)
{
	GDir *dir;
	const char *item;
	NstPlugin *p = NULL;
	gboolean (*nst_init_plugin)(NstPlugin *p);
	GError *err = NULL;

	dir = g_dir_open (PLUGINDIR, 0, &err);

	if (dir == NULL) {
		g_warning ("Can't open the plugins dir: %s", err ? err->message : "No reason");
		if (err)
			g_error_free (err);
		return FALSE;
	} else {
		while ((item = g_dir_read_name(dir))) {
			if (g_str_has_suffix (item, SOEXT)) {
				char *module_path;

				p = g_new0(NstPlugin, 1);
				module_path = g_module_build_path (PLUGINDIR, item);
				p->module = g_module_open (module_path, G_MODULE_BIND_LAZY);
			        if (!p->module) {
                			g_warning ("error opening %s: %s", module_path, g_module_error ());
					g_free (module_path);
					continue;
				}
				g_free (module_path);

				if (!g_module_symbol (p->module, "nst_init_plugin", (gpointer *) &nst_init_plugin)) {
			                g_warning ("error: %s", g_module_error ());
					g_module_close (p->module);
					continue;
				}

				nst_init_plugin (p);
				if (p->info->init(p)) {
					plugin_list = g_list_append (plugin_list, p);
				} else {
					if (!p->info->never_unload)
						g_module_close (p->module);
					g_free (p);
				}
			}
		}
		g_dir_close (dir);
	}
	return g_list_length (plugin_list) != 0;
}

static char *
escape_ampersands (const char *url)
{
	int i;
	char *str, *ptr;

	/* Count the number of ampersands */
	i = 0;
	ptr = (char *) url;
	while ((ptr = strchr (ptr, '&')) != NULL) {
		i++;
		ptr++;
	}

	/* No ampersands ? */
	if (i == 0)
		return NULL;

	/* Replace the '&' */
	str = g_malloc0 (strlen (url) - i + 3 * i + 1);
	ptr = str;
	for (i = 0; url[i] != '\0'; i++) {
		if (url[i] != '&') {
			*ptr++ = url[i];
		} else {
			*ptr++ = '%';
			*ptr++ = '2';
			*ptr++ = '6';
		}
	}

	return str;
}

static void
nautilus_sendto_init (void)
{
	int i;

	if (g_module_supported() == FALSE)
		g_error ("Could not initialize gmodule support");

	if (default_url == NULL) {
		default_url = g_get_current_dir ();
	}

	for (i = 0; filenames != NULL && filenames[i] != NULL; i++) {
		const char *filename;
 		char *path;

		filename = filenames[i];
 
 		if (g_str_has_prefix (filename, "file://")) {
 			file_list = g_list_prepend (file_list,
 					g_strdup (filename));
 			path = g_filename_from_uri (filename, NULL, NULL);
 			if (path != NULL
 			    && g_file_test (path, G_FILE_TEST_IS_DIR)) {
 				force_user_to_compress = TRUE;
 			}
 			g_free (path);
 		} else {
 			char *uri, *escaped;

 			if (filename[0] != G_DIR_SEPARATOR) {
 				path = g_build_filename (default_url,
						filename, NULL);
			} else {
				path = g_strdup (filename);
			}
 
 			uri = g_filename_to_uri (path, NULL, NULL);
			escaped = escape_ampersands (uri);
			if (escaped) {
				file_list = g_list_prepend (file_list, escaped);
				g_free (uri);
			} else {
				file_list = g_list_prepend (file_list, uri);
			}
 
 			if (g_file_test (path, G_FILE_TEST_IS_DIR))
 				force_user_to_compress = TRUE;
 
 			g_free (path);
 		}
	}

	if (file_list == NULL) {
		g_print (_("Expects URIs or filenames to be passed as options\n"));
		exit (1);
	}

	file_list = g_list_reverse (file_list);
}

int main (int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);
	context = g_option_context_new (_("Nautilus Sendto"));
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print (_("Could not parse command-line options: %s\n"), error->message);
		g_error_free (error);
		return 1;
	}

	gconf_client = gconf_client_get_default();
	nautilus_sendto_init ();
	if (nautilus_sendto_plugin_init () == FALSE) {
		GtkWidget *error_dialog;

		error_dialog =
			gtk_message_dialog_new (NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						_("Could not load any plugins."));
		gtk_message_dialog_format_secondary_text
			(GTK_MESSAGE_DIALOG (error_dialog),
			 _("Please verify your installation"));

		gtk_window_set_title (GTK_WINDOW (error_dialog), ""); /* as per HIG */
		gtk_container_set_border_width (GTK_CONTAINER (error_dialog), 5);
		gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
						 GTK_RESPONSE_OK);
		gtk_dialog_run (GTK_DIALOG (error_dialog));
		return 1;
	}
	nautilus_sendto_create_ui ();
			
	gtk_main ();
	g_object_unref(gconf_client);

	return 0;
}

