/* $Id$ */
/*-
 * Copyright (c) 2003-2004 Benedikt Meurer <benny@xfce.org>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *                                                                              
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                                                                              
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifndef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk-pixbuf/gdk-pixdata.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <libxfce4mcs/mcs-manager.h>
#include <libxfcegui4/libxfcegui4.h>
#include <xfce-mcs-manager/manager-plugin.h>

#include <xfce4-session/xfsm-splash-engine.h>
#include <xfce4-session/xfsm-util.h>

#include <settings/splash/module.h>
#include <settings/splash/nopreview.h>


/*
   Declarations
 */
enum
{
  COLUMN_NAME,
  COLUMN_MODULE,
  N_COLUMNS,
};


/*
   Global variables
 */
static GList     *modules = NULL;
static XfceRc    *modules_rc = NULL;
static gboolean   splash_centered;
static GtkWidget *splash_dialog = NULL;
static GtkWidget *splash_treeview;
static GtkWidget *splash_button_cfg;
static GtkWidget *splash_button_test;
static GtkWidget *splash_image;
static GtkWidget *splash_descr0;
static GtkWidget *splash_descr1;
static GtkWidget *splash_version0;
static GtkWidget *splash_version1;
static GtkWidget *splash_author0;
static GtkWidget *splash_author1;
static GtkWidget *splash_www0;
static GtkWidget *splash_www1;


/*
   Module loading/unloading
 */
static void
splash_load_modules (void)
{
  const gchar *entry;
  Module      *module;
  gchar       *file;
  GDir        *dir;

  modules_rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG,
                                    "xfce4-session/xfce4-splash.rc",
                                    TRUE);

  dir = g_dir_open (MODULESDIR, 0, NULL);
  if (G_LIKELY (dir != NULL))
    {
      while ((entry = g_dir_read_name (dir)) != NULL)
        {
          if (*entry == '\0' || *entry == '.')
            continue;

          if (!g_str_has_suffix (entry, "." G_MODULE_SUFFIX))
            continue;

          file = g_strconcat (MODULESDIR, G_DIR_SEPARATOR_S, entry, NULL);
          module = module_load (file, modules_rc);
          if (G_LIKELY (module != NULL))
            modules = g_list_append (modules, module);
          g_free (file);
        }

      g_dir_close (dir);
    }
}


static void
splash_unload_modules (void)
{
  GList *lp;
  
  if (G_LIKELY (modules != NULL))
    {
      for (lp = modules; lp != NULL; lp = lp->next)
        module_free (MODULE (lp->data));
      g_list_free (modules);
      modules = NULL;
    }

  if (G_LIKELY (modules_rc != NULL))
    {
      xfce_rc_close (modules_rc);
      modules_rc = NULL;
    }
}


/*
   Dialog
 */
static gboolean
splash_response (void)
{
  if (G_LIKELY (splash_dialog != NULL))
    {
      gtk_widget_destroy (splash_dialog);
      splash_dialog = NULL;
    }

  splash_unload_modules ();

  return TRUE;
}


