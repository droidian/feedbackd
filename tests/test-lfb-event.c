/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "libfeedback.h"
#include <gio/gio.h>

/* Property set/get */
static void
test_lfb_event_props (void)
{
  g_autoptr(LfbEvent) event = NULL;
  g_autofree gchar *evname = NULL;
  g_autofree gchar *profile = NULL;
  gint timeout;

  g_assert_true (lfb_init (TEST_APP_ID, NULL));

  event = lfb_event_new ("window-close");
  g_assert_true (LFB_IS_EVENT (event));

  g_object_get (event, "event", &evname, NULL);
  g_assert_cmpstr (evname, ==, "window-close");

  g_object_get (event, "timeout", &timeout, NULL);
  g_assert_cmpint (timeout, ==, -1);

  g_assert_cmpint (lfb_event_get_end_reason (event), ==, LFB_EVENT_END_REASON_NATURAL);
  g_assert_cmpint (lfb_event_get_state (event), ==, LFB_EVENT_STATE_NONE);

  g_object_get (event, "feedback-profile", &profile, NULL);
  g_assert_null (profile);
  g_object_set (event, "feedback-profile", "full", NULL);
  g_object_get (event, "feedback-profile", &profile, NULL);
  g_assert_cmpstr (profile, ==, "full");
  g_free (profile);
  profile = lfb_event_get_feedback_profile (event);
  g_assert_cmpstr (profile, ==, "full");

  lfb_uninit ();
}

/* Test failure paths when no feedback daemon is running */
static void
test_lfb_event_trigger (void)
{
  g_autoptr(LfbEvent) event = NULL;
  g_autofree gchar *evname = NULL;
  g_autoptr (GError) err = NULL;

  g_assert_true (lfb_init (TEST_APP_ID, NULL));

  event = lfb_event_new ("window-close");
  g_assert_false (lfb_event_trigger_feedback (event, &err));
  g_assert_nonnull (err);
  g_clear_error (&err);

  g_assert_false (lfb_event_end_feedback (event, &err));
  g_assert_nonnull (err);
  g_clear_error (&err);

  lfb_uninit ();
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_autoptr(GTestDBus) bus = g_test_dbus_new (0);
  g_test_dbus_up (bus);

  g_test_add_func("/feedbackd/libfeedback/lfb-event/props", test_lfb_event_props);
  g_test_add_func("/feedbackd/libfeedback/lfb-event/trigger", test_lfb_event_trigger);

  return g_test_run();
}
