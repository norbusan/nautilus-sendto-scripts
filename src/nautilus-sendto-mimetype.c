/*
 * Copyright (C) 2010 Bastien Nocera
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

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "nautilus-sendto-mimetype.h"
#include "totem-mime-types.h"

static gboolean
is_video (const char *mimetype)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (video_mime_types); i++)
		if (g_str_equal (video_mime_types[i], mimetype))
			return TRUE;

	return FALSE;
}

char *
nst_title_from_mime_types (const char **mimetypes,
			   guint        num_files,
			   guint        num_dirs)
{
	guint i;

	if (num_dirs >= 1) {
		if (num_files == 0) {
			return g_strdup_printf (ngettext ("Sharing %d folder", "Sharing %d folders", num_dirs), num_dirs);
		}
		return g_strdup_printf (_("Sharing %d folders and files"), num_dirs + num_files);
	}

	guint num_elems = 0;
	guint num_videos = 0;
	guint num_photos = 0;
	guint num_images = 0;
	guint num_folders = 0;
	guint num_text = 0;

	for (i = 0; mimetypes[i] != NULL; i++) {
		if (is_video (mimetypes[i]))
			num_videos++;
		else if (g_content_type_is_a (mimetypes[i], "image/jpeg"))
			num_photos++;
		else if (g_str_equal (mimetypes[i], "inode/directory"))
			num_folders++;
		else if (g_content_type_is_a (mimetypes[i], "image/*"))
			num_images++;
		else if (g_content_type_is_a (mimetypes[i], "text/plain"))
			num_text++;
		num_elems++;
	}

	if (num_videos == num_elems) {
		return g_strdup_printf (ngettext ("Sharing %d video", "Sharing %d videos", num_files), num_files);
	} else if (num_photos == num_elems) {
		return g_strdup_printf (ngettext ("Sharing %d photo", "Sharing %d photos", num_files), num_files);
	} else if (num_images + num_photos == num_elems) {
		return g_strdup_printf (ngettext ("Sharing %d image", "Sharing %d images", num_files), num_files);
	} else if (num_text == num_elems) {
		return g_strdup_printf (ngettext ("Sharing %d text file", "Sharing %d text files", num_files), num_files);
	} else {
		return g_strdup_printf (ngettext ("Sharing %d file", "Sharing %d files", num_files), num_files);
	}

	return NULL;
}

