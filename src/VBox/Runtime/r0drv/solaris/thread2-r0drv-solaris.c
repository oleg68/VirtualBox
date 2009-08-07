/* $Id$ */
/** @file
 * IPRT - Threads (Part 2), Ring-0 Driver, Solaris.
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
#include "the-solaris-kernel.h"
#include "internal/iprt.h"
#include <iprt/thread.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/process.h>
#include "internal/thread.h"



int rtThreadNativeInit(void)
{
    return VINF_SUCCESS;
}


RTDECL(RTTHREAD) RTThreadSelf(void)
{
    return rtThreadGetByNative(RTThreadNativeSelf());
}


int rtThreadNativeSetPriority(PRTTHREADINT pThread, RTTHREADTYPE enmType)
{
    int iPriority;
    switch (enmType)
    {
        case RTTHREADTYPE_INFREQUENT_POLLER:    iPriority = 60;             break;
        case RTTHREADTYPE_EMULATION:            iPriority = 66;             break;
        case RTTHREADTYPE_DEFAULT:              iPriority = 72;             break;
        case RTTHREADTYPE_MSG_PUMP:             iPriority = 78;             break;
        case RTTHREADTYPE_IO:                   iPriority = 84;             break;
        case RTTHREADTYPE_TIMER:                iPriority = 99;             break;
        default:
            AssertMsgFailed(("enmType=%d\n", enmType));
            return VERR_INVALID_PARAMETER;
    }

    thread_lock(curthread);
    thread_change_pri(curthread, iPriority, 0);
    thread_unlock(curthread);
    return VINF_SUCCESS;
}


int rtThreadNativeAdopt(PRTTHREADINT pThread)
{
    NOREF(pThread);
    /* There is nothing special that needs doing here, but the
       user really better know what he's cooking. */
    return VINF_SUCCESS;
}


/**
 * Native thread main function.
 *
 * @param   pvThreadInt     The thread structure.
 */
static void rtThreadNativeMain(void *pvThreadInt)
{
    PRTTHREADINT pThreadInt = (PRTTHREADINT)pvThreadInt;
    int rc;

    rc = rtThreadMain(pThreadInt, (RTNATIVETHREAD)curthread, &pThreadInt->szName[0]);
    thread_exit();
}


int rtThreadNativeCreate(PRTTHREADINT pThreadInt, PRTNATIVETHREAD pNativeThread)
{
    kthread_t *pKernThread;
    RT_ASSERT_PREEMPTIBLE();

    pKernThread = thread_create(NULL, NULL, rtThreadNativeMain, pThreadInt, 0,
                                (void *)RTR0ProcHandleSelf(), TS_RUN, minclsyspri);
    if (pKernThread)
    {
        *pNativeThread = (RTNATIVETHREAD)pvKernThread;
        return VINF_SUCCESS;
    }

    return VERR_OUT_OF_RESOURCES;
}

