/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "fbd-feedback-manager"

#include "lfb-names.h"
#include "fbd.h"
#include "fbd-dev-vibra.h"
#include "fbd-event.h"
#include "fbd-feedback-vibra.h"
#include "fbd-feedback-manager.h"
#include "fbd-feedback-theme.h"

#include <gio/gio.h>
#include <glib-unix.h>
#include <gudev/gudev.h>

#define FEEDBACKD_SCHEMA_ID "org.sigxcpu.feedbackd"
#define FEEDBACKD_KEY_PROFILE "profile"

#define APP_SCHEMA FEEDBACKD_SCHEMA_ID ".application"
#define APP_PREFIX "/org/sigxcpu/feedbackd/application/"


/**
 * SECTION:fbd-feedback-manager
 * @short_description: The manager processing incoming events
 * @Title: FbdFeedbackManager
 *
 * The #FbdFeedbackManager listens for DBus messages and triggers feedbacks
 * based on the incoming events.
 */

typedef struct _FbdFeedbackManager {
  LfbGdbusFeedbackSkeleton parent;

  GSettings *settings;
  FbdFeedbackProfileLevel level;
  FbdFeedbackTheme *theme;
  guint next_id;

  GHashTable *events;

  /* Hardware interaction */
  GUdevClient *client;
  FbdDevVibra *vibra;
  FbdDevSound *sound;
} FbdFeedbackManager;

static void fbd_feedback_manager_feedback_iface_init (LfbGdbusFeedbackIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdFeedbackManager,
                         fbd_feedback_manager,
                         LFB_GDBUS_TYPE_FEEDBACK_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           LFB_GDBUS_TYPE_FEEDBACK,
                           fbd_feedback_manager_feedback_iface_init));

static void
device_changes (FbdFeedbackManager *self, gchar *action, GUdevDevice *device,
                GUdevClient        *client)
{
  g_debug ("Device changes: action = %s, device = %s",
           action, g_udev_device_get_sysfs_path (device));

  if (g_strcmp0 (action, "remove") == 0 && self->vibra) {
    GUdevDevice *dev = fbd_dev_vibra_get_device (self->vibra);

    if (g_strcmp0 (g_udev_device_get_sysfs_path (dev),
		   g_udev_device_get_sysfs_path (device)) == 0) {
      g_debug ("Vibra device %s got removed", g_udev_device_get_sysfs_path (dev));
      g_clear_object (&self->vibra);
    }
  } else if (g_strcmp0 (action, "add") == 0 || !self->vibra) {
    if (!g_strcmp0 (g_udev_device_get_property (device, "FEEDBACKD_TYPE"), "vibra")) {
      g_autoptr (GError) err = NULL;

      g_debug ("Found hotplugged vibra device at %s", g_udev_device_get_sysfs_path (device));
      self->vibra = fbd_dev_vibra_new (device, &err);
      if (!self->vibra)
	g_warning ("Failed to init vibra device: %s", err->message);
    }
  }
}

static gchar *
munge_app_id (const gchar *app_id)
{
  gchar *id = g_strdup (app_id);
  gint i;

  g_strcanon (id,
              "0123456789"
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "-",
              '-');
  for (i = 0; id[i] != '\0'; i++)
    id[i] = g_ascii_tolower (id[i]);

  return id;
}

static FbdFeedbackProfileLevel
app_get_feedback_level (const gchar *app_id)
{
  g_autofree gchar *profile = NULL;
  g_autofree gchar *munged_app_id = munge_app_id (app_id);
  g_autofree gchar *path = g_strconcat (APP_PREFIX, munged_app_id, "/", NULL);
  g_autoptr (GSettings) setting =  g_settings_new_with_path (APP_SCHEMA, path);

  profile = g_settings_get_string (setting, FEEDBACKD_KEY_PROFILE);
  g_debug ("%s uses app profile %s", app_id, profile);
  return fbd_feedback_profile_level (profile);
}

static void
init_devices (FbdFeedbackManager *self)
{
  GList *devices, *l;
  g_autoptr(GError) err = NULL;

  devices = g_udev_client_query_by_subsystem (self->client, "input");

  for (l = devices; l != NULL; l = l->next) {
    GUdevDevice *dev = l->data;

    if (!g_strcmp0 (g_udev_device_get_property (dev, "FEEDBACKD_TYPE"), "vibra")) {
      g_debug ("Found vibra device");
      self->vibra = fbd_dev_vibra_new (dev, &err);
      if (!self->vibra) {
        g_warning ("Failed to init vibra device: %s", err->message);
        g_clear_error (&err);
      }
    }
  }
  if (!self->vibra)
    g_debug ("No vibra capable device found");

  self->sound = fbd_dev_sound_new (&err);
  if (!self->sound) {
    g_warning ("Failed to init sound device: %s", err->message);
    g_clear_error (&err);
  }
}

