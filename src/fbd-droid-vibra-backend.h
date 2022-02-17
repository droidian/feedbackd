/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FBD_TYPE_DROID_VIBRA_BACKEND fbd_droid_vibra_backend_get_type()
G_DECLARE_INTERFACE (FbdDroidVibraBackend, fbd_droid_vibra_backend, FBD, DROID_VIBRA_BACKEND, GObject)

struct _FbdDroidVibraBackendInterface
{
  GTypeInterface parent_iface;

  gboolean (*on)  (FbdDroidVibraBackend *self,
                   int                   duration);
  gboolean (*off) (FbdDroidVibraBackend *self);
};

gboolean fbd_droid_vibra_backend_on  (FbdDroidVibraBackend *self,
                                      int                   duration);
gboolean fbd_droid_vibra_backend_off (FbdDroidVibraBackend  *self);

G_END_DECLS
