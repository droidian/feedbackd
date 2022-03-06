/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * Copyright (C) 2021 Erfan Abdi
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 *         Erfan Abdi
 */

#define G_LOG_DOMAIN "fbd-droid-leds-backend"

#include "fbd-droid-leds-backend.h"

G_DEFINE_INTERFACE (FbdDroidLedsBackend, fbd_droid_leds_backend, G_TYPE_OBJECT)

static void
fbd_droid_leds_backend_default_init (FbdDroidLedsBackendInterface *iface)
{
    /* Nothing yet */
}

int32_t
fbd_droid_leds_backend_get_argb_color (FbdFeedbackLedColor color,
                                       guint               max_brightness)
{
  int32_t alpha, max, argb_color;

  max = (max_brightness / 100.0) * 0xff;
  alpha = 0xff; // Full Alpha

  argb_color = (alpha & 0xff) << 24;
  switch (color) {
      case FBD_FEEDBACK_LED_COLOR_WHITE:
          argb_color += ((max & 0xff) << 16) + ((max & 0xff) << 8) + (max & 0xff);
          break;
      case FBD_FEEDBACK_LED_COLOR_RED:
          argb_color += (max & 0xff) << 16;
          break;
      case FBD_FEEDBACK_LED_COLOR_GREEN:
          argb_color += (max & 0xff) << 8;
          break;
      case FBD_FEEDBACK_LED_COLOR_BLUE:
          argb_color += max & 0xff;
          break;
  }

  return argb_color;
}

gboolean
fbd_droid_leds_backend_is_supported (FbdDroidLedsBackend *self)
{
  FbdDroidLedsBackendInterface *iface;
  
  g_return_val_if_fail (FBD_IS_DROID_LEDS_BACKEND (self), FALSE);
  
  iface = FBD_DROID_LEDS_BACKEND_GET_IFACE (self);
  g_return_val_if_fail (iface->is_supported != NULL, FALSE);
  return iface->is_supported (self);
}

gboolean
fbd_droid_leds_backend_start_periodic (FbdDroidLedsBackend *self,
                                       FbdFeedbackLedColor color,
                                       guint               max_brightness,
                                       guint               freq)
{
  FbdDroidLedsBackendInterface *iface;
  
  g_return_val_if_fail (FBD_IS_DROID_LEDS_BACKEND (self), FALSE);
  
  iface = FBD_DROID_LEDS_BACKEND_GET_IFACE (self);
  g_return_val_if_fail (iface->start_periodic != NULL, FALSE);
  return iface->start_periodic (self, color, max_brightness, freq);
}

gboolean
fbd_droid_leds_backend_stop (FbdDroidLedsBackend *self,
                            FbdFeedbackLedColor color)
{
  FbdDroidLedsBackendInterface *iface;
  
  g_return_val_if_fail (FBD_IS_DROID_LEDS_BACKEND (self), FALSE);
  
  iface = FBD_DROID_LEDS_BACKEND_GET_IFACE (self);
  g_return_val_if_fail (iface->stop != NULL, FALSE);
  return iface->stop (self, color);
}
