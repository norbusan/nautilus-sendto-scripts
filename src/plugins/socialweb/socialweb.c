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
#include "nautilus-sendto-filelist.h"
#include "nautilus-sendto-progress.h"
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

	gboolean has_photos;
	gboolean has_videos;
};

struct _SocialwebPluginClass {
	PeasExtensionBaseClass parent_class;
};

typedef struct {
	GtkWidget *page;
	GtkWidget *bar;
	GtkWidget *progress;
	SwClientService *service;
	NstFileList *list;

	GAsyncReadyCallback callback;
	gpointer user_data;
	guint64 written; /* Size of the files uploaded not including current one */
	guint64 current_size; /* Size of the currently uploading file */

	SocialwebPlugin *plugin;
} SocialwebPage;

NAUTILUS_PLUGIN_REGISTER(SOCIALWEB_TYPE_PLUGIN, SocialwebPlugin, socialweb_plugin)

static void send_one (SocialwebPage *page);

static void
send_error (SocialwebPage *page,
	    GError        *error)
{
	GSimpleAsyncResult *simple;

	simple = g_simple_async_result_new_from_error (G_OBJECT (page->plugin),
						       page->callback,
						       page->user_data,
						       error);
	page->callback = NULL;
	page->user_data = NULL;

	if (page->list != NULL) {
		g_object_unref (page->list);
		page->list = NULL;
	}

	g_error_free (error);
	g_simple_async_result_complete_in_idle (simple);
}

static void
prepare_send_one (SocialwebPage *page)
{
	page->written += page->current_size;

	nst_progress_bar_set_uploaded (NST_PROGRESS_BAR (page->progress), page->written);

	/* And onto the next file */
	send_one (page);
}

static void
send_one_photo_cb (GObject *source_object,
		   GAsyncResult *res,
		   gpointer user_data)
{
	SocialwebPage *page;
	GError *error = NULL;

	g_message ("finished sending one photo");

	page = (SocialwebPage *) user_data;
	if (sw_client_service_upload_photo_finish (page->service, res, &error) == FALSE) {
		send_error (page, error);
		return;
	}

	prepare_send_one (page);
}

static void
send_one_video_cb (GObject *source_object,
		   GAsyncResult *res,
		   gpointer user_data)
{
	SocialwebPage *page;
	GError *error = NULL;

	g_message ("finished sending one video");

	page = (SocialwebPage *) user_data;
	if (sw_client_service_upload_video_finish (page->service, res, &error) == FALSE) {
		send_error (page, error);
		return;
	}

	prepare_send_one (page);
}

static void
progress_callback (goffset current_num_bytes,
		   goffset total_num_bytes,
		   gpointer user_data)
{
	SocialwebPage *page;

	page = (SocialwebPage *) user_data;

	nst_progress_bar_set_uploaded (NST_PROGRESS_BAR (page->progress),
				       current_num_bytes + page->written);
}

static void
send_one (SocialwebPage *page)
{
	NstFile *file;
	char *path;
	char *label;

	/* Get the first file out of the list */
	file = nst_file_list_pop_file (page->list);

	if (file == NULL) {
		GSimpleAsyncResult *simple;

		/* We should be all done now */
		g_object_unref (page->list);
		page->list = NULL;

		simple = g_simple_async_result_new (G_OBJECT (page->plugin),
						    page->callback,
						    page->user_data,
						    nautilus_sendto_plugin_send_files);
		page->callback = NULL;
		page->user_data = NULL;

		g_simple_async_result_set_op_res_gpointer (simple,
							   GINT_TO_POINTER (NST_SEND_STATUS_SUCCESS_DONE),
							   NULL);
		g_simple_async_result_complete_in_idle (simple);
		return;
	}

	label = g_strdup_printf (_("Uploading '%s'"), file->display_name);
	nst_progress_bar_set_label (NST_PROGRESS_BAR (page->progress), label);
	g_free (label);

	path = g_file_get_path (file->file);
	/* This would happen if we got a path in the main code
	 * but couldn't get one now, not sure this could happen */
	g_assert (path != NULL);

	page->current_size = file->size;

	if (g_content_type_is_a (file->mime_type, "video/*")) {
		sw_client_service_upload_video (page->service,
						path,
						NULL,
						NULL,
						progress_callback,
						page,
						send_one_video_cb,
						page);
	} else if (g_content_type_is_a (file->mime_type, "image/*")) {
		sw_client_service_upload_photo (page->service,
						path,
						NULL,
						NULL,
						progress_callback,
						page,
						send_one_photo_cb,
						page);
	} else {
		g_assert_not_reached ();
	}

	g_free (path);

	g_boxed_free (NST_TYPE_FILE, file);
}

