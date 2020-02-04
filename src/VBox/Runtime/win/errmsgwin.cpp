/* $Id$ */
/** @file
 * IPRT - Status code messages.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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
#include <iprt/win/windows.h>

#include <iprt/errcore.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/errcore.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Array of messages.
 * The data is generated by a sed script.
 */
static const RTWINERRMSG  g_aStatusMsgs[] =
{
#if !defined(IPRT_NO_ERROR_DATA) && !defined(DOXYGEN_RUNNING)
# include "errmsgcomdata.h"
# if defined(VBOX) && !defined(IN_GUEST)
#  include "errmsgvboxcomdata.h"
# endif
#else
    { "Success.", "ERROR_SUCCESS", 0 },
#endif
    { NULL, NULL, 0 }
};


/** Temporary buffers to format unknown messages in.
 * @{
 */
static char                 g_aszUnknownStr[4][64];
static RTWINERRMSG          g_aUnknownMsgs[4] =
{
    { &g_aszUnknownStr[0][0], &g_aszUnknownStr[0][0], 0 },
    { &g_aszUnknownStr[1][0], &g_aszUnknownStr[1][0], 0 },
    { &g_aszUnknownStr[2][0], &g_aszUnknownStr[2][0], 0 },
    { &g_aszUnknownStr[3][0], &g_aszUnknownStr[3][0], 0 }
};
/** Last used index in g_aUnknownMsgs. */
static volatile uint32_t    g_iUnknownMsgs;
/** @} */


/**
 * Get the message corresponding to a given status code.
 *
 * @returns Pointer to read-only message description.
 * @param   rc      The status code.
 */
RTDECL(PCRTWINERRMSG) RTErrWinGet(long rc)
{
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(g_aStatusMsgs) - 1U; i++)
        if (g_aStatusMsgs[i].iCode == rc)
            return &g_aStatusMsgs[i];

    /* The g_aStatusMsgs table contains a wild mix of error codes with and
     * without included facility and severity. So the chance is high that there
     * was no exact match. Try to find a non-exact match, and include the
     * actual value in case we pick the wrong entry. Better than always using
     * the "Unknown Status" case. */
    for (i = 0; i < RT_ELEMENTS(g_aStatusMsgs) - 1U; i++)
        if (g_aStatusMsgs[i].iCode == HRESULT_CODE(rc))
        {
            int32_t iMsg = (ASMAtomicIncU32(&g_iUnknownMsgs) - 1) % RT_ELEMENTS(g_aUnknownMsgs);
            RTStrPrintf(&g_aszUnknownStr[iMsg][0], sizeof(g_aszUnknownStr[iMsg]), "%s 0x%X", g_aStatusMsgs[i].pszDefine, rc);
            return &g_aUnknownMsgs[iMsg];
        }

    /*
     * Need to use the temporary stuff.
     */
    int32_t iMsg = (ASMAtomicIncU32(&g_iUnknownMsgs) - 1) % RT_ELEMENTS(g_aUnknownMsgs);
    RTStrPrintf(&g_aszUnknownStr[iMsg][0], sizeof(g_aszUnknownStr[iMsg]), "Unknown Status 0x%X", rc);
    return &g_aUnknownMsgs[iMsg];
}


RTDECL(PCRTCOMERRMSG) RTErrCOMGet(uint32_t rc)
{
    return RTErrWinGet((long)rc);
}

