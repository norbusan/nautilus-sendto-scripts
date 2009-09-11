/*
 * Copyright (C) 2008, 2009 Collabora Ltd.
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
 * Authors: Jonny Lamb <jonny.lamb@collabora.co.uk>
 *          Cosimo Cecchi <cosimo.cecchi@collabora.co.uk>
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <telepathy-glib/enums.h>

#include <libempathy/empathy-contact.h>
#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-contact-manager.h>
#include <libempathy/empathy-ft-factory.h>
#include <libempathy/empathy-ft-handler.h>
#include <libempathy/empathy-tp-file.h>
#include <libempathy/empathy-account-manager.h>

#include <libempathy-gtk/empathy-contact-selector.h>
#include <libempathy-gtk/empathy-ui-utils.h>

#include "nautilus-sendto-plugin.h"

static EmpathyAccountManager *acc_manager = NULL;
static EmpathyFTFactory *factory = NULL;
static guint transfers = 0;

static gboolean destroy (NstPlugin *plugin);

static void
handle_account_manager_ready ()
{
  TpConnectionPresenceType presence;

  presence = empathy_account_manager_get_global_presence (acc_manager,
      NULL, NULL);

  if (presence < TP_CONNECTION_PRESENCE_TYPE_AVAILABLE)
    return;
}

static void
acc_manager_ready_cb (EmpathyAccountManager *am,
    GParamSpec *pspec,
    gpointer _user_data)
{
  if (!empathy_account_manager_is_ready (am))
    return;

  handle_account_manager_ready ();
}

static gboolean
init (NstPlugin *plugin)
{
  g_print ("Init %s plugin\n", plugin->info->id);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  empathy_gtk_init ();

  acc_manager = empathy_account_manager_dup_singleton ();

  if (empathy_account_manager_is_ready (acc_manager))
    handle_account_manager_ready ();
  else
    g_signal_connect (acc_manager, "notify::ready",
        G_CALLBACK (acc_manager_ready_cb), NULL);

  return TRUE;
}

static GtkWidget *
get_contacts_widget (NstPlugin *plugin)
{
  EmpathyContactManager *manager;
  GtkWidget *selector;

  manager = empathy_contact_manager_dup_singleton ();
  selector = empathy_contact_selector_new (EMPATHY_CONTACT_LIST (manager));

  empathy_contact_selector_set_visible (EMPATHY_CONTACT_SELECTOR (selector),
      (EmpathyContactSelectorFilterFunc) empathy_contact_can_send_files, NULL);

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
transfer_done_cb (EmpathyFTHandler *handler,
                  EmpathyTpFile *tp_file,
                  NstPlugin *plugin)
{
  quit ();  
}

static void
transfer_error_cb (EmpathyFTHandler *handler,
                   GError *error,
                   NstPlugin *plugin)
{
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
handler_ready_cb (EmpathyFTFactory *factory,
                  EmpathyFTHandler *handler,
                  GError *error,
                  NstPlugin *plugin)
{
  if (error != NULL)
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
      g_signal_connect (handler, "transfer-done",
          G_CALLBACK (transfer_done_cb), plugin);
      g_signal_connect (handler, "transfer-error",
          G_CALLBACK (transfer_error_cb), plugin);

      empathy_ft_handler_start_transfer (handler);
    }
}

static gboolean
send_files (NstPlugin *plugin,
            GtkWidget *contact_widget,
            GList *file_list)
{
  EmpathyContact *contact;
  GList *l;

  contact = get_selected_contact (contact_widget);

  if (!contact)
    return FALSE;

  factory = empathy_ft_factory_dup_singleton ();

  g_signal_connect (factory, "new-ft-handler",
      G_CALLBACK (handler_ready_cb), plugin);

  for (l = file_list; l; l = l->next)
    {
      gchar *path = l->data;
      GFile *file;

      file = g_file_new_for_uri (path);

      ++transfers;

      empathy_ft_factory_new_transfer_outgoing (factory,
          contact, file);

      g_object_unref (file);
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
  if (acc_manager)
    g_object_unref (acc_manager);

  if (factory)
    g_object_unref (factory);

  return TRUE;
}

static
NstPluginInfo plugin_info = {
  "im",
  "empathy",
  N_("Instant Message (Empathy)"),
  TRUE,
  NAUTILUS_CAPS_NONE,
  init,
  get_contacts_widget,
  validate_destination,
  send_files,
  destroy
};

NST_INIT_PLUGIN (plugin_info)

