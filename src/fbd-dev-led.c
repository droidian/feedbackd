/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * See Documentation/ABI/testing/sysfs-class-led-trigger-pattern
 */

#define G_LOG_DOMAIN "fbd-dev-led"

#include "fbd-dev-led.h"
#include "fbd-enums.h"
#include "fbd-udev.h"

#define LED_BRIGHTNESS_ATTR      "brightness"
#define LED_MAX_BRIGHTNESS_ATTR  "max_brightness"
#define LED_MULTI_INDEX_ATTR     "multi_index"
#define LED_MULTI_INDEX_RED      "red"
#define LED_MULTI_INDEX_GREEN    "green"
#define LED_MULTI_INDEX_BLUE     "blue"
#define LED_MULTI_INTENSITY_ATTR "multi_intensity"
#define LED_PATTERN_ATTR         "pattern"

void
fbd_dev_led_free (FbdDevLed *led)
{
  g_object_unref (led->dev);
  g_free (led);
}


FbdDevLed *
fbd_dev_led_new (GUdevDevice *dev)
{

  FbdDevLed *led = NULL;
  const gchar *name, *path;

  name = g_udev_device_get_name (dev);
  /* We don't know anything about diffusors that can combine different
     color LEDSs so go with fixed colors until the kernel gives us
     enough information */
  for (int i = 0; i <= FBD_FEEDBACK_LED_COLOR_LAST; i++) {
    g_autofree char *color = NULL;
    g_autofree char *enum_name = NULL;
    const gchar * const *index;
    guint counter = 0;
    gchar *c;

    enum_name = g_enum_to_string (FBD_TYPE_FEEDBACK_LED_COLOR, i);
    c = strrchr (enum_name, '_');
    color = g_ascii_strdown (c+1, -1);
    if (g_strstr_len (name, -1, color)) {
      g_autoptr (GError) err = NULL;
      guint brightness = g_udev_device_get_sysfs_attr_as_int (dev, LED_MAX_BRIGHTNESS_ATTR);
      index = g_udev_device_get_sysfs_attr_as_strv (dev, LED_MULTI_INDEX_ATTR);

      if (!brightness)
        continue;

      led = g_malloc0 (sizeof(FbdDevLed));
      led->dev = g_object_ref (dev);
      led->color = i;
      led->max_brightness = brightness;

      if (index) {
        for (int j = 0; j < g_strv_length ((gchar **) index); j++) {
          g_debug ("Index: %s", index[j]);
          if (g_strcmp0 (index[j], LED_MULTI_INDEX_RED) == 0) {
            led->red_index = counter;
            counter++;
          } else if (g_strcmp0 (index[j], LED_MULTI_INDEX_GREEN) == 0) {
            led->green_index = counter;
            counter++;
          } else if (g_strcmp0 (index[j], LED_MULTI_INDEX_BLUE) == 0) {
            led->blue_index = counter;
            counter++;
          } else {
            g_warning ("Unsupport LED color index: %d %s", counter, index[j]);
          }
        }
      }

      path = g_udev_device_get_sysfs_path (dev);
      g_debug ("LED at '%s' usable", path);
      break;
    }
  }

  return led;
}


gboolean
fbd_dev_led_set_brightness (FbdDevLed *led, guint brightness)
{
  g_autoptr (GError) err = NULL;

  if (!fbd_udev_set_sysfs_path_attr_as_int (led->dev, LED_BRIGHTNESS_ATTR, brightness, &err)) {
    g_warning ("Failed to setup brightness: %s", err->message);
    return FALSE;
  }

  return TRUE;
}  


gboolean
fbd_dev_led_start_periodic (FbdDevLed           *led,
                            FbdFeedbackLedColor  color,
                            guint                max_brightness_percentage,
                            guint                freq)
{
  g_autoptr (GError) err = NULL;
  g_autofree gchar *str = NULL;
  gboolean success = FALSE;
  gdouble max;
  gdouble t;

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
fbd_dev_led_has_color (FbdDevLed *led, FbdFeedbackLedColor color)
{
  return (led->color == FBD_FEEDBACK_LED_COLOR_RGB || led->color == color);
}
