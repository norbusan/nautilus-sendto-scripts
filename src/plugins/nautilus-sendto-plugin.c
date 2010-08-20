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
 * Author: Bastien Nocera <hadess@hadess.net>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "nautilus-sendto-plugin.h"

G_DEFINE_INTERFACE(NautilusSendtoPlugin, nautilus_sendto_plugin, G_TYPE_OBJECT)

static void
nautilus_sendto_plugin_default_init (NautilusSendtoPluginInterface *iface)
{
}

/**
 * nautilus_sendto_plugin_get_widget:
 * @plugin: a #NautilusSendtoPlugin instance
 * @file_list: (element-type utf8): a #GList of strings representing the files to send.
 * The file list should not be cached, and can be used to create a good archive name,
 * if the plugin allows for compression, for example.
 *
 * Returns a #GtkWidget for the plugin in question.
 *
 * Return value: a #GtkWidget, or %NULL if the plugin does not implement
 * the required interface.
 */
GtkWidget *
nautilus_sendto_plugin_get_widget (NautilusSendtoPlugin  *plugin,
				   GList                 *file_list)
{
	NautilusSendtoPluginInterface *iface;

	g_return_val_if_fail (NAUTILUS_SENDTO_IS_PLUGIN (plugin), FALSE);

	iface = NAUTILUS_SENDTO_PLUGIN_GET_IFACE (plugin);

	if (G_LIKELY (iface->get_widget != NULL))
		return iface->get_widget (plugin, file_list);

	return NULL;
}

/**
 * nautilus_sendto_plugin_supports_mime_types:
 * @plugin: a #NautilusSendtoPlugin instance
 * @mime_types: a list of mime-types for the file types to send.
 *
 * Returns a #gboolean.
 *
 * Return value: %FALSE if one or more of the mime_types cannot be sent through that plugin.
 */
gboolean
nautilus_sendto_plugin_supports_mime_types (NautilusSendtoPlugin  *plugin,
					    const char           **mime_types)
{
	NautilusSendtoPluginInterface *iface;

	g_return_val_if_fail (NAUTILUS_SENDTO_IS_PLUGIN (plugin), FALSE);

	iface = NAUTILUS_SENDTO_PLUGIN_GET_IFACE (plugin);

	if (G_LIKELY (iface->supports_mime_types != NULL))
		return iface->supports_mime_types (plugin, mime_types);

	return FALSE;
}

/**
 * nautilus_sendto_plugin_send_files:
 * @plugin: a #NautilusSendtoPlugin instance
 * @file_list: (element-type utf8): a #GList of strings representing the files to send
 *
 * Returns a #NautilusSendtoSendStatus representing failure or success
 *
 * Return value: %NST_SEND_STATUS_SUCCESS on success,
 * %NST_SEND_STATUS_IN_PROGRESS if the send will take a while,
 * %NST_SEND_STATUS_FAILED if it failed.
 */
NautilusSendtoSendStatus
nautilus_sendto_plugin_send_files (NautilusSendtoPlugin *plugin,
				   GList                *file_list)
{
	NautilusSendtoPluginInterface *iface;

	g_return_val_if_fail (NAUTILUS_SENDTO_IS_PLUGIN (plugin), FALSE);

	iface = NAUTILUS_SENDTO_PLUGIN_GET_IFACE (plugin);

	if (G_LIKELY (iface->send_files != NULL))
		return iface->send_files (plugin, file_list);

	return NST_SEND_STATUS_FAILED;
}
