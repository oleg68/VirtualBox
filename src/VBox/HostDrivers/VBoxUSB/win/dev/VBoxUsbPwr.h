/* $Id$ */
/** @file
 * USB Power state Handling
 */
/*
 * Copyright (C) 2011-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef VBOX_INCLUDED_SRC_VBoxUSB_win_dev_VBoxUsbPwr_h
#define VBOX_INCLUDED_SRC_VBoxUSB_win_dev_VBoxUsbPwr_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

typedef struct VBOXUSB_PWRSTATE
{
    POWER_STATE PowerState;
    ULONG PowerDownLevel;
} VBOXUSB_PWRSTATE, *PVBOXUSB_PWRSTATE;

DECLHIDDEN(VOID) vboxUsbPwrStateInit(PVBOXUSBDEV_EXT pDevExt);
DECLHIDDEN(NTSTATUS) vboxUsbDispatchPower(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

#endif /* !VBOX_INCLUDED_SRC_VBoxUSB_win_dev_VBoxUsbPwr_h */
