/*
 * Copyright (C) 2008 Collabora Ltd.
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
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <telepathy-glib/enums.h>

#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-contact-manager.h>
#include <libempathy/empathy-dispatcher.h>
#include <libempathy/empathy-utils.h>
#include <libempathy/empathy-tp-file.h>

#include <libempathy-gtk/empathy-contact-selector.h>
#include <libempathy-gtk/empathy-ui-utils.h>

#include "nautilus-sendto-plugin.h"

static MissionControl *mc = NULL;
static EmpathyDispatcher *dispatcher = NULL;
static guint transfers = 0;

static gboolean destroy (NstPlugin *plugin);

static gboolean
init (NstPlugin *plugin)
{
  GSList *accounts = NULL;
  GSList *l;

  g_print ("Init %s plugin\n", plugin->info->id);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  empathy_gtk_init ();

  mc = empathy_mission_control_dup_singleton ();
  accounts = mission_control_get_online_connections (mc, FALSE);

  if (g_slist_length (accounts) == 0)
    {
      destroy (plugin);
      return FALSE;
    }

  for (l = accounts; l; l = l->next)
    g_object_unref (l->data);

  g_slist_free (accounts);

  return TRUE;
}

static GtkWidget *
get_contacts_widget (NstPlugin *plugin)
{
  EmpathyContactManager *manager;
  GtkWidget *selector;

  manager = empathy_contact_manager_dup_singleton ();
  selector = empathy_contact_selector_new (EMPATHY_CONTACT_LIST (manager));

  g_object_unref (manager);

  return selector;
}

static EmpathyContact *
get_selected_contact (GtkWidget *contact_widget)
{
  EmpathyContact *contact;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (contact_widget), &iter))
    return NULL;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (contact_widget));
  gtk_tree_model_get (model, &iter,
      EMPATHY_CONTACT_LIST_STORE_COL_CONTACT, &contact, -1);

  return contact;
}

static gboolean
validate_destination (NstPlugin *plugin,
                      GtkWidget *contact_widget,
                      gchar **error)
{
  EmpathyContact *contact = NULL;
  gboolean ret = TRUE;

  contact = get_selected_contact (contact_widget);

  if (!contact)
    return FALSE;

  if (!empathy_contact_can_send_files (contact))
    {
      *error = g_strdup (_("The contact selected cannot receive files."));
      ret = FALSE;
    }

  if (ret && !empathy_contact_is_online (contact))
    {
      *error = g_strdup (_("The contact selected is offline."));
      ret = FALSE;
    }

  g_object_unref (contact);

  return ret;
}

static void
quit (void)
{
  if (--transfers > 0)
    return;

  destroy (NULL);
  gtk_main_quit ();
}

static void
state_changed_cb (EmpathyTpFile *tp_file,
                  GParamSpec *arg,
                  gpointer user_data)
{
  TpFileTransferState state;

  state = empathy_tp_file_get_state (tp_file, NULL);

  if (state == TP_FILE_TRANSFER_STATE_COMPLETED || state == TP_FILE_TRANSFER_STATE_CANCELLED)
    quit ();
}

static void
error_dialog_cb (GtkDialog *dialog,
                 gint arg,
                 gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
  quit ();
}

static void
send_file_cb (EmpathyDispatchOperation *dispatch,
              const GError *error,
              gpointer user_data)
{
  GFile *file = (GFile *) user_data;

  if (error)
    {
      GtkWidget *dialog;
      dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE, "%s",
          error->message ? error->message : _("No error message"));

      g_signal_connect (dialog, "response", G_CALLBACK (error_dialog_cb), NULL);
      gtk_widget_show (dialog);
    }
  else
    {
      EmpathyTpFile *tp_file;

      tp_file = EMPATHY_TP_FILE (
          empathy_dispatch_operation_get_channel_wrapper (dispatch));

      g_signal_connect (tp_file, "notify::state",
          G_CALLBACK (state_changed_cb), NULL);

      empathy_tp_file_offer (tp_file, file, NULL);
    }

  g_object_unref (file);

}

static gboolean
send_files (NstPlugin *plugin,
            GtkWidget *contact_widget,
            GList *file_list)
{
  EmpathyContact *contact;
  GList *l;

  contact = get_selected_contact (contact_widget);

  dispatcher = empathy_dispatcher_dup_singleton ();

  if (!contact)
    return FALSE;

  for (l = file_list; l; l = l->next)
    {
      gchar *path = l->data;
      GFile *file;
      GFileInfo *info;
      GError *error = NULL;
      GTimeVal mod_timeval;

      file = g_file_new_for_uri (path);

      info = g_file_query_info (file,
          G_FILE_ATTRIBUTE_STANDARD_SIZE ","
          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
          G_FILE_ATTRIBUTE_TIME_MODIFIED ","
          G_FILE_ATTRIBUTE_STANDARD_NAME,
          0, NULL, &error);

      if (error)
        {
          GtkWidget *dialog;
          dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
              GTK_BUTTONS_CLOSE, "Failed to get information for %s",
              path);
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
              "%s", error->message ? error->message : _("No error message"));
          gtk_dialog_run (GTK_DIALOG (dialog));
          gtk_widget_destroy (dialog);

          g_object_unref (file);
          g_object_unref (contact);
          continue;
        }

      g_file_info_get_modification_time (info, &mod_timeval);

      empathy_dispatcher_send_file_to_contact (contact,
          g_file_info_get_name (info),
          g_file_info_get_size (info),
          mod_timeval.tv_sec,
          g_file_info_get_content_type (info),
          send_file_cb, file);

      transfers++;

      g_object_unref (info);
    }

  g_object_unref (contact);

  if (transfers == 0)
    {
      destroy (NULL);
      return TRUE;
    }

  return FALSE;
}

static gboolean
destroy (NstPlugin *plugin)
{
  if (mc)
    g_object_unref (mc);

  if (dispatcher)
    g_object_unref (dispatcher);

  return TRUE;
}

static
NstPluginInfo plugin_info = {
  "im",
  "empathy",
  N_("Instant Message (Empathy)"),
  TRUE,
  FALSE,
  init,
  get_contacts_widget,
  validate_destination,
  send_files,
  destroy
};

NST_INIT_PLUGIN (plugin_info)

