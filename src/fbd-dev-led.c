/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * See Documentation/ABI/testing/sysfs-class-led-trigger-pattern
 */

#define G_LOG_DOMAIN "fbd-dev-led"

#include "fbd-dev-led.h"
#include "fbd-udev.h"

#define LED_BRIGHTNESS_ATTR      "brightness"

void
fbd_dev_led_free (FbdDevLed *led)
{
  g_object_unref (led->dev);
  g_free (led);
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
