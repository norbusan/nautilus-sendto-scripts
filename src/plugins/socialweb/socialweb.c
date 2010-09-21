/*
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
 * Authors:  Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include "nautilus-sendto-plugin.h"
#include <libsocialweb-client/sw-client.h>

#define SOCIALWEB_TYPE_PLUGIN         (socialweb_plugin_get_type ())
#define SOCIALWEB_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOCIALWEB_TYPE_PLUGIN, SocialwebPlugin))
#define SOCIALWEB_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SOCIALWEB_TYPE_PLUGIN, SocialwebPlugin))
#define SOCIALWEB_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOCIALWEB_TYPE_PLUGIN))
#define SOCIALWEB_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOCIALWEB_TYPE_PLUGIN))
#define SOCIALWEB_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOCIALWEB_TYPE_PLUGIN, SocialwebPluginClass))

typedef struct _SocialwebPlugin       SocialwebPlugin;
typedef struct _SocialwebPluginClass  SocialwebPluginClass;

struct _SocialwebPlugin {
	PeasExtensionBase parent_instance;

	SwClient *client;
	GHashTable *pages;

	/* See socialweb_plugin_supports_mime_types() */
	gboolean has_non_photos;
};

struct _SocialwebPluginClass {
	PeasExtensionBaseClass parent_class;
};

typedef struct {
	GtkWidget *page;
	GtkWidget *bar;
	SwClientService *service;
} SocialwebPage;

NAUTILUS_PLUGIN_REGISTER(SOCIALWEB_TYPE_PLUGIN, SocialwebPlugin, socialweb_plugin)

static void
socialweb_plugin_send_files (NautilusSendtoPlugin *plugin,
			     const char           *id,
			     GList                *file_list,
			     GAsyncReadyCallback   callback,
			     gpointer              user_data)
{
	SocialwebPlugin *p = SOCIALWEB_PLUGIN (plugin);
	SocialwebPage *page;

	g_message ("socialweb_plugin_send_files %s", id);

	page = g_hash_table_lookup (p->pages, id);
	g_return_if_fail (page != NULL);

	/* FIXME call sw_client_service_upload_photo */

	/* FIXME
	 * hack for facebook, convert files to jpeg for upload */
#if 0
	GtkListStore *store;
	GtkTreeIter iter;
	GMount *dest_mount;
	GFile *mount_root;
	GSimpleAsyncResult *simple;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (p->cb), &iter) == FALSE) {
		/* FIXME: This should not happen */
		g_assert_not_reached ();
		return;
	}

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (p->cb)));
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &dest_mount, -1);
	mount_root = g_mount_get_root (dest_mount);

	simple = g_simple_async_result_new (G_OBJECT (plugin),
					    callback,
					    user_data,
					    nautilus_sendto_plugin_send_files);

	copy_files_to (file_list, mount_root);

	g_object_unref (mount_root);

	/* FIXME: Report errors properly */
	g_simple_async_result_set_op_res_gpointer (simple,
						   GINT_TO_POINTER (NST_SEND_STATUS_SUCCESS_DONE),
						   NULL);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);
#endif
}

static SocialwebPage *
create_widget (SwClientService *service)
{
	SocialwebPage *page;

	page = g_new0 (SocialwebPage, 1);
	page->page = gtk_vbox_new (FALSE, 8);
	page->service = g_object_ref (service);

	return page;
}

static void
infobar_response_cb (GtkInfoBar *info_bar,
		     gint        response_id,
		     SocialwebPlugin *plugin)
{
	const char *id;

	id = g_object_get_data (G_OBJECT (info_bar), "id");
	/* FIXME: implement */
	g_warning ("Spawn configuration tool for '%s'", id);
}

static gboolean
update_infobar_for_caps (SocialwebPage   *page,
			 const char     **caps,
			 SocialwebPlugin *plugin)
{
	GtkWidget *label;
	char *msg;
	gboolean can_send;

	/* Remove existing widget */
	if (page->bar != NULL) {
		gtk_widget_hide (page->bar);
		gtk_widget_destroy (page->bar);
		page->bar = NULL;
	}

	can_send = FALSE;

	if (sw_client_service_has_cap (caps, IS_CONFIGURED) == FALSE) {
		page->bar = g_object_new (GTK_TYPE_INFO_BAR,
					  "message-type", GTK_MESSAGE_QUESTION,
					  NULL);
		msg = g_strdup_printf (_("Service '%s' is not configured."),
				       sw_client_service_get_display_name (page->service));
		gtk_info_bar_add_button (GTK_INFO_BAR (page->bar),
					 _("_Configure"),
					 1);
	} else if (sw_client_service_has_cap (caps, CAN_VERIFY_CREDENTIALS)) {
		if (sw_client_service_has_cap (caps, CREDENTIALS_VALID)) {
			msg = g_strdup_printf (_("Logged in to service '%s'."),
						 sw_client_service_get_display_name (page->service));
			page->bar = g_object_new (GTK_TYPE_INFO_BAR,
						  "message-type", GTK_MESSAGE_INFO,
						  NULL);
			can_send = TRUE;
		} else {
			page->bar = g_object_new (GTK_TYPE_INFO_BAR,
						  "message-type", GTK_MESSAGE_ERROR,
						  NULL);
			msg = g_strdup_printf (_("Could not log in to service '%s'."),
					       sw_client_service_get_display_name (page->service));
			gtk_info_bar_add_button (GTK_INFO_BAR (page->bar),
						 _("_Configure"),
						 1);
		}
	} else {
		page->bar = g_object_new (GTK_TYPE_INFO_BAR,
					  "message-type", GTK_MESSAGE_INFO,
					  NULL);
		msg = g_strdup_printf (_("Logged in to service '%s'."),
				       sw_client_service_get_display_name (page->service));
		can_send = TRUE;
	}

	/* So that the button knows where it belongs to */
	g_object_set_data (G_OBJECT (page->bar),
			   "id", (gpointer) sw_client_service_get_name (page->service));

	g_assert (msg);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (page->bar))),
			   label);
	gtk_box_pack_start (GTK_BOX (page->page), page->bar,
			    FALSE, FALSE, 0);
	gtk_widget_show_all (page->page);

	g_signal_connect (page->bar, "response",
			  G_CALLBACK (infobar_response_cb), plugin);

	return can_send;
}

