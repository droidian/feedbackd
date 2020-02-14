/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "libfeedback.h"
#include <gio/gio.h>

typedef struct {
  GTestDBus *dbus;

} TestFixture;

GMainLoop *mainloop;

static void
fixture_setup (TestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  gchar *relative, *servicesdir;
  gint success;

  fixture->dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
  relative = g_test_build_filename (G_TEST_BUILT, "services", NULL);
  servicesdir = g_canonicalize_filename (relative, NULL);
  g_free (relative);

  g_test_dbus_add_service_dir (fixture->dbus, servicesdir);
  g_free (servicesdir);
  g_setenv ("FEEDBACK_THEME", TEST_DATA_DIR "/test.json", TRUE);
  g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);
  g_test_dbus_up (fixture->dbus);

  g_assert_null (mainloop);
  mainloop = g_main_loop_new (NULL, FALSE);

  success = lfb_init (TEST_APP_ID, &err);
  g_assert_no_error (err);
  g_assert_true (success);
}

static void
fixture_teardown (TestFixture *fixture, gconstpointer unused)
{
  g_clear_pointer (&mainloop, g_main_loop_unref);
  lfb_uninit ();

  g_test_dbus_down (fixture->dbus);
  g_object_unref (fixture->dbus);
}

static void
on_feedback_ended (LfbEvent *event, LfbEvent **cmp)
{
  g_assert_true (LFB_IS_EVENT (event));
  g_assert_null (*cmp);

  g_debug ("Feedback ended for %s", lfb_event_get_event (event));
  g_assert_cmpint (lfb_event_get_end_reason (event), ==, LFB_EVENT_END_REASON_EXPLICIT);

  /* "Return" event */
  *cmp = event;
  g_main_loop_quit (mainloop);
}

static void
test_lfb_integration_event_sync (void)
{
  g_autoptr(LfbEvent) event0 = NULL;
  g_autoptr(LfbEvent) event1 = NULL;
  g_autoptr(LfbEvent) event10 = NULL;
  g_autofree gchar *evname = NULL;
  g_autoptr (GError) err = NULL;
  LfbEvent *cmp = NULL;
  gboolean success;

  event0 = lfb_event_new ("test-dummy-0");
  success = lfb_event_trigger_feedback (event0, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  event10 = lfb_event_new ("test-dummy-10");
  g_signal_connect (event10, "feedback-ended", (GCallback)on_feedback_ended, &cmp);
  success = lfb_event_trigger_feedback (event10, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  success = lfb_event_end_feedback (event10, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  g_main_loop_run (mainloop);

  /* If the signal fired cmp will match event */
  g_assert_true (event10 == cmp);
  g_assert_cmpint (lfb_event_get_state (event10), ==, LFB_EVENT_STATE_ENDED);
  g_assert_cmpint (lfb_event_get_end_reason (event10), ==, LFB_EVENT_END_REASON_EXPLICIT);
}

static void
on_event_triggered (LfbEvent      *event,
		    GAsyncResult  *res,
		    LfbEvent     **cmp)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_assert_true (LFB_IS_EVENT (event));
  g_assert_null (*cmp);

  g_debug ("%s", __func__);
  success = lfb_event_trigger_feedback_finish (event, res, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  /* "Return" event */
  *cmp = event;
  g_main_loop_quit (mainloop);
}

static void
on_event_ended (LfbEvent      *event,
		GAsyncResult  *res,
		LfbEvent     **cmp)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_debug ("%s", __func__);
  g_assert_true (LFB_IS_EVENT (event));
  g_assert_null (*cmp);

  success = lfb_event_end_feedback_finish (event, res, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  /* "Return" event */
  *cmp = event;
  g_main_loop_quit (mainloop);
}

static void
test_lfb_integration_event_async (void)
{
  g_autoptr(LfbEvent) event0 = NULL;
  g_autoptr(LfbEvent) event10 = NULL;
  g_autofree gchar *evname = NULL;
  g_autoptr (GError) err = NULL;
  LfbEvent *cmp = NULL;
  gboolean success;

  event0 = lfb_event_new ("test-dummy-0");
  lfb_event_trigger_feedback_async (event0,
				    NULL,
				    (GAsyncReadyCallback)on_event_triggered,
				    &cmp);
  g_main_loop_run (mainloop);
  /* The async finish callback saw the right event */
  g_assert_true (event0 == cmp);
  cmp = NULL;

  event10 = lfb_event_new ("test-dummy-10");
  success = lfb_event_trigger_feedback (event10, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  lfb_event_end_feedback_async (event10,
				NULL,
				(GAsyncReadyCallback)on_event_ended,
				&cmp);

  g_main_loop_run (mainloop);
  /* The async finish callback saw the right event */
  g_assert_true (event10 == cmp);
  g_assert_cmpint (lfb_event_get_state (event10), ==, LFB_EVENT_STATE_ENDED);
}

static void
on_profile_changed (LfbGdbusFeedback *proxy, GParamSpec *psepc, const gchar **profile)
{
  g_assert_null (*profile);
  *profile = lfb_get_feedback_profile();
  g_debug("Set feedback profile to: '%s'", *profile);

  g_main_loop_quit (mainloop);
}

static void
test_lfb_integration_profile (void)
{
  g_autoptr (GError) err = NULL;
  LfbGdbusFeedback *proxy;
  gchar *cmp = NULL;

  g_assert_cmpstr (lfb_get_feedback_profile (), ==, "full");
  proxy = lfb_get_proxy ();
  g_assert_nonnull (proxy);

  lfb_set_feedback_profile ("quiet");
  g_signal_connect (proxy, "notify::profile", (GCallback)on_profile_changed, &cmp);
  g_main_loop_run (mainloop);
  g_assert_cmpstr (lfb_get_feedback_profile (), ==, "quiet");
  g_assert_cmpstr (cmp, ==, "quiet");
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add("/feedbackd/lfb-integration/event_sync", TestFixture, NULL,
	     (gpointer)fixture_setup,
	     (gpointer)test_lfb_integration_event_sync,
	     (gpointer)fixture_teardown);

  g_test_add("/feedbackd/lfb-integration/event_async", TestFixture, NULL,
	     (gpointer)fixture_setup,
	     (gpointer)test_lfb_integration_event_async,
	     (gpointer)fixture_teardown);

  g_test_add("/feedbackd/lfb-integration/profile", TestFixture, NULL,
	     (gpointer)fixture_setup,
	     (gpointer)test_lfb_integration_profile,
	     (gpointer)fixture_teardown);

  return g_test_run();
}
