/*
 * Copyright (C) 2021 Giuseppe Corti
 * Copyright (C) 2021 Erfan Abdi
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Giuseppe Corti <giuseppe.corti@pm.me>
 *         Erfan Abdi
 *         Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-leds"
#define MAXIMUM_HYBRIS_LED_BRIGHTNESS		100

#include "fbd-droid-leds.h"
#include "fbd-binder.h"

#include "fbd-droid-leds-backend.h"
#include "fbd-droid-leds-backend-hidl.h"
#include "fbd-droid-leds-backend-aidl.h"
#include "fbd-droid-leds-backend-sysfs.h"

#include <gio/gio.h>

#include <memory.h>

/**
 * SECTION:fbd-droid-leds
 * @short_description: Android LED device interface (gbinder)
 * @Title: FbdDroidLeds
 *
 * #FbdDevLeds is used to interface with LEDS via gbinder.
 */

typedef struct _FbdDevLeds {
    GObject      parent;

    FbdDroidLedsBackend *backend;
} FbdDevLeds;

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDevLeds, fbd_dev_leds, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init));


static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    FbdDevLeds *self = FBD_DEV_LEDS (initable);
    gboolean success;

    g_debug ("initializing droid leds");

    if (g_file_test ("/usr/lib/droidian/device/leds-sysfs", G_FILE_TEST_EXISTS)) {
        self->backend = (FbdDroidLedsBackend *) fbd_droid_leds_backend_sysfs_new (error);
        if (!self->backend) {
            g_set_error (error,
                         G_IO_ERROR, G_IO_ERROR_FAILED,
                         "Failed to initialize leds backend using sysfs");
            return FALSE;
        }

        g_debug ("Droid leds device initialized using sysfs backend");
        return TRUE;
    }

    /* Try with AIDL first */
    self->backend = (FbdDroidLedsBackend *) fbd_droid_leds_backend_aidl_new (error);

    if (!self->backend) {
        /* No luck, try with HIDL */
        self->backend = (FbdDroidLedsBackend *) fbd_droid_leds_backend_hidl_new (error);

        if (!self->backend) {
            g_set_error (error,
                         G_IO_ERROR, G_IO_ERROR_FAILED,
                         "Failed to obtain suitable light hal");
            return FALSE;
        }
    }

    success = fbd_droid_leds_backend_is_supported (self->backend);

    if (!success) {
        g_set_error (error,
                     G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Failed to get supported LED types");
        return FALSE;
    }

    g_debug ("Droid leds device usable");
    return TRUE;
}


static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}


static void
fbd_dev_leds_dispose (GObject *object)
{
    FbdDevLeds *self = FBD_DEV_LEDS (object);

    g_debug("Disposing droid leds");

    g_clear_object (&self->backend);

    G_OBJECT_CLASS (fbd_dev_leds_parent_class)->dispose (object);
}

static void
fbd_dev_leds_class_init (FbdDevLedsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = fbd_dev_leds_dispose;
}


static void
fbd_dev_leds_init (FbdDevLeds *self)
{
}


FbdDevLeds *
fbd_dev_leds_new (GError **error)
{
    return FBD_DEV_LEDS (g_initable_new (FBD_TYPE_DEV_LEDS,
                                          NULL,
                                          error,
                                          NULL));
}

/**
 * fbd_dev_leds_start_periodic:
 * @self: The #FbdDevLeds
 * @color: The color to use for the LED pattern
 * @max_brightness: The max brightness (in percent) to use for the pattern
 * @freq: The pattern's frequency in mHz
 *
 * Start periodic feedback.
 */
gboolean
fbd_dev_leds_start_periodic (FbdDevLeds *self, FbdFeedbackLedColor color,
                             guint max_brightness, guint freq)
{
    g_debug ("droid LED start flashing");

    return fbd_droid_leds_backend_start_periodic (self->backend, color,
      max_brightness, freq);
}

gboolean
fbd_dev_leds_stop (FbdDevLeds *self, FbdFeedbackLedColor color)
{
    g_debug ("droid LED stop flashing");

    return fbd_droid_leds_backend_stop (self->backend, color);
}

