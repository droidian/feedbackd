/*
 * Copyright (C) 2021 Giuseppe Corti
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Giuseppe Corti <giuseppe.corti@pm.me>
 */

#define G_LOG_DOMAIN "fbd-droid-vibra"

#include "fbd-droid-vibra.h"

#include <gio/gio.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gbinder.h>

/**
 * SECTION:fbd-droid-vibra
 * @short_description: Android HAL haptic motor device interface (gbinder)
 * @Title: FbdDevVibra
 *
 * The #FbdDevVibra is used to interface with haptic motor via the force
 * feedback interface. It currently only supports one id at a time.
 */

#define BINDER_VIBRATOR_SERVICE_DEVICE "/dev/hwbinder"
#define BINDER_VIBRATOR_SERVICE_IFACE "android.hardware.vibrator@1.0::IVibrator"
#define BINDER_VIBRATOR_SERVICE_SLOT "default"

enum {
    PROP_0,
    PROP_DEVICE,
    PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef enum {
    FBD_DEV_VIBRA_FEATURE_RUMBLE,
    FBD_DEV_VIBRA_FEATURE_PERIODIC,
    FBD_DEV_VIBRA_FEATURE_GAIN,
} FbdDevVibraFeatureFlags;

typedef struct _FbdDevVibra {
    GObject parent;

    GUdevDevice *device;

    GBinderServiceManager* sm;  
    GBinderRemoteObject* remote;
    GBinderClient* client;

    FbdDevVibraFeatureFlags features;
} FbdDevVibra;

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDevVibra, fbd_dev_vibra, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init));

static void
fbd_dev_vibra_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);

    switch (property_id) {
    case PROP_DEVICE:
        g_clear_object (&self->device);
        self->device = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
fbd_dev_vibra_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (initable);
    char *fqname =
        (BINDER_VIBRATOR_SERVICE_IFACE "/" BINDER_VIBRATOR_SERVICE_SLOT);

    g_debug("initializing droid vibra");

    self->sm = gbinder_servicemanager_new(BINDER_VIBRATOR_SERVICE_DEVICE);
    if (!self->sm) {
        g_set_error (error,
                     G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Failed to init servicemanager");
        return FALSE;
    }
    self->remote = gbinder_servicemanager_get_service_sync(self->sm,
        fqname, NULL);
    if (!self->remote) {
        g_set_error (error,
                     G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Failed to get vibrator hal service remote");
        gbinder_servicemanager_unref(self->sm);
        return FALSE;
    }
    self->client = gbinder_client_new(self->remote, BINDER_VIBRATOR_SERVICE_IFACE);
    if (!self->client) {
        g_set_error (error,
                     G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Failed to get vibrator hal service client");
        gbinder_remote_object_unref(self->remote);
        gbinder_servicemanager_unref(self->sm);
        return FALSE;
    }

    g_debug ("Droid vibra device usable");
    return TRUE;
}


static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}


static void
fbd_dev_vibra_dispose (GObject *object)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);

    g_clear_object (&self->device);

    G_OBJECT_CLASS (fbd_dev_vibra_parent_class)->dispose (object);
}


static void
fbd_dev_vibra_finalize (GObject *object)
{
    FbdDevVibra *self = FBD_DEV_VIBRA (object);

    g_debug("Finalizing droid vibra");
    if (self->client)
        gbinder_client_unref(self->client);
    if (self->sm)
        gbinder_servicemanager_unref(self->sm);
    G_OBJECT_CLASS (fbd_dev_vibra_parent_class)->finalize (object);
}

static void
fbd_dev_vibra_class_init (FbdDevVibraClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = fbd_dev_vibra_set_property;
    object_class->get_property = fbd_dev_vibra_get_property;

    object_class->dispose = fbd_dev_vibra_dispose;
    object_class->finalize = fbd_dev_vibra_finalize;

    props[PROP_DEVICE] =
          g_param_spec_object (
          "device",
          "Device",
          "The udev device",
          G_UDEV_TYPE_DEVICE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
fbd_dev_vibra_init (FbdDevVibra *self)
{
}


FbdDevVibra *
fbd_dev_vibra_new (GUdevDevice *device, GError **error)
{
    return FBD_DEV_VIBRA (g_initable_new (FBD_TYPE_DEV_VIBRA,
                                          NULL,
                                          error,
                                          "device", device,
                                          NULL));
}


gboolean
fbd_dev_vibra_rumble (FbdDevVibra *self, guint duration, gboolean upload)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Playing rumbling vibra effect");
    GBinderLocalRequest* req = gbinder_client_new_request(self->client);
    GBinderRemoteReply* reply;
    int status;

    gbinder_local_request_append_int32(req, duration);
    reply = gbinder_client_transact_sync_reply(self->client,
        1 /* on */, req, &status);
    gbinder_local_request_unref(req);

    if (status == GBINDER_STATUS_OK) {
        GBinderReader reader;
        guint value;

        gbinder_remote_reply_init_reader(reply, &reader);
        status = gbinder_reader_read_uint32(&reader, &value);
        if (value != GBINDER_STATUS_OK)
            return FALSE;
    } else {
        return FALSE;
    }

    return TRUE;
}


gboolean
fbd_dev_vibra_periodic (FbdDevVibra *self, guint duration, guint magnitude, guint fade_in_level, guint fade_in_time)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Playing periodic vibra effect");
    GBinderLocalRequest* req = gbinder_client_new_request(self->client);
    GBinderRemoteReply* reply;
    int status;

    gbinder_local_request_append_int32(req, duration);
    reply = gbinder_client_transact_sync_reply(self->client,
        1 /* on */, req, &status);
    gbinder_local_request_unref(req);

    if (status == GBINDER_STATUS_OK) {
        GBinderReader reader;
        guint value;

        gbinder_remote_reply_init_reader(reply, &reader);
        status = gbinder_reader_read_uint32(&reader, &value);
        if (value != GBINDER_STATUS_OK)
            return FALSE;
    } else {
        return FALSE;
    }

    return TRUE;
}


gboolean
fbd_dev_vibra_remove_effect (FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    g_debug("Erasing vibra effect");
    GBinderLocalRequest* req = gbinder_client_new_request(self->client);
    GBinderRemoteReply* reply;
    int status;

    reply = gbinder_client_transact_sync_reply(self->client,
        2 /* off */, req, &status);
    gbinder_local_request_unref(req);

    if (status == GBINDER_STATUS_OK) {
        GBinderReader reader;
        guint value;

        gbinder_remote_reply_init_reader(reply, &reader);
        status = gbinder_reader_read_uint32(&reader, &value);
        if (value != GBINDER_STATUS_OK)
            return FALSE;
    } else {
        return FALSE;
    }

    return TRUE;
}


gboolean
fbd_dev_vibra_stop(FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    return fbd_dev_vibra_remove_effect (self);
}


GUdevDevice *
fbd_dev_vibra_get_device(FbdDevVibra *self)
{
    g_return_val_if_fail (FBD_IS_DEV_VIBRA (self), FALSE);

    return self->device;
}
