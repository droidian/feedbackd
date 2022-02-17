/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * Copyright (C) 2021 Erfan Abdi
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 *         Erfan Abdi
 */

#define G_LOG_DOMAIN "fbd-binder"

#include "fbd-binder.h"

#include <gio/gio.h>

#include <errno.h>

gboolean
fbd_binder_status_is_ok (GBinderReader *reader)
{
  int status;

  if (gbinder_reader_read_int32 (reader, &status) && status == 0) {
    return TRUE;
  }

  return FALSE;
}

gboolean
fbd_binder_reply_status_is_ok (GBinderRemoteReply *reply)
{
  GBinderReader reader;
  
  gbinder_remote_reply_init_reader (reply, &reader);
  
  return fbd_binder_status_is_ok (&reader);
}

gboolean
fbd_binder_init (char                   *device,
                 char                   *iface,
                 char                   *fqname,
                 GBinderServiceManager **service_manager,
                 GBinderRemoteObject   **remote,
                 GBinderClient         **client)
{


  *service_manager = gbinder_servicemanager_new (device);
  if (!*service_manager) {
    g_warning ("Failed to init servicemanager on %s", device);
    return FALSE;
  }
  
  *remote = gbinder_servicemanager_get_service_sync (*service_manager,
                                                     fqname, NULL);
  if (!*remote) {
    g_warning ("Failed to get hal service remote for %s", fqname);
    g_clear_object (&*service_manager);
    *service_manager = NULL;
    return FALSE;
  }
  
  *client = gbinder_client_new (*remote, iface);
  if (!*client) {
    g_warning ("Failed to get hal service client for %s", iface);
    g_clear_object (&*remote);
    g_clear_object (&*service_manager);
    return FALSE;
  }

  /* Ensure we keep a reference on the remote */
  gbinder_remote_object_ref (*remote);

  return TRUE;
}
