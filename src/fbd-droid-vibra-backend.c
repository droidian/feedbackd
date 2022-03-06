/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-vibra-backend"

#include "fbd-droid-vibra-backend.h"

G_DEFINE_INTERFACE (FbdDroidVibraBackend, fbd_droid_vibra_backend, G_TYPE_OBJECT)

static void
fbd_droid_vibra_backend_default_init (FbdDroidVibraBackendInterface *iface)
{
    /* Nothing yet */
}

gboolean
fbd_droid_vibra_backend_on (FbdDroidVibraBackend *self,
                            int                   duration)
{
  FbdDroidVibraBackendInterface *iface;
  
  g_return_val_if_fail (FBD_IS_DROID_VIBRA_BACKEND (self), FALSE);
  
  iface = FBD_DROID_VIBRA_BACKEND_GET_IFACE (self);
  g_return_val_if_fail (iface->on != NULL, FALSE);
  return iface->on (self, duration);
}

gboolean
fbd_droid_vibra_backend_off (FbdDroidVibraBackend *self)
{
  FbdDroidVibraBackendInterface *iface;
  
  g_return_val_if_fail (FBD_IS_DROID_VIBRA_BACKEND (self), FALSE);
  
  iface = FBD_DROID_VIBRA_BACKEND_GET_IFACE (self);
  g_return_val_if_fail (iface->off != NULL, FALSE);
  return iface->off (self);
}
