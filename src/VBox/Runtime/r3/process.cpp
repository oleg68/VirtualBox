/* $Id$ */
/** @file
 * IPRT - Process, Common.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/process.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include "internal/process.h"
#include "internal/thread.h"

#ifdef RT_OS_WINDOWS
# include <process.h>
#else
# include <unistd.h>
#endif


/**
 * Get the identifier for the current process.
 *
 * @returns Process identifier.
 */
RTDECL(RTPROCESS) RTProcSelf(void)
{
    RTPROCESS Self = g_ProcessSelf;
    if (Self != NIL_RTPROCESS)
        return Self;

    /* lazy init. */
#ifdef _MSC_VER
    Self = _getpid(); /* crappy ansi compiler */
#else
    Self = getpid();
#endif
    g_ProcessSelf = Self;
    return Self;
}


/**
 * Attempts to alter the priority of the current process.
 *
 * @returns iprt status code.
 * @param   enmPriority     The new priority.
 */
RTR3DECL(int) RTProcSetPriority(RTPROCPRIORITY enmPriority)
{
    if (enmPriority > RTPROCPRIORITY_INVALID && enmPriority < RTPROCPRIORITY_LAST)
        return rtThreadDoSetProcPriority(enmPriority);
    AssertMsgFailed(("enmPriority=%d\n", enmPriority));
    return VERR_INVALID_PARAMETER;
}


/**
 * Gets the current priority of this process.
 *
 * @returns The priority (see RTPROCPRIORITY).
 */
RTR3DECL(RTPROCPRIORITY) RTProcGetPriority(void)
{
    return g_enmProcessPriority;
}


RTR3DECL(char *) RTProcGetExecutableName(char *pszExecName, size_t cchExecName)
{
    AssertReturn(g_szrtProcExePath[0] != '\0', NULL);

    /*
     * Calc the length and check if there is space before copying.
     */
    size_t cch = g_cchrtProcExePath;
    if (cch <= cchExecName)
    {
        memcpy(pszExecName, g_szrtProcExePath, cch);
        pszExecName[cch] = '\0';
        return pszExecName;
    }

    AssertMsgFailed(("Buffer too small (%zu <= %zu)\n", cchExecName, cch));
    return NULL;
}

