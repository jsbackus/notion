/*
 * ion/ioncore/kbresize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_KBRESIZE_H
#define ION_IONCORE_KBRESIZE_H

#include "common.h"
#include "frame.h"
#include "resize.h"


extern void frame_moveres_begin(WFrame *frame);

extern void moveresmode_end(WMoveresMode *mode);
extern void moveresmode_cancel(WMoveresMode *mode);
extern void moveresmode_move(WMoveresMode *mode, 
                             int horizmul, int vertmul);
extern void moveresmode_resize(WMoveresMode *mode, 
                               int left, int right, int top, int bottom);
extern void moveresmode_accel(WMoveresMode *mode, 
                              int *wu, int *hu, int accel_mode);

#endif /* ION_IONCORE_KBRESIZE_H */