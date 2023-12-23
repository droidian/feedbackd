/*
 * Copyright (C) 2024 Bardia Moshiri
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Bardia Moshiri <fakeshell@bardia.tech>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FBD_TYPE_DROID_VIBRA_BACKEND_SYSFS fbd_droid_vibra_backend_sysfs_get_type ()
G_DECLARE_FINAL_TYPE (FbdDroidVibraBackendSysfs, fbd_droid_vibra_backend_sysfs, FBD, DROID_VIBRA_BACKEND_SYSFS, GObject)

FbdDroidVibraBackendSysfs *fbd_droid_vibra_backend_sysfs_new (GError **error);

G_END_DECLS
