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
 * Author:  Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include "nautilus-sendto-progress.h"

struct NstProgressBarPrivate {
	GtkWidget *label;
	GtkWidget *progress_bar;
	GtkWidget *cancel;

	guint64 total_size;
	guint64 uploaded;
};

G_DEFINE_TYPE (NstProgressBar, nst_progress_bar, GTK_TYPE_INFO_BAR)
#define NST_PROGRESS_BAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NST_TYPE_PROGRESS_BAR, NstProgressBarPrivate))

enum {
	PROP_0,
	PROP_TOTAL_SIZE,
	PROP_UPLOADED,
	PROP_LABEL
};

void
nst_progress_bar_set_uploaded (NstProgressBar *bar,
			       guint64         uploaded)
{
	g_return_if_fail (NST_IS_PROGRESS_BAR (bar));

	bar->priv->uploaded = uploaded;
	if (bar->priv->total_size > 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress_bar),
					       (double) uploaded / bar->priv->total_size);
	else
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress_bar), 0);
}

void
nst_progress_bar_set_total_size (NstProgressBar *bar,
				 guint           total_size)
{
	g_return_if_fail (NST_IS_PROGRESS_BAR (bar));

	bar->priv->total_size = total_size;
	if (bar->priv->total_size > 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress_bar),
					       (double) bar->priv->uploaded / total_size);
	else
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress_bar), 0);
}

void
nst_progress_bar_set_label (NstProgressBar *bar,
			    const char     *label)
{
	g_return_if_fail (NST_IS_PROGRESS_BAR (bar));

	gtk_label_set_label (GTK_LABEL (bar->priv->label), label);
}

static void
nst_progress_bar_set_property (GObject          *object,
			       guint             property_id,
			       const GValue     *value,
			       GParamSpec       *pspec)
{
	NstProgressBar *bar;

	bar = NST_PROGRESS_BAR (object);

	switch (property_id) {
	case PROP_TOTAL_SIZE:
		nst_progress_bar_set_total_size (bar, g_value_get_uint64 (value));
		break;
	case PROP_UPLOADED:
		nst_progress_bar_set_uploaded (bar, g_value_get_uint64 (value));
		break;
	case PROP_LABEL:
		nst_progress_bar_set_label (bar, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
nst_progress_bar_get_property (GObject    *object,
			       guint       property_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	NstProgressBar *bar;

	bar = NST_PROGRESS_BAR (object);

	switch (property_id) {
	case PROP_TOTAL_SIZE:
		g_value_set_uint64 (value, bar->priv->total_size);
		break;
	case PROP_UPLOADED:
		g_value_set_uint64 (value, bar->priv->uploaded);
		break;
	case PROP_LABEL:
		g_value_set_string (value, gtk_label_get_label (GTK_LABEL (bar->priv->label)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
nst_progress_bar_class_init (NstProgressBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = nst_progress_bar_get_property;
	object_class->set_property = nst_progress_bar_set_property;

	g_object_class_install_property (object_class, PROP_TOTAL_SIZE,
					 g_param_spec_uint64 ("total-size", "Total size", "The total size of the upload.",
							      0, G_MAXUINT64, 0, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_UPLOADED,
					 g_param_spec_uint64 ("uploaded", "Uploaded", "Size uploaded so far.",
							      0, G_MAXUINT64, 0, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_LABEL,
					 g_param_spec_string ("label", "Label", "Label.",
							       NULL, G_PARAM_READWRITE));

	g_type_class_add_private (klass, sizeof (NstProgressBarPrivate));
}

static void
nst_progress_bar_init (NstProgressBar *bar)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *content;

	bar->priv = NST_PROGRESS_BAR_GET_PRIVATE (bar);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (vbox);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0); 

	bar->priv->label = gtk_label_new ("");
	gtk_widget_show (bar->priv->label);
	gtk_box_pack_start (GTK_BOX (hbox), bar->priv->label, TRUE, TRUE, 0); 
	gtk_label_set_use_markup (GTK_LABEL (bar->priv->label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0, 0.5);
	gtk_label_set_ellipsize (GTK_LABEL (bar->priv->label), 
				 PANGO_ELLIPSIZE_END);

	bar->priv->progress_bar = gtk_progress_bar_new (); 
	gtk_widget_show (bar->priv->progress_bar);
	gtk_box_pack_start (GTK_BOX (vbox), bar->priv->progress_bar, TRUE, FALSE, 0); 
	gtk_widget_set_size_request (bar->priv->progress_bar, -1, 15);

	content = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));
	gtk_container_add (GTK_CONTAINER (content), vbox);

	bar->priv->cancel = gtk_info_bar_add_button (GTK_INFO_BAR (bar),
						     _("_Cancel"), GTK_RESPONSE_CANCEL);
	gtk_widget_hide (bar->priv->cancel);
	gtk_widget_set_no_show_all (bar->priv->cancel, TRUE);
}

GtkWidget *
nst_progress_bar_new (void)
{
	return g_object_new (NST_TYPE_PROGRESS_BAR, NULL);
}

