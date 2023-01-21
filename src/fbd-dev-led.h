/*
 * Copyright (C) 2023 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include "fbd-feedback-led.h"

#include <gudev/gudev.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _FbdDevLed FbdDevLed;

void                fbd_dev_led_free (FbdDevLed *led);
gboolean            fbd_dev_led_set_brightness (FbdDevLed *led, guint brightness);
FbdDevLed          *fbd_dev_led_new  (GUdevDevice *dev);
gboolean            fbd_dev_led_start_periodic (FbdDevLed           *led,
                                                FbdFeedbackLedColor  color,
                                                guint                max_brightness_percentage,
                                                guint                freq);
gboolean            fbd_dev_led_has_color (FbdDevLed *led, FbdFeedbackLedColor color);

G_END_DECLS
