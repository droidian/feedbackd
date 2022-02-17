/*
 * Copyright (C) 2021 Erfan Abdi
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Erfan Abdi
 *         Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-leds-backend-hidl"

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gbinder.h>

#include "fbd-binder.h"
#include "fbd-droid-leds-backend.h"
#include "fbd-droid-leds-backend-hidl.h"

#define BINDER_LIGHT_DEFAULT_HIDL_DEVICE "/dev/hwbinder"

#define BINDER_LIGHT_HIDL_IFACE(v) "android.hardware.light@" v "::ILight"
#define BINDER_LIGHT_HIDL_SLOT "default"

#define BINDER_LIGHT_HIDL_2_0_IFACE BINDER_LIGHT_HIDL_IFACE("2.0")

/* Methods */
enum
{
  /* setLight(Type type, LightState state) generates (Status status); */
  BINDER_LIGHT_HIDL_2_0_SET_LIGHT = 1,
  /* getSupportedTypes() generates (vec<Type> types); */
  BINDER_LIGHT_HIDL_2_0_GET_SUPPORTED_TYPES = 2,
};

struct _FbdDroidLedsBackendHidl
{
  GObject parent_instance;

  GBinderServiceManager *service_manager;
  GBinderRemoteObject   *remote;
  GBinderClient         *client;
};

static void initable_interface_init (GInitableIface *iface);
static void fbd_droid_leds_backend_interface_init (FbdDroidLedsBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDroidLedsBackendHidl, fbd_droid_leds_backend_hidl, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_interface_init)
                         G_IMPLEMENT_INTERFACE (FBD_TYPE_DROID_LEDS_BACKEND,
                                                fbd_droid_leds_backend_interface_init))

static gboolean
fbd_droid_leds_backend_hidl_is_supported (FbdDroidLedsBackend *backend)
{
  FbdDroidLedsBackendHidl *self = FBD_DROID_LEDS_BACKEND_HIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  GBinderReader reader;
  int status;
  gsize count = 0, vecSize = 0;
  const int32_t *types;

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_LIGHT_HIDL_2_0_GET_SUPPORTED_TYPES,
                                              req, &status);
  gbinder_local_request_unref (req);

  gbinder_remote_reply_init_reader (reply, &reader);

  if (status == GBINDER_STATUS_OK && fbd_binder_status_is_ok (&reader)) {
    types = gbinder_reader_read_hidl_vec(&reader, &count, &vecSize);
    for (int i = 0; i < count; i++) {
        if (types[i] == LIGHT_TYPE_NOTIFICATIONS) {
            g_debug ("droid LED usable");
            return TRUE;
        }
    }
    g_warning ("No suitable notification LED found");
  } else {
    g_warning ("Failed to get supported LED types");
  }

  return FALSE;
}

static gboolean
fbd_droid_leds_backend_hidl_start_periodic (FbdDroidLedsBackend *backend,
                                            FbdFeedbackLedColor color,
                                            guint               max_brightness,
                                            guint               freq)
{
  FbdDroidLedsBackendHidl *self = FBD_DROID_LEDS_BACKEND_HIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  GBinderWriter writer;
  LightState* notification_state;
  int32_t status, argb_color, t;

  argb_color = fbd_droid_leds_backend_get_argb_color (color, max_brightness);
  t = 1000 * 1000 / freq / 2;

  gbinder_local_request_init_writer (req, &writer);
  notification_state = gbinder_writer_new0 (&writer, LightState);
  notification_state->color = argb_color;
  notification_state->flashMode = FLASH_TYPE_TIMED;
  notification_state->flashOnMs = t;
  notification_state->flashOffMs = t;
  notification_state->brightnessMode = BRIGHTNESS_MODE_USER;

  gbinder_writer_append_int32 (&writer, LIGHT_TYPE_NOTIFICATIONS);
  gbinder_writer_append_buffer_object (&writer, notification_state,
    sizeof(*notification_state));

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_LIGHT_HIDL_2_0_SET_LIGHT,
                                              req, &status);
  gbinder_local_request_unref (req);
  
  if (status == GBINDER_STATUS_OK && fbd_binder_reply_status_is_ok (reply)) {
    return TRUE;
  } else {
    g_warning ("Unable to turn to set notification LED");
    return FALSE;
  }
}

