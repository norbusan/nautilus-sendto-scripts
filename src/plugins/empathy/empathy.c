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

#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-contact-manager.h>
#include <libempathy/empathy-dispatcher.h>
#include <libempathy/empathy-utils.h>

#include <libempathy-gtk/empathy-contact-list-store.h>

#include "nautilus-sendto-plugin.h"

static EmpathyContactManager *manager = NULL;
static MissionControl *mc = NULL;

static gboolean destroy (NstPlugin *plugin);

static gboolean
init (NstPlugin *plugin)
{
  GSList *accounts = NULL;
  GSList *l;

  g_print ("Init %s plugin\n", plugin->info->id);

  empathy_debug_set_flags (g_getenv ("EMPATHY_DEBUG"));

  mc = empathy_mission_control_new ();
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
  EmpathyContactListStore *store;
  GtkWidget *combo;
  GtkCellRenderer *renderer;

  /* TODO: Replace all this with EmpathyContactSelector once it's fixed up and
   * merged into libempathy-gtk. */
  manager = empathy_contact_manager_new ();
  store = empathy_contact_list_store_new (EMPATHY_CONTACT_LIST (manager));

  empathy_contact_list_store_set_is_compact (store, TRUE);
  empathy_contact_list_store_set_show_groups (store, FALSE);

  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo),
      renderer, "text", EMPATHY_CONTACT_LIST_STORE_COL_NAME);

  return combo;
}

static gboolean
get_selected_contact (GtkWidget *contact_widget,
                      EmpathyContact **contact)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (contact_widget), &iter))
    return FALSE;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (contact_widget));
  gtk_tree_model_get (model, &iter,
      EMPATHY_CONTACT_LIST_STORE_COL_CONTACT, contact, -1);

  if (*contact == NULL)
    return FALSE;

  return TRUE;
}

static gboolean
validate_destination (NstPlugin *plugin,
                      GtkWidget *contact_widget,
                      gchar **error)
{
  EmpathyContact *contact = NULL;

  if (!get_selected_contact (contact_widget, &contact))
    return FALSE;

  if (!empathy_contact_can_send_files (contact))
    {
      *error = g_strdup (_("The contact selected cannot receive files."));
      return FALSE;
    }

  if (!empathy_contact_is_online (contact))
    {
      *error = g_strdup (_("The contact selected is offline."));
      return FALSE;
    }

  g_object_unref (contact);

  return TRUE;
}

static gboolean
send_files (NstPlugin *plugin,
            GtkWidget *contact_widget,
            GList *file_list)
{
  EmpathyContact *contact = NULL;
  GList *l;

  if (!get_selected_contact (contact_widget, &contact))
    return FALSE;

  for (l = file_list; l; l = l->next)
    {
      gchar *path = l->data;
      GFile *file;

      file = g_file_new_for_uri (path);
      empathy_dispatcher_send_file (contact, file);

      g_object_unref (file);
      g_free (path);
    }

  g_object_unref (contact);

  return TRUE;
}

static gboolean
destroy (NstPlugin *plugin)
{
  if (manager)
    g_object_unref (manager);

  if (mc)
    g_object_unref (mc);

  return TRUE;
}

static
NstPluginInfo plugin_info = {
  "im",
  "empathy",
  N_("Instant Message (Empathy)"),
  TRUE,
  init,
  get_contacts_widget,
  validate_destination,
  send_files,
  destroy
};

NST_INIT_PLUGIN (plugin_info)

