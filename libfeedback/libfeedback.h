/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define __LIBFEEDBACK_H_INSIDE__

#ifndef LIBFEEDBACK_USE_UNSTABLE_API
#error    LIBFEEDBACK is unstable API. You must define LIBFEEDBACK_USE_UNSTABLE_API before including libfeedback.h
#endif

#include "lfb-enums.h"
#include "lfb-event.h"
#include "lfb-gdbus.h"

gboolean    lfb_init (const gchar *app_id, GError **error);
void        lfb_uninit (void);
void        lfb_set_app_id (const char *app_id);
const char *lfb_get_app_id (void);
gboolean    lfb_is_initted (void);
void        lfb_set_feedback_profile (const char *profile);
const char *lfb_get_feedback_profile (void);
LfbGdbusFeedback *lfb_get_proxy (void);

G_END_DECLS
