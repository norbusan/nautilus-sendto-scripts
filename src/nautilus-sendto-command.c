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
#include <sys/types.h>
#include <dirent.h>
#include <gnome.h>
#include <glade/glade.h>
#include "nautilus-sendto-plugin.h"

static 
gchar *default_url = NULL;
gboolean force_user_to_compress = FALSE;
GList *file_list = NULL;
GList *plugin_list = NULL;
GHashTable *hash ;
guint option;

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
};

struct poptOption options[] = {
	{ "default-dir", 0, POPT_ARG_STRING, &default_url, 0,
	  N_("Default folder to use"),
	  N_("FOLDER") },
	{ NULL, '\0', 0, NULL, 0 }
};

void 
destroy_dialog (GtkWidget *widget, gpointer data )
{
        gtk_main_quit ();
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
		GString *cmd, *packed_file;
		gchar *pack_type, *tmp_dir, *tmp_work_dir;
		
		tmp_dir = g_strdup_printf ("%s/nautilus-sendto-%s", 
				   g_get_tmp_dir(), g_get_user_name());	
		mkdir (tmp_dir, 0700);
		tmp_work_dir = g_strdup_printf ("%s/nautilus-sendto-%s/%i", 
						g_get_tmp_dir(), g_get_user_name(),
						time(NULL));
		mkdir (tmp_work_dir, 0700);
		g_free(tmp_dir);

		switch (gtk_combo_box_get_active(GTK_COMBO_BOX(ui->pack_combobox)))
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
		}

		cmd = g_string_new ("");
		g_string_printf (cmd, "%s --add-to=\"%s/%s%s\"",
				 file_roller_cmd, tmp_work_dir,
				 gtk_entry_get_text(GTK_ENTRY(ui->pack_entry)),
				 pack_type);

		/* file-roller doesn't understand URIs */
		for (l = file_list ; l; l=l->next){
			char *file;

			file = g_filename_from_uri (l->data, NULL, NULL);
			g_string_append_printf (cmd," \"%s\"", file);
			g_free (file);
		}

		g_printf ("%s\n", cmd->str);
		g_spawn_command_line_sync (cmd->str, NULL, NULL, NULL, NULL);
		g_string_free (cmd, TRUE);
		packed_file = g_string_new("");
		g_string_printf (packed_file,"%s/%s%s",tmp_work_dir,
 				 gtk_entry_get_text(GTK_ENTRY(ui->pack_entry)),
				 pack_type);
		g_free (tmp_work_dir);
		return g_string_free (packed_file, FALSE);
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

static void
send_button_cb (GtkWidget *widget, gpointer data)
{
	NS_ui *ui = (NS_ui *) data;
	gchar *f;
	NstPlugin *p;
	GtkWidget *w;
	GtkWidget *error_dialog;
	
	p = (NstPlugin *) g_list_nth_data (plugin_list, option);
	w = (GtkWidget *) g_list_nth_data (ui->contact_widgets, option);
	
	if (p == NULL)
		return;
		
	if (force_user_to_compress){
		f = pack_files (ui);
		if (f != NULL){
			GList *packed_file = NULL;		
			packed_file = g_list_append (packed_file, f);
			if (!p->info->send_files (p, w, packed_file))
				return;
		}else{
			return;
		}
	}else{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->pack_checkbutton))){
			f = pack_files (ui);
			if (f != NULL){
				GList *packed_file = NULL;
				packed_file = g_list_append (packed_file, f);
				if (!p->info->send_files (p, w, packed_file))
					return;
			}else{
				return;
			}
				
		}else{
			if (!p->info->send_files (p, w, file_list))
				return;
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

void
toggle_pack_check (GtkWidget *widget, gpointer data )
{
	GtkToggleButton *t = GTK_TOGGLE_BUTTON (widget);
	NS_ui *ui_x = (NS_ui *) data ;
	gint toogle ;

	toogle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t));
	gtk_widget_set_sensitive (ui_x->pack_combobox, toogle);
	gtk_widget_set_sensitive (ui_x->pack_entry, toogle);
}

void
option_changed (GtkComboBox *cb, gpointer data){
	NS_ui *ui = (NS_ui *) data ;
	GList *aux;	

	aux = g_list_nth (ui->contact_widgets, option);
	option = gtk_combo_box_get_active (GTK_COMBO_BOX(cb));
	gtk_widget_hide ((GtkWidget *) aux->data);
	aux = g_list_nth (ui->contact_widgets, option);
	gtk_widget_show ((GtkWidget *) aux->data);
}

void
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
	if (ui->contact_widgets)
		gtk_widget_show ((GtkWidget* ) ui->contact_widgets->data);
}

