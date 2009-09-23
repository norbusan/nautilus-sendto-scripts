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

#ifndef _NAUTILUS_SENDTO_PLUGIN_H_
#define _NAUTILUS_SENDTO_PLUGIN_H_

#include <gmodule.h>
#include <gtk/gtk.h>

typedef struct _NstPluginInfo NstPluginInfo;
typedef struct _NstPlugin NstPlugin;

typedef enum {
	NAUTILUS_CAPS_NONE = 0,
	NAUTILUS_CAPS_SEND_DIRECTORIES = 1 << 0,
	NAUTILUS_CAPS_SEND_IMAGES = 1 << 1,
} NstPluginCapabilities;

struct _NstPluginInfo 
{
	gchar *icon;
	gchar *id;
	gchar *description;
	gboolean never_unload;
	NstPluginCapabilities capabilities;
	gboolean (*init) (NstPlugin *plugin);
	GtkWidget* (*get_contacts_widget) (NstPlugin *plugin);
	gboolean (*validate_destination) (NstPlugin *plugin, GtkWidget *contact_widget, char **error);
	gboolean (*send_files) (NstPlugin *plugin,
				GtkWidget *contact_widget,
				GList *file_list);
	gboolean (*destroy) (NstPlugin *plugin) ;
};

struct _NstPlugin
{
	GModule *module;
	NstPluginInfo *info;
};

# define NST_INIT_PLUGIN(plugininfo)					\
	gboolean nst_init_plugin(NstPlugin *plugin);			\
        G_MODULE_EXPORT gboolean nst_init_plugin(NstPlugin *plugin) {	\
		plugin->info = &(plugininfo);				\
                return TRUE;						\
        }	

#endif /* _NAUTILUS_SENDTO_PLUGIN_H_ */

