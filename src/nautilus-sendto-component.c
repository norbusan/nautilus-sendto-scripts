/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

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

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libbonobo.h>
#include "nautilus-sendto-component.h"


static char *
get_path_from_url (const char *url)
{
	GnomeVFSURI *uri     = NULL;
	char        *escaped = NULL;
	char        *path    = NULL;
	
	uri = gnome_vfs_uri_new (url);

	if (uri != NULL)
		escaped = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);

	if (escaped != NULL)
		path = gnome_vfs_unescape_string (escaped, NULL);
	
	if (uri != NULL)
		gnome_vfs_uri_unref (uri);
	g_free (escaped);
	
	return path;
}


static void
impl_Bonobo_Listener_event (PortableServer_Servant servant,
			    const CORBA_char *event_name,
			    const CORBA_any *args,
			    CORBA_Environment *ev)
{
	NautilusSendtoComponent *nsc;
	const CORBA_sequence_CORBA_string *list;	
	char    *cmd, *current_dir, *first_path;
	GString *str;
	int      i;       

	nsc = NAUTILUS_SENDTO_COMPONENT (bonobo_object_from_servant (servant));

	if (!CORBA_TypeCode_equivalent (args->_type, TC_CORBA_sequence_CORBA_string, ev)) {
		return;
	}

	list = (CORBA_sequence_CORBA_string *)args->_value;

	g_return_if_fail (nsc != NULL);
	g_return_if_fail (list != NULL);

	first_path = get_path_from_url (list->_buffer[0]);
	current_dir = g_path_get_dirname (first_path);
	g_free (first_path);

	str = g_string_new ("nautilus-sendto");		
	g_string_append_printf (str, " --default-dir=\"%s\"", current_dir);
	
	for (i = 0; i < list->_length; i++) {
		char *path = get_path_from_url (list->_buffer[i]);
		char *file_or_dir_name = g_path_get_basename (path);
		
		g_string_append_printf (str, " \"%s\"", file_or_dir_name);
		g_free (path);
		g_free (file_or_dir_name);
	}
	g_print(">> %s\n", str->str);
	g_spawn_command_line_async (str->str, NULL);
	
	g_string_free (str, TRUE);
	g_free (current_dir);

}


/* initialize the class */
static void
nautilus_sendto_component_class_init (NautilusSendtoComponentClass *class)
{
	POA_Bonobo_Listener__epv *epv = &class->epv;
	epv->event = impl_Bonobo_Listener_event;
}


static void
nautilus_sendto_component_init (NautilusSendtoComponent *frc)
{
}


BONOBO_TYPE_FUNC_FULL (NautilusSendtoComponent, 
		       Bonobo_Listener, 
		       BONOBO_TYPE_OBJECT,
		       nautilus_sendto_component);
