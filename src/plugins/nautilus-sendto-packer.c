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
 * Author:  Maxim Ermilov <ermilov.maxim@gmail.com>
 */

#include "config.h"

#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include "nautilus-sendto-packer.h"

#define NAUTILUS_SENDTO_LAST_COMPRESS	"last-compress"

struct NstPackWidgetPrivate {
	GtkWidget *pack_combobox;
	GtkWidget *pack_entry;
	GtkWidget *pack_checkbutton;
	gboolean can_send;
};

G_DEFINE_TYPE (NstPackWidget, nst_pack_widget, GTK_TYPE_VBOX)
#define NST_PACK_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NST_TYPE_PACK_WIDGET, NstPackWidgetPrivate))

static void pack_entry_changed_cb (GObject *object, GParamSpec *spec, NstPackWidget *widget);
static void toggle_pack_check (GtkWidget *toggle, NstPackWidget *widget);

enum {
	PROP_0,
	PROP_CAN_SEND
};

static char *
get_filename_from_list (GList *file_list)
{
	GList *l;
	GString *common_part = NULL;
	gboolean matches = TRUE;
	guint offset = 0;
	const char *encoding;
	gboolean use_utf8 = TRUE;

	encoding = g_getenv ("G_FILENAME_ENCODING");

	if (encoding != NULL && strcasecmp (encoding, "UTF-8") != 0)
		use_utf8 = FALSE;

	if (file_list == NULL)
		return NULL;

	common_part = g_string_new ("");

	while (TRUE) {
		gunichar cur_char = '\0';
		for (l = file_list; l ; l = l->next) {
			char *path = NULL, *name = NULL;
			char *offset_name = NULL;

			path = g_filename_from_uri ((char *) l->data, NULL, NULL);
			if (!path)
				break;

			name = g_path_get_basename (path);

			if (!use_utf8) {
				char *tmp;

				tmp = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
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
		if (matches == TRUE &&
		    cur_char != '\0' &&
		    cur_char != '-' &&
		    cur_char != '_') {
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

static void
nst_pack_widget_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	NstPackWidget *widget;

	widget = NST_PACK_WIDGET (object);

	switch (property_id) {
	case PROP_CAN_SEND:
		g_value_set_boolean (value, widget->priv->can_send);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
nst_pack_widget_class_init (NstPackWidgetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = nst_pack_widget_get_property;

	g_object_class_install_property (object_class, PROP_CAN_SEND,
					 g_param_spec_boolean ("can-send", "Can send", "Whether the text entry is filled if compression is active.",
							       TRUE, G_PARAM_READABLE));

	g_type_class_add_private (klass, sizeof (NstPackWidgetPrivate));
}

static void
nst_pack_widget_init (NstPackWidget *self)
{
	GtkBuilder *builder;
	GError *error = NULL;
	GtkWidget *vbox;
	NstPackWidgetPrivate *priv;
	GSettings *settings;
	const char *ui_file;

	self->priv = NST_PACK_WIDGET_GET_PRIVATE (self);
	priv = self->priv;

	builder = gtk_builder_new ();
	if (g_getenv ("NST_RUN_FROM_BUILDDIR") != NULL)
		ui_file = "../data/pack-entry.ui";
	else
		ui_file = UIDIR "/" "pack-entry.ui";

	if (!gtk_builder_add_from_file (builder, ui_file, &error)) {
		g_warning ("Couldn't load builder file '%s': %s", ui_file, error->message);
		g_error_free (error);
	}

	vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox4"));
	priv->pack_combobox = GTK_WIDGET (gtk_builder_get_object (builder, "pack_combobox"));
	priv->pack_entry = GTK_WIDGET (gtk_builder_get_object (builder, "pack_entry"));
	priv->pack_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "pack_checkbutton"));

	gtk_entry_set_text (GTK_ENTRY (priv->pack_entry), _("Files"));

	g_signal_connect (G_OBJECT (priv->pack_entry), "notify::text",
			  G_CALLBACK (pack_entry_changed_cb), self);
	g_signal_connect (G_OBJECT (priv->pack_checkbutton), "toggled",
			  G_CALLBACK (toggle_pack_check), self);
	nst_pack_widget_set_enabled (self, FALSE);
	/* Because the default is FALSE, and we're not changing the value */
	toggle_pack_check (priv->pack_checkbutton, self);

	settings = g_settings_new ("org.gnome.Nautilus.Sendto");
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->pack_combobox),
				  g_settings_get_int (settings,
						      NAUTILUS_SENDTO_LAST_COMPRESS));
	g_object_unref (settings);

	gtk_container_add (GTK_CONTAINER (self), vbox);
}

void
nst_pack_widget_set_enabled (NstPackWidget *widget,
			     gboolean       enabled)
{
	g_return_if_fail (NST_PACK_WIDGET (widget));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->priv->pack_checkbutton), enabled);
	/* The toggled callback will take care of making the entry unsensitive */
}

gboolean
nst_pack_widget_get_enabled (NstPackWidget *widget)
{
	g_return_val_if_fail (NST_PACK_WIDGET (widget), FALSE);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget->priv->pack_checkbutton));
}

