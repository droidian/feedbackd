/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FBD_TYPE_DROID_VIBRA_BACKEND_HIDL fbd_droid_vibra_backend_hidl_get_type ()
G_DECLARE_FINAL_TYPE (FbdDroidVibraBackendHidl, fbd_droid_vibra_backend_hidl, FBD, DROID_VIBRA_BACKEND_HIDL, GObject)

FbdDroidVibraBackendHidl *fbd_droid_vibra_backend_hidl_new (GError **error);

G_END_DECLS
