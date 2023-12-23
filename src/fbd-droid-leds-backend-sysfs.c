/*
 * Copyright (C) 2024 Bardia Moshiri
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Bardia Moshiri <fakeshell@bardia.tech>
 */

#define G_LOG_DOMAIN "fbd-droid-leds-backend-sysfs"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "fbd-droid-leds-backend.h"
#include "fbd-droid-leds-backend-sysfs.h"

#define LED_PATH "/sys/class/leds/"
#define BRIGHTNESS_FILE "brightness"
#define BLINK_FILE "blink"

struct _FbdDroidLedsBackendSysfs
{
  GObject parent_instance;
  gchar *led_paths[3];
};

static void initable_interface_init (GInitableIface *iface);
static void fbd_droid_leds_backend_interface_init (FbdDroidLedsBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDroidLedsBackendSysfs, fbd_droid_leds_backend_sysfs, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_interface_init)
                         G_IMPLEMENT_INTERFACE (FBD_TYPE_DROID_LEDS_BACKEND,
                                                fbd_droid_leds_backend_interface_init))

static gboolean
set_led_brightness (const gchar *led_path, guint brightness) {
  int fd, fd_blink;
  ssize_t ret, ret_blink;
  gboolean result = FALSE;
  gchar *brightness_str, *blink_str;
  gchar *brightness_path, *blink_path;

  brightness_path = g_build_filename (led_path, BRIGHTNESS_FILE, NULL);
  blink_path = g_build_filename (led_path, BLINK_FILE, NULL);

  brightness_str = g_strdup_printf ("%d", brightness);
  blink_str = g_strdup_printf ("%d", brightness > 0 ? 1 : 0);

  fd = open (brightness_path, O_WRONLY);
  if (fd == -1) {
    g_warning ("Failed to open %s: %s", brightness_path, strerror (errno));
  } else {
    ret = write (fd, brightness_str, strlen (brightness_str));
    if (ret == -1) {
      g_warning ("Failed to write to %s: %s", brightness_path, strerror (errno));
    } else {
      result = TRUE;
    }

    close (fd);
  }


  fd_blink = open (blink_path, O_WRONLY);
  if (fd_blink == -1) {
    g_warning ("Failed to open %s: %s", blink_path, strerror (errno));
  } else {
    ret_blink = write (fd_blink, blink_str, strlen (blink_str));
    if (ret_blink == -1) {
      g_warning ("Failed to write to %s: %s", blink_path, strerror (errno));
    } else {
      result = TRUE;
    }
    close (fd_blink);
  }

  g_free (brightness_path);
  g_free (brightness_str);
  g_free (blink_path);
  g_free (blink_str);

  return result;
}

static gboolean
fbd_droid_leds_backend_sysfs_is_supported (FbdDroidLedsBackend *backend)
{
  return TRUE;
}

static gboolean
fbd_droid_leds_backend_sysfs_start_periodic (FbdDroidLedsBackend *backend,
                                            FbdFeedbackLedColor color,
                                            guint               max_brightness,
                                            guint               freq)
{
  FbdDroidLedsBackendSysfs *self = FBD_DROID_LEDS_BACKEND_SYSFS (backend);
  g_return_val_if_fail (FBD_IS_DROID_LEDS_BACKEND_SYSFS (self), FALSE);

  gboolean success = TRUE;

  for (int i = 0; i < 3; ++i) {
    if (!set_led_brightness (self->led_paths[i], 1)) {
        success = FALSE;
    }
  }

  return success;
}

static gboolean
fbd_droid_leds_backend_sysfs_stop (FbdDroidLedsBackend *backend,
                                  FbdFeedbackLedColor  color)
{
  FbdDroidLedsBackendSysfs *self = FBD_DROID_LEDS_BACKEND_SYSFS (backend);
  g_return_val_if_fail (FBD_IS_DROID_LEDS_BACKEND_SYSFS (self), FALSE);

  gboolean success = TRUE;

  for (int i = 0; i < 3; ++i) {
    if (!set_led_brightness (self->led_paths[i], 0)) {
        success = FALSE;
    }
  }

  return success;
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
  return TRUE;
}

static void
fbd_droid_leds_backend_sysfs_finalize (GObject *object)
{
  FbdDroidLedsBackendSysfs *self = FBD_DROID_LEDS_BACKEND_SYSFS (object);

  for (int i = 0; i < 3; ++i) {
    g_free (self->led_paths[i]);
  }

  G_OBJECT_CLASS (fbd_droid_leds_backend_sysfs_parent_class)->finalize (object);
}

static void
fbd_droid_leds_backend_sysfs_class_init (FbdDroidLedsBackendSysfsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = fbd_droid_leds_backend_sysfs_finalize;
}

static void
initable_interface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}

static void
fbd_droid_leds_backend_interface_init (FbdDroidLedsBackendInterface *iface)
{
  iface->is_supported    = fbd_droid_leds_backend_sysfs_is_supported;
  iface->start_periodic  = fbd_droid_leds_backend_sysfs_start_periodic;
  iface->stop            = fbd_droid_leds_backend_sysfs_stop;
}

static void
fbd_droid_leds_backend_sysfs_init (FbdDroidLedsBackendSysfs *self)
{
  self->led_paths[0] = g_strdup (LED_PATH "blue");
  self->led_paths[1] = g_strdup (LED_PATH "green");
  self->led_paths[2] = g_strdup (LED_PATH "red");
}

FbdDroidLedsBackendSysfs *
fbd_droid_leds_backend_sysfs_new (GError **error)
{
  return g_object_new (FBD_TYPE_DROID_LEDS_BACKEND_SYSFS, NULL);
}