static void
info_gathered_cb (NstFileList     *list,
		  gboolean         success,
		  guint64          size,
		  SocialwebPage   *page)
{
	gtk_widget_hide (page->bar);
	nst_progress_bar_set_total_size (NST_PROGRESS_BAR (page->progress), size);
	send_one (page);
}

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

	page->callback = callback;
	page->user_data = user_data;
	page->written = 0;
	page->current_size = 0;

	gtk_widget_show (page->progress);
	nst_progress_bar_set_label (NST_PROGRESS_BAR (page->progress), _("Preparing upload"));

	page->list = nst_file_list_new ();
	g_signal_connect (G_OBJECT (page->list), "info-gathered",
			  G_CALLBACK (info_gathered_cb), page);
	nst_file_list_set_files (page->list, file_list);
}

static SocialwebPage *
create_widget (SocialwebPlugin *plugin,
	       SwClientService *service)
{
	SocialwebPage *page;

	page = g_new0 (SocialwebPage, 1);
	page->page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
	page->service = g_object_ref (service);
	page->plugin = plugin;
	page->progress = nst_progress_bar_new ();
	gtk_widget_set_no_show_all (page->progress, TRUE);
	gtk_box_pack_start (GTK_BOX (page->page), page->progress,
			    FALSE, FALSE, 0);

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
			    TRUE, FALSE, 0);
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

	page = create_widget (plugin, service);
	can_send = update_infobar_for_caps (page, caps, plugin);

	g_hash_table_insert (plugin->pages,
			     (gpointer) sw_client_service_get_name (service),
			     page);

	g_signal_emit_by_name (G_OBJECT (plugin),
			       "add-widget",
	                       sw_client_service_get_name (service),
			       sw_client_service_get_display_name (service),
			       NULL, /* FIXME, icon name, see http://bugs.meego.com/show_bug.cgi?id=5944 */
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

static gboolean
service_supports_files (SocialwebPlugin *p,
			const char     **caps)
{
	if (p->has_videos && p->has_photos) {
		return (sw_client_service_has_cap (caps, "has-photo-upload-iface") != FALSE &&
			sw_client_service_has_cap (caps, "has-video-upload-iface") != FALSE);
	} else if (p->has_videos) {
		return sw_client_service_has_cap (caps, "has-video-upload-iface");
	} else if (p->has_photos) {
		return sw_client_service_has_cap (caps, "has-photo-upload-iface");
	} else {
		g_assert_not_reached ();
	}
}

static void
got_static_caps_cb (SwClientService *service,
		    const char     **caps,
		    const GError    *error,
		    SocialwebPlugin *plugin)
{
	g_message ("got_static_caps_cb '%p' for service '%s'", caps, sw_client_service_get_name (service));

	if (service_supports_files (plugin, caps)) {
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

static void
socialweb_plugin_create_widgets (NautilusSendtoPlugin *plugin,
				 GList                *file_list,
				 const char          **mime_types)
{
	SocialwebPlugin *p = (SocialwebPlugin *) plugin;
	guint i;

	for (i = 0; mime_types[i] != NULL; i++) {
		if (g_content_type_is_a (mime_types[i], "image/*")) {
			p->has_photos = TRUE;
		} else if (g_content_type_is_a (mime_types[i], "video/*")) {
			p->has_videos = TRUE;
		} else {
			/* Don't even bother trying to get services, we
			 * can't support those file types */
			return;
		}
	}

	sw_client_get_services (p->client,
				(SwClientGetServicesCallback) get_services_cb,
				plugin);
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

