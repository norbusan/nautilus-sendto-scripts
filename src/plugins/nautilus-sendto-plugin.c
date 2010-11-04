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
#include "nst-plugin-marshal.h"

G_DEFINE_INTERFACE(NautilusSendtoPlugin, nautilus_sendto_plugin, G_TYPE_OBJECT)

static void
nautilus_sendto_plugin_default_init (NautilusSendtoPluginInterface *iface)
{
	g_signal_new ("add-widget",
		      NAUTILUS_SENDTO_TYPE_PLUGIN,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (NautilusSendtoPluginInterface, add_widget),
		      NULL, NULL,
		      nst_plugin_marshal_VOID__STRING_STRING_STRING_OBJECT,
		      G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT);
	g_signal_new ("can-send",
		      NAUTILUS_SENDTO_TYPE_PLUGIN,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (NautilusSendtoPluginInterface, can_send),
		      NULL, NULL,
		      nst_plugin_marshal_VOID__STRING_BOOLEAN,
		      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);
}

/**
 * nautilus_sendto_plugin_get_object:
 * @plugin: a #NautilusSendtoPlugin instance
 *
 * Returns: (transfer none): a #NautilusSendtoPlugin object.
 */
GObject *
nautilus_sendto_plugin_get_object (NautilusSendtoPlugin *plugin)
{
	return G_OBJECT (plugin);
}

/**
 * nautilus_sendto_plugin_create_widgets:
 * @plugin: a #NautilusSendtoPlugin instance
 * @file_list: (element-type utf8): a #GList of strings representing the files to send
 * @mime_types: a list of mime-types for the file types to send.
 *
 * Causes the plugin to create its widgets, if all mime types are supported.
 * The new widgets are returned via add-widget signal.
 */
void
nautilus_sendto_plugin_create_widgets (NautilusSendtoPlugin  *plugin,
                                       GList                 *file_list,
                                       const char           **mime_types)
{
	NautilusSendtoPluginInterface *iface;

	g_return_if_fail (NAUTILUS_SENDTO_IS_PLUGIN (plugin));

	iface = NAUTILUS_SENDTO_PLUGIN_GET_IFACE (plugin);

	if (G_LIKELY (iface->create_widgets != NULL))
		iface->create_widgets (plugin, file_list, mime_types);
}

/**
 * nautilus_sendto_plugin_send_files:
 * @plugin: a #NautilusSendtoPlugin instance
 * @file_list: (element-type utf8): a #GList of strings representing the files to send
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 *
 * Sends the list of files in @file_list, and calls the @callback
 * when done. You should then call nautilus_sendto_plugin_send_files_finish().
 */
void
nautilus_sendto_plugin_send_files (NautilusSendtoPlugin *plugin,
				   const char           *id,
				   GList                *file_list,
				   GAsyncReadyCallback   callback,
				   gpointer              user_data)
{
	NautilusSendtoPluginInterface *iface;

	g_return_if_fail (NAUTILUS_SENDTO_IS_PLUGIN (plugin));
	g_return_if_fail (id != NULL);

	iface = NAUTILUS_SENDTO_PLUGIN_GET_IFACE (plugin);

	if (G_LIKELY (iface->send_files != NULL))
		iface->send_files (plugin, id, file_list, callback, user_data);
}

/**
 * nautilus_sendto_plugin_send_files_finish:
 * @plugin: a #NautilusSendtoPlugin instance
 * @res: a #GAsyncResult.
 * @error: a #GError, or %NULL
 *
 * Returns: the #NautilusSendtoSendStatus representing the
 * result of the operation.
 */
NautilusSendtoSendStatus
nautilus_sendto_plugin_send_files_finish (NautilusSendtoPlugin *plugin,
					  GAsyncResult         *res,
					  GError              **error)
{
	GSimpleAsyncResult *simple;
	NautilusSendtoSendStatus status;

	g_return_val_if_fail (g_simple_async_result_is_valid (res,
							      G_OBJECT (plugin),
							      nautilus_sendto_plugin_send_files),
			      NST_SEND_STATUS_FAILED);

	simple = (GSimpleAsyncResult *) res;

	if (g_simple_async_result_propagate_error (simple, error))
		return NST_SEND_STATUS_FAILED;

	status = GPOINTER_TO_INT (g_simple_async_result_get_op_res_gpointer (simple));

	return status;
}

GQuark
nautilus_sendto_plugin_error_quark (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("nautilus_sendto_plugin_error");

	return quark;
}

