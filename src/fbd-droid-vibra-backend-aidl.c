/*
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-vibra-backend-aidl"

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gbinder.h>

#include "fbd-binder.h"
#include "fbd-droid-vibra-backend.h"
#include "fbd-droid-vibra-backend-aidl.h"


#define BINDER_VIBRATOR_DEFAULT_AIDL_DEVICE "/dev/binder"

#define BINDER_VIBRATOR_AIDL_IFACE "android.hardware.vibrator.IVibrator"
#define BINDER_VIBRATOR_AIDL_CALLBACK_IFACE "android.hardware.vibrator.IVibratorCallback"
#define BINDER_VIBRATOR_AIDL_SLOT "default"

/* Methods */
enum
{
  /* int getCapabilities(); */
  BINDER_VIBRATOR_AIDL_GET_CAPABILITIES = 1,
  /* void on(in int timeoutMs, in android.hardware.vibrator.IVibratorCallback callback); */
  BINDER_VIBRATOR_AIDL_ON = 3,
  /* void off(); */
  BINDER_VIBRATOR_AIDL_OFF = 2,
};

/* Capabilities */
typedef enum
{
  BINDER_VIBRATOR_AIDL_CAP_NONE = 0,
  BINDER_VIBRATOR_AIDL_CAP_ON_CALLBACK = 1,
  BINDER_VIBRATOR_AIDL_CAP_PERFORM_CALLBACK = 2,
  BINDER_VIBRATOR_AIDL_CAP_AMPLITUDE_CONTROL = 4,
  BINDER_VIBRATOR_AIDL_CAP_EXTERNAL_CONTROL = 8,
  BINDER_VIBRATOR_AIDL_CAP_EXTERNAL_AMPLITUDE_CONTROL = 8,
  BINDER_VIBRATOR_AIDL_CAP_COMPOSE_EFFECTS = 32,
  BINDER_VIBRATOR_AIDL_CAP_ALWAYS_ON_CONTROL = 64,
} FbdDroidVibraBackendAidlCapabilities;

struct _FbdDroidVibraBackendAidl
{
  GObject parent_instance;

  GBinderServiceManager *service_manager;
  GBinderRemoteObject   *remote;
  GBinderClient         *client;
  
  GBinderLocalObject    *callback_object;
};

static void initable_interface_init (GInitableIface *iface);
static void fbd_droid_vibra_backend_interface_init (FbdDroidVibraBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDroidVibraBackendAidl, fbd_droid_vibra_backend_aidl, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_interface_init)
                         G_IMPLEMENT_INTERFACE (FBD_TYPE_DROID_VIBRA_BACKEND,
                                                fbd_droid_vibra_backend_interface_init))

static GBinderLocalReply *
fbd_droid_vibra_backend_aidl_callback (GBinderLocalObject   *obj,
                                       GBinderRemoteRequest *req,
                                       guint                 code,
                                       guint                 flags,
                                       int                  *status,
                                       void                 *user_data)
{
  /* Unused for now */

  return NULL;
}

#if 0
/* Unused for now, but could be useful in future to determine whether the
 * underlying HAL supports callbacks at all.
*/
static FbdDroidVibraBackendAidlCapabilities
fbd_droid_vibra_backend_get_capabilities (FbdDroidVibraBackend *backend)
{
  FbdDroidVibraBackendAidl *self = FBD_DROID_VIBRA_BACKEND_AIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  GBinderReader reader;
  int status;
  int capabilities;

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_VIBRATOR_AIDL_GET_CAPABILITIES,
                                              req, &status);
  gbinder_local_request_unref (req);
  
  gbinder_remote_reply_init_reader (reply, &reader);
  
  if (status == GBINDER_STATUS_OK && fbd_binder_status_is_ok (&reader) &&
      gbinder_reader_read_int32 (&reader, &capabilities)) {
    return (FbdDroidVibraBackendAidlCapabilities) capabilities;
  } else {
    g_warning ("Unable to get capabilities!");
    return BINDER_VIBRATOR_AIDL_CAP_NONE;
  }
}
#endif /* Unused */

static gboolean
fbd_droid_vibra_backend_aidl_on (FbdDroidVibraBackend *backend,
                                 int                       duration)
{
  FbdDroidVibraBackendAidl *self = FBD_DROID_VIBRA_BACKEND_AIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  int status;

  gbinder_local_request_append_int32 (req, duration); /* duration */
  gbinder_local_request_append_local_object (req, self->callback_object); /* callback */

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_VIBRATOR_AIDL_ON,
                                              req, &status);
  gbinder_local_request_unref (req);
  
  if (status == GBINDER_STATUS_OK && fbd_binder_reply_status_is_ok (reply)) {
    return TRUE;
  } else {
    g_warning ("Unable to turn the vibrator on");
    return FALSE;
  }
}

