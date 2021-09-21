/*
 * Copyright (C) 2021 Giuseppe Corti
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Giuseppe Corti <giuseppe.corti@pm.me>
 */

#define G_LOG_DOMAIN "fbd-dev-leds"
#define MAXIMUM_HYBRIS_LED_BRIGHTNESS		100

#include "fbd.h"
#include "fbd-enums.h"
#include "fbd-droid-leds.h"
#include "fbd-feedback-led.h"

#include <gio/gio.h>

#include <memory.h>
#include <gbinder.h>

/**
 * SECTION:fbd-droid-leds
 * @short_description: Android LED device interface (gbinder)
 * @Title: FbdDroidLeds
 *
 * #FbdDevLeds is used to interface with LEDS via gbinder.
 */

#define BINDER_LIGHT_SERVICE_DEVICE "/dev/hwbinder"
#define BINDER_LIGHT_SERVICE_IFACE "android.hardware.light@2.0::ILight"
#define BINDER_LIGHT_SERVICE_SLOT "default"

typedef struct _FbdDevLeds {
    GObject      parent;

    GBinderServiceManager* sm;  
    GBinderRemoteObject* remote;
    GBinderClient* client;

} FbdDevLeds;


static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (FbdDevLeds, fbd_dev_leds, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init));

static gboolean
initable_init (GInitable    *initable,
               GCancellable *cancellable,
               GError      **error)
{
    FbdDevLeds *self = FBD_DEV_LEDS (initable);
    char *fqname =
        (BINDER_LIGHT_SERVICE_IFACE "/" BINDER_LIGHT_SERVICE_SLOT);

    g_debug("initializing droid leds");

    self->sm = gbinder_servicemanager_new(BINDER_LIGHT_SERVICE_DEVICE);
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
                     "Failed to get light hal service remote");
        gbinder_servicemanager_unref(self->sm);
        return FALSE;
    }
    self->client = gbinder_client_new(self->remote, BINDER_LIGHT_SERVICE_IFACE);
    if (!self->client) {
        g_set_error (error,
                     G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Failed to get light hal service client");
        gbinder_remote_object_unref(self->remote);
        gbinder_servicemanager_unref(self->sm);
        return FALSE;
    }

    GBinderLocalRequest* req = gbinder_client_new_request(self->client);
    GBinderRemoteReply* reply;
    int status;

    reply = gbinder_client_transact_sync_reply(self->client,
        2 /* getSupportedTypes */, req, &status);
    gbinder_local_request_unref(req);

    if (status == GBINDER_STATUS_OK) {
        GBinderReader reader;
        guint value;

        gbinder_remote_reply_init_reader(reply, &reader);
        status = gbinder_reader_read_uint32(&reader, &value);
        if (value != GBINDER_STATUS_OK) {
            g_set_error (error,
                         G_IO_ERROR, G_IO_ERROR_FAILED,
                         "Failed to get leds supported types");
            return FALSE;
        }
        
        gsize count = 0;
        gsize vecSize = 0;
        const int32_t *types;
        types = gbinder_reader_read_hidl_vec(&reader, &count, &vecSize);
        for (int i = 0; i < count; i++) {
            if (types[i] == LIGHT_TYPE_NOTIFICATIONS) {
                g_debug ("droid LED usable");
                return TRUE;
            }
        }
    }

    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Failed to find notification led on light hal");
    return FALSE;
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
fbd_dev_leds_dispose (GObject *object)
{
    FbdDevLeds *self = FBD_DEV_LEDS (object);

    g_debug("Disposing droid leds");
    if (self->client)
        gbinder_client_unref(self->client);
    if (self->sm)
        gbinder_servicemanager_unref(self->sm);
    G_OBJECT_CLASS (fbd_dev_leds_parent_class)->dispose (object);
}

static void
fbd_dev_leds_class_init (FbdDevLedsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = fbd_dev_leds_dispose;
}

static void
fbd_dev_leds_init (FbdDevLeds *self)
{
}

FbdDevLeds *
fbd_dev_leds_new (GError **error)
{
    return FBD_DEV_LEDS (g_initable_new (FBD_TYPE_DEV_LEDS,
                                         NULL,
                                         error,
                                         NULL));
}

/**
 * fbd_dev_leds_start_periodic:
 * @self: The #FbdDevLeds
 * @color: The color to use for the LED pattern
 * @max_brightness: The max brightness (in percent) to use for the pattern
 * @freq: The pattern's frequency in mHz
 *
 * Start periodic feedback.
 */
gboolean
fbd_dev_leds_start_periodic (FbdDevLeds *self, FbdFeedbackLedColor color,
                             guint max_brightness, guint freq)
{
    g_debug ("droid LED start flashing");
    uint32_t max, argb_color;
    int32_t t;

    max = (max_brightness / 100.0) * 0xff;
    t = 1000 * 1000 / freq / 2;

    int alpha = 0xff; // Full Alpha
    argb_color = (alpha & 0xff) << 24;
    switch (color) {
        case FBD_FEEDBACK_LED_COLOR_WHITE:
            argb_color += ((max & 0xff) << 16) + ((max & 0xff) << 8) + (max & 0xff);
            break;
        case FBD_FEEDBACK_LED_COLOR_RED:
            argb_color += (max & 0xff) << 16;
            break;
        case FBD_FEEDBACK_LED_COLOR_GREEN:
            argb_color += (max & 0xff) << 8;
            break;
        case FBD_FEEDBACK_LED_COLOR_BLUE:
            argb_color += max & 0xff;
            break;
    }

    GBinderLocalRequest* req = gbinder_client_new_request(self->client);
    GBinderRemoteReply* reply;
    GBinderWriter writer;
    LightState* notification_state;
    int status;

    gbinder_local_request_init_writer(req, &writer);
    notification_state = gbinder_writer_new0(&writer, LightState);
    notification_state->color = argb_color;
    notification_state->flashMode = FLASH_TYPE_TIMED;
    notification_state->flashOnMs = t;
    notification_state->flashOffMs = t;
    notification_state->brightnessMode = BRIGHTNESS_MODE_USER;

    gbinder_writer_append_int32(&writer, LIGHT_TYPE_NOTIFICATIONS);
    gbinder_writer_append_buffer_object(&writer, notification_state,
            sizeof(*notification_state));

    reply = gbinder_client_transact_sync_reply(self->client,
        1 /* setLight */, req, &status);
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
fbd_dev_leds_stop (FbdDevLeds *self, FbdFeedbackLedColor color)
{
    g_debug ("droid LED stop flashing");

    GBinderLocalRequest* req = gbinder_client_new_request(self->client);
    GBinderRemoteReply* reply;
    GBinderWriter writer;
    LightState* notification_state;
    int status;

    gbinder_local_request_init_writer(req, &writer);
    notification_state = gbinder_writer_new0(&writer, LightState);
    notification_state->color = 0;
    notification_state->flashMode = FLASH_TYPE_NONE;
    notification_state->flashOnMs = 0;
    notification_state->flashOffMs = 0;
    notification_state->brightnessMode = BRIGHTNESS_MODE_USER;

    gbinder_writer_append_int32(&writer, LIGHT_TYPE_NOTIFICATIONS);
    gbinder_writer_append_buffer_object(&writer, notification_state,
            sizeof(*notification_state));

    reply = gbinder_client_transact_sync_reply(self->client,
        1 /* setLight */, req, &status);
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
