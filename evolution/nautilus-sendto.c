/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the program; if not, see <http://www.gnu.org/licenses/>  
 *
 *
 * Authors:
 *		Johnny Jacob <johnnyjacob@gmail.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#include <config.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <e-util/e-config.h>
#include <e-util/e-popup.h>
#include <mail/em-popup.h>
#include <mail/em-menu.h>
#include <mail/em-utils.h>
#include <misc/e-attachment.h>

static void send_file (EPlugin *ep, EPopupTarget *t, void *data);
void org_gnome_evolution_send_file_attachments (EPlugin *ep, EMPopupTargetAttachments *t);
void org_gnome_evolution_send_file_part (EPlugin *ep, EMPopupTargetPart *t);

static void
popup_free (EPopup *ep, GSList *items, void *data)
{
	g_slist_free (items);
}

static EPopupItem popup_attachment_items[] = {
	{ E_POPUP_BAR, "25.display.00"},
	{ E_POPUP_ITEM, "25.display.01", N_("_Send to..."), (EPopupActivateFunc)send_file, NULL, "document-send"}
};

void org_gnome_evolution_send_file_attachments (EPlugin *ep, EMPopupTargetAttachments *t)
{
	GSList *menus = NULL;
	int len = 0;

	g_message ("org_gnome_evolution_send_file_attachments called");

	len = g_slist_length(t->attachments);

	if (len != 1)
		return;

	menus = g_slist_prepend (menus, &popup_attachment_items[0]);
	menus = g_slist_prepend (menus, &popup_attachment_items[1]);
	e_popup_add_items (t->target.popup, menus, GETTEXT_PACKAGE, popup_free, t);
}

void org_gnome_evolution_send_file_part (EPlugin *ep, EMPopupTargetPart *t)
{
	GSList *menus = NULL;

	g_message ("org_gnome_evolution_send_file_attachments called");

	menus = g_slist_prepend (menus, &popup_attachment_items[0]);
	menus = g_slist_prepend (menus, &popup_attachment_items[1]);
	e_popup_add_items (t->target.popup, menus, GETTEXT_PACKAGE, popup_free, t);
}

static void
send_file (EPlugin *ep, EPopupTarget *t, void *data)
{
	CamelMimePart *part;
	char *path;
	EPopupTarget *target = (EPopupTarget *) data;
	GPtrArray *argv;
	gboolean ret;
	GError *err = NULL;

	if (target->type == EM_POPUP_TARGET_ATTACHMENTS)
		part = ((EAttachment *) ((EMPopupTargetAttachments *) target)->attachments->data)->body;
	else
		part = ((EMPopupTargetPart *) target)->part;

	path = em_utils_temp_save_part (NULL, part, FALSE);
	g_message ("saved part as %s", path);

	argv = g_ptr_array_new ();
	g_ptr_array_add (argv, "nautilus-sendto");
	g_ptr_array_add (argv, path);
	g_ptr_array_add (argv, NULL);

	ret = g_spawn_async (NULL, (gchar **) argv->pdata,
			     NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
	g_ptr_array_free (argv, TRUE);

	if (ret == FALSE) {
		g_warning ("Couldn't send the attachment: %s", err->message);
		g_error_free (err);
	}

	g_free (path);
}

