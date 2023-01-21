/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * See Documentation/ABI/testing/sysfs-class-led-trigger-pattern
 */

#define G_LOG_DOMAIN "fbd-dev-leds"

#include "fbd.h"
#include "fbd-enums.h"
#include "fbd-dev-led.h"
#include "fbd-dev-leds.h"
#include "fbd-feedback-led.h"
#include "fbd-udev.h"

#include <gio/gio.h>

#define LED_MULTI_INTENSITY_ATTR "multi_intensity"
#define LED_PATTERN_ATTR         "pattern"
#define LED_SUBSYSTEM            "leds"

/**
 * FbdDevLeds:
 *
 * LED device interface
 *
 * #FbdDevLeds is used to interface with LEDS via sysfs
 * It currently only supports one pattern per led at a time.
 */
typedef struct _FbdDevLeds {
  GObject      parent;

  GUdevClient *client;
  GSList      *leds;
} FbdDevLeds;

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDevLeds, fbd_dev_leds, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init));

static FbdDevLed *
find_led_by_color (FbdDevLeds *self, FbdFeedbackLedColor color)
{
  g_return_val_if_fail (self->leds, NULL);

  for (GSList *l = self->leds; l != NULL; l = l->next) {
    FbdDevLed *led = l->data;
    if (led->color == FBD_FEEDBACK_LED_COLOR_RGB || led->color == color)
      return led;
  }

  /* If we did not match a color pick the first */
  return self->leds->data;
}

static gboolean
initable_init (GInitable    *initable,
               GCancellable *cancellable,
               GError      **error)
{
  const gchar * const subsystems[] = { LED_SUBSYSTEM, NULL };
  FbdDevLeds *self = FBD_DEV_LEDS (initable);
  g_autolist (GUdevDevice) leds = NULL;
  gboolean found = FALSE;

  self->client = g_udev_client_new (subsystems);

  leds = g_udev_client_query_by_subsystem (self->client, LED_SUBSYSTEM);

  for (GList *l = leds; l != NULL; l = l->next) {
    GUdevDevice *dev = G_UDEV_DEVICE (l->data);
    FbdDevLed *led;

    if (g_strcmp0 (g_udev_device_get_property (dev, FEEDBACKD_UDEV_ATTR),
                   FEEDBACKD_UDEV_VAL_LED)) {
      continue;
    }

    led = fbd_dev_led_new (dev);
    if (led) {
      self->leds = g_slist_append (self->leds, led);
      found = TRUE;
    }
  }

  /* TODO: listen for new leds via udev events */

  if (!found) {
    g_set_error (error,
                 G_FILE_ERROR, G_FILE_ERROR_FAILED,
                 "No usable LEDs found");
  }

  return found;
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

  g_clear_object (&self->client);
  g_slist_free_full (self->leds, (GDestroyNotify)fbd_dev_led_free);
  self->leds = NULL;

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
 * @max_brightness_percentage: The max brightness (in percent) to use for the pattern
 * @freq: The pattern's frequency in mHz
 *
 * Start periodic feedback.
 */
gboolean
fbd_dev_leds_start_periodic (FbdDevLeds *self, FbdFeedbackLedColor color,
                             guint max_brightness_percentage, guint freq)
{
  FbdDevLed *led;
  gdouble max;
  gdouble t;
  g_autofree gchar *str = NULL;

  g_autoptr (GError) err = NULL;
  gboolean success;

  g_return_val_if_fail (FBD_IS_DEV_LEDS (self), FALSE);
  g_return_val_if_fail (max_brightness_percentage <= 100.0, FALSE);
  led = find_led_by_color (self, color);
  g_return_val_if_fail (led, FALSE);

  if (led->color == FBD_FEEDBACK_LED_COLOR_RGB) {
    g_autofree char *intensity = NULL;
    guint colors[] = { 0, 0, 0 };
    switch (color) {
    case FBD_FEEDBACK_LED_COLOR_WHITE:
      colors[led->red_index] = led->max_brightness;
      colors[led->green_index] = led->max_brightness;
      colors[led->blue_index] = led->max_brightness;
      break;
    case FBD_FEEDBACK_LED_COLOR_RED:
      colors[led->red_index] = led->max_brightness;
      colors[led->green_index] = 0;
      colors[led->blue_index] = 0;
      break;
    case FBD_FEEDBACK_LED_COLOR_GREEN:
      colors[led->red_index] = 0;
      colors[led->green_index] = led->max_brightness;
      colors[led->blue_index] = 0;
      break;
    case FBD_FEEDBACK_LED_COLOR_BLUE:
      colors[led->red_index] = 0;
      colors[led->green_index] = 0;
      colors[led->blue_index] = led->max_brightness;
      break;
    default:
      g_warning("Unhandled color: %d\n", color);
      return FALSE;
    }
    intensity = g_strdup_printf ("%d %d %d\n", colors[0], colors[1], colors[2]);
    fbd_dev_led_set_brightness (led, led->max_brightness);
    success = fbd_udev_set_sysfs_path_attr_as_string (led->dev, LED_MULTI_INTENSITY_ATTR, intensity, &err);
    if (!success) {
      g_warning ("Failed to set multi intensity: %s", err->message);
      g_clear_error (&err);
    }
  }

  max =  led->max_brightness * (max_brightness_percentage / 100.0);
  /*  ms     mHz           T/2 */
  t = 1000.0 * 1000.0 / freq / 2.0;
  str = g_strdup_printf ("0 %d %d %d\n", (gint)t, (gint)max, (gint)t);
  g_debug ("Freq %d mHz, Brightness: %d%%, Blink pattern: %s", freq, max_brightness_percentage, str);
  success = fbd_udev_set_sysfs_path_attr_as_string (led->dev, LED_PATTERN_ATTR, str, &err);
  if (!success)
    g_warning ("Failed to set led pattern: %s", err->message);

  return success;
}

gboolean
fbd_dev_leds_stop (FbdDevLeds *self, FbdFeedbackLedColor color)
{
  FbdDevLed *led;

  g_return_val_if_fail (FBD_IS_DEV_LEDS (self), FALSE);

  led = find_led_by_color (self, color);
  g_return_val_if_fail (led, FALSE);

  return fbd_dev_led_set_brightness (led, 0);
}
