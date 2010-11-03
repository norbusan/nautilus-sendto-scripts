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
#include "nst-plugin-marshal.h"
#include "nautilus-sendto-filelist.h"

static gpointer
nst_file_copy (gpointer boxed)
{
	NstFile *origin = (NstFile *) boxed;

	NstFile *result = g_new (NstFile, 1);
	result->file = g_object_ref (origin->file);
	result->mime_type = g_strdup (origin->mime_type);
	result->size = origin->size;

	return (gpointer)result;
}

static void
nst_file_free (gpointer boxed)
{
	NstFile *obj = (NstFile *) boxed;

	g_free (obj->mime_type);
	g_object_unref (obj->file);
	g_free (obj);
}

G_DEFINE_BOXED_TYPE(NstFile, nst_file, nst_file_copy, nst_file_free)

struct NstFileListPrivate {
	GList *files; /* A list of NstFiles */
	GList *current; /* For total size computation */
	guint64 size; /* Total size of the files to upload */
};

G_DEFINE_TYPE (NstFileList, nst_file_list, G_TYPE_OBJECT)
#define NST_FILE_LIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NST_TYPE_FILE_LIST, NstFileListPrivate))

enum {
	SIGNAL_INFO_GATHERED,
	SIGNAL_LAST
};

guint signals[SIGNAL_LAST];

static void
nst_file_list_class_init (NstFileListClass *klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

	signals[SIGNAL_INFO_GATHERED] = g_signal_new ("info-gathered", G_TYPE_FROM_CLASS (gobject_class),
						      G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, 0, NULL, NULL, nst_plugin_marshal_VOID__BOOLEAN_UINT64,
						      G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_UINT64);

	g_type_class_add_private (klass, sizeof (NstFileListPrivate));
}

static void
nst_file_list_init (NstFileList *self)
{
	self->priv = NST_FILE_LIST_GET_PRIVATE (self);
}

static void get_file_info (NstFileList *list);

static void
get_file_info_cb (GObject *source_object,
		  GAsyncResult *res,
		  gpointer user_data)
{
	NstFileList *list = (NstFileList *) user_data;
	NstFileListPrivate *p = list->priv;
	GFileInfo *info;
	NstFile *file;

	info = g_file_query_info_finish (G_FILE (source_object),
					 res, NULL);
	if (info == NULL) {
		//FIXME remove file from the list
		g_message ("failed to get info, remove from list");
	}

	file = p->current->data;
	file->size = g_file_info_get_attribute_uint64 (info,
						       G_FILE_ATTRIBUTE_STANDARD_SIZE);
	file->mime_type = g_strdup (g_file_info_get_content_type (info));

	/* And onto the next file */
	p->current = p->current->next;

	get_file_info (list);
}

static void
compute_total_size (NstFile *file,
		    guint64 *total)
{
	*total += file->size;
}

static void
get_file_info (NstFileList *list)
{
	NstFileListPrivate *p = list->priv;
	NstFile *file;
	GFile *f;

	if (p->current == NULL) {
		/* Populated the metadata, so start sending files */
		p->current = NULL;
		g_list_foreach (p->files, (GFunc) compute_total_size, &p->size);

		/* Fire off the signal */
		g_signal_emit (G_OBJECT (list), signals[SIGNAL_INFO_GATHERED], 0, p->files != NULL, p->size);
		return;
	}

	/* Get our file */
	file = p->current->data;
	f = file->file;

	g_file_query_info_async (f,
				 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE","G_FILE_ATTRIBUTE_STANDARD_SIZE,
				 G_FILE_QUERY_INFO_NONE,
				 0,
				 NULL,
				 get_file_info_cb,
				 list);
}

gboolean
nst_file_list_set_files (NstFileList *list,
			 GList       *file_list)
{
	NstFileListPrivate *p;
	GList *l;

	g_return_val_if_fail (NST_IS_FILE_LIST (list), FALSE);
	g_return_val_if_fail (file_list != NULL, FALSE);

	p = list->priv;

	g_return_val_if_fail (p->files == NULL, FALSE);

	for (l = file_list; l != NULL; l = l->next) {
		const char *uri = l->data;
		GFile *f;
		NstFile *file;

		f = g_file_new_for_uri (uri);
		file = g_new0 (NstFile, 1);
		file->file = f;
		p->files = g_list_prepend (p->files, file);
	}

	p->current = p->files;
	get_file_info (list);

	return TRUE;
}

NstFile *
nst_file_list_pop_file (NstFileList *list)
{
	NstFileListPrivate *p;
	NstFile *file;

	g_return_val_if_fail (NST_IS_FILE_LIST (list), NULL);

	p = list->priv;

	if (p->files == NULL)
		return NULL;

	file = p->files->data;
	p->files = g_list_delete_link (p->files, p->files);

	return file;
}

NstFileList *
nst_file_list_new (void)
{
	return g_object_new (NST_TYPE_FILE_LIST, NULL);
}