static void
splash_selection_changed (GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  const gchar  *str;
  GdkPixbuf    *preview;
  Module       *module;
  XfceRc       *rc;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, COLUMN_MODULE, &module, -1);

      rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG,
                                "xfce4-session/xfce4-session.rc",
                                FALSE);
      xfce_rc_set_group (rc, "Splash Screen");

      if (module != NULL)
        {
          str = module_descr (module);
          if (G_LIKELY (str != NULL))
            {
              gtk_label_set_text (GTK_LABEL (splash_descr1), str);
              gtk_widget_show (splash_descr0);
              gtk_widget_show (splash_descr1);
            }
          else
            {
              gtk_widget_hide (splash_descr0);
              gtk_widget_hide (splash_descr1);
            }
          gtk_widget_set_sensitive (splash_descr1, TRUE);

          str = module_version (module);
          if (G_LIKELY (str != NULL))
            {
              gtk_label_set_text (GTK_LABEL (splash_version1), str);
              gtk_widget_show (splash_version0);
              gtk_widget_show (splash_version1);
            }
          else
            {
              gtk_widget_hide (splash_version0);
              gtk_widget_hide (splash_version1);
            }
          gtk_widget_set_sensitive (splash_version1, TRUE);

          str = module_author (module);
          if (G_LIKELY (str != NULL))
            {
              gtk_label_set_text (GTK_LABEL (splash_author1), str);
              gtk_widget_show (splash_author0);
              gtk_widget_show (splash_author1);
            }
          else
            {
              gtk_widget_hide (splash_author0);
              gtk_widget_hide (splash_author1);
            }
          gtk_widget_set_sensitive (splash_author1, TRUE);

          str = module_homepage (module);
          if (G_LIKELY (str != NULL))
            {
              gtk_label_set_text (GTK_LABEL (splash_www1), str);
              gtk_widget_show (splash_www0);
              gtk_widget_show (splash_www1);
            }
          else
            {
              gtk_widget_hide (splash_www0);
              gtk_widget_hide (splash_www1);
            }
          gtk_widget_set_sensitive (splash_www1, TRUE);

          preview = module_preview (module);
          if (G_UNLIKELY (preview == NULL))
            preview = gdk_pixbuf_from_pixdata (&nopreview, FALSE, NULL);
          gtk_image_set_from_pixbuf (GTK_IMAGE (splash_image), preview);
          g_object_unref (G_OBJECT (preview));

          xfce_rc_write_entry (rc, "Engine", module_engine (module));
        }
      else
        {
          preview = gdk_pixbuf_from_pixdata (&nopreview, FALSE, NULL);
          gtk_image_set_from_pixbuf (GTK_IMAGE (splash_image), preview);
          g_object_unref (G_OBJECT (preview));

          gtk_label_set_text (GTK_LABEL (splash_descr1), _("None"));
          gtk_widget_set_sensitive (splash_descr1, FALSE);

          gtk_label_set_text (GTK_LABEL (splash_version1), _("None"));
          gtk_widget_set_sensitive (splash_version1, FALSE);

          gtk_label_set_text (GTK_LABEL (splash_author1), _("None"));
          gtk_widget_set_sensitive (splash_author1, FALSE);

          gtk_label_set_text (GTK_LABEL (splash_www1), _("None"));
          gtk_widget_set_sensitive (splash_www1, FALSE);

          xfce_rc_write_entry (rc, "Engine", "");
        }

      xfce_rc_close (rc);
    }

  /* centering must be delayed! */
  if (!splash_centered)
    {
      xfce_gtk_window_center_on_monitor_with_pointer(GTK_WINDOW(splash_dialog));
      splash_centered = TRUE;
    }
}


