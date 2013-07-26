/** @file $Id$
 *
 * VirtualBox Additions Linux kernel video driver
 */

/*
 * Copyright (C) 2011-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on
 * vboxvideo_device.c
 * with the following copyright and permission notice:
 *
 * Copyright 2010 Matt Turner.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Matt Turner
 */

#include <linux/version.h>

#include "the-linux-kernel.h"

#include "vboxvideo_drv.h"

#include <VBox/VBoxVideoGuest.h>

static int vboxvideo_vram_init(struct vboxvideo_device *gdev)
{
    unsigned size;
    int ret;

    /* set accessible VRAM */
    gdev->mc.vram_base = pci_resource_start(gdev->ddev->pdev, 1);
    gdev->mc.vram_size = pci_resource_len(gdev->ddev->pdev, 1);

    gdev->fAnyX        = VBoxVideoAnyWidthAllowed();
    gdev->mc.vram_size = VBoxVideoGetVRAMSize();

    if (!request_region(gdev->mc.vram_base, gdev->mc.vram_size, "vboxvideofb_vram")) {
        VBOXVIDEO_ERROR("can't region_reserve VRAM\n");
        return -ENXIO;
    }

    ret = drm_addmap(gdev->ddev, gdev->mc.vram_base, gdev->mc.vram_size,
        _DRM_FRAME_BUFFER, _DRM_WRITE_COMBINING,
        &gdev->framebuffer);
    return 0;
}

static void vboxvideo_vram_fini(struct vboxvideo_device *gdev)
{
    if (gdev->framebuffer)
        drm_rmmap(gdev->ddev, gdev->framebuffer);
    if (gdev->mc.vram_base)
        release_region(gdev->mc.vram_base, gdev->mc.vram_size);
}

int vboxvideo_device_init(struct vboxvideo_device *gdev,
              struct drm_device *ddev,
              struct pci_dev *pdev,
              uint32_t flags)
{
    int ret;

    gdev->dev      = &pdev->dev;
    gdev->ddev     = ddev;
    gdev->pdev     = pdev;
    gdev->flags    = flags;
    gdev->num_crtc = 1;

    /** @todo hardware initialisation goes here once we start doing more complex
     *        stuff.
     */
    ret = vboxvideo_vram_init(gdev);
    if (ret)
        return ret;

    return 0;
}

void vboxvideo_device_fini(struct vboxvideo_device *gdev)
{
    vboxvideo_vram_fini(gdev);
}
