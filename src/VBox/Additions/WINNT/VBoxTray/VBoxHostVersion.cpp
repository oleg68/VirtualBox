/* $Id$ */
/** @file
 * VBoxHostVersion - Checks the host's VirtualBox version and notifies
 *                   the user in case of an update.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#include "VBoxHostVersion.h"
#include "VBoxTray.h"
#include "helpers.h"

#include <VBox/VBoxGuestLib.h>


/** @todo Move this part in VbglR3 and just provide a callback for the platform-specific 
          notification stuff, since this is very similar to the VBoxClient code. */
int VBoxCheckHostVersion ()
{
    int rc;
    uint32_t uGuestPropSvcClientID;

    rc = VbglR3GuestPropConnect(&uGuestPropSvcClientID);
    if (RT_SUCCESS(rc))
    {
        char *pszHostVersion;
        char *pszGuestVersion;
        bool bUpdate;
        rc = VbglR3HostVersionCheckForUpdate(uGuestPropSvcClientID, &bUpdate, &pszHostVersion, &pszGuestVersion);
        if (RT_SUCCESS(rc))
        {
            if (bUpdate)
            {
                char szMsg[256]; /* Sizes according to MSDN. */
                char szTitle[64];
        
                /** @todo add some translation macros here */
                _snprintf(szTitle, sizeof(szTitle), "VirtualBox Guest Additions update available!");
                _snprintf(szMsg, sizeof(szMsg), "Your guest is currently running the Guest Additions version %s. "
                                                "We recommend updating to the latest version (%s) by choosing the "
                                                "install option from the Devices menu.", pszGuestVersion, pszHostVersion);
        
                rc = showBalloonTip(gInstance, gToolWindow, ID_TRAYICON, szMsg, szTitle, 5000, 0);
                if (RT_FAILURE(rc))
                    Log(("VBoxTray: Guest Additions update found; however: could not show version notifier balloon tooltip! rc = %d\n", rc));
            }

            /* Store host version to not notify again */
            rc = VbglR3HostVersionLastCheckedStore(uGuestPropSvcClientID, pszHostVersion);
    
            VbglR3GuestPropReadValueFree(pszHostVersion);
            VbglR3GuestPropReadValueFree(pszGuestVersion);
        }
        VbglR3GuestPropDisconnect(uGuestPropSvcClientID);
    }
    return rc;
}

