/* $Id$ */
/** @file
 * IPRT - RTUtf16NCmp.
 */

/*
 * Copyright (C) 2010-2022 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/utf16.h>
#include "internal/iprt.h"


RTDECL(int) RTUtf16NCmp(PCRTUTF16 pwsz1, PCRTUTF16 pwsz2, size_t cwcMax)
{
    if (pwsz1 == pwsz2)
        return 0;
    if (!pwsz1)
        return -1;
    if (!pwsz2 || !cwcMax)
        return 1;

    while (cwcMax-- > 0)
    {
        RTUTF16 wcs = *pwsz1;
        int     iDiff = wcs - *pwsz2;
        if (iDiff || !wcs)
            return iDiff;
        pwsz1++;
        pwsz2++;
    }
    return 0;
}
RT_EXPORT_SYMBOL(RTUtf16NCmp);
