/*
 * Copyright (C) 2021 Giuseppe Corti
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Giuseppe Corti <giuseppe.corti@pm.me>
 */

#define G_LOG_DOMAIN "fbd-dev-leds"
#define MAXIMUM_HYBRIS_LED_BRIGHTNESS		100

#include "fbd.h"
#include "fbd-enums.h"
#include "fbd-droid-leds.h"
#include "fbd-feedback-led.h"

#include <gio/gio.h>

#include <memory.h>
#include <hardware/lights.h>
#include <hardware/hardware.h>

/**
 * SECTION:fbd-droid-leds
 * @short_description: Android LED device interface (libhardware)
 * @Title: FbdDroidLeds
 *
 * #FbdDevLeds is used to interface with LEDS via libhardware.
 */

typedef struct _FbdDevLeds {
    GObject      parent;

    struct light_device_t *notifications;

} FbdDevLeds;


static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDevLeds, fbd_dev_leds, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init));

static gboolean
initable_init (GInitable    *initable,
               GCancellable *cancellable,
               GError      **error)
{
    FbdDevLeds *self = FBD_DEV_LEDS (initable);
    int err;
    hw_device_t* droid_device;

    struct hw_module_t    *hwmod;

    err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, (const hw_module_t**) &hwmod);
    if (err == 0) {

        err = hwmod->methods->open(hwmod, LIGHT_ID_NOTIFICATIONS,(hw_device_t **) &self->notifications);
        if (!err == 0 && !droid_device) {

            g_debug ("Failed to access droid notification lights");
            return FALSE;
        }

    } else {
          g_debug ("Failed to access droid led hardware");
          return FALSE;
    }

    g_debug ("droid LED usable");

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

    /* close droid device */
    hw_device_t* droid_device = (hw_device_t *) self->notifications;
    droid_device->close(droid_device);

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
    gdouble max;
    gdouble t;
    g_autofree gchar *str = NULL;
    g_autoptr (GError) err = NULL;
    struct light_state_t   notification_state;

    max = MAXIMUM_HYBRIS_LED_BRIGHTNESS * (max_brightness / 100.0);;

    t = 1000.0 * 1000.0 / freq / 2.0;
    str = g_strdup_printf ("0 %d %d %d\n", (gint)t, (gint)max, (gint)t);
    g_debug ("Freq %d mHz, Brightness: %d%%, Blink pattern: %s", freq, max_brightness, str);

    /* turn on droid leds */

	  memset(&notification_state, 0, sizeof(struct light_state_t));
	  notification_state.color = color;
	  notification_state.flashMode = LIGHT_FLASH_TIMED;
	  notification_state.flashOnMS = t;
	  notification_state.flashOffMS = t;
  	notification_state.brightnessMode = BRIGHTNESS_MODE_USER;

  	self->notifications->set_light(self->notifications, &notification_state);

    return TRUE;
}

gboolean
fbd_dev_leds_stop (FbdDevLeds *self, FbdFeedbackLedColor color)
{
    struct light_state_t   notification_state;

    /* turn off droid leds */
    memset(&notification_state, 0, sizeof(struct light_state_t));
  	notification_state.color = 0x00000000;
  	notification_state.flashMode = LIGHT_FLASH_NONE;
  	notification_state.flashOnMS = 0;
  	notification_state.flashOffMS = 0;
  	notification_state.brightnessMode = 0;

  	self->notifications->set_light(self->notifications, &notification_state);
    return TRUE;
}
