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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Author:  Roberto Majadas <roberto.majadas@openshine.com>
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <errno.h>

#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "prpl.h"
#include "notify.h"
#include "eventloop.h"
#include "gtkft.h"
#include "server.h"

#include "gtkplugin.h"
#include "pidgin.h"
#include "version.h"
#include "prefs.h"

#define PLUGIN_HOME ".gnome2/nautilus-sendto"
#define B_ONLINE "pidgin_buddies_online"
#define SAVE_TIMEOUT 5000


static guint take_spool_files_handler;
static gboolean taking_files;
static GString *buddies_str = NULL;

static char *
get_buddies_path (void)
{
	return g_build_filename (g_get_home_dir(), PLUGIN_HOME, B_ONLINE, NULL);
}

static void
get_online_buddies (PurpleBlistNode *node, GString *str){

	PurpleBlistNode *aux;

	if (node == NULL)
		return;

	for (aux = node ; aux != NULL ;aux = aux->next) {
		if (PURPLE_BLIST_NODE_IS_BUDDY(aux)){
			PurpleBuddy *buddy;
			buddy = (PurpleBuddy*)aux;
			if (PURPLE_BUDDY_IS_ONLINE(buddy)){
				char *alias;

				if (buddy->alias == NULL){
					if (buddy->server_alias == NULL){
						alias = g_strdup (buddy->name);
					}else{
						alias = g_strdup (buddy->server_alias);
					}
				}else{
					alias = g_strdup (buddy->alias);
				}

				g_string_append_printf (str,"%s\n%s\n%s\n%s\n",
							buddy->account->username,
							buddy->name,
							alias,
							buddy->account->protocol_id);

				g_free (alias);
			}
		} else {
			get_online_buddies (aux->child, str);
		}
	}
}

static void
save_online_buddies (PurpleBuddy *buddy, gpointer data)
{
	PurpleBuddyList *blist;
	GString *str;
	char *fd_name;

	fd_name = get_buddies_path ();

	blist = purple_get_blist();
	str = g_string_new ("---\n");
	get_online_buddies (blist->root, str);
	str = g_string_append (str, "---\n");

	if (!g_string_equal (buddies_str, str)) {
		GError *err = NULL;
		if (g_file_set_contents (fd_name, str->str, str->len, &err) == FALSE) {
			purple_debug_info ("nautilus", "couldn't save '%s': %s\n", fd_name, err->message);
			g_error_free (err);
			g_string_free (str, TRUE);
		} else {
			purple_debug_info ("nautilus", "saved blist online\n");
			g_string_free (buddies_str, TRUE);
			buddies_str = str;
		}
	} else {
		g_string_free (str, TRUE);
		purple_debug_info ("nautilus", "don't save blist online. No change\n");
	}
	g_free (fd_name);
}

static gboolean
init_plugin_stuff (void)
{
	char *spool_tmp;
	gboolean retval;

	spool_tmp = g_build_filename (g_get_home_dir(), PLUGIN_HOME, "spool", "tmp", NULL);
	if (g_mkdir_with_parents (spool_tmp, 0755) < 0) {
		int error = errno;
		g_warning ("Failed to create '%s': %s", spool_tmp, g_strerror (error));
		retval = FALSE;
	} else {
		retval = TRUE;
	}

	g_free (spool_tmp);

	return retval;
}

static void
send_file (GString *username, GString *cname,
	   GString *protocol, GString *file)
{
	PurpleAccount *account;

	account = purple_accounts_find (username->str, protocol->str);
	if (account == NULL)
		return;

	serv_send_file (account->gc, cname->str, file->str);
}

static void
process_file (const char *file)
{
	GIOChannel *io;
	GString *username;
	GString *cname;
	GString *protocol;
	GString *file_to_send;

	username = g_string_new ("");
	cname = g_string_new ("");
	protocol = g_string_new ("");
	file_to_send = g_string_new ("");

	io = g_io_channel_new_file (file,"r",NULL);
	if (io == NULL)
		return;

	purple_debug_info ("nautilus","Open spool file : %s\n",file);
	g_io_channel_read_line_string (io, username, NULL, NULL);
	username = g_string_truncate (username, username->len - 1);
	g_io_channel_read_line_string (io, cname, NULL, NULL);
	cname = g_string_truncate (cname, cname->len - 1);
	g_io_channel_read_line_string (io, protocol, NULL, NULL);
	protocol = g_string_truncate (protocol, protocol->len - 1);

	while (G_IO_STATUS_EOF != g_io_channel_read_line_string (io,file_to_send, NULL, NULL)) {
		if (file_to_send->len <=1)
			continue;
		file_to_send = g_string_truncate (file_to_send,
						  file_to_send->len - 1);
		send_file (username, cname,
			   protocol, file_to_send);
	}

	g_string_free (username, TRUE);
	g_string_free (cname, TRUE);
	g_string_free (protocol, TRUE);
	g_string_free (file_to_send, TRUE);
	g_io_channel_shutdown (io, TRUE, NULL);
	remove (file);
}

