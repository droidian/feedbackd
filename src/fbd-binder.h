/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <glib.h>
#include <gbinder.h>

G_BEGIN_DECLS

/* Source: https://android.googlesource.com/platform/frameworks/native/+/master/libs/binder/include/binder/Stability.h */
enum BinderStability {
  BINDER_STABILITY_UNDECLARED = 0,
  
  BINDER_STABILITY_VENDOR = 0b000011,
  BINDER_STABILITY_SYSTEM = 0b001100,
  BINDER_STABILITY_VINTF  = 0b111111,
};

gboolean fbd_binder_init (char                   *device,
                          char                   *iface,
                          char                   *fqname,
                          GBinderServiceManager **service_manager,
                          GBinderRemoteObject   **remote,
                          GBinderClient         **client);
gboolean fbd_binder_status_is_ok (GBinderReader *reader);
gboolean fbd_binder_reply_status_is_ok (GBinderRemoteReply *reply);

G_END_DECLS
