/*
 * Copyright (C) 2024 Bardia Moshiri
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Bardia Moshiri <fakeshell@bardia.tech>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FBD_TYPE_DROID_LEDS_BACKEND_SYSFS fbd_droid_leds_backend_sysfs_get_type ()
G_DECLARE_FINAL_TYPE (FbdDroidLedsBackendSysfs, fbd_droid_leds_backend_sysfs, FBD, DROID_LEDS_BACKEND_SYSFS, GObject)

FbdDroidLedsBackendSysfs *fbd_droid_leds_backend_sysfs_new (GError **error);

G_END_DECLS
