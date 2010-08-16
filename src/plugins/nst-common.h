/*
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
 *          Bastien Nocera <hadess@hadess.net>
 */

#include <gio/gio.h>

gboolean copy_files_to (GList *file_list, GFile *dest);

#include <gtk/gtk.h>

#define NST_TYPE_PACK_WIDGET         (nst_pack_widget_get_type ())
#define NST_PACK_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NST_TYPE_PACK_WIDGET, NstPackWidget))
#define NST_PACK_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), NST_TYPE_PACK_WIDGET, NstPackWidget))
#define NST_IS_PACK_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NST_TYPE_PACK_WIDGET))
#define NST_IS_PACK_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NST_TYPE_PACK_WIDGET))
#define NST_PACK_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NST_TYPE_PACK_WIDGET, NstPackWidgetClass))

typedef struct NstPackWidgetPrivate NstPackWidgetPrivate;

typedef struct {
	GtkVBox parent;
	NstPackWidgetPrivate *priv;
} NstPackWidget;

typedef struct {
	GtkVBoxClass parent;
} NstPackWidgetClass;

GType nst_pack_widget_get_type      (void);
GtkWidget *nst_pack_widget_new      (void);

char *nst_pack_widget_pack_files     (NstPackWidget *widget,
				      GList         *file_list);
void nst_pack_widget_set_from_names  (NstPackWidget *widget,
				      GList         *file_list);
void nst_pack_widget_set_enabled     (NstPackWidget *widget,
				      gboolean       enabled);
gboolean nst_pack_widget_get_enabled (NstPackWidget *widget);
void nst_pack_widget_set_force_enabled     (NstPackWidget *widget,
					    gboolean       force_enabled);
gboolean nst_pack_widget_get_force_enabled (NstPackWidget *widget);

