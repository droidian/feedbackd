/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#pragma once

#include <stdint.h>
#include <glib-object.h>

#include "fbd-feedback-led.h"

G_BEGIN_DECLS

#define ALIGNED(x) __attribute__ ((aligned(x)))

#define FBD_TYPE_DROID_LEDS_BACKEND fbd_droid_leds_backend_get_type()
G_DECLARE_INTERFACE (FbdDroidLedsBackend, fbd_droid_leds_backend, FBD, DROID_LEDS_BACKEND, GObject)

/* Light types */
typedef enum light_type {
  LIGHT_TYPE_BACKLIGHT = 0,
  LIGHT_TYPE_KEYBOARD = 1,
  LIGHT_TYPE_BUTTONS = 2,
  LIGHT_TYPE_BATTERY = 3,
  LIGHT_TYPE_NOTIFICATIONS = 4,
  LIGHT_TYPE_ATTENTION = 5,
  LIGHT_TYPE_BLUETOOTH = 6,
  LIGHT_TYPE_WIFI = 7,
  LIGHT_TYPE_COUNT = 8,
} LightType;

/* Flash types */
typedef enum flash_type {
  FLASH_TYPE_NONE = 0,
  FLASH_TYPE_TIMED = 1,
  FLASH_TYPE_HARDWARE = 2,
} FlashType;

/* Brightness types */
typedef enum brightness_type {
  BRIGHTNESS_MODE_USER = 0,
  BRIGHTNESS_MODE_SENSOR = 1,
  BRIGHTNESS_MODE_LOW_PERSISTENCE = 2,
} BrightnessType;

/* The light state */
typedef struct light_state {
  uint32_t color;
  FlashType flashMode ALIGNED(4);
  int32_t flashOnMs;
  int32_t flashOffMs;
  BrightnessType brightnessMode ALIGNED(4);
} LightState;
G_STATIC_ASSERT(sizeof(LightState) == 20);

struct _FbdDroidLedsBackendInterface
{
  GTypeInterface parent_iface;

  gboolean (*is_supported) (FbdDroidLedsBackend *self);
  gboolean (*start_periodic) (FbdDroidLedsBackend *self,
                              FbdFeedbackLedColor color,
                              guint               max_brightness,
                              guint               freq);
  gboolean (*stop) (FbdDroidLedsBackend *self,
                    FbdFeedbackLedColor color);
};

int32_t fbd_droid_leds_backend_get_argb_color (FbdFeedbackLedColor color,
                                               guint               max_brightness);
gboolean fbd_droid_leds_backend_is_supported (FbdDroidLedsBackend *self);
gboolean fbd_droid_leds_backend_start_periodic (FbdDroidLedsBackend *self,
                                                FbdFeedbackLedColor color,
                                                guint               max_brightness,
                                                guint               freq);
gboolean fbd_droid_leds_backend_stop (FbdDroidLedsBackend  *self,
                                      FbdFeedbackLedColor color);

G_END_DECLS
