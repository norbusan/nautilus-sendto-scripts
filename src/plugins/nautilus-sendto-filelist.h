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
 * Author:  Bastien Nocera <hadess@hadess.net>
 */

#include <gio/gio.h>

typedef struct {
	GFile *file;
	guint64 size;
	char *mime_type;
} NstFile;

#define NST_TYPE_FILE (nst_file_get_type ())
GType nst_file_get_type (void) G_GNUC_CONST;

#define NST_TYPE_FILE_LIST         (nst_file_list_get_type ())
#define NST_FILE_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NST_TYPE_FILE_LIST, NstFileList))
#define NST_FILE_LIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), NST_TYPE_FILE_LIST, NstFileList))
#define NST_IS_FILE_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NST_TYPE_FILE_LIST))
#define NST_IS_FILE_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NST_TYPE_FILE_LIST))
#define NST_FILE_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NST_TYPE_FILE_LIST, NstFileListClass))

typedef struct NstFileListPrivate NstFileListPrivate;

typedef struct {
	GObject parent;
	NstFileListPrivate *priv;
} NstFileList;

typedef struct {
	GObjectClass parent;
} NstFileListClass;

GType nst_file_list_get_type      (void);
NstFileList *nst_file_list_new    (void);
gboolean nst_file_list_set_files  (NstFileList *list,
				   GList       *file_list);
NstFile *nst_file_list_pop_file   (NstFileList *list);
