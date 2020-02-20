/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "libfeedback.h"
#include "lfb-priv.h"

#include <gio/gio.h>

/**
 * SECTION:lfb-event
 * @Short_description: An event triggering feedback to the user
 * @Title: LfbEvent
 *
 * #LfbEvent represents an event that should trigger
 * audio, haptic and/or visual feedback to the user by triggering
 * feedback on a feedback daemon.
 *
 * One event can trigger several feedbacks at once (e.g. audio and
 * haptic feedback). This is determined by the feedback theme in
 * use (which is not under the appliction's control) and the active
 * feedback profile (see #lfb_set_feedback_profile()).
 *
 * After initializing the library feedback can be triggered like
 *
 * |[
 *    g_autoptr (GError) err = NULL;
 *    LpfEvent *event = lfb_event_new ("message-new-instant");
 *    ret = lfb_event_trigger_feedback (event, &err);
 * ]|
 *
 * When all feedback for this event has ended the #LfbEvent::feedback-ended
 * signal is emitted. If you want to end the feedback ahead of time use
 * #lfb_event_end_feedback ().
 * Since these methods involve DBus calls there are asynchronous variants
 * available, see e.g. #lfb_event_trigger_feedback_async().
 */

enum {
  PROP_0,
  PROP_EVENT,
  PROP_TIMEOUT,
  PROP_STATE,
  PROP_END_REASON,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
      SIGNAL_FEEDBACK_ENDED,
      N_SIGNALS,
};
static guint signals[N_SIGNALS];

typedef struct _LfbEvent {
  GObject        parent;

  char          *event;
  gint           timeout;

  guint          id;
  LfbEventState  state;
  gint           end_reason;
  gulong         handler_id;
} LfbEvent;

G_DEFINE_TYPE (LfbEvent, lfb_event, G_TYPE_OBJECT);

typedef struct _LpfAsyncData {
  LfbEvent *event;
  GTask    *task;
} LpfAsyncData;

static void
lfb_event_set_state (LfbEvent *self, LfbEventState state)
{
  if (self->state == state)
    return;

  self->state = state;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);
}

static void
lfb_event_set_end_reason (LfbEvent *self, LfbEventEndReason reason)
{
  if (self->end_reason == reason)
    return;

  self->end_reason = reason;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_END_REASON]);
}

static GVariant *
build_hints (void)
{
  GVariantBuilder hints_builder;

  g_variant_builder_init (&hints_builder, G_VARIANT_TYPE ("a{sv}"));
  return g_variant_new ("a{sv}", &hints_builder);
}

static void
on_trigger_feedback_finished (LfbGdbusFeedback *proxy,
                              GAsyncResult     *res,
                              LpfAsyncData     *data)

{
  GTask *task = data->task;
  LfbEvent *self = data->event;
  GError *err = NULL;
  gboolean success;
  LfbEventState state;

  g_return_if_fail (G_IS_TASK (task));
  g_return_if_fail (LFB_GDBUS_IS_FEEDBACK (proxy));
  g_return_if_fail (LFB_IS_EVENT (self));

  success = lfb_gdbus_feedback_call_trigger_feedback_finish (proxy,
                                                             &self->id,
                                                             res,
                                                             &err);
  if (!success) {
    g_task_return_error (task, err);
    state = LFB_EVENT_STATE_ERRORED;
  } else {
    g_task_return_boolean (task, TRUE);
    state = LFB_EVENT_STATE_RUNNING;
  }

  lfb_event_set_state (self, state);
  g_free (data);
  g_object_unref (task);
  g_object_unref (self);
}

static void
on_end_feedback_finished (LfbGdbusFeedback *proxy,
                          GAsyncResult     *res,
                          LpfAsyncData     *data)

{
  GTask *task = data->task;
  LfbEvent *self = data->event;
  GError *err = NULL;
  gboolean success;

  g_return_if_fail (G_IS_TASK (task));
  g_return_if_fail (LFB_GDBUS_IS_FEEDBACK (proxy));
  g_return_if_fail (LFB_IS_EVENT (self));

  success = lfb_gdbus_feedback_call_end_feedback_finish (proxy,
							 res,
							 &err);
  if (!success) {
    g_task_return_error (task, err);
  } else
    g_task_return_boolean (task, TRUE);

  g_free (data);
  g_object_unref (task);
  g_object_unref (self);
}

