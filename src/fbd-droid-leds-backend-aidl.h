/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <glib-object.h>
#include <stdint.h>

#include "fbd-droid-leds-backend.h"

G_BEGIN_DECLS

#define ALIGNED(x) __attribute__ ((aligned(x)))

#define FBD_TYPE_DROID_LEDS_BACKEND_AIDL fbd_droid_leds_backend_aidl_get_type ()
G_DECLARE_FINAL_TYPE (FbdDroidLedsBackendAidl, fbd_droid_leds_backend_aidl, FBD, DROID_LEDS_BACKEND_AIDL, GObject)

typedef struct aidl_hw_light {
  int32_t id;
  int32_t ordinal;
  LightType type ALIGNED(4);
} AidlHwLight;

FbdDroidLedsBackendAidl *fbd_droid_leds_backend_aidl_new (GError **error);

G_END_DECLS
