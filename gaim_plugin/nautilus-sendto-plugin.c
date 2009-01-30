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

#ifndef GAIM_PLUGINS
#define GAIM_PLUGINS
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "prpl.h"
#include "notify.h"
#include "eventloop.h"
#include "gtkft.h"
#include "server.h"

#include "gtkplugin.h"
#include "gaim.h"
#include "version.h"
#include "prefs.h"

#define PLUGIN_HOME ".gnome2/nautilus-sendto"
#define B_ONLINE "buddies_online"
#define SAVE_TIMEOUT 5000


static 
guint save_blist_timeout_handler, take_spool_files_handler;
gboolean taking_files;
GString *buddies_str;

void
get_online_buddies (GaimBlistNode *node, GString *str){

    GaimBlistNode *aux;

    if (node == NULL)
	return;
    for (aux = node ; aux != NULL ;aux = aux->next){
	if (GAIM_BLIST_NODE_IS_BUDDY(aux)){
	    GaimBuddy *buddy;
	    buddy = (GaimBuddy*)aux;
	    if (GAIM_BUDDY_IS_ONLINE(buddy)){
		gchar *alias;

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
	}else{
	    get_online_buddies (aux->child, str);
	}
    }

}

gint
save_online_buddies (){
    GaimBuddyList *blist;
    GString *str;
    gchar *fd_name;
    FILE *fd;

    fd_name = g_build_path ("/", g_get_home_dir(), PLUGIN_HOME, 
			    B_ONLINE, NULL);

    blist = gaim_get_blist();
    str = g_string_new ("---\n");
    get_online_buddies (blist->root, str);       
    str = g_string_append (str, "---\n");

    if (!g_string_equal (buddies_str, str)){
	fd = fopen (fd_name, "w");
	if (fd){
	    fwrite (str->str, 1, str->len, fd);
	    fclose (fd);
	    g_string_free (buddies_str, TRUE);
	    buddies_str = str;
	    gaim_debug_info ("nautilus", "save blist online\n");
	}else{
	    g_string_free (str, TRUE);
	    gaim_debug_info ("nautilus", "don't save blist online. No change\n");
	}
	g_free (fd_name);
    }else{
	g_string_free (str, TRUE);
	gaim_debug_info ("nautilus", "don't save blist online. No change\n");
    }
    
    return TRUE;
}

void
init_plugin_stuff (){
    gchar *plugin_home;
    gchar *spool;
    gchar *spool_tmp;
    plugin_home = g_build_path ("/", g_get_home_dir(), 
				PLUGIN_HOME, NULL);
    
    if (!g_file_test (plugin_home,G_FILE_TEST_IS_DIR)){
	mkdir (plugin_home, 0755);
	gaim_debug_info ("nautilus", "creating: %s\n",plugin_home);
    }
    g_free (plugin_home);
    spool = g_build_path ("/", g_get_home_dir(), PLUGIN_HOME, "spool", NULL);
    if (!g_file_test (spool,G_FILE_TEST_IS_DIR)){
	mkdir (spool, 0755);
	gaim_debug_info ("nautilus", "creating: %s\n", spool);
    }
    g_free (spool);
    spool_tmp = g_build_path ("/", g_get_home_dir(), PLUGIN_HOME, "spool", "tmp", NULL);
    if (!g_file_test (spool_tmp,G_FILE_TEST_IS_DIR)){
	mkdir (spool_tmp, 0755);
	gaim_debug_info ("nautilus", "creating: %s\n", spool_tmp);
    }
    g_free (spool_tmp);
}

void
send_file (GString *username, GString *cname,
	   GString *protocol, GString *file){

    GaimAccount *account;
    GaimXfer *xfer;

    account = gaim_accounts_find (username->str, protocol->str);
    if (account == NULL)
	return;

    serv_send_file (account->gc, cname->str, file->str);
}

void
process_file (gchar *file){
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

    gaim_debug_info ("nautilus","Open spool file : %s\n",file);
    g_io_channel_read_line_string (io, username, NULL, NULL);
    username = g_string_truncate (username, username->len - 1);
    g_io_channel_read_line_string (io, cname, NULL, NULL);
    cname = g_string_truncate (cname, cname->len - 1);
    g_io_channel_read_line_string (io, protocol, NULL, NULL);
    protocol = g_string_truncate (protocol, protocol->len - 1);

    while (G_IO_STATUS_EOF != 
	   g_io_channel_read_line_string (io,file_to_send, 
					  NULL, NULL))
    {	
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

gint
take_spool_files(){
    DIR *dir;
    struct dirent *ep;
    gchar *plugin_spool;
    
    if (taking_files == FALSE){
	    taking_files = TRUE;
	    plugin_spool = g_build_path ("/", g_get_home_dir(),PLUGIN_HOME,"spool", NULL);
	    dir = opendir (plugin_spool);
	    g_free (plugin_spool);
	    if (dir == NULL){
		    gaim_debug_info ("nautilus","Can't open the spool dir\n");
	    }else{
		    while (ep = readdir(dir)){
			    gchar *file;

			    if ((strcmp (ep->d_name,".")==0)   || 
				(strcmp (ep->d_name, "..")==0) ||
				(strcmp (ep->d_name, "tmp")==0))
				    continue ;
			    			    			    
			    file = g_build_path ("/", g_get_home_dir(), 
						 PLUGIN_HOME,"spool", 
						 ep->d_name, NULL);
			    process_file (file);
			    g_free (file);
		    }
		    closedir(dir);
	    }
	    taking_files = FALSE;
    }
    return TRUE;
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	
	
	init_plugin_stuff ();
	buddies_str = g_string_new ("");
	save_blist_timeout_handler = gaim_timeout_add (5000,
						       save_online_buddies,
						       NULL);
	taking_files = FALSE;
	take_spool_files_handler = gaim_timeout_add (3000,
						     take_spool_files,
						     NULL);
	return TRUE;
}

static gboolean
plugin_unload() {
    gchar *fd_name;

    fd_name = g_build_path ("/", g_get_home_dir(), PLUGIN_HOME, 
			    B_ONLINE, NULL);    
    gaim_timeout_remove (save_blist_timeout_handler);
    gaim_timeout_remove (take_spool_files_handler);
    remove (fd_name);
    g_free(fd_name);
    g_string_free (buddies_str, TRUE);
    gaim_debug_info ("nautilus", "Stop nautilus plugin\n");
    
    return TRUE;
}

static void 
init_plugin(GaimPlugin *plugin) {
	
}

static GaimPluginInfo info = {
    GAIM_PLUGIN_MAGIC,			/* api version */
    GAIM_MAJOR_VERSION,
    GAIM_MINOR_VERSION,
    GAIM_PLUGIN_STANDARD,		/* type */
    GAIM_GTK_PLUGIN_TYPE,		/* ui requirement */
    0,					/* flags */
    NULL,				/* dependencies */
    GAIM_PRIORITY_DEFAULT,		/* priority */
    
    "gtk-nautilus",					/* id */
    N_("Nautilus Integration"),				/* name */
    "0.8",						/* version */
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

GAIM_INIT_PLUGIN(nautilus, init_plugin, info)

