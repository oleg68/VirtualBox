/* $Id$ */
/** @file
 * IPRT - Threads, Ring-0 Driver, Solaris.
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

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mp.h>



RTDECL(RTNATIVETHREAD) RTThreadNativeSelf(void)
{
    return (RTNATIVETHREAD)curthread;
}


RTDECL(int) RTThreadSleep(unsigned cMillies)
{
    clock_t cTicks;
    unsigned long timeout;
    RT_ASSERT_PREEMPTIBLE();

    if (!cMillies)
    {
        RTThreadYield();
        return VINF_SUCCESS;
    }

    if (cMillies != RT_INDEFINITE_WAIT)
        cTicks = drv_usectohz((clock_t)(cMillies * 1000L));
    else
        cTicks = 0;

#if 0
    timeout = ddi_get_lbolt();
    timeout += cTicks;

    kcondvar_t cnd;
    kmutex_t mtx;
    mutex_init(&mtx, "IPRT Sleep Mutex", MUTEX_DRIVER, NULL);
    cv_init(&cnd, "IPRT Sleep CV", CV_DRIVER, NULL);
    mutex_enter(&mtx);
    cv_timedwait (&cnd, &mtx, timeout);
    mutex_exit(&mtx);
    cv_destroy(&cnd);
    mutex_destroy(&mtx);
#endif

#if 1
    delay(cTicks);
#endif

#if 0
    /*   Hmm, no same effect as using delay() */
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = cMillies * 1000000L;
    nanosleep (&t, NULL);
#endif

    return VINF_SUCCESS;
}


RTDECL(bool) RTThreadYield(void)
{
    RT_ASSERT_PREEMPTIBLE();
    schedctl_set_yield(curthread, 0); /** @todo this is incomplete/wrong. */
    return true;
}


RTDECL(bool) RTThreadPreemptIsEnabled(RTTHREAD hThread)
{
    Assert(hThread == NIL_RTTHREAD);
    if (curthread->t_preempt != 0)
        return false;
    if (!ASMIntAreEnabled())
        return false;
    return true;
}


RTDECL(bool) RTThreadPreemptIsPending(RTTHREAD hThread)
{
    Assert(hThread == NIL_RTTHREAD);
    /** @todo Review this! */
    return CPU->cpu_runrun   != 0
        || CPU->cpu_kprunrun != 0;
}


RTDECL(bool) RTThreadPreemptIsPendingTrusty(void)
{
    /* yes, RTThreadPreemptIsPending is reliable. */
    return true;
}


RTDECL(bool) RTThreadPreemptIsPossible(void)
{
    /* yes, kernel preemption is possible. */
    return true;
}


RTDECL(void) RTThreadPreemptDisable(PRTTHREADPREEMPTSTATE pState)
{
    AssertPtr(pState);
    Assert(pState->u32Reserved == 0);
    pState->u32Reserved = 42;

    kpreempt_disable();
    RT_ASSERT_PREEMPT_CPUID_DISABLE(pState);
}


RTDECL(void) RTThreadPreemptRestore(PRTTHREADPREEMPTSTATE pState)
{
    AssertPtr(pState);
    Assert(pState->u32Reserved == 42);
    pState->u32Reserved = 0;
    RT_ASSERT_PREEMPT_CPUID_RESTORE(pState);

    kpreempt_enable();
}


RTDECL(bool) RTThreadIsInInterrupt(RTTHREAD hThread)
{
    /* This is the best we currently can do here. :-( */
    return !RTThreadPreemptIsEnabled(hThread)
        && getpil() > 0;
}

