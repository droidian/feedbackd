#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
    FBD_ERROR_FAILED = 0,
} FbdError;

GQuark fbd_error_quark (void);

G_END_DECLS
