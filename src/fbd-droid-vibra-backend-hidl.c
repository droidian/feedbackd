/*
 * Copyright (C) 2021 Erfan Abdi
 * Copyright (C) 2022 Eugenio "g7" Paolantonio
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Erfan Abdi
 *         Eugenio "g7" Paolantonio <me@medesimo.eu>
 */

#define G_LOG_DOMAIN "fbd-droid-vibra-backend-hidl"

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gbinder.h>

#include "fbd-binder.h"
#include "fbd-droid-vibra-backend.h"
#include "fbd-droid-vibra-backend-hidl.h"

#define BINDER_VIBRATOR_DEFAULT_HIDL_DEVICE "/dev/hwbinder"

#define BINDER_VIBRATOR_HIDL_IFACE(v) "android.hardware.vibrator@" v "::IVibrator"
#define BINDER_VIBRATOR_HIDL_SLOT "default"

#define BINDER_VIBRATOR_HIDL_1_0_IFACE BINDER_VIBRATOR_HIDL_IFACE("1.0")

/* Methods */
enum
{
  /* on(uint32_t timeoutMs) generates (Status vibratorOnRet); */
  BINDER_VIBRATOR_HIDL_1_0_ON = 1,
  /* off() generates (Status vibratorOffRet); */
  BINDER_VIBRATOR_HIDL_1_0_OFF = 2,
};

struct _FbdDroidVibraBackendHidl
{
  GObject parent_instance;

  GBinderServiceManager *service_manager;
  GBinderRemoteObject   *remote;
  GBinderClient         *client;
};

static void initable_interface_init (GInitableIface *iface);
static void fbd_droid_vibra_backend_interface_init (FbdDroidVibraBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDroidVibraBackendHidl, fbd_droid_vibra_backend_hidl, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_interface_init)
                         G_IMPLEMENT_INTERFACE (FBD_TYPE_DROID_VIBRA_BACKEND,
                                                fbd_droid_vibra_backend_interface_init))

static gboolean
fbd_droid_vibra_backend_hidl_on (FbdDroidVibraBackend *backend,
                                 int                       duration)
{
  FbdDroidVibraBackendHidl *self = FBD_DROID_VIBRA_BACKEND_HIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  int status;

  gbinder_local_request_append_int32 (req, duration); /* duration */

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_VIBRATOR_HIDL_1_0_ON,
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
fbd_droid_vibra_backend_hidl_off (FbdDroidVibraBackend *backend)
{
  FbdDroidVibraBackendHidl *self = FBD_DROID_VIBRA_BACKEND_HIDL (backend);
  GBinderLocalRequest *req = gbinder_client_new_request (self->client);
  GBinderRemoteReply *reply;
  int status;

  reply = gbinder_client_transact_sync_reply (self->client,
                                              BINDER_VIBRATOR_HIDL_1_0_OFF,
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
  FbdDroidVibraBackendHidl *self = FBD_DROID_VIBRA_BACKEND_HIDL (initable);
  gboolean success;

  g_debug ("Initializing droid vibra hidl");

  success = fbd_binder_init (BINDER_VIBRATOR_DEFAULT_HIDL_DEVICE,
                             BINDER_VIBRATOR_HIDL_1_0_IFACE,
                             (BINDER_VIBRATOR_HIDL_1_0_IFACE "/" BINDER_VIBRATOR_HIDL_SLOT),
                             &self->service_manager,
                             &self->remote,
                             &self->client);


  if (!success) {
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Failed to obtain suitable vibrator hal");
    return FALSE;
  }

  return TRUE;
}

static void
fbd_droid_vibra_backend_hidl_constructed (GObject *obj)
{
  FbdDroidVibraBackendHidl *self = FBD_DROID_VIBRA_BACKEND_HIDL (obj);

  G_OBJECT_CLASS (fbd_droid_vibra_backend_hidl_parent_class)->constructed (obj);

  self->service_manager = NULL;
  self->remote = NULL;
  self->client = NULL;
}

static void
fbd_droid_vibra_backend_hidl_dispose (GObject *obj)
{
  FbdDroidVibraBackendHidl *self = FBD_DROID_VIBRA_BACKEND_HIDL (obj);

  g_debug ("Disposing droid vibra hidl");

  if (self->client) {
    gbinder_client_unref (self->client);
  }
  
  if (self->remote) {
    gbinder_remote_object_unref (self->remote);
  }
  
  if (self->service_manager) {
    gbinder_servicemanager_unref (self->service_manager);
  }

  G_OBJECT_CLASS (fbd_droid_vibra_backend_hidl_parent_class)->dispose (obj);
}

static void
fbd_droid_vibra_backend_hidl_set_property (GObject      *obj,
                                           uint          property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  /* No properties supported yet */
}

static void
fbd_droid_vibra_backend_hidl_get_property (GObject    *obj,
                                           uint        property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  /* No properties supported yet */
  G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
}


static void
fbd_droid_vibra_backend_hidl_class_init (FbdDroidVibraBackendHidlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = fbd_droid_vibra_backend_hidl_constructed;
  object_class->dispose      = fbd_droid_vibra_backend_hidl_dispose;
  object_class->set_property = fbd_droid_vibra_backend_hidl_set_property;
  object_class->get_property = fbd_droid_vibra_backend_hidl_get_property;
}

static void
initable_interface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}

static void
fbd_droid_vibra_backend_interface_init (FbdDroidVibraBackendInterface *iface)
{
  iface->on  = fbd_droid_vibra_backend_hidl_on;
  iface->off = fbd_droid_vibra_backend_hidl_off;
}

static void
fbd_droid_vibra_backend_hidl_init (FbdDroidVibraBackendHidl *self)
{
}

FbdDroidVibraBackendHidl *
fbd_droid_vibra_backend_hidl_new (GError **error)
{
  return FBD_DROID_VIBRA_BACKEND_HIDL (
    g_initable_new (FBD_TYPE_DROID_VIBRA_BACKEND_HIDL,
                    NULL,
                    error,
                    NULL));
}
