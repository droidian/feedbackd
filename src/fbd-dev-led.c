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