static void
splash_run (McsPlugin *plugin)
{
  GtkTreeSelection  *selection;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  GtkListStore      *store;
  const gchar       *engine;
  GtkTreePath       *path;
  GtkTreeIter        iter;
  GtkWidget         *header;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *table;
  XfceRc            *rc;
  GList             *lp;

  if (G_UNLIKELY (splash_dialog != NULL))
    {
      gtk_window_present (GTK_WINDOW (splash_dialog));
      return;
    }

  /* load splash modules */
  splash_load_modules ();

  /* load config */
  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG,
                            "xfce4-session/xfce4-session.rc",
                            TRUE);
  xfce_rc_set_group (rc, "Splash Screen");
  engine = xfce_rc_read_entry (rc, "Engine", "");

  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COLUMN_NAME, _("None"),
                      COLUMN_MODULE, NULL,
                      -1);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);

  for (lp = modules; lp != NULL; lp = lp->next)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_NAME, module_name (MODULE (lp->data)),
                          COLUMN_MODULE, MODULE (lp->data),
                          -1);

      if (strcmp (module_engine (MODULE (lp->data)), engine) == 0)
        {
          gtk_tree_path_free (path);
          path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
        }
    }

  xfce_rc_close (rc);

  splash_centered = FALSE;
  splash_dialog = gtk_dialog_new_with_buttons (_("Splash Screen Settings"),
                                               NULL,
                                               GTK_DIALOG_NO_SEPARATOR,
                                               GTK_STOCK_CLOSE,
                                               GTK_RESPONSE_CLOSE,
                                               NULL);

  gtk_window_set_icon (GTK_WINDOW (splash_dialog), plugin->icon);

  g_signal_connect (G_OBJECT (splash_dialog), "response",
                    G_CALLBACK (splash_response), NULL);
  g_signal_connect (G_OBJECT (splash_dialog), "delete-event",
                    G_CALLBACK (splash_response), NULL);

  header = xfce_create_header (plugin->icon, _("Splash Screen Settings"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (splash_dialog)->vbox), header,
                      FALSE, FALSE, 0);
  gtk_widget_show (header);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (splash_dialog)->vbox), hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  splash_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (splash_treeview), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), splash_treeview, TRUE, TRUE, 0);
  gtk_widget_show (splash_treeview);
  g_object_unref (G_OBJECT (store));

  /* add tree view column */
  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_NAME,
                                       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (splash_treeview), column);

  splash_button_cfg = xfsm_imgbtn_new (_("Configure"), GTK_STOCK_PREFERENCES,
                                       NULL);
  gtk_widget_set_sensitive (splash_button_cfg, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), splash_button_cfg, FALSE, FALSE, 0);
  gtk_widget_show (splash_button_cfg);

  splash_button_test = xfsm_imgbtn_new (_("Test"), GTK_STOCK_EXECUTE, NULL);
  gtk_widget_set_sensitive (splash_button_test, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), splash_button_test, FALSE, FALSE, 0);
  gtk_widget_show (splash_button_test);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  splash_image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (vbox), splash_image, TRUE, TRUE, 0);
  gtk_widget_show (splash_image);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  splash_descr0 = gtk_label_new (_("<b>Description:</b>"));
  gtk_label_set_use_markup (GTK_LABEL (splash_descr0), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_descr0), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_descr0,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_descr0);

  splash_descr1 = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (splash_descr1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_descr1), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_descr1,
                    1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_descr1);

  splash_version0 = gtk_label_new (_("<b>Version:</b>"));
  gtk_label_set_use_markup (GTK_LABEL (splash_version0), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_version0), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_version0,
                    0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_version0);

  splash_version1 = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (splash_version1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_version1), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_version1,
                    1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_version1);

  splash_author0 = gtk_label_new (_("<b>Author:</b>"));
  gtk_label_set_use_markup (GTK_LABEL (splash_author0), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_author0), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_author0,
                    0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_author0);

  splash_author1 = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (splash_author1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_author1), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_author1,
                    1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_author1);

  splash_www0 = gtk_label_new (_("<b>Homepage:</b>"));
  gtk_label_set_use_markup (GTK_LABEL (splash_www0), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_www0), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_www0,
                    0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_www0);

  splash_www1 = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (splash_www1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (splash_www1), 0, 0);
  gtk_table_attach (GTK_TABLE (table), splash_www1,
                    1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (splash_www1);

  /* handle selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (splash_treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (splash_selection_changed), NULL);
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (splash_treeview), path, NULL, FALSE);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (splash_treeview), path, NULL,
                                TRUE, 0.5, 0.0);
  gtk_tree_path_free (path);

  gtk_widget_show (splash_dialog);
}


/*
   Mcs interface
 */
McsPluginInitResult
mcs_plugin_init (McsPlugin *plugin)
{
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  plugin->plugin_name = g_strdup ("splash");
  plugin->caption = g_strdup (_("Splash Screen"));
  plugin->run_dialog = splash_run;
  plugin->icon = xfce_themed_icon_load ("xfce4-splash", 48);

  return MCS_PLUGIN_INIT_OK;
}


MCS_PLUGIN_CHECK_INIT;