static int
take_spool_files (gpointer user_data)
{
	char *plugin_spool;

	if (taking_files == FALSE)
	{
		GDir *dir;
		GError *err = NULL;

		taking_files = TRUE;
		plugin_spool = g_build_filename (g_get_home_dir(), PLUGIN_HOME, "spool", NULL);
		dir = g_dir_open (plugin_spool, 0, &err);
		g_free (plugin_spool);
		if (dir == NULL) {
			purple_debug_info ("nautilus","Can't open the spool dir: %s\n", err->message);
			g_error_free (err);
		} else {
			const char *filename;

			filename = g_dir_read_name (dir);
			while (filename) {
				char *file;

				if (g_str_equal (filename, "tmp")) {
					filename = g_dir_read_name (dir);
					continue;
				}

				file = g_build_filename (g_get_home_dir(),
							 PLUGIN_HOME, "spool",
							 filename, NULL);
				process_file (file);
				g_free (file);

				filename = g_dir_read_name (dir);
			}
			g_dir_close (dir);
		}
		taking_files = FALSE;
	}
	return TRUE;
}

static gboolean
plugin_load (PurplePlugin *plugin)
{
	void *blist_handle;

	if (init_plugin_stuff () == FALSE)
		return FALSE;
	buddies_str = g_string_new ("");

	blist_handle = purple_blist_get_handle();

	purple_signal_connect (blist_handle, "buddy-signed-on",
			       plugin, (PurpleCallback) save_online_buddies,
			       NULL);
	purple_signal_connect (blist_handle, "buddy-signed-off",
			       plugin, (PurpleCallback) save_online_buddies,
			       NULL);
	taking_files = FALSE;
	take_spool_files_handler = purple_timeout_add_seconds (3, (GSourceFunc) take_spool_files, NULL);

	/* And save a list already */
	save_online_buddies (NULL, NULL);

	return TRUE;
}

static gboolean
plugin_unload (PurplePlugin *plugin)
{
	void *blist_handle;
	char *fd_name;

	blist_handle = purple_blist_get_handle();

	purple_signal_disconnect (blist_handle, "buddy-signed-on",
				  plugin, (PurpleCallback) save_online_buddies);
	purple_signal_disconnect (blist_handle, "buddy-signed-off",
				  plugin, (PurpleCallback) save_online_buddies);

	fd_name = get_buddies_path ();
	purple_timeout_remove (take_spool_files_handler);
	g_unlink (fd_name);
	g_free (fd_name);
	g_string_free (buddies_str, TRUE);
	buddies_str = NULL;
	purple_debug_info ("nautilus", "Stop nautilus plugin\n");

	return TRUE;
}

static gboolean
force_load_once (gpointer data)
{
	PurplePlugin *plugin = (PurplePlugin *)data;

	if (!purple_prefs_get_bool ("/plugins/gtk/nautilus/auto_loaded")) {
		purple_debug_info ("nautilus", "Force loading nautilus plugin\n");
		purple_plugin_load (plugin);
		purple_plugins_save_loaded (PIDGIN_PREFS_ROOT "/plugins/loaded");
		purple_prefs_set_bool ("/plugins/gtk/nautilus/auto_loaded", TRUE);
	}

	return FALSE;
}

static void 
init_plugin(PurplePlugin *plugin)
{
	purple_prefs_add_none ("/plugins/gtk/nautilus");
	purple_prefs_add_bool ("/plugins/gtk/nautilus/auto_loaded", FALSE);
	g_idle_add(force_load_once, plugin);
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,		/* api version */
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,		/* type */
	PIDGIN_PLUGIN_TYPE,			/* ui requirement */
	PURPLE_PRIORITY_DEFAULT,		/* flags */
	NULL,				/* dependencies */
	PURPLE_PRIORITY_DEFAULT,		/* priority */

	"gtk-nautilus",			/* id */
	N_("Nautilus Integration"),		/* name */
	"0.8",				/* version */
	N_("Provides integration with Nautilus"),		/* summary */ 
	N_("Provides integration with Nautilus"),		/* description */

	"Roberto Majadas <roberto.majadas@openshine.com>",	/* author */
	"www.gnome.org",                    /* homepage */

	plugin_load,			/* load */
	plugin_unload,			/* unload */
	NULL,				/* destroy */
	NULL,				/* ui info */
	NULL,				/* extra info */
	NULL				/* actions info */
};

PURPLE_INIT_PLUGIN(nautilus, init_plugin, info)