void
nst_pack_widget_set_force_enabled (NstPackWidget *widget,
				   gboolean       force_enabled)
{
	g_return_if_fail (NST_PACK_WIDGET (widget));

	if (force_enabled) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->priv->pack_checkbutton), TRUE);
		gtk_widget_set_sensitive (widget->priv->pack_checkbutton, FALSE);
	} else {
		gtk_widget_set_sensitive (widget->priv->pack_checkbutton, TRUE);
	}
}

gboolean
nst_pack_widget_get_force_enabled (NstPackWidget *widget)
{
	g_return_val_if_fail (NST_PACK_WIDGET (widget), FALSE);

	return gtk_widget_get_sensitive (widget->priv->pack_checkbutton);
}

void
nst_pack_widget_set_from_names (NstPackWidget *widget,
				GList         *file_list)
{
	NstPackWidgetPrivate *priv;
	gboolean one_file;

	priv = widget->priv;

	if (file_list != NULL && file_list->next != NULL)
		one_file = FALSE;
	else if (file_list != NULL)
		one_file = TRUE;

	if (one_file) {
		char *filepath, *filename;

		filepath = g_filename_from_uri ((char *)file_list->data,
						NULL, NULL);
		filename = NULL;

		if (filepath != NULL)
			filename = g_path_get_basename (filepath);
		if (filename != NULL && filename[0] != '\0')
			gtk_entry_set_text (GTK_ENTRY (priv->pack_entry), filename);

		g_free (filename);
		g_free (filepath);
	} else {
		char *filename;

		filename = get_filename_from_list (file_list);
		if (filename != NULL && filename[0] != '\0') {
			gtk_entry_set_text (GTK_ENTRY (priv->pack_entry),
					    filename);
		}
		g_free (filename);
	}
}

char *
nst_pack_widget_pack_files (NstPackWidget *widget,
			    GList         *file_list)
{
	char *file_roller_cmd;
	const char *filename;
	GList *l;
	GString *cmd, *tmp;
	char *pack_type, *tmp_dir, *tmp_work_dir, *packed_file;
	NstPackWidgetPrivate *priv;
	GSettings *settings;

	priv = widget->priv;
	file_roller_cmd = g_find_program_in_path ("file-roller");
	filename = gtk_entry_get_text (GTK_ENTRY (widget->priv->pack_entry));

	g_assert (filename != NULL && *filename != '\0');

	tmp_dir = g_strdup_printf ("%s/nautilus-sendto-%s",
				   g_get_tmp_dir (), g_get_user_name ());
	g_mkdir (tmp_dir, 0700);
	tmp_work_dir = g_strdup_printf ("%s/nautilus-sendto-%s/%li",
					g_get_tmp_dir (), g_get_user_name (),
					time (NULL));
	g_mkdir (tmp_work_dir, 0700);
	g_free (tmp_dir);

	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (priv->pack_combobox)))
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

	settings = g_settings_new ("org.gnome.Nautilus.Sendto");
	g_settings_set_int (settings,
			    NAUTILUS_SENDTO_LAST_COMPRESS,
			    gtk_combo_box_get_active (GTK_COMBO_BOX (priv->pack_combobox)));
	g_object_unref (settings);

	cmd = g_string_new ("");
	g_string_printf (cmd, "%s --add-to=\"%s/%s%s\"",
			 file_roller_cmd, tmp_work_dir,
			 filename,
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
			 filename,
			 pack_type);
	g_free (tmp_work_dir);
	packed_file = g_filename_to_uri (tmp->str, NULL, NULL);
	g_string_free(tmp, TRUE);

	return packed_file;
}

static void
pack_entry_set_send_enabled (NstPackWidget *widget,
			     gboolean       send_enabled)
{
	if (send_enabled != widget->priv->can_send) {
		widget->priv->can_send = send_enabled;

		g_object_notify (G_OBJECT (widget), "can-send");
	}
}

static void
toggle_pack_check (GtkWidget *toggle, NstPackWidget *widget)
{
	GtkToggleButton *t = GTK_TOGGLE_BUTTON (toggle);
	gboolean enabled, send_enabled;
	NstPackWidgetPrivate *priv;

	priv = widget->priv;
	enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t));
	gtk_widget_set_sensitive (priv->pack_combobox, enabled);
	gtk_widget_set_sensitive (priv->pack_entry, enabled);

	send_enabled = TRUE;

	if (enabled) {
		const char *filename;

		filename = gtk_entry_get_text (GTK_ENTRY (priv->pack_entry));
		if (filename == NULL || *filename == '\0')
			send_enabled = FALSE;
	}

	pack_entry_set_send_enabled (widget, send_enabled);
}

static void
pack_entry_changed_cb (GObject *object, GParamSpec *spec, NstPackWidget *widget)
{
	gboolean send_enabled;
	NstPackWidgetPrivate *priv;

	priv = widget->priv;
	send_enabled = TRUE;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->pack_checkbutton))) {
		const char *filename;

		filename = gtk_entry_get_text(GTK_ENTRY(priv->pack_entry));
		if (filename == NULL || *filename == '\0')
			send_enabled = FALSE;
	}

	pack_entry_set_send_enabled (widget, send_enabled);
}

GtkWidget *
nst_pack_widget_new (void)
{
	return g_object_new (NST_TYPE_PACK_WIDGET, NULL);
}

