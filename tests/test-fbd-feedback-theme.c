/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "fbd-feedback-dummy.h"
#include "fbd-feedback-theme.h"

#include <json-glib/json-glib.h>

#define THEME_NAME "test"

static void
test_fbd_feedback_theme_name (void)
{
  g_autoptr(FbdFeedbackTheme) theme = fbd_feedback_theme_new (THEME_NAME);

  g_assert_true (FBD_IS_FEEDBACK_THEME (theme));

  g_assert_cmpstr (fbd_feedback_theme_get_name (theme), ==, THEME_NAME);
}

static void
test_fbd_feedback_theme_profiles (void)
{
  g_autoptr (FbdFeedbackDummy) quiet_fb1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY,
							 "event-name", "event1",
							 NULL);
  g_autoptr (FbdFeedbackDummy) quiet_fb2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY,
							 "event-name", "event2",
							 NULL);
  g_autoptr (FbdFeedbackDummy) full_fb1 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY,
							"event-name", "event1",
							NULL);
  g_autoptr (FbdFeedbackDummy) full_fb2 = g_object_new (FBD_TYPE_FEEDBACK_DUMMY,
							"event-name", "event2",
							NULL);
  g_autoptr(FbdFeedbackTheme) theme = fbd_feedback_theme_new (THEME_NAME);

  FbdFeedbackProfile *profile_full = fbd_feedback_profile_new ("full");
  FbdFeedbackProfile *profile_quiet = fbd_feedback_profile_new ("quiet");
  FbdFeedbackProfile *profile;

  fbd_feedback_profile_add_feedback (profile_quiet, FBD_FEEDBACK_BASE(quiet_fb1));
  fbd_feedback_profile_add_feedback (profile_quiet, FBD_FEEDBACK_BASE(quiet_fb2));

  fbd_feedback_profile_add_feedback (profile_full, FBD_FEEDBACK_BASE(full_fb1));
  fbd_feedback_profile_add_feedback (profile_full, FBD_FEEDBACK_BASE(full_fb2));

  g_assert_true (FBD_IS_FEEDBACK_THEME (theme));
  fbd_feedback_theme_add_profile (theme, profile_quiet);
  fbd_feedback_theme_add_profile (theme, profile_full);

  profile = fbd_feedback_theme_get_profile (theme, "full");
  g_assert_true (FBD_IS_FEEDBACK_PROFILE (profile));

  g_print ("%s\n", json_gobject_to_data (G_OBJECT(theme), NULL));
}


static void
test_fbd_feedback_theme_parse (void)
{
  const char *json ="                             "
        "{                                        "
        "  \"name\" : \"test\",                   "
        "  \"profiles\" : [                       "
        "    {                                    "
        "      \"name\" : \"full\",               "
        "      \"feedbacks\" : [                  "
        "        {                                "
        "          \"type\" : \"dummy\",          "
        "          \"event-name\" : \"event1\"    "
        "        },                               "
        "        {                                "
        "          \"type\" : \"dummy\",          "
        "          \"event-name\" : \"event2\"    "
        "        }                                "
        "      ]                                  "
        "    },                                   "
        "    {                                    "
        "      \"name\" : \"quiet\",              "
        "      \"feedbacks\" : [                  "
        "        {                                "
        "          \"type\" : \"dummy\",          "
        "          \"event-name\" : \"event1\"    "
        "        },                               "
        "        {                                "
        "          \"type\" : \"dummy\",          "
        "          \"event-name\" : \"event2\"    "
        "        }                                "
        "      ]                                  "
        "    }                                    "
        "  ]                                      "
        "}                                        ";
  g_autoptr (GError) err = NULL;
  g_autoptr (FbdFeedbackTheme) theme = NULL;
  FbdFeedbackProfile *profile;

  theme = fbd_feedback_theme_new_from_data (json, &err);
  g_assert_no_error (err);
  g_assert_nonnull (theme);
  profile = fbd_feedback_theme_get_profile (theme, "full");
  g_assert_true (FBD_IS_FEEDBACK_PROFILE(profile));
  g_assert_cmpstr ("full", ==,
		   fbd_feedback_profile_get_name (profile));
  profile = fbd_feedback_theme_get_profile (theme, "quiet");
  g_assert_true (FBD_IS_FEEDBACK_PROFILE(profile));
  g_assert_cmpstr ("quiet", ==,
		   fbd_feedback_profile_get_name (profile));
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/feedbackd/fbd/feedback-theme/name", test_fbd_feedback_theme_name);
  g_test_add_func("/feedbackd/fbd/feedback-theme/profiles", test_fbd_feedback_theme_profiles);
  g_test_add_func("/feedbackd/fbd/feedback-theme/parse", test_fbd_feedback_theme_parse);

  return g_test_run();
}
