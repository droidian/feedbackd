/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FBD_TYPE_DROID_VIBRA_BACKEND_AIDL fbd_droid_vibra_backend_aidl_get_type ()
G_DECLARE_FINAL_TYPE (FbdDroidVibraBackendAidl, fbd_droid_vibra_backend_aidl, FBD, DROID_VIBRA_BACKEND_AIDL, GObject)

FbdDroidVibraBackendAidl *fbd_droid_vibra_backend_aidl_new (GError **error);

G_END_DECLS