void
set_model_for_options_combobox (NS_ui *ui){
	GdkPixbuf *pixbuf;
        GtkTreeIter iter;
        GtkTreeStore *model;
	GtkIconTheme *it;
	GtkCellRenderer *renderer;
	GList *aux;
	NstPlugin *p;
	
	it = gtk_icon_theme_get_default ();

	model = gtk_tree_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	for (aux = plugin_list; aux; aux = aux->next){
		p = (NstPlugin *) aux->data;
		pixbuf = gtk_icon_theme_load_icon (it, p->info->icon, 16, 
						   GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
		gtk_tree_store_append (model, &iter, NULL);
		gtk_tree_store_set (model, &iter,
					0, pixbuf,
					1, p->info->description,
					-1);
	}
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
	gtk_combo_box_set_active (GTK_COMBO_BOX (ui->options_combobox), 0);
	option = 0;
}

static void
nautilus_sendto_create_ui (void)
{
	GladeXML *app;	
	gint toggle;
	NS_ui *ui;

	if (force_user_to_compress == FALSE)
		app = glade_xml_new (GLADEDIR "/" "nautilus-sendto.glade", NULL, NULL);
	else
		app = glade_xml_new (GLADEDIR "/" "nautilus-sendto-force.glade", NULL, NULL);

	ui = g_new0 (NS_ui, 1);

	ui->hbox_contacts_ws = glade_xml_get_widget (app, "hbox_contacts_widgets");
	ui->options_combobox = glade_xml_get_widget (app, "options_combobox");
	ui->dialog = glade_xml_get_widget (app, "nautilus_sendto_dialog");
	ui->cancel_button = glade_xml_get_widget (app, "cancel_button");
	ui->send_button = glade_xml_get_widget (app, "send_button");
	ui->pack_combobox = glade_xml_get_widget (app, "pack_combobox");	
	ui->pack_entry = glade_xml_get_widget (app, "pack_entry");
	
		
	gtk_combo_box_set_active (GTK_COMBO_BOX(ui->pack_combobox), 0);
	gtk_entry_set_text (GTK_ENTRY (ui->pack_entry), _("Files"));
/*	create_entry_completion (ui->entry); */
	set_contact_widgets (ui);
	set_model_for_options_combobox (ui);
	g_signal_connect (G_OBJECT (ui->options_combobox), "changed",
                          G_CALLBACK (option_changed), ui);
	g_signal_connect (G_OBJECT (ui->dialog), "destroy",
                          G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->cancel_button), "clicked",
			  G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->send_button), "clicked",
			  G_CALLBACK (send_button_cb), ui);
	g_signal_connect (G_OBJECT (ui->pack_entry), "activate",
			  G_CALLBACK (send_button_cb), ui);

	if (force_user_to_compress == FALSE){
		ui->pack_checkbutton = glade_xml_get_widget (app, "pack_checkbutton");
		toggle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton));
		gtk_widget_set_sensitive (ui->pack_combobox, toggle);
		gtk_widget_set_sensitive (ui->pack_entry, toggle);
		g_signal_connect (G_OBJECT (ui->pack_checkbutton), "toggled",
				  G_CALLBACK (toggle_pack_check), ui);
	}
	gtk_widget_show (ui->dialog);

}

static void
nautilus_sendto_plugin_init (void)
{
	GDir *dir;
	const char *item;
	NstPlugin *p = NULL;
	gboolean (*nst_init_plugin)(NstPlugin *p);
	GError *err = NULL;

	dir = g_dir_open (PLUGINDIR, 0, &err);

	if (dir == NULL){
		g_error ("Can't open the plugins dir: %s", err ? err->message : "No reason");
		if (err)
			g_error_free (err);
	}else{
		while (item = g_dir_read_name(dir)){
			if (g_str_has_suffix (item, SOEXT)){
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

				if (!g_module_symbol (p->module, "nst_init_plugin", (gpointer *)&nst_init_plugin)) {
			                g_warning ("error: %s", g_module_error ());
					g_module_close (p->module);
					continue;
				}

				nst_init_plugin (p);
				if (p->info->init(p)){
					plugin_list = g_list_append (plugin_list, p);
				}else{
					if (!p->info->never_unload)
						g_module_close (p->module);
					g_free (p);
				}
			}
		}
		g_dir_close (dir);
	}	
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
nautilus_sendto_init (GnomeProgram *program, int argc, char **argv)
{
	poptContext pctx;
	GValue value = { 0 };
	const char *filename;

	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT,
                               g_value_init (&value, G_TYPE_POINTER));
	
	pctx = g_value_get_pointer (&value);
	glade_gnome_init ();

	if (g_module_supported() == FALSE)
		g_error ("Could not initialize gmodule support");

	if (default_url == NULL) {
		default_url = g_get_current_dir ();
	}

	while ((filename = poptGetArg (pctx)) != NULL) {
 		char *path;
 
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
		poptPrintHelp (pctx, stdout, 0);
		exit (1);
	}

	file_list = g_list_reverse (file_list);
}

int main (int argc, char **argv)
{
	GnomeProgram *program;
		
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
	program = gnome_program_init ("nautilus-sendto", VERSION,
				      LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_HUMAN_READABLE_NAME, _("Nautilus Sendto"),
				      NULL);

	nautilus_sendto_init (program, argc, argv);	
	nautilus_sendto_plugin_init ();
	nautilus_sendto_create_ui();
			
	gtk_main ();
	return 0;
}

