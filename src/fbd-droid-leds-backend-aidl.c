/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-leds-backend-aidl"

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gbinder.h>

#include "fbd-binder.h"
#include "fbd-droid-leds-backend.h"
#include "fbd-droid-leds-backend-aidl.h"

#define BINDER_LIGHT_DEFAULT_AIDL_DEVICE "/dev/binder"

#define BINDER_LIGHT_AIDL_IFACE "android.hardware.light.ILights"
#define BINDER_LIGHT_AIDL_SLOT "default"

/* Methods */
enum
{
  /* void setLightState(in int id, in android.hardware.light.HwLightState state); */
  BINDER_LIGHT_AIDL_SET_LIGHT_STATE = 1,
  /* android.hardware.light.HwLight[] getLights(); */
  BINDER_LIGHT_AIDL_GET_LIGHTS = 2,
};

struct _FbdDroidLedsBackendAidl
{
  GObject parent_instance;

  GBinderServiceManager *service_manager;
  GBinderRemoteObject   *remote;
  GBinderClient         *client;
};

static void initable_interface_init (GInitableIface *iface);
static void fbd_droid_leds_backend_interface_init (FbdDroidLedsBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDroidLedsBackendAidl, fbd_droid_leds_backend_aidl, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_interface_init)
                         G_IMPLEMENT_INTERFACE (FBD_TYPE_DROID_LEDS_BACKEND,
                                                fbd_droid_leds_backend_interface_init))

static gboolean
fbd_droid_leds_backend_aidl_is_supported (FbdDroidLedsBackend *backend)
{
  FbdDroidLedsBackendAidl *self = FBD_DROID_LEDS_BACKEND_AIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  GBinderReader reader;
  int status;
  int count = 0;
  AidlHwLight *light;

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_LIGHT_AIDL_GET_LIGHTS,
                                              req, &status);
  gbinder_local_request_unref (req);

  gbinder_remote_reply_init_reader (reply, &reader);

  if (status == GBINDER_STATUS_OK && fbd_binder_status_is_ok (&reader)) {
    gbinder_reader_read_int32 (&reader, &count); /* led count */

    for (int i=0; i < count; i++) {
      light = (AidlHwLight *)gbinder_reader_read_parcelable (&reader, NULL);
      if (light->type == LIGHT_TYPE_NOTIFICATIONS) {
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
fbd_droid_leds_backend_aidl_start_periodic (FbdDroidLedsBackend *backend,
                                            FbdFeedbackLedColor color,
                                            guint               max_brightness,
                                            guint               freq)
{
  FbdDroidLedsBackendAidl *self = FBD_DROID_LEDS_BACKEND_AIDL (backend);
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
  gbinder_writer_append_parcelable (&writer, notification_state, sizeof(*notification_state));
  gbinder_writer_append_int32 (&writer, BINDER_STABILITY_VINTF); /* stability */

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_LIGHT_AIDL_SET_LIGHT_STATE,
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
fbd_droid_leds_backend_aidl_stop (FbdDroidLedsBackend *backend,
                                  FbdFeedbackLedColor  color)
{
  FbdDroidLedsBackendAidl *self = FBD_DROID_LEDS_BACKEND_AIDL (backend);
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
  gbinder_writer_append_parcelable (&writer, notification_state, sizeof(*notification_state));
  gbinder_writer_append_int32 (&writer, BINDER_STABILITY_VINTF); /* stability */

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_LIGHT_AIDL_SET_LIGHT_STATE,
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
  FbdDroidLedsBackendAidl *self = FBD_DROID_LEDS_BACKEND_AIDL (initable);
  gboolean success;

  g_debug ("Initializing droid leds aidl");

  success = fbd_binder_init (BINDER_LIGHT_DEFAULT_AIDL_DEVICE,
                             BINDER_LIGHT_AIDL_IFACE,
                             (BINDER_LIGHT_AIDL_IFACE "/" BINDER_LIGHT_AIDL_SLOT),
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
fbd_droid_leds_backend_aidl_constructed (GObject *obj)
{
  FbdDroidLedsBackendAidl *self = FBD_DROID_LEDS_BACKEND_AIDL (obj);

  G_OBJECT_CLASS (fbd_droid_leds_backend_aidl_parent_class)->constructed (obj);

  self->service_manager = NULL;
  self->remote = NULL;
  self->client = NULL;
}

static void
fbd_droid_leds_backend_aidl_dispose (GObject *obj)
{
  FbdDroidLedsBackendAidl *self = FBD_DROID_LEDS_BACKEND_AIDL (obj);

  g_debug ("Disposing droid leds aidl");

  if (self->client) {
    gbinder_client_unref (self->client);
  }
  
  if (self->remote) {
    gbinder_remote_object_unref (self->remote);
  }
  
  if (self->service_manager) {
    gbinder_servicemanager_unref (self->service_manager);
  }

  G_OBJECT_CLASS (fbd_droid_leds_backend_aidl_parent_class)->dispose (obj);
}

static void
fbd_droid_leds_backend_aidl_class_init (FbdDroidLedsBackendAidlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = fbd_droid_leds_backend_aidl_constructed;
  object_class->dispose      = fbd_droid_leds_backend_aidl_dispose;
}

static void
initable_interface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}

static void
fbd_droid_leds_backend_interface_init (FbdDroidLedsBackendInterface *iface)
{
  iface->is_supported    = fbd_droid_leds_backend_aidl_is_supported;
  iface->start_periodic  = fbd_droid_leds_backend_aidl_start_periodic;
  iface->stop            = fbd_droid_leds_backend_aidl_stop;
}

static void
fbd_droid_leds_backend_aidl_init (FbdDroidLedsBackendAidl *self)
{
}

FbdDroidLedsBackendAidl *
fbd_droid_leds_backend_aidl_new (GError **error)
{
  return FBD_DROID_LEDS_BACKEND_AIDL (
    g_initable_new (FBD_TYPE_DROID_LEDS_BACKEND_AIDL,
                    NULL,
                    error,
                    NULL));
}