static gboolean
fbd_droid_leds_backend_hidl_stop (FbdDroidLedsBackend *backend,
                                  FbdFeedbackLedColor  color)
{
  FbdDroidLedsBackendHidl *self = FBD_DROID_LEDS_BACKEND_HIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  GBinderWriter writer;
  LightState* notification_state;
  int status;

  gbinder_local_request_init_writer (req, &writer);
  notification_state = gbinder_writer_new0 (&writer, LightState);
  notification_state->color = 0;
  notification_state->flashMode = FLASH_TYPE_NONE;
  notification_state->flashOnMs = 0;
  notification_state->flashOffMs = 0;
  notification_state->brightnessMode = BRIGHTNESS_MODE_USER;

  gbinder_writer_append_int32 (&writer, LIGHT_TYPE_NOTIFICATIONS);
  gbinder_writer_append_buffer_object (&writer, notification_state,
    sizeof(*notification_state));

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_LIGHT_HIDL_2_0_SET_LIGHT,
                                              req, &status);
  gbinder_local_request_unref (req);
  
  if (status == GBINDER_STATUS_OK && fbd_binder_reply_status_is_ok (reply)) {
    return TRUE;
  } else {
    g_warning ("Unable to stop notification LED");
    return FALSE;
  }
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
  FbdDroidLedsBackendHidl *self = FBD_DROID_LEDS_BACKEND_HIDL (initable);
  gboolean success;

  g_debug ("Initializing droid leds hidl");

  success = fbd_binder_init (BINDER_LIGHT_DEFAULT_HIDL_DEVICE,
                             BINDER_LIGHT_HIDL_2_0_IFACE,
                             (BINDER_LIGHT_HIDL_2_0_IFACE "/" BINDER_LIGHT_HIDL_SLOT),
                             &self->service_manager,
                             &self->remote,
                             &self->client);

  if (!success) {
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Failed to obtain suitable light hal");
    return FALSE;
  }

  return TRUE;
}

static void
fbd_droid_leds_backend_hidl_constructed (GObject *obj)
{
  FbdDroidLedsBackendHidl *self = FBD_DROID_LEDS_BACKEND_HIDL (obj);

  G_OBJECT_CLASS (fbd_droid_leds_backend_hidl_parent_class)->constructed (obj);

  self->service_manager = NULL;
  self->remote = NULL;
  self->client = NULL;
}

static void
fbd_droid_leds_backend_hidl_dispose (GObject *obj)
{
  FbdDroidLedsBackendHidl *self = FBD_DROID_LEDS_BACKEND_HIDL (obj);

  g_debug ("Disposing droid leds hidl");

  if (self->client) {
    gbinder_client_unref (self->client);
  }
  
  if (self->remote) {
    gbinder_remote_object_unref (self->remote);
  }
  
  if (self->service_manager) {
    gbinder_servicemanager_unref (self->service_manager);
  }

  G_OBJECT_CLASS (fbd_droid_leds_backend_hidl_parent_class)->dispose (obj);
}

static void
fbd_droid_leds_backend_hidl_class_init (FbdDroidLedsBackendHidlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = fbd_droid_leds_backend_hidl_constructed;
  object_class->dispose      = fbd_droid_leds_backend_hidl_dispose;
}

static void
initable_interface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}

static void
fbd_droid_leds_backend_interface_init (FbdDroidLedsBackendInterface *iface)
{
  iface->is_supported    = fbd_droid_leds_backend_hidl_is_supported;
  iface->start_periodic  = fbd_droid_leds_backend_hidl_start_periodic;
  iface->stop            = fbd_droid_leds_backend_hidl_stop;
}

static void
fbd_droid_leds_backend_hidl_init (FbdDroidLedsBackendHidl *self)
{
}

FbdDroidLedsBackendHidl *
fbd_droid_leds_backend_hidl_new (GError **error)
{
  return FBD_DROID_LEDS_BACKEND_HIDL (
    g_initable_new (FBD_TYPE_DROID_LEDS_BACKEND_HIDL,
                    NULL,
                    error,
                    NULL));
}
