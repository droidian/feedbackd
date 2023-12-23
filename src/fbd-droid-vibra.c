/*
 * Copyright (C) 2021 Giuseppe Corti
 * Copyright (C) 2021 Erfan Abdi
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Giuseppe Corti <giuseppe.corti@pm.me>
 *         Erfan Abdi
 *         Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-vibra"

#include "fbd-droid-vibra.h"
#include "fbd-binder.h"

#include "fbd-droid-vibra-backend.h"
#include "fbd-droid-vibra-backend-hidl.h"
#include "fbd-droid-vibra-backend-aidl.h"
#include "fbd-droid-vibra-backend-sysfs.h"

#include <gio/gio.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * SECTION:fbd-droid-vibra
 * @short_description: Android HAL haptic motor device interface (gbinder)
 * @Title: FbdDevVibra
 *
 * The #FbdDevVibra is used to interface with haptic motor via the force
 * feedback interface. It currently only supports one id at a time.
 */

enum {
    PROP_0,
    PROP_DEVICE,
    PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef enum {
    FBD_DEV_VIBRA_FEATURE_RUMBLE,
    FBD_DEV_VIBRA_FEATURE_PERIODIC,
    FBD_DEV_VIBRA_FEATURE_GAIN,
} FbdDevVibraFeatureFlags;

typedef struct _FbdDevVibra {
    GObject parent;

    GUdevDevice *device;

    FbdDroidVibraBackend *backend;
} FbdDevVibra;

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDevVibra, fbd_dev_vibra, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init));

static void
fbd_dev_vibra_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);

    switch (property_id) {
    case PROP_DEVICE:
        g_clear_object (&self->device);
        self->device = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
fbd_dev_vibra_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (initable);
    char *device, *iface, *fqname;
    int i;

    g_debug ("initializing droid vibra");

    // Some devices cannot use either hidl or aidl backends, but sysfs works fine for them
    if (g_file_test ("/usr/lib/droidian/device/vibrator-sysfs", G_FILE_TEST_EXISTS)) {
        self->backend = (FbdDroidVibraBackend *) fbd_droid_vibra_backend_sysfs_new (error);
        if (!self->backend) {
            g_set_error (error,
                         G_IO_ERROR, G_IO_ERROR_FAILED,
                         "Failed to initialize vibrator backend using sysfs");
            return FALSE;
        }

        g_debug ("Droid vibra device initialized using sysfs backend");
        return TRUE;
    }

    /* Try with AIDL first */
    self->backend = (FbdDroidVibraBackend *) fbd_droid_vibra_backend_aidl_new (error);

    if (!self->backend) {
        /* No luck, try with HIDL */
        self->backend = (FbdDroidVibraBackend *) fbd_droid_vibra_backend_hidl_new (error);

        if (!self->backend) {
            g_set_error (error,
                         G_IO_ERROR, G_IO_ERROR_FAILED,
                         "Failed to obtain suitable vibrator hal");
            return FALSE;
        }
    }

    g_debug ("Droid vibra device usable");
    return TRUE;
}


static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}


static void
fbd_dev_vibra_dispose (GObject *object)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);

    g_debug("Disposing droid vibra");

    g_clear_object (&self->device);
    g_clear_object (&self->backend);

    G_OBJECT_CLASS (fbd_dev_vibra_parent_class)->dispose (object);
}


static void
fbd_dev_vibra_finalize (GObject *object)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);

    G_OBJECT_CLASS (fbd_dev_vibra_parent_class)->finalize (object);
}

static void
fbd_dev_vibra_class_init (FbdDevVibraClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = fbd_dev_vibra_set_property;
    object_class->get_property = fbd_dev_vibra_get_property;

    object_class->dispose = fbd_dev_vibra_dispose;
    object_class->finalize = fbd_dev_vibra_finalize;

    props[PROP_DEVICE] =
          g_param_spec_object (
          "device",
          "Device",
          "The udev device",
          G_UDEV_TYPE_DEVICE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
fbd_dev_vibra_init (FbdDevVibra *self)
{
}


FbdDevVibra *
fbd_dev_vibra_new (GUdevDevice *device, GError **error)
{
    return FBD_DEV_VIBRA (g_initable_new (FBD_TYPE_DEV_VIBRA,
                                          NULL,
                                          error,
                                          "device", device,
                                          NULL));
}

gboolean
fbd_dev_vibra_rumble (FbdDevVibra *self, guint duration, gboolean upload)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Playing rumbling vibra effect");

    return fbd_droid_vibra_backend_on (self->backend, duration);
}


gboolean
fbd_dev_vibra_periodic (FbdDevVibra *self, guint duration, guint magnitude, guint fade_in_level, guint fade_in_time)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Playing periodic vibra effect");

    return fbd_droid_vibra_backend_on (self->backend, duration);
}


gboolean
fbd_dev_vibra_remove_effect (FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Erasing vibra effect");

    return fbd_droid_vibra_backend_off (self->backend);
}


gboolean
fbd_dev_vibra_stop(FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    return fbd_dev_vibra_remove_effect (self);
}


GUdevDevice *
fbd_dev_vibra_get_device(FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    return self->device;
}
