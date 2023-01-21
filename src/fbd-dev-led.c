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

enum {
  PROP_0,
  PROP_DEV,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _FbdDevLedPrivate {
  GUdevDevice        *dev;
  guint               max_brightness;
  guint               red_index;
  guint               green_index;
  guint               blue_index;
  /*
   * We just use the colors from the feedback until we
   * do rgb mixing, etc
   */
  FbdFeedbackLedColor color;
} FbdDevLedPrivate;


G_DEFINE_TYPE_WITH_PRIVATE (FbdDevLed, fbd_dev_led, G_TYPE_OBJECT)


static void
fbd_dev_led_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  FbdDevLed *led = FBD_DEV_LED (object);
  FbdDevLedPrivate *priv = fbd_dev_led_get_instance_private (led);

  switch (property_id) {
  case PROP_DEV:
    priv->dev = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
fbd_dev_led_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  FbdDevLed *led = FBD_DEV_LED (object);
  FbdDevLedPrivate *priv = fbd_dev_led_get_instance_private (led);

  switch (property_id) {
  case PROP_DEV:
    g_value_set_object (value, priv->dev);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
fbd_dev_led_finalize (GObject *object)
{
  FbdDevLed *self = FBD_DEV_LED (object);
  FbdDevLedPrivate *priv = fbd_dev_led_get_instance_private (self);

  g_clear_object (&priv->dev);

  G_OBJECT_CLASS (fbd_dev_led_parent_class)->finalize (object);
}


static void
fbd_dev_led_class_init (FbdDevLedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = fbd_dev_led_get_property;
  object_class->set_property = fbd_dev_led_set_property;
  object_class->finalize = fbd_dev_led_finalize;

  props[PROP_DEV] =
    g_param_spec_object ("dev", "", "",
                         G_UDEV_TYPE_DEVICE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
fbd_dev_led_init (FbdDevLed *self)
{
}


FbdDevLed *
fbd_dev_led_new (GUdevDevice *dev)
{
  FbdDevLed *led = NULL;
  FbdDevLedPrivate *priv;
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

      led = g_object_new (FBD_TYPE_DEV_LED, NULL);
      priv = fbd_dev_led_get_instance_private (led);
      priv->dev = g_object_ref (dev);
      priv->color = i;
      priv->max_brightness = brightness;

      if (index) {
        for (int j = 0; j < g_strv_length ((gchar **) index); j++) {
          g_debug ("Index: %s", index[j]);
          if (g_strcmp0 (index[j], LED_MULTI_INDEX_RED) == 0) {
            priv->red_index = counter;
            counter++;
          } else if (g_strcmp0 (index[j], LED_MULTI_INDEX_GREEN) == 0) {
            priv->green_index = counter;
            counter++;
          } else if (g_strcmp0 (index[j], LED_MULTI_INDEX_BLUE) == 0) {
            priv->blue_index = counter;
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
  FbdDevLedPrivate *priv;
  g_autoptr (GError) err = NULL;

  g_return_val_if_fail (FBD_IS_DEV_LED (led), FALSE);
  priv = fbd_dev_led_get_instance_private (led);

  if (!fbd_udev_set_sysfs_path_attr_as_int (priv->dev, LED_BRIGHTNESS_ATTR, brightness, &err)) {
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
  FbdDevLedPrivate *priv;
  g_autoptr (GError) err = NULL;
  g_autofree gchar *str = NULL;
  gboolean success = FALSE;
  gdouble max;
  gdouble t;

  g_return_val_if_fail (FBD_IS_DEV_LED (led), FALSE);
  priv = fbd_dev_led_get_instance_private (led);

  if (priv->color == FBD_FEEDBACK_LED_COLOR_RGB) {
    g_autofree char *intensity = NULL;
    guint colors[] = { 0, 0, 0 };
    switch (color) {
    case FBD_FEEDBACK_LED_COLOR_WHITE:
      colors[priv->red_index] = priv->max_brightness;
      colors[priv->green_index] = priv->max_brightness;
      colors[priv->blue_index] = priv->max_brightness;
      break;
    case FBD_FEEDBACK_LED_COLOR_RED:
      colors[priv->red_index] = priv->max_brightness;
      colors[priv->green_index] = 0;
      colors[priv->blue_index] = 0;
      break;
    case FBD_FEEDBACK_LED_COLOR_GREEN:
      colors[priv->red_index] = 0;
      colors[priv->green_index] = priv->max_brightness;
      colors[priv->blue_index] = 0;
      break;
    case FBD_FEEDBACK_LED_COLOR_BLUE:
      colors[priv->red_index] = 0;
      colors[priv->green_index] = 0;
      colors[priv->blue_index] = priv->max_brightness;
      break;
    default:
      g_warning("Unhandled color: %d\n", color);
      return FALSE;
    }
    intensity = g_strdup_printf ("%d %d %d\n", colors[0], colors[1], colors[2]);
    fbd_dev_led_set_brightness (led, priv->max_brightness);
    success = fbd_udev_set_sysfs_path_attr_as_string (priv->dev, LED_MULTI_INTENSITY_ATTR, intensity, &err);
    if (!success) {
      g_warning ("Failed to set multi intensity: %s", err->message);
      g_clear_error (&err);
    }
  }

  max =  priv->max_brightness * (max_brightness_percentage / 100.0);
  /*  ms     mHz           T/2 */
  t = 1000.0 * 1000.0 / freq / 2.0;
  str = g_strdup_printf ("0 %d %d %d\n", (gint)t, (gint)max, (gint)t);
  g_debug ("Freq %d mHz, Brightness: %d%%, Blink pattern: %s", freq, max_brightness_percentage, str);
  success = fbd_udev_set_sysfs_path_attr_as_string (priv->dev, LED_PATTERN_ATTR, str, &err);
  if (!success)
    g_warning ("Failed to set led pattern: %s", err->message);

  return success;
}


gboolean
fbd_dev_led_has_color (FbdDevLed *led, FbdFeedbackLedColor color)
{
  FbdDevLedPrivate *priv;

  g_return_val_if_fail (FBD_IS_DEV_LED (led), FALSE);
  priv = fbd_dev_led_get_instance_private (led);

  return (priv->color == FBD_FEEDBACK_LED_COLOR_RGB || priv->color == color);
}
