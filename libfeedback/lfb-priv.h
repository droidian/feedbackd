/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <gio/gio.h>
#include "lfb-gdbus.h"

G_BEGIN_DECLS

LfbGdbusFeedback *_lfb_get_proxy (void);

G_END_DECLS
