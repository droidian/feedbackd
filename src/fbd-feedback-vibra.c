/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "fbd-feedback-vibra"

#include "fbd-enums.h"
#include "fbd-feedback-vibra.h"
#include "fbd-feedback-manager.h"

/**
 * SECTION:fbd-feedback-vibra
 * @short_description: Describes a feedback via a haptic motor
 * @Title: FbdFeedbackVibra
 *
 * The #FbdVibraVibra describes the properties of a haptic feedback
 * event. It knows nothing about the hardware itself but calls
 * #FbdDevVibra for that.
 */

enum {
  PROP_0,
  PROP_DURATION,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _FbdFeedbackVibraPrivate {
  guint duration;
  guint timer_id;
} FbdFeedbackVibraPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (FbdFeedbackVibra, fbd_feedback_vibra, FBD_TYPE_FEEDBACK_BASE);


static gboolean
on_timeout_expired (FbdFeedbackVibra *self)
{
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default ();
  FbdDevVibra *dev = fbd_feedback_manager_get_dev_vibra (manager);

  fbd_dev_vibra_remove_effect (dev);
  fbd_feedback_base_done (FBD_FEEDBACK_BASE(self));
  return G_SOURCE_REMOVE;
}

static void
fbd_feedback_vibra_run (FbdFeedbackBase *base)
{
  FbdFeedbackVibra *self = FBD_FEEDBACK_VIBRA (base);
  FbdFeedbackVibraPrivate *priv = fbd_feedback_vibra_get_instance_private (self);
  FbdFeedbackVibraClass *klass;

  klass = FBD_FEEDBACK_VIBRA_GET_CLASS (self);
  g_return_if_fail (klass->start_vibra);
  klass->start_vibra (self);

  priv->timer_id = g_timeout_add (priv->duration,
				  (GSourceFunc)on_timeout_expired,
				  self);
  g_source_set_name_by_id (priv->timer_id, "feedback-vibra-timer");
}


static void
fbd_feedback_vibra_end (FbdFeedbackBase *base)
{
  FbdFeedbackVibra *self = FBD_FEEDBACK_VIBRA (base);
  FbdFeedbackVibraPrivate *priv = fbd_feedback_vibra_get_instance_private (self);
  FbdFeedbackVibraClass *klass = FBD_FEEDBACK_VIBRA_GET_CLASS (self);

  if (!priv->timer_id)
    return;

  g_return_if_fail (klass->end_vibra);
  klass->end_vibra(self);
  g_clear_handle_id(&priv->timer_id, g_source_remove);
  fbd_feedback_base_done (FBD_FEEDBACK_BASE(self));
}


static void
fbd_feedback_vibra_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  FbdFeedbackVibra *self = FBD_FEEDBACK_VIBRA (object);
  FbdFeedbackVibraPrivate *priv = fbd_feedback_vibra_get_instance_private (self);

  switch (property_id) {
  case PROP_DURATION:
    priv->duration = g_value_get_uint (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
fbd_feedback_vibra_get_property (GObject    *object,
				 guint       property_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
  FbdFeedbackVibra *self = FBD_FEEDBACK_VIBRA (object);
  FbdFeedbackVibraPrivate *priv = fbd_feedback_vibra_get_instance_private (self);

  switch (property_id) {
  case PROP_DURATION:
    g_value_set_uint (value, priv->duration);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
fbd_feedback_vibra_class_init (FbdFeedbackVibraClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  FbdFeedbackBaseClass *base_class = FBD_FEEDBACK_BASE_CLASS (klass);

  object_class->set_property = fbd_feedback_vibra_set_property;
  object_class->get_property = fbd_feedback_vibra_get_property;

  base_class->run = fbd_feedback_vibra_run;
  base_class->end = fbd_feedback_vibra_end;

  props[PROP_DURATION] =
    g_param_spec_uint (
      "duration",
      "Duration",
      "Vibra event duration in msecs",
      0, G_MAXUINT, FBD_FEEDBACK_VIBRA_DEFAULT_DURATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
fbd_feedback_vibra_init (FbdFeedbackVibra *self)
{
}

guint
fbd_feedback_vibra_get_duration (FbdFeedbackVibra *self)
{
  FbdFeedbackVibraPrivate *priv;

  g_return_val_if_fail (FBD_IS_FEEDBACK_VIBRA (self), 0);
  priv = fbd_feedback_vibra_get_instance_private (self);
  return priv->duration;
}