static void
on_event_feedbacks_ended (FbdFeedbackManager *self, FbdEvent *event)
{
  guint event_id;

  g_return_if_fail (FBD_IS_FEEDBACK_MANAGER (self));
  g_return_if_fail (FBD_IS_EVENT (event));

  event_id = fbd_event_get_id (event);
  event = g_hash_table_lookup (self->events, GUINT_TO_POINTER (event_id));
  if (!event) {
    g_warning ("Feedback ended for unknown event %d", event_id);
    return;
  }

  g_return_if_fail (fbd_event_get_feedbacks_ended (event));

  lfb_gdbus_feedback_emit_feedback_ended
    (LFB_GDBUS_FEEDBACK(self), event_id,
     fbd_event_get_end_reason (event));

  g_debug ("All feedbacks for event %d finished", event_id);
  g_hash_table_remove (self->events, GUINT_TO_POINTER (event_id));
}

static void
on_profile_changed (FbdFeedbackManager *self, GParamSpec *psepc, gpointer unused)
{
  const gchar *pname;

  g_return_if_fail (FBD_IS_FEEDBACK_MANAGER (self));

  pname = lfb_gdbus_feedback_get_profile (LFB_GDBUS_FEEDBACK (self));

  if (!fbd_feedback_manager_set_profile (self, pname))
    g_warning ("Invalid profile '%s'", pname);

  /* TODO: end running feedbacks that aren't allowed in new profile immediately */
}

static void
on_feedbackd_setting_changed (FbdFeedbackManager *self,
                              const gchar        *key,
                              GSettings          *settings)
{
  g_autofree gchar *profile = NULL;

  g_return_if_fail (FBD_IS_FEEDBACK_MANAGER (self));
  g_return_if_fail (G_IS_SETTINGS (settings));
  g_return_if_fail (!g_strcmp0 (key, FEEDBACKD_KEY_PROFILE));

  profile = g_settings_get_string (settings, key);
  fbd_feedback_manager_set_profile (self, profile);
}

static gboolean
fbd_feedback_manager_handle_trigger_feedback (LfbGdbusFeedback *object,
                                              GDBusMethodInvocation *invocation,
                                              const gchar *arg_app_id,
                                              const gchar *arg_event,
                                              GVariant *arg_hints,
                                              gint arg_timeout)
{
  FbdFeedbackManager *self;
  FbdEvent *event;
  GSList *feedbacks, *l;
  gint event_id;
  FbdFeedbackProfileLevel app_level, level;

  g_debug ("Event '%s' for '%s'", arg_event, arg_app_id);

  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (object), FALSE);
  g_return_val_if_fail (arg_app_id, FALSE);
  g_return_val_if_fail (arg_event, FALSE);

  self = FBD_FEEDBACK_MANAGER (object);
  if (!strlen (arg_app_id)) {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_INVALID_ARGS,
                                             "Invalid app id %s", arg_app_id);
    return TRUE;
  }

  if (!strlen (arg_event)) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_INVALID_ARGS,
                                           "Invalid event %s", arg_event);
    return TRUE;
  }

  if (arg_timeout < -1)
    arg_timeout = -1;

  event_id = self->next_id++;

  event = fbd_event_new (event_id, arg_app_id, arg_event, arg_timeout);
  g_hash_table_insert (self->events, GUINT_TO_POINTER (event_id), event);

  /* If user configured a lower feedback level for this app honor that */
  app_level = app_get_feedback_level (arg_app_id);
  level = self->level > app_level ? app_level : self->level;
  feedbacks = fbd_feedback_theme_lookup_feedback (self->theme, level, event);

  if (feedbacks) {
    for (l = feedbacks; l; l = l->next) {
      FbdFeedbackBase *fb = l->data;
      fbd_event_add_feedback (event, fb);
      g_object_unref (fb);
    }
    g_slist_free (feedbacks);

    g_signal_connect_object (event, "feedbacks-ended",
			     (GCallback) on_event_feedbacks_ended,
			     self,
			     G_CONNECT_SWAPPED);

    fbd_event_run_feedbacks (event);
  } else {
    /* No feedbacks found, so we're done */
    lfb_gdbus_feedback_emit_feedback_ended
      (LFB_GDBUS_FEEDBACK(self), event_id,
       FBD_EVENT_END_REASON_NOT_FOUND);
  }

  lfb_gdbus_feedback_complete_trigger_feedback (object, invocation, event_id);
  return TRUE;
}

