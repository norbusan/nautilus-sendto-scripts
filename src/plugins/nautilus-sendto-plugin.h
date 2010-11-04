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

#ifndef __NAUTILUS_SENDTO_PLUGIN_H__
#define __NAUTILUS_SENDTO_PLUGIN_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

G_BEGIN_DECLS

#define NAUTILUS_SENDTO_TYPE_PLUGIN              (nautilus_sendto_plugin_get_type ())
#define NAUTILUS_SENDTO_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), NAUTILUS_SENDTO_TYPE_PLUGIN, NautilusSendtoPlugin))
#define NAUTILUS_SENDTO_PLUGIN_IFACE(obj)        (G_TYPE_CHECK_CLASS_CAST ((obj), NAUTILUS_SENDTO_TYPE_PLUGIN, NautilusSendtoPluginInterface))
#define NAUTILUS_SENDTO_IS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NAUTILUS_SENDTO_TYPE_PLUGIN))
#define NAUTILUS_SENDTO_PLUGIN_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), NAUTILUS_SENDTO_TYPE_PLUGIN, NautilusSendtoPluginInterface))

typedef struct _NautilusSendtoPlugin           NautilusSendtoPlugin;
typedef struct _NautilusSendtoPluginInterface  NautilusSendtoPluginInterface;

typedef enum {
	NST_SEND_STATUS_SUCCESS,
	NST_SEND_STATUS_SUCCESS_DONE,
	NST_SEND_STATUS_FAILED
} NautilusSendtoSendStatus;

typedef enum {
	NAUTILUS_SENDTO_PLUGIN_ERROR_GENERIC
} NautilusSendtoPluginError;

#define NAUTILUS_SENDTO_PLUGIN_ERROR (nautilus_sendto_plugin_error_quark ())

GQuark nautilus_sendto_plugin_error_quark (void);

struct _NautilusSendtoPluginInterface
{
	GTypeInterface g_iface;

	void	    (*create_widgets) (NautilusSendtoPlugin *plugin,
	                               GList                *file_list,
	                               const char          **mime_types);

	void	    (*send_files)  (NautilusSendtoPlugin *plugin,
		                    const char           *id,
				    GList                *file_list,
				    GAsyncReadyCallback   callback,
				    gpointer              user_data);

	/* signals */
	void        (*add_widget)  (NautilusSendtoPlugin *plugin,
	                            const char           *id,
				    const char           *name,
				    const char           *icon_name,
				    GtkWidget            *widget);

	void        (*can_send)    (NautilusSendtoPlugin *plugin,
				    const char           *id,
				    gboolean              can_send);
};

GType       nautilus_sendto_plugin_get_type       (void);
GObject    *nautilus_sendto_plugin_get_object     (NautilusSendtoPlugin *plugin);

void	    nautilus_sendto_plugin_create_widgets (NautilusSendtoPlugin *plugin,
						   GList                *file_list,
						   const char          **mime_types);

void        nautilus_sendto_plugin_send_files	  (NautilusSendtoPlugin *plugin,
						   const char           *id,
					           GList                *file_list,
					           GAsyncReadyCallback   callback,
					           gpointer              user_data);

NautilusSendtoSendStatus nautilus_sendto_plugin_send_files_finish (NautilusSendtoPlugin *plugin,
								   GAsyncResult         *res,
								   GError              **error);

#define NAUTILUS_PLUGIN_REGISTER(TYPE_NAME, TypeName, type_name)					\
	GType type_name##_get_type (void) G_GNUC_CONST;							\
	G_MODULE_EXPORT void  peas_register_types (PeasObjectModule *module);				\
	static void type_name##_send_files (NautilusSendtoPlugin *plugin, const char *id, GList *file_list, GAsyncReadyCallback callback, gpointer user_data); \
	static void type_name##_create_widgets (NautilusSendtoPlugin *plugin, GList *file_list, const char **mime_types); \
	static void nautilus_sendto_plugin_iface_init (NautilusSendtoPluginInterface *iface);		\
	static void type_name##_finalize (GObject *object);						\
	G_DEFINE_DYNAMIC_TYPE_EXTENDED (TypeName,							\
					type_name,							\
					PEAS_TYPE_EXTENSION_BASE,					\
					0,								\
					G_IMPLEMENT_INTERFACE_DYNAMIC (NAUTILUS_SENDTO_TYPE_PLUGIN,	\
								       nautilus_sendto_plugin_iface_init)) \
	static void											\
	nautilus_sendto_plugin_iface_init (NautilusSendtoPluginInterface *iface)			\
	{												\
		iface->send_files = type_name##_send_files;						\
		iface->create_widgets = type_name##_create_widgets;					\
	}												\
	static void											\
	type_name##_class_init (TypeName##Class *klass)							\
	{												\
		GObjectClass *object_class = G_OBJECT_CLASS (klass);					\
		object_class->finalize = type_name##_finalize;						\
	}												\
	static void											\
	type_name##_class_finalize (TypeName##Class *klass)						\
	{												\
	}												\
	G_MODULE_EXPORT void										\
	peas_register_types (PeasObjectModule *module)							\
	{												\
		type_name##_register_type (G_TYPE_MODULE (module));					\
		peas_object_module_register_extension_type (module,					\
							    NAUTILUS_SENDTO_TYPE_PLUGIN,		\
							    TYPE_NAME);					\
	}

G_END_DECLS

#endif /* __NAUTILUS_SENDTO_PLUGIN_MANAGER_H__  */
