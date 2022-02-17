/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FBD_TYPE_DROID_LEDS_BACKEND_HIDL fbd_droid_leds_backend_hidl_get_type ()
G_DECLARE_FINAL_TYPE (FbdDroidLedsBackendHidl, fbd_droid_leds_backend_hidl, FBD, DROID_LEDS_BACKEND_HIDL, GObject)

FbdDroidLedsBackendHidl *fbd_droid_leds_backend_hidl_new (GError **error);

G_END_DECLS
