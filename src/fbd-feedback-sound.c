/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "fbd-feedback-sound"

#include "fbd-enums.h"
#include "fbd-feedback-sound.h"
#include "fbd-feedback-manager.h"

/**
 * SECTION:fbd-feedback-sound
 * @short_description: Describes a feedback via a haptic motor
 * @Title: FbdFeedbackSound
 *
 * The #FbdSoundSound describes the properties of a sound feedback
 * event. It knows nothing about how to play sound itself but calls
 * #FbdDevSound for that.
 */

enum {
  PROP_0,
  PROP_EFFECT,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _FbdFeedbackSound {
  FbdFeedbackBase parent;

  gchar *effect;
} FbdFeedbackSound;

G_DEFINE_TYPE (FbdFeedbackSound, fbd_feedback_sound, FBD_TYPE_FEEDBACK_BASE);

static void
on_effect_finished (FbdFeedbackSound *self)

{
  fbd_feedback_base_done (FBD_FEEDBACK_BASE(self));
}

static void
fbd_feedback_sound_run (FbdFeedbackBase *base)
{
  FbdFeedbackSound *self = FBD_FEEDBACK_SOUND (base);
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default ();
  FbdDevSound *sound = fbd_feedback_manager_get_dev_sound (manager);

  g_return_if_fail (FBD_IS_DEV_SOUND (sound));
  g_debug ("Sound event %s", self->effect);
  fbd_dev_sound_play (sound, self, on_effect_finished);
}


static void
fbd_feedback_sound_end (FbdFeedbackBase *base)
{
  FbdFeedbackSound *self = FBD_FEEDBACK_SOUND (base);
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default ();
  FbdDevSound *sound = fbd_feedback_manager_get_dev_sound (manager);

  fbd_dev_sound_stop (sound, self);
}

static gboolean
fbd_feedback_sound_is_available (FbdFeedbackBase *base)
{
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default ();
  FbdDevSound *sound = fbd_feedback_manager_get_dev_sound (manager);

  return FBD_IS_DEV_SOUND (sound);
}

static void
fbd_feedback_sound_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  FbdFeedbackSound *self = FBD_FEEDBACK_SOUND (object);

  switch (property_id) {
  case PROP_EFFECT:
    self->effect = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
fbd_feedback_sound_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  FbdFeedbackSound *self = FBD_FEEDBACK_SOUND (object);

  switch (property_id) {
  case PROP_EFFECT:
    g_value_set_string (value, fbd_feedback_sound_get_effect (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
fbd_feedback_sound_finalize (GObject *object)
{
  FbdFeedbackSound *self = FBD_FEEDBACK_SOUND (object);

  g_clear_pointer (&self->effect, g_free);

  G_OBJECT_CLASS (fbd_feedback_sound_parent_class)->finalize (object);
}

static void
fbd_feedback_sound_class_init (FbdFeedbackSoundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  FbdFeedbackBaseClass *base_class = FBD_FEEDBACK_BASE_CLASS (klass);

  object_class->finalize = fbd_feedback_sound_finalize;
  object_class->set_property = fbd_feedback_sound_set_property;
  object_class->get_property = fbd_feedback_sound_get_property;

  base_class->run = fbd_feedback_sound_run;
  base_class->end = fbd_feedback_sound_end;
  base_class->is_available = fbd_feedback_sound_is_available;

  props[PROP_EFFECT] =
    g_param_spec_string (
      "effect",
      "Effect",
      "The sound effects name",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
fbd_feedback_sound_init (FbdFeedbackSound *self)
{
}

const char *fbd_feedback_sound_get_effect (FbdFeedbackSound *self)
{
  return self->effect;
}

gboolean
fbd_dev_sound_stop(FbdDevSound *self, FbdFeedbackSound *feedback)
{
  /* TODO: can we use cancellable to actually end playback? */
  return TRUE;
}
