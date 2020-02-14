/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "fbd-event.h"
#include "fbd-feedback-dummy.h"

#define TEST_EVENT "window-close"

static void
test_fbd_event (void)
{
  g_autoptr(FbdEvent) event = NULL;
  g_autofree gchar *appid = NULL;
  g_autofree gchar *name = NULL;
  FbdEventEndReason reason;
  gint timeout;

  event = fbd_event_new (1, TEST_APP_ID, TEST_EVENT, 2);

  g_assert_true (FBD_IS_EVENT (event));
  g_object_get (event,
                "end-reason", &reason,
                "event", &name,
                "app-id", &appid,
                "timeout", &timeout,
                NULL);

  g_assert_cmpstr (fbd_event_get_event (event), ==, TEST_EVENT);
  g_assert_cmpstr (name, ==, TEST_EVENT);

  g_assert_cmpstr (fbd_event_get_app_id (event), ==, TEST_APP_ID);
  g_assert_cmpstr (appid, ==, TEST_APP_ID);

  g_assert_cmpint (fbd_event_get_timeout (event), ==, timeout);
  g_assert_cmpint (timeout, ==, 2);

  g_assert_cmpint (fbd_event_get_end_reason (event), ==, FBD_EVENT_END_REASON_NORMAL);
  g_assert_cmpint (reason, ==, FBD_EVENT_END_REASON_NORMAL);
}

static void
test_fbd_event_feedback (void)
{
  GSList *feedbacks;
  g_autoptr(FbdEvent) event = NULL;
  g_autoptr(FbdFeedbackDummy) feedback1 = NULL;
  g_autoptr(FbdFeedbackDummy) feedback2 = NULL;

  event = fbd_event_new (1, TEST_APP_ID, TEST_EVENT, -1);
  feedback1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  feedback2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);

  feedbacks = fbd_event_get_feedbacks (event);
  g_assert_cmpint (g_slist_length (feedbacks), ==, 0);

  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback1));
  feedbacks = fbd_event_get_feedbacks (event);
  g_assert_cmpint (g_slist_length (feedbacks), ==, 1);

  /* Remove non existing feedback */
  fbd_event_remove_feedback (event, FBD_FEEDBACK_BASE(feedback2));
  feedbacks = fbd_event_get_feedbacks (event);
  g_assert_cmpint (g_slist_length (feedbacks), ==, 1);

  fbd_event_remove_feedback (event, FBD_FEEDBACK_BASE(feedback1));
  feedbacks = fbd_event_get_feedbacks (event);
  g_assert_cmpint (g_slist_length (feedbacks), ==, 0);

  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback1));
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback2));
  feedbacks = fbd_event_get_feedbacks (event);
  g_assert_cmpint (g_slist_length (feedbacks), ==, 2);

  g_assert_false (fbd_event_get_feedbacks_ended (event));
  fbd_event_end_feedbacks (event);
  /* Dummy feedback ends immediately */
  g_assert_true (fbd_event_get_feedbacks_ended (event));
}

static void
on_feedbacks_ended (FbdEvent *event, gboolean *ended)
{
  FBD_IS_EVENT (event);

  *ended = TRUE;
}

static void
test_fbd_event_feedback_ended (void)
{
  g_autoptr(FbdEvent) event = NULL;
  g_autoptr(FbdFeedbackDummy) feedback1 = NULL;
  g_autoptr(FbdFeedbackDummy) feedback2 = NULL;
  gboolean ended = FALSE;

  event = fbd_event_new (1, TEST_APP_ID, TEST_EVENT, FBD_EVENT_TIMEOUT_ONESHOT);
  feedback1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback1));
  feedback2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback2));

  g_signal_connect (event, "feedbacks-ended",
                    (GCallback)on_feedbacks_ended, &ended);

  fbd_event_end_feedbacks (event);
  g_assert_true (ended);
}

static void
test_fbd_event_feedback_loop (void)
{
  g_autoptr(FbdEvent) event = NULL;
  g_autoptr(FbdFeedbackDummy) feedback1 = NULL;
  g_autoptr(FbdFeedbackDummy) feedback2 = NULL;
  gboolean ended = FALSE;

  event = fbd_event_new (1, TEST_APP_ID, TEST_EVENT, FBD_EVENT_TIMEOUT_LOOP);
  feedback1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback1));
  feedback2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback2));

  g_signal_connect (event, "feedbacks-ended",
                    (GCallback)on_feedbacks_ended, &ended);
  g_assert_false (ended);

  fbd_event_end_feedbacks (event);
  g_assert_true (ended);
}

static void
test_fbd_event_feedback_timeout (void)
{
  g_autoptr(FbdEvent) event = NULL;
  g_autoptr(FbdFeedbackDummy) feedback1 = NULL;
  g_autoptr(FbdFeedbackDummy) feedback2 = NULL;
  gboolean ended = FALSE;

  event = fbd_event_new (1, TEST_APP_ID, TEST_EVENT, 1);
  feedback1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback1));
  feedback2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY, NULL);
  fbd_event_add_feedback (event, FBD_FEEDBACK_BASE(feedback2));

  g_signal_connect (event, "feedbacks-ended",
                    (GCallback)on_feedbacks_ended, &ended);
  g_assert_false (ended);

  fbd_event_end_feedbacks (event);
  g_assert_true (ended);
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/feedbackd/fbd/event/new", test_fbd_event);
  g_test_add_func("/feedbackd/fbd/event/feedbacks/props", test_fbd_event_feedback);
  g_test_add_func("/feedbackd/fbd/event/feedbacks/ended", test_fbd_event_feedback_ended);
  g_test_add_func("/feedbackd/fbd/event/feedbacks/loop", test_fbd_event_feedback_loop);
  g_test_add_func("/feedbackd/fbd/event/feedbacks/timeout", test_fbd_event_feedback_timeout);

  return g_test_run();
}