static gboolean
fbd_droid_vibra_backend_aidl_off (FbdDroidVibraBackend *backend)
{
  FbdDroidVibraBackendAidl *self = FBD_DROID_VIBRA_BACKEND_AIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  int status;

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_VIBRATOR_AIDL_OFF,
                                              req, &status);
  gbinder_local_request_unref(req);
  
  if (status == GBINDER_STATUS_OK && fbd_binder_reply_status_is_ok (reply)) {
    return TRUE;
  } else {
    g_warning ("Unable to turn the vibrator off");
    return FALSE;
  }
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
  FbdDroidVibraBackendAidl *self = FBD_DROID_VIBRA_BACKEND_AIDL (initable);
  gboolean success;

  g_debug ("Initializing droid vibra aidl");

  success = fbd_binder_init (BINDER_VIBRATOR_DEFAULT_AIDL_DEVICE,
                             BINDER_VIBRATOR_AIDL_IFACE,
                             (BINDER_VIBRATOR_AIDL_IFACE "/" BINDER_VIBRATOR_AIDL_SLOT),
                             &self->service_manager,
                             &self->remote,
                             &self->client);


  if (!success) {
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Failed to obtain suitable vibrator hal");
    return FALSE;
  }

  self->callback_object =
    gbinder_servicemanager_new_local_object (self->service_manager,
                                             BINDER_VIBRATOR_AIDL_CALLBACK_IFACE,
                                             fbd_droid_vibra_backend_aidl_callback,
                                             self);

  return TRUE;
}

static void
fbd_droid_vibra_backend_aidl_constructed (GObject *obj)
{
  FbdDroidVibraBackendAidl *self = FBD_DROID_VIBRA_BACKEND_AIDL (obj);

  G_OBJECT_CLASS (fbd_droid_vibra_backend_aidl_parent_class)->constructed (obj);

  self->service_manager = NULL;
  self->remote = NULL;
  self->client = NULL;
  self->callback_object = NULL;
}

static void
fbd_droid_vibra_backend_aidl_dispose (GObject *obj)
{
  FbdDroidVibraBackendAidl *self = FBD_DROID_VIBRA_BACKEND_AIDL (obj);

  g_debug ("Disposing droid vibra aidl");

  if (self->callback_object) {
    gbinder_local_object_unref (self->callback_object);
  }

  if (self->client) {
    gbinder_client_unref (self->client);
  }

  if (self->remote) {
    gbinder_remote_object_unref (self->remote);
  }

  if (self->service_manager) {
    gbinder_servicemanager_unref (self->service_manager);
  }

  G_OBJECT_CLASS (fbd_droid_vibra_backend_aidl_parent_class)->dispose (obj);
}

static void
fbd_droid_vibra_backend_aidl_set_property (GObject      *obj,
                                           uint          property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  /* No properties supported yet */
}

static void
fbd_droid_vibra_backend_aidl_get_property (GObject    *obj,
                                           uint        property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  /* No properties supported yet */
  G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
}


static void
fbd_droid_vibra_backend_aidl_class_init (FbdDroidVibraBackendAidlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = fbd_droid_vibra_backend_aidl_constructed;
  object_class->dispose      = fbd_droid_vibra_backend_aidl_dispose;
  object_class->set_property = fbd_droid_vibra_backend_aidl_set_property;
  object_class->get_property = fbd_droid_vibra_backend_aidl_get_property;
}

static void
initable_interface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}

static void
fbd_droid_vibra_backend_interface_init (FbdDroidVibraBackendInterface *iface)
{
  iface->on  = fbd_droid_vibra_backend_aidl_on;
  iface->off = fbd_droid_vibra_backend_aidl_off;
}

static void
fbd_droid_vibra_backend_aidl_init (FbdDroidVibraBackendAidl *self)
{
}

FbdDroidVibraBackendAidl *
fbd_droid_vibra_backend_aidl_new (GError **error)
{
  return FBD_DROID_VIBRA_BACKEND_AIDL (
    g_initable_new (FBD_TYPE_DROID_VIBRA_BACKEND_AIDL,
                    NULL,
                    error,
                    NULL));
}
