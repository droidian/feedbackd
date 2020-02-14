/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include "fbd-feedback-base.h"

G_BEGIN_DECLS

#define FBD_TYPE_FEEDBACK_SOUND (fbd_feedback_sound_get_type())

G_DECLARE_FINAL_TYPE (FbdFeedbackSound, fbd_feedback_sound, FBD, FEEDBACK_SOUND, FbdFeedbackBase);

const char *fbd_feedback_sound_get_effect (FbdFeedbackSound *self);

G_END_DECLS
