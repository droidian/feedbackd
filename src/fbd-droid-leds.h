/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include "fbd-feedback-led.h"

#include <stdint.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define ALIGNED(x) __attribute__ ((aligned(x)))

#define FBD_TYPE_DEV_LEDS (fbd_dev_leds_get_type ())

G_DECLARE_FINAL_TYPE (FbdDevLeds, fbd_dev_leds, FBD, DEV_LEDS, GObject);

enum {
    LIGHT_TYPE_BACKLIGHT = 0,
    LIGHT_TYPE_KEYBOARD = 1,
    LIGHT_TYPE_BUTTONS = 2,
    LIGHT_TYPE_BATTERY = 3,
    LIGHT_TYPE_NOTIFICATIONS = 4,
    LIGHT_TYPE_ATTENTION = 5,
    LIGHT_TYPE_BLUETOOTH = 6,
    LIGHT_TYPE_WIFI = 7,
    LIGHT_TYPE_COUNT = 8,
};

enum {
    FLASH_TYPE_NONE = 0,
    FLASH_TYPE_TIMED = 1,
    FLASH_TYPE_HARDWARE = 2,
};

enum {
    BRIGHTNESS_MODE_USER = 0,
    BRIGHTNESS_MODE_SENSOR = 1,
    BRIGHTNESS_MODE_LOW_PERSISTENCE = 2,
};

typedef struct light_state {
    uint32_t color ALIGNED(4);
    int32_t flashMode ALIGNED(4);
    int32_t flashOnMs ALIGNED(4);
    int32_t flashOffMs ALIGNED(4);
    int32_t brightnessMode ALIGNED(4);
} ALIGNED(4) LightState;
G_STATIC_ASSERT(sizeof(LightState) == 20);

FbdDevLeds *fbd_dev_leds_new (GError **error);
gboolean    fbd_dev_leds_start_periodic (FbdDevLeds *self,
                                         FbdFeedbackLedColor color,
                                         guint max_brighness, guint freq);
gboolean    fbd_dev_leds_stop (FbdDevLeds         *self,
                               FbdFeedbackLedColor color);

G_END_DECLS
