/*
 * Copyright (C) 2021 Giuseppe Corti
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Giuseppe Corti <giuseppe.corti@pm.me>
 */

#define G_LOG_DOMAIN "fbd-droid-vibra"

#include "fbd-droid-vibra.h"

#include <gio/gio.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-version.h>
#if ANDROID_VERSION_MAJOR >= 7
#include <hardware/vibrator.h>
#else
#include <hardware_legacy/vibrator.h>
#endif

/**
 * SECTION:fbd-droid-vibra
 * @short_description: Android HAL haptic motor device interface
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
    gint id; /* currently used id */

#if ANDROID_VERSION_MAJOR >= 7
    vibrator_device_t *droid_device; /* droid vibrator */
#endif

    FbdDevVibraFeatureFlags features;
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

#if ANDROID_VERSION_MAJOR >= 7
    /* android >= 7 vibrator init */
    self->droid_device = 0;
    struct hw_module_t *hwmod = 0;

    hw_get_module(VIBRATOR_HARDWARE_MODULE_ID, (const hw_module_t **)(&hwmod));

    if (!hwmod || vibrator_open(hwmod, &self->droid_device) < 0) {
        g_set_error (error,
                     G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "Unable to open droid vibrator: %s",
                     g_strerror (errno));

        return FALSE;
    }
#endif

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

    g_clear_object (&self->device);

    G_OBJECT_CLASS (fbd_dev_vibra_parent_class)->dispose (object);
}


static void
fbd_dev_vibra_finalize (GObject *object)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);
#if ANDROID_VERSION_MAJOR >= 7
    self->droid_device->common.close((hw_device_t *)(self->droid_device));
#endif
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

    /* vibrate */
#if ANDROID_VERSION_MAJOR >= 7
    if (self->droid_device) {
        self->droid_device -> vibrator_on(self->droid_device, duration);
    }
#else
    vibrator_on(duration)
#endif
    g_debug("Playing rumbling vibra effect");

    return TRUE;
}


gboolean
fbd_dev_vibra_periodic (FbdDevVibra *self, guint duration, guint magnitude, guint fade_in_level, guint fade_in_time)
{
    /* vibrate */
#if ANDROID_VERSION_MAJOR >= 7
    if (self->droid_device) {
        self->droid_device -> vibrator_on(self->droid_device, duration);
    }
#else
    vibrator_on(duration)
#endif
    g_debug("Playing periodic vibra effect");

    return TRUE;
}


gboolean
fbd_dev_vibra_remove_effect (FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Erasing vibra effect");

#if ANDROID_VERSION_MAJOR >= 7
    /* stop vibration */
    if (self->droid_device) {
        self->droid_device -> vibrator_off(self->droid_device);
    }
#else
    vibrator_off();
#endif
    return TRUE;
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
