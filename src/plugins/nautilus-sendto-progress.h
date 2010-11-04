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

#include <gtk/gtk.h>

#define NST_TYPE_PROGRESS_BAR         (nst_progress_bar_get_type ())
#define NST_PROGRESS_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NST_TYPE_PROGRESS_BAR, NstProgressBar))
#define NST_PROGRESS_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), NST_TYPE_PROGRESS_BAR, NstProgressBar))
#define NST_IS_PROGRESS_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NST_TYPE_PROGRESS_BAR))
#define NST_IS_PROGRESS_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NST_TYPE_PROGRESS_BAR))
#define NST_PROGRESS_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NST_TYPE_PROGRESS_BAR, NstProgressBarClass))

typedef struct NstProgressBarPrivate NstProgressBarPrivate;

typedef struct {
	GtkInfoBar parent;
	NstProgressBarPrivate *priv;
} NstProgressBar;

typedef struct {
	GtkInfoBarClass parent;
} NstProgressBarClass;

GType nst_progress_bar_get_type      (void);
GtkWidget *nst_progress_bar_new      (void);
void nst_progress_bar_set_total_size (NstProgressBar *bar,
				      guint           total_size);
void nst_progress_bar_set_uploaded   (NstProgressBar *bar,
				      guint64         uploaded);
void nst_progress_bar_set_label      (NstProgressBar *bar,
				      const char     *label);
