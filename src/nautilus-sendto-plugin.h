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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:  Roberto Majadas <roberto.majadas@hispalinux.es>
 */

#ifndef _NAUTILUS_SENDTO_PLUGIN_H_
#define _NAUTILUS_SENDTO_PLUGIN_H_

#include <gnome.h>
#include <glib.h>
#include <gmodule.h>

typedef struct _NstPluginInfo NstPluginInfo;
typedef struct _NstPlugin NstPlugin;

struct _NstPluginInfo 
{
	gchar *icon;
	gchar *description;
	gboolean (*init)(NstPlugin *plugin);
	GtkWidget* (*get_contacts_widget)(NstPlugin *plugin);
	gboolean (*send_files)(NstPlugin *plugin, GtkWidget *contact_widget,
				GList *file_list);
    	gboolean (*destroy)(NstPlugin *plugin) ;
};

struct _NstPlugin
{
	GModule *module;
	NstPluginInfo *info;
};

# define NST_INIT_PLUGIN(plugininfo) \
        G_MODULE_EXPORT gboolean nst_init_plugin(NstPlugin *plugin) { \
		plugin->info = &(plugininfo);\
                return TRUE;\
        }	

#define SOEXT           ("." G_MODULE_SUFFIX)
#define SOEXT_LEN       (strlen (SOEXT))


#endif /* _NAUTILUS_SENDTO_PLUGIN_H_ */