static void
lfb_event_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LfbEvent *self = LFB_EVENT (object);

  switch (property_id) {
  case PROP_EVENT:
    g_free (self->event);
    self->event = g_value_dup_string (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EVENT]);
    break;
  case PROP_TIMEOUT:
    lfb_event_set_timeout (self, g_value_get_int (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
lfb_event_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LfbEvent *self = LFB_EVENT (object);

  switch (property_id) {
  case PROP_EVENT:
    g_value_set_string (value, self->event);
    break;
  case PROP_TIMEOUT:
    g_value_set_int (value, self->timeout);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lfb_event_finalize (GObject *object)
{
  LfbEvent *self = LFB_EVENT (object);

  /* Signal handler is disconnected automatically due to due to g_signal_connect_object */
  self->handler_id = 0;

  g_clear_pointer (&self->event, g_free);

  G_OBJECT_CLASS (lfb_event_parent_class)->finalize (object);
}

static void
lfb_event_class_init (LfbEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = lfb_event_set_property;
  object_class->get_property = lfb_event_get_property;

  object_class->finalize = lfb_event_finalize;

  /**
   * LpfEvent:event:
   *
   * The type of event from the Event naming spec, e.g. 'message-new-instant'.
   */
  props[PROP_EVENT] =
    g_param_spec_string (
      "event",
      "Event",
      "The name of the event triggering the feedback",
      NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * LpfEvent:timeout:
   *
   * How long feedback should be provided in milliseconds. The special value
   * %-1 uses the natural length of each feedback while %0 plays each feedback
   * in a loop until ended explicitly.
   */
  props[PROP_TIMEOUT] =
    g_param_spec_int (
      "timeout",
      "Timeout",
      "When the event should timeout",
      -1, G_MAXINT, -1,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_STATE] =
    g_param_spec_enum (
      "state",
      "State",
      "The event's state",
      LFB_TYPE_EVENT_STATE,
      LFB_EVENT_END_REASON_NATURAL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_END_REASON] =
    g_param_spec_enum (
      "end-reason",
      "End reason",
      "The reason why the feedbacks ended",
      LFB_TYPE_EVENT_END_REASON,
      LFB_EVENT_END_REASON_NATURAL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  /**
   * LfbEvent::feedback-ended:
   *
   * Emitted when all feedbacks triggered by the event have ended.
   */
  signals[SIGNAL_FEEDBACK_ENDED] = g_signal_new ("feedback-ended",
                                                 G_TYPE_FROM_CLASS (klass),
                                                 G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                 NULL,
                                                 G_TYPE_NONE,
                                                 0);
}

static void
lfb_event_init (LfbEvent *self)
{
  self->timeout = -1;
  self->state = LFB_EVENT_STATE_NONE;
  self->end_reason = LFB_EVENT_END_REASON_NATURAL;
}

/**
 * lfb_event_new:
 * @event: The event's name.
 *
 * Creates a new #LfbEvent based on the given event
 * name. See #LfbEvent:event for details.
 *
 * Returns: The #LfbEvent.
 */
LfbEvent *
lfb_event_new (const char *event)
{
  return g_object_new (LFB_TYPE_EVENT, "event", event, NULL);
}

static void
on_feedback_ended (LfbEvent         *self,
                   guint             event_id,
                   guint             reason,
                   LfbGdbusFeedback *proxy)
{
  g_return_if_fail (LFB_IS_EVENT (self));
  g_return_if_fail (LFB_GDBUS_IS_FEEDBACK (proxy));

  if (event_id != self->id)
    return;

  lfb_event_set_end_reason (self, reason);
  lfb_event_set_state (self, LFB_EVENT_STATE_ENDED);
  g_signal_emit (self, signals[SIGNAL_FEEDBACK_ENDED], 0);
  self->id = 0;
  g_signal_handler_disconnect (proxy, self->handler_id);
  self->handler_id = 0;
}

/**
 * lfb_event_trigger_feedback:
 * @self: The event to trigger feedback for.
 * @error: The returned error information.
 *
 * Tells the feedback server to provide proper feedback for the give
 * event to the user.
 *
 * Returns: %TRUE if successful. On error, this will return %FALSE and set
 *          @error.
 */
gboolean
lfb_event_trigger_feedback (LfbEvent *self, GError **error)
{
  LfbGdbusFeedback *proxy;
  gboolean success;

  g_return_val_if_fail (LFB_IS_EVENT (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

   if (!lfb_is_initted ()) {
     g_warning ("you must call lfb_init() before triggering events");
     g_assert_not_reached ();
   }

   proxy = _lfb_get_proxy ();
   g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), FALSE);

   if (self->handler_id == 0) {
     self->handler_id = g_signal_connect_object (proxy,
                                                 "feedback-ended",
                                                 G_CALLBACK (on_feedback_ended),
                                                 self,
                                                 G_CONNECT_SWAPPED);
   }

   success =  lfb_gdbus_feedback_call_trigger_feedback_sync (proxy,
                                                             lfb_get_app_id (),
                                                             self->event,
                                                             build_hints (),
                                                             self->timeout,
                                                             &self->id,
                                                             NULL,
                                                             error);
   lfb_event_set_state (self, success ? LFB_EVENT_STATE_RUNNING : LFB_EVENT_STATE_ERRORED);
   return success;
}

/**
 * lfb_event_trigger_feedback_async:
 * @self: The event to trigger feedback for.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Tells the feedback server to provide proper feedback for the give
 * event to the user. This is the sync version of
 * #lfb_event_trigger_feedback.
 */
void
lfb_event_trigger_feedback_async (LfbEvent            *self,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  LpfAsyncData *data;
  LfbGdbusFeedback *proxy;

  g_return_if_fail (LFB_IS_EVENT (self));
  if (!lfb_is_initted ()) {
     g_warning ("you must call lfb_init() before triggering events");
     g_assert_not_reached ();
  }

  proxy = _lfb_get_proxy ();
  g_return_if_fail (LFB_GDBUS_IS_FEEDBACK (proxy));

  data = g_new0 (LpfAsyncData, 1);
  data->task = g_task_new (self, cancellable, callback, user_data);
  data->event = g_object_ref (self);
  lfb_gdbus_feedback_call_trigger_feedback (proxy,
                                            lfb_get_app_id (),
                                            self->event,
                                            build_hints (),
                                            self->timeout,
                                            cancellable,
                                            (GAsyncReadyCallback)on_trigger_feedback_finished,
                                            data);
}

/**
 * lfb_event_trigger_feedback_finish:
 * @self: the event
 * @res: Result object passed to the callback of
 *  #lfb_event_trigger_feedback_async
 * @error: Return location for error
 *
 * Finish an async operation started by lfb_event_trigger_feedback_async. You
 * must call this function in the callback to free memory and receive any
 * errors which occurred.
 *
 * Returns: %TRUE if playing finished successfully
 */
gboolean
lfb_event_trigger_feedback_finish (LfbEvent      *self,
                                   GAsyncResult  *res,
                                   GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (res, self), FALSE);

  return g_task_propagate_boolean (G_TASK (res), error);
}

/**
 * lfb_event_end_feedback:
 * @self: The event to end feedback for.
 * @error: The returned error information.
 *
 * Tells the feedback server to end all feedback for the given event as
 * soon as possible.
 *
 * Returns: %TRUE if successful. On error, this will return %FALSE and set
 *          @error.
 */
gboolean
lfb_event_end_feedback (LfbEvent *self, GError **error)
{
  LfbGdbusFeedback *proxy;

  g_return_val_if_fail (LFB_IS_EVENT (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!lfb_is_initted ()) {
     g_warning ("you must call lfb_init() before ending events");
     g_assert_not_reached ();
  }

  proxy = _lfb_get_proxy ();
  g_return_val_if_fail (LFB_GDBUS_IS_FEEDBACK (proxy), FALSE);
  return lfb_gdbus_feedback_call_end_feedback_sync (proxy, self->id, NULL, error);
}

/**
 * lfb_event_end_feedback_finish:
 * @self: the event
 * @res: Result object passed to the callback of
 *  #lfb_event_end_feedback_async
 * @error: Return location for error
 *
 * Finish an async operation started by lfb_event_end_feedback_async. You
 * must call this function in the callback to free memory and receive any
 * errors which occurred.
 *
 * Returns: %TRUE if playing finished successfully
 */
gboolean
lfb_event_end_feedback_finish (LfbEvent      *self,
			       GAsyncResult  *res,
			       GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (res, self), FALSE);

  return g_task_propagate_boolean (G_TASK (res), error);
}

/**
 * lfb_event_end_feedback_async:
 * @self: The event to end feedback for.
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL.
 * @user_data: User data to pass to @callback.
 *
 * Tells the feedback server to end all feedback for the given event as
 * soon as possible.
 */
void
lfb_event_end_feedback_async (LfbEvent            *self,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
  LpfAsyncData *data;
  LfbGdbusFeedback *proxy;

  g_return_if_fail (LFB_IS_EVENT (self));
  if (!lfb_is_initted ()) {
     g_warning ("you must call lfb_init() before ending events");
     g_assert_not_reached ();
  }

  proxy = _lfb_get_proxy ();
  g_return_if_fail (LFB_GDBUS_IS_FEEDBACK (proxy));

  data = g_new0 (LpfAsyncData, 1);
  data->task = g_task_new (self, cancellable, callback, user_data);
  data->event = g_object_ref (self);
  lfb_gdbus_feedback_call_end_feedback (proxy,
                                        self->id,
                                        cancellable,
                                        (GAsyncReadyCallback)on_end_feedback_finished,
                                        data);
}

/**
 * lfb_event_set_timeout:
 * @self: The event
 * @timeout: The timeout
 *
 * Tells the feedback server to end feedack after #timeout seconds.
 * The value -1 indicates to not set a timeout and let feedbacks stop
 * on their own while 0 indicates to loop all feedbacks endlessly.
 * They must be stopped via #lfb_event_end_feedback () in this case.
 *
 * It is an error to change the timeout after the feedback has been triggered
 * via lfb_event_trigger.
 */
void
lfb_event_set_timeout (LfbEvent *self, gint timeout)
{
  g_return_if_fail (LFB_IS_EVENT (self));

  if (self->timeout == timeout)
    return;

  self->timeout = timeout;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TIMEOUT]);
}

/**
 * lfb_event_get_event:
 * @self: The event
 *
 * Get the event's name according to the event naming spec.
 *
 * Returns: The event name
 */
const char *
lfb_event_get_event (LfbEvent *self)
{
  g_return_val_if_fail (LFB_IS_EVENT (self), NULL);
  return self->event;
}

/**
 * lfb_event_get_timeout:
 * @self: The event
 *
 * Get the currently set timeout.
 *
 * Returns: The event timeout in msecs
 */
gint
lfb_event_get_timeout (LfbEvent *self)
{
  g_return_val_if_fail (LFB_IS_EVENT (self), -1);
  return self->timeout;
}

/**
 * lfb_event_get_state:
 * @self: The event
 *
 * Get the current event state (e.g. if triggered feeedback is
 * currently running.
 *
 * Returns: The state of the feedback triggered by event.
 */
LfbEventState
lfb_event_get_state (LfbEvent *self)
{
  g_return_val_if_fail (LFB_IS_EVENT (self), LFB_EVENT_STATE_NONE);
  return self->state;
}

/**
 * lfb_event_get_end_reason:
 * @self: The event
 *
 * Get the reason why the feadback ended.
 *
 * Returns: The reason why feedback ended.
 */
LfbEventEndReason
lfb_event_get_end_reason (LfbEvent *self)
{
  g_return_val_if_fail (LFB_IS_EVENT (self), LFB_EVENT_END_REASON_NATURAL);
  return self->end_reason;
}