static gboolean
fbd_feedback_manager_handle_end_feedback (LfbGdbusFeedback *object,
                                          GDBusMethodInvocation *invocation,
                                          guint event_id)
{
  FbdFeedbackManager *self;
  FbdEvent *event;

  g_debug ("Ending feedback for event '%d'", event_id);

  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (object), FALSE);
  g_return_val_if_fail (event_id, FALSE);

  self = FBD_FEEDBACK_MANAGER (object);

  event = g_hash_table_lookup (self->events, GUINT_TO_POINTER (event_id));
  if (event) {
    /* The last feedback ending will trigger event disposal via
       `on_fb_ended` */
    fbd_event_end_feedbacks (event);
  } else {
    g_warning ("Tried to end non-existing event %d", event_id);
  }

  lfb_gdbus_feedback_complete_end_feedback (object, invocation);
  return TRUE;
}

static void
fbd_feedback_manager_constructed (GObject *object)
{
  g_autoptr (GError) err = NULL;
  FbdFeedbackManager *self = FBD_FEEDBACK_MANAGER (object);
  const gchar *themefile;

  G_OBJECT_CLASS (fbd_feedback_manager_parent_class)->constructed (object);

  themefile = g_getenv ("FEEDBACK_THEME");
  if (!themefile)
    themefile = FEEDBACKD_THEME_DIR "/default.json";

  self->theme = fbd_feedback_theme_new_from_file (themefile, &err);
  if (!self->theme) {
    /* No point to carry on */
    g_error ("Failed to load default theme: %s", err->message);
  }

  g_signal_connect (self, "notify::profile", (GCallback)on_profile_changed, NULL);
  lfb_gdbus_feedback_set_profile (LFB_GDBUS_FEEDBACK (self),
                                  fbd_feedback_profile_level_to_string(self->level));

  self->settings = g_settings_new (FEEDBACKD_SCHEMA_ID);
  g_signal_connect_swapped (self->settings, "changed::" FEEDBACKD_KEY_PROFILE,
                            G_CALLBACK (on_feedbackd_setting_changed), self);
}

static void
fbd_feedback_manager_dispose (GObject *object)
{
  FbdFeedbackManager *self = FBD_FEEDBACK_MANAGER (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->theme);
  g_clear_object (&self->vibra);
  g_clear_object (&self->client);
  g_clear_pointer (&self->events, g_hash_table_destroy);

  G_OBJECT_CLASS (fbd_feedback_manager_parent_class)->dispose (object);
}

static void
fbd_feedback_manager_feedback_iface_init (LfbGdbusFeedbackIface *iface)
{
  iface->handle_trigger_feedback = fbd_feedback_manager_handle_trigger_feedback;
  iface->handle_end_feedback = fbd_feedback_manager_handle_end_feedback;
}

static void
fbd_feedback_manager_class_init (FbdFeedbackManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = fbd_feedback_manager_constructed;
  object_class->dispose = fbd_feedback_manager_dispose;
}

static void
fbd_feedback_manager_init (FbdFeedbackManager *self)
{
  const gchar * const subsystems[] = { "input", NULL };

  self->next_id = 1;
  self->level = FBD_FEEDBACK_PROFILE_LEVEL_FULL;

  self->client = g_udev_client_new (subsystems);
  g_signal_connect_swapped (G_OBJECT (self->client), "uevent",
			    G_CALLBACK (device_changes), self);
  init_devices (self);

  self->events = g_hash_table_new_full (g_direct_hash,
                                        g_direct_equal,
                                        NULL,
                                        (GDestroyNotify)g_object_unref);
}

FbdFeedbackManager *
fbd_feedback_manager_get_default (void)
{
  static FbdFeedbackManager *instance;

  if (instance == NULL) {
    instance = g_object_new (FBD_TYPE_FEEDBACK_MANAGER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}

FbdDevVibra *
fbd_feedback_manager_get_dev_vibra (FbdFeedbackManager *self)
{
  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (self), NULL);

  return self->vibra;
}

FbdDevSound *
fbd_feedback_manager_get_dev_sound (FbdFeedbackManager *self)
{
  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (self), NULL);

  return self->sound;
}

gboolean
fbd_feedback_manager_set_profile (FbdFeedbackManager *self, const gchar *profile)
{
  FbdFeedbackProfileLevel level;

  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (self), FALSE);

  level = fbd_feedback_profile_level (profile);

  if (level == FBD_FEEDBACK_PROFILE_LEVEL_UNKNOWN)
    return FALSE;

  if (level == self->level)
    return TRUE;

  g_debug ("Switching profile to '%s'", profile);
  self->level = level;
  lfb_gdbus_feedback_set_profile (LFB_GDBUS_FEEDBACK (self), profile);
  g_settings_set_string (self->settings, FEEDBACKD_KEY_PROFILE, profile);
  return TRUE;
}
