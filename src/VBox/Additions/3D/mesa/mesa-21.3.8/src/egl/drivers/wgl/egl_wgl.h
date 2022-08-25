/*
 * Copyright © Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

#include <egldriver.h>
#include <egldisplay.h>
#include <eglconfig.h>

#include <stw_pixelformat.h>
#include <windows.h>

struct wgl_egl_display
{
   int ref_count;
   struct pipe_screen *screen;
};

struct wgl_egl_config
{
   _EGLConfig                         base;
   const struct stw_pixelformat_info *stw_config[2];
};

struct wgl_egl_context
{
   _EGLContext base;
   struct stw_context *ctx;
};

struct wgl_egl_surface
{
   _EGLSurface base;
   struct stw_framebuffer *fb;
};

_EGL_DRIVER_STANDARD_TYPECASTS(wgl_egl)