static void
got_dynamic_caps_cb (SwClientService *service,
		     const char     **caps,
		     const GError    *error,
		     SocialwebPlugin *plugin)
{
	SocialwebPage *page;
	gboolean can_send;

	g_message ("got_dynamic_caps_cb '%p' for service '%s'", caps, sw_client_service_get_name (service));

	if (caps == NULL) {
		g_warning ("An error occurred getting caps for service '%s': %s",
			   sw_client_service_get_name (service), error ? error->message : "No reason");
		return;
	}

	page = create_widget (service);
	can_send = update_infobar_for_caps (page, caps, plugin);

	g_hash_table_insert (plugin->pages,
			     (gpointer) sw_client_service_get_name (service),
			     page);

	g_signal_emit_by_name (G_OBJECT (plugin),
			       "add-widget",
			       sw_client_service_get_display_name (service),
			       NULL, /* FIXME, icon name, see http://bugs.meego.com/show_bug.cgi?id=5944 */
			       sw_client_service_get_name (service),
			       page->page);

	g_signal_emit_by_name (G_OBJECT (plugin),
			       "can-send",
			       sw_client_service_get_name (service),
			       can_send);
}

static void
service_capability_changed (SwClientService  *service,
			    const char      **caps,
			    SocialwebPlugin  *plugin)
{
	SocialwebPage *page;
	gboolean can_send;

	page = g_hash_table_lookup (plugin->pages,
				    sw_client_service_get_name (service));
	if (page == NULL)
		return;

	can_send = update_infobar_for_caps (page, caps, plugin);
	g_signal_emit_by_name (G_OBJECT (plugin),
			       "can-send",
			       sw_client_service_get_name (service),
			       can_send);
}

static void
got_static_caps_cb (SwClientService *service,
		    const char     **caps,
		    const GError    *error,
		    SocialwebPlugin *plugin)
{
	g_message ("got_static_caps_cb '%p' for service '%s'", caps, sw_client_service_get_name (service));

	if (sw_client_service_has_cap (caps, "has-photo-upload-iface") != FALSE) {
		sw_client_service_get_dynamic_capabilities (service,
							    (SwClientServiceGetCapabilitiesCallback) got_dynamic_caps_cb,
							    plugin);
		g_signal_connect (G_OBJECT (service), "capabilities-changed",
				  G_CALLBACK (service_capability_changed), plugin);
	}
}

static void
get_services_cb (SwClient        *client,
		 const GList     *services,
		 SocialwebPlugin *plugin)
{
	const GList *l;

	g_message ("got services cb '%p'", services);

	for (l = services; l != NULL; l = l->next) {
		SwClientService *service;

		service = sw_client_get_service (plugin->client, (char*)l->data);
		g_message ("checking service '%s' for static caps", sw_client_service_get_name (service));

		sw_client_service_get_static_capabilities (service,
							   (SwClientServiceGetCapabilitiesCallback) got_static_caps_cb,
							   plugin);
	}
}

static gboolean
socialweb_plugin_supports_mime_types (NautilusSendtoPlugin *plugin,
				      GList                *file_list,
				      const char          **mime_types)
{
	SocialwebPlugin *p = (SocialwebPlugin *) plugin;
	gboolean retval = FALSE;

	if (g_strv_length ((char **) mime_types) != 1) {
		g_message ("more than one mime type");
		return FALSE;
	} else if (g_str_equal (mime_types[0], "image/jpeg")) {
		retval = TRUE;
	} else if (g_content_type_is_a (mime_types[0], "image/*")) {
		/* Some of the plugins, such as Facebook,
		 * don't support sending non-JPEG images */
		p->has_non_photos = TRUE;
		retval = TRUE;
	} else {
		return FALSE;
	}

	sw_client_get_services (p->client,
				(SwClientGetServicesCallback) get_services_cb,
				plugin);

	return retval;
}

static void
destroy_page (gpointer data)
{
	SocialwebPage *page = (SocialwebPage *) data;

	g_object_unref (page->service);
	/* The widgets will be destroyed when
	 * the dialogue is destroyed */
	page = NULL;
}

static void
socialweb_plugin_init (SocialwebPlugin *p)
{
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	p->client = sw_client_new ();
	p->pages = g_hash_table_new_full (g_str_hash, g_str_equal,
					  NULL, destroy_page);
	/* FIXME use is-online? */
}

static void
socialweb_plugin_finalize (GObject *object)
{
	SocialwebPlugin *p = SOCIALWEB_PLUGIN (object);

	g_hash_table_destroy (p->pages);
	p->pages = NULL;

	G_OBJECT_CLASS (socialweb_plugin_parent_class)->finalize (object);
}

