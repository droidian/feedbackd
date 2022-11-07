/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "fbd"

#include "fbd.h"
#include "fbd-feedback-manager.h"
#include "lfb-names.h"
#include "lfb-gdbus.h"

#include <gio/gio.h>
#include <glib-unix.h>


static GMainLoop *loop;


GQuark
fbd_error_quark (void)
{
  static GQuark quark = 0;

  if (!quark)
    quark = g_quark_from_static_string("fbd");

  return quark;
}


static gboolean
quit_cb (gpointer user_data)
{
  g_info ("Caught signal, shutting down...");

  if (loop)
    g_idle_add ((GSourceFunc) g_main_loop_quit, loop);
  else
    exit (0);

  return FALSE;
}

static gboolean
reload_cb (gpointer user_data)
{
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default();

  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (manager), FALSE);

  g_debug ("Caught signal, reloading feedback theme...");
  fbd_feedback_manager_load_theme (manager);

  return TRUE;
}

static void
bus_acquired_cb (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default ();

  g_debug ("Bus acquired, creating manager...");

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (manager),
                                    connection,
                                    FB_DBUS_PATH,
                                    NULL);
}


static void
name_acquired_cb (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{
  g_debug ("Service name '%s' was acquired", name);
}

static void
name_lost_cb (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
  /* Note that we're not allowing replacement, so once the name acquired, the
   * process won't lose it. */
  if (!name) {
    g_warning ("Could not get the session bus. Make sure "
               "the message bus daemon is running!");
  } else {
    if (connection)
      g_warning ("Could not acquire the '%s' service name", name);
    else
      g_debug ("DBus connection close");
  }

  g_main_loop_quit (loop);
}


int main(int argc, char *argv[])
{
  g_autoptr(GError) err = NULL;
  g_autoptr(GOptionContext) opt_context = NULL;

  opt_context = g_option_context_new ("- A daemon to trigger event feedback");
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  g_unix_signal_add (SIGTERM, quit_cb, NULL);
  g_unix_signal_add (SIGINT, quit_cb, NULL);
  g_unix_signal_add (SIGHUP, reload_cb, NULL);

  loop = g_main_loop_new (NULL, FALSE);

  g_bus_own_name (FB_DBUS_TYPE,
                  FB_DBUS_NAME,
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  bus_acquired_cb,
                  name_acquired_cb,
                  name_lost_cb,
                  NULL,
                  NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (fbd_feedback_manager_get_default ());
}
