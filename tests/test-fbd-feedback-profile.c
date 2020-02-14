/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "fbd-feedback-profile.h"
#include "fbd-feedback-dummy.h"
#include "fbd-feedback-vibra.h"

#include <json-glib/json-glib.h>

#define PROFILE_NAME "test"

static void
test_fbd_feedback_profile_name (void)
{
  g_autoptr(FbdFeedbackProfile) profile = fbd_feedback_profile_new (PROFILE_NAME);

  g_assert_true (FBD_IS_FEEDBACK_PROFILE (profile));

  g_assert_cmpstr (fbd_feedback_profile_get_name (profile), ==, PROFILE_NAME);
}

static void
test_fbd_feedback_profile_feedbacks (void)
{
  GHashTable *feedbacks;
  FbdFeedbackBase *fb;
  g_autoptr (FbdFeedbackDummy) fb1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY,
						   "event-name", "event1",
						   NULL);
  g_autoptr (FbdFeedbackDummy) fb2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY,
						   "event-name", "event2",
						   NULL);
  g_autoptr(FbdFeedbackProfile) profile = fbd_feedback_profile_new (PROFILE_NAME);

  g_assert_true (FBD_IS_FEEDBACK_PROFILE (profile));
  g_object_get (profile, "feedbacks", &feedbacks, NULL);
  g_assert_nonnull (feedbacks);
  g_hash_table_unref (feedbacks);
  fbd_feedback_profile_add_feedback (profile, FBD_FEEDBACK_BASE(fb1));
  fbd_feedback_profile_add_feedback (profile, FBD_FEEDBACK_BASE(fb2));

  fb = fbd_feedback_profile_get_feedback (profile, "event1");
  g_assert_cmpstr (fbd_feedback_get_event_name (fb), ==, "event1");

  fb = fbd_feedback_profile_get_feedback (profile, "does-not-exist");
  g_assert_null (fb);
}

static void
test_fbd_feedback_profile_parse (void)
{
  const char *json ="                             "
        "    {                                    "
        "      \"name\" : \"full\",               "
        "      \"feedbacks\" : [                  "
        "        {                                "
        "          \"type\" : \"vibra\",          "
        "          \"event-name\" : \"event1\"    "
        "        },                               "
        "        {                                "
        "          \"type\" : \"dummy\",          "
        "          \"event-name\" : \"event2\"    "
        "        }                                "
        "      ]                                  "
        "    }                                    ";
  g_autoptr (GError) err = NULL;
  g_autoptr (FbdFeedbackProfile) profile = NULL;
  JsonNode *node;
  FbdFeedbackBase *fb;

  node = json_from_string(json, &err);
  g_assert_no_error (err);
  profile = FBD_FEEDBACK_PROFILE (json_gobject_deserialize (FBD_TYPE_FEEDBACK_PROFILE, node));
  g_assert_nonnull (profile);
  fb = fbd_feedback_profile_get_feedback (profile, "event2");
  g_assert_true (FBD_IS_FEEDBACK_DUMMY(fb));
  fb = fbd_feedback_profile_get_feedback (profile, "event1");
  g_assert_true (FBD_IS_FEEDBACK_VIBRA(fb));
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/feedbackd/fbd/feedback-profile/name", test_fbd_feedback_profile_name);
  g_test_add_func("/feedbackd/fbd/feedback-profile/feedbacks", test_fbd_feedback_profile_feedbacks);
  g_test_add_func("/feedbackd/fbd/feedback-profile/parse", test_fbd_feedback_profile_parse);

  return g_test_run();
}
