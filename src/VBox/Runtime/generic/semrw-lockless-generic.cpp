/* $Id$ */
/** @file
 * IPRT Testcase - RTSemXRoads, generic implementation.
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
#define RTASSERT_QUIET
#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/lockvalidator.h>
#include <iprt/mem.h>
#include <iprt/thread.h>

#include "internal/magics.h"
#include "internal/strict.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
typedef struct RTSEMRWINTERNAL
{
    /** Magic value (RTSEMRW_MAGIC).  */
    uint32_t volatile       u32Magic;
    uint32_t                u32Padding; /**< alignment padding.*/
    /* The state variable.
     * All accesses are atomic and it bits are defined like this:
     *      Bits 0..14  - cReads.
     *      Bit 15      - Unused.
     *      Bits 16..31 - cWrites. - doesn't make sense here
     *      Bit 31      - fDirection; 0=Read, 1=Write.
     *      Bits 32..46 - cWaitingReads
     *      Bit 47      - Unused.
     *      Bits 48..62 - cWaitingWrites
     *      Bit 63      - Unused.
     */
    uint64_t volatile       u64State;
    /** The write owner. */
    RTNATIVETHREAD volatile hNativeWriter;
    /** The number of reads made by the current writer. */
    uint32_t volatile       cWriterReads;
    /** The number of reads made by the current writer. */
    uint32_t volatile       cWriteRecursions;

    /** What the writer threads are blocking on. */
    RTSEMEVENT              hEvtWrite;
    /** What the read threads are blocking on when waiting for the writer to
     * finish. */
    RTSEMEVENTMULTI         hEvtRead;
    /** Indicates whether hEvtRead needs resetting. */
    bool volatile           fNeedReset;

#ifdef RTSEMRW_STRICT
    /** The validator record for the writer. */
    RTLOCKVALRECEXCL        ValidatorWrite;
    /** The validator record for the readers. */
    RTLOCKVALRECSHRD        ValidatorRead;
#endif
} RTSEMRWINTERNAL;


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define RTSEMRW_CNT_BITS            15
#define RTSEMRW_CNT_MASK            UINT64_C(0x00007fff)

#define RTSEMRW_CNT_RD_SHIFT        0
#define RTSEMRW_CNT_RD_MASK         (RTSEMRW_CNT_MASK << RTSEMRW_CNT_RD_SHIFT)
#define RTSEMRW_CNT_WR_SHIFT        16
#define RTSEMRW_CNT_WR_MASK         (RTSEMRW_CNT_MASK << RTSEMRW_CNT_WR_SHIFT)
#define RTSEMRW_DIR_SHIFT           31
#define RTSEMRW_DIR_MASK            RT_BIT_64(RTSEMRW_DIR_SHIFT)
#define RTSEMRW_DIR_READ            UINT64_C(0)
#define RTSEMRW_DIR_WRITE           UINT64_C(1)

#define RTSEMRW_WAIT_CNT_RD_SHIFT   32
#define RTSEMRW_WAIT_CNT_RD_MASK    (RTSEMRW_CNT_MASK << RTSEMRW_WAIT_CNT_RD_SHIFT)
//#define RTSEMRW_WAIT_CNT_WR_SHIFT   48
//#define RTSEMRW_WAIT_CNT_WR_MASK    (RTSEMRW_CNT_MASK << RTSEMRW_WAIT_CNT_WR_SHIFT)



RTDECL(int) RTSemRWCreate(PRTSEMRW phRWSem)
{
    RTSEMRWINTERNAL *pThis = (RTSEMRWINTERNAL *)RTMemAlloc(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;

    int rc = RTSemEventMultiCreate(&pThis->hEvtRead);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventCreate(&pThis->hEvtWrite);
        if (RT_SUCCESS(rc))
        {
            pThis->u32Magic             = RTSEMRW_MAGIC;
            pThis->u32Padding           = 0;
            pThis->u64State             = 0;
            pThis->hNativeWriter        = NIL_RTNATIVETHREAD;
            pThis->cWriterReads         = 0;
            pThis->cWriteRecursions     = 0;
            pThis->fNeedReset           = false;
#ifdef RTSEMRW_STRICT
            RTLockValidatorRecExclInit(&pThis->ValidatorWrite, NIL_RTLOCKVALIDATORCLASS, RTLOCKVALIDATOR_SUB_CLASS_NONE, "RTSemRW", pThis);
            RTLockValidatorRecSharedInit(&pThis->ValidatorRead,  NIL_RTLOCKVALIDATORCLASS, RTLOCKVALIDATOR_SUB_CLASS_NONE, "RTSemRW", pThis, false /*fSignaller*/);
            RTLockValidatorRecMakeSiblings(&pThis->ValidatorWrite.Core, &pThis->ValidatorRead.Core);
#endif

            *phRWSem = pThis;
            return VINF_SUCCESS;
        }
        RTSemEventMultiDestroy(pThis->hEvtRead);
    }
    return rc;
}


RTDECL(int) RTSemRWDestroy(RTSEMRW hRWSem)
{
    /*
     * Validate input.
     */
    RTSEMRWINTERNAL *pThis = hRWSem;
    if (pThis == NIL_RTSEMRW)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);
    Assert(!(ASMAtomicReadU64(&pThis->u64State) & (RTSEMRW_CNT_RD_MASK | RTSEMRW_CNT_WR_MASK)));

    /*
     * Invalidate the object and free up the resources.
     */
    AssertReturn(ASMAtomicCmpXchgU32(&pThis->u32Magic, ~RTSEMRW_MAGIC, RTSEMRW_MAGIC), VERR_INVALID_HANDLE);

    RTSEMEVENTMULTI hEvtRead;
    ASMAtomicXchgHandle(&pThis->hEvtRead, NIL_RTSEMEVENTMULTI, &hEvtRead);
    int rc = RTSemEventMultiDestroy(hEvtRead);
    AssertRC(rc);

    RTSEMEVENT hEvtWrite;
    ASMAtomicXchgHandle(&pThis->hEvtWrite, NIL_RTSEMEVENT, &hEvtWrite);
    rc = RTSemEventDestroy(hEvtWrite);
    AssertRC(rc);

#ifdef RTSEMRW_STRICT
    RTLockValidatorRecSharedDelete(&pThis->ValidatorRead);
    RTLockValidatorRecExclDelete(&pThis->ValidatorWrite);
#endif
    RTMemFree(pThis);
    return VINF_SUCCESS;
}


static int rtSemRWRequestRead(RTSEMRW hRWSem, unsigned cMillies, bool fInterruptible, PCRTLOCKVALSRCPOS pSrcPos)
{
    /*
     * Validate input.
     */
    RTSEMRWINTERNAL *pThis = hRWSem;
    if (pThis == NIL_RTSEMRW)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

#ifdef RTSEMRW_STRICT
    RTTHREAD hThreadSelf = RTThreadSelfAutoAdopt();
    if (cMillies > 0)
    {
        int rc9 = RTLockValidatorRecSharedCheckOrder(&pThis->ValidatorRead, hThreadSelf, pSrcPos);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    /*
     * Get cracking...
     */
    uint64_t u64State    = ASMAtomicReadU64(&pThis->u64State);
    uint64_t u64OldState = u64State;

    for (;;)
    {
        if ((u64State & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT))
        {
            /* It flows in the right direction, try follow it before it changes. */
            uint64_t c = (u64State & RTSEMRW_CNT_RD_MASK) >> RTSEMRW_CNT_RD_SHIFT;
            c++;
            Assert(c < RTSEMRW_CNT_MASK / 2);
            u64State &= ~RTSEMRW_CNT_RD_MASK;
            u64State |= c << RTSEMRW_CNT_RD_SHIFT;
            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
            {
#ifdef RTSEMRW_STRICT
                RTLockValidatorRecSharedAddOwner(&pThis->ValidatorRead, hThreadSelf, pSrcPos);
#endif
                break;
            }
        }
        else if ((u64State & (RTSEMRW_CNT_RD_MASK | RTSEMRW_CNT_WR_MASK)) == 0)
        {
            /* Wrong direction, but we're alone here and can simply try switch the direction. */
            u64State &= ~(RTSEMRW_CNT_RD_MASK | RTSEMRW_CNT_WR_MASK | RTSEMRW_DIR_MASK);
            u64State |= (UINT64_C(1) << RTSEMRW_CNT_RD_SHIFT) | (RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT);
            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
            {
                Assert(!pThis->fNeedReset);
#ifdef RTSEMRW_STRICT
                RTLockValidatorRecSharedAddOwner(&pThis->ValidatorRead, hThreadSelf, pSrcPos);
#endif
                break;
            }
        }
        else
        {
            /* Is the writer perhaps doing a read recursion? */
            RTNATIVETHREAD hNativeSelf = RTThreadNativeSelf();
            RTNATIVETHREAD hNativeWriter;
            ASMAtomicReadHandle(&pThis->hNativeWriter, &hNativeWriter);
            if (hNativeSelf == hNativeWriter)
            {
#ifdef RTSEMRW_STRICT
                int rc9 = RTLockValidatorRecExclRecursionMixed(&pThis->ValidatorWrite, &pThis->ValidatorRead.Core, pSrcPos);
                if (RT_FAILURE(rc9))
                    return rc9;
#endif
                Assert(pThis->cWriterReads < UINT32_MAX / 2);
                ASMAtomicIncU32(&pThis->cWriterReads);
                return VINF_SUCCESS; /* don't break! */
            }

            /* If the timeout is 0, return already. */
            if (!cMillies)
                return VERR_TIMEOUT;

            /* Add ourselves to the queue and wait for the direction to change. */
            uint64_t c = (u64State & RTSEMRW_CNT_RD_MASK) >> RTSEMRW_CNT_RD_SHIFT;
            c++;
            Assert(c < RTSEMRW_CNT_MASK / 2);

            uint64_t cWait = (u64State & RTSEMRW_WAIT_CNT_RD_MASK) >> RTSEMRW_WAIT_CNT_RD_SHIFT;
            cWait++;
            Assert(cWait <= c);
            Assert(cWait < RTSEMRW_CNT_MASK / 2);

            u64State &= ~(RTSEMRW_CNT_RD_MASK | RTSEMRW_WAIT_CNT_RD_MASK);
            u64State |= (c << RTSEMRW_CNT_RD_SHIFT) | (cWait << RTSEMRW_WAIT_CNT_RD_SHIFT);

            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
            {
                for (uint32_t iLoop = 0; ; iLoop++)
                {
                    int rc;
#ifdef RTSEMRW_STRICT
                    rc = RTLockValidatorRecSharedCheckBlocking(&pThis->ValidatorRead, hThreadSelf, pSrcPos, true,
                                                               RTTHREADSTATE_RW_READ, false);
                    if (RT_SUCCESS(rc))
#else
                    RTTHREAD hThreadSelf = RTThreadSelf();
                    RTThreadBlocking(hThreadSelf, RTTHREADSTATE_RW_READ, false);
#endif
                    {
                        if (fInterruptible)
                            rc = RTSemEventMultiWaitNoResume(pThis->hEvtRead, cMillies);
                        else
                            rc = RTSemEventMultiWait(pThis->hEvtRead, cMillies);
                        RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_RW_READ);
                        if (pThis->u32Magic != RTSEMRW_MAGIC)
                            return VERR_SEM_DESTROYED;
                    }
                    if (RT_FAILURE(rc))
                    {
                        /* Decrement the counts and return the error. */
                        for (;;)
                        {
                            u64OldState = u64State = ASMAtomicReadU64(&pThis->u64State);
                            c = (u64State & RTSEMRW_CNT_RD_MASK) >> RTSEMRW_CNT_RD_SHIFT; Assert(c > 0);
                            c--;
                            cWait = (u64State & RTSEMRW_WAIT_CNT_RD_MASK) >> RTSEMRW_WAIT_CNT_RD_SHIFT; Assert(cWait > 0);
                            cWait--;
                            u64State &= ~(RTSEMRW_CNT_RD_MASK | RTSEMRW_WAIT_CNT_RD_MASK);
                            u64State |= (c << RTSEMRW_CNT_RD_SHIFT) | (cWait << RTSEMRW_WAIT_CNT_RD_SHIFT);
                            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                                break;
                        }
                        return rc;
                    }

                    Assert(pThis->fNeedReset);
                    u64State = ASMAtomicReadU64(&pThis->u64State);
                    if ((u64State & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT))
                        break;
                    AssertMsg(iLoop < 1, ("%u\n", iLoop));
                }

                /* Decrement the wait count and maybe reset the semaphore (if we're last). */
                for (;;)
                {
                    u64OldState = u64State;

                    cWait = (u64State & RTSEMRW_WAIT_CNT_RD_MASK) >> RTSEMRW_WAIT_CNT_RD_SHIFT;
                    Assert(cWait > 0);
                    cWait--;
                    u64State &= ~RTSEMRW_WAIT_CNT_RD_MASK;
                    u64State |= cWait << RTSEMRW_WAIT_CNT_RD_SHIFT;

                    if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                    {
                        if (cWait == 0)
                        {
                            if (ASMAtomicXchgBool(&pThis->fNeedReset, false))
                            {
                                int rc = RTSemEventMultiReset(pThis->hEvtRead);
                                AssertRCReturn(rc, rc);
                            }
                        }
                        break;
                    }
                    u64State = ASMAtomicReadU64(&pThis->u64State);
                }

#ifdef RTSEMRW_STRICT
                RTLockValidatorRecSharedAddOwner(&pThis->ValidatorRead, hThreadSelf, pSrcPos);
#endif
                break;
            }
        }

        if (pThis->u32Magic != RTSEMRW_MAGIC)
            return VERR_SEM_DESTROYED;

        ASMNopPause();
        u64State = ASMAtomicReadU64(&pThis->u64State);
        u64OldState = u64State;
    }

    /* got it! */
    Assert((ASMAtomicReadU64(&pThis->u64State) & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT));
    return VINF_SUCCESS;

}


#undef RTSemRWRequestRead
RTDECL(int) RTSemRWRequestRead(RTSEMRW RWSem, unsigned cMillies)
{
#ifndef RTSEMRW_STRICT
    return rtSemRWRequestRead(RWSem, cMillies, false, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtSemRWRequestRead(RWSem, cMillies, false, &SrcPos);
#endif
}
RT_EXPORT_SYMBOL(RTSemRWRequestRead);


RTDECL(int) RTSemRWRequestReadDebug(RTSEMRW RWSem, unsigned cMillies, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtSemRWRequestRead(RWSem, cMillies, false, &SrcPos);
}
RT_EXPORT_SYMBOL(RTSemRWRequestReadDebug);


#undef RTSemRWRequestReadNoResume
RTDECL(int) RTSemRWRequestReadNoResume(RTSEMRW RWSem, unsigned cMillies)
{
#ifndef RTSEMRW_STRICT
    return rtSemRWRequestRead(RWSem, cMillies, true, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtSemRWRequestRead(RWSem, cMillies, true, &SrcPos);
#endif
}
RT_EXPORT_SYMBOL(RTSemRWRequestReadNoResume);


RTDECL(int) RTSemRWRequestReadNoResumeDebug(RTSEMRW RWSem, unsigned cMillies, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtSemRWRequestRead(RWSem, cMillies, true, &SrcPos);
}
RT_EXPORT_SYMBOL(RTSemRWRequestReadNoResumeDebug);



RTDECL(int) RTSemRWReleaseRead(RTSEMRW RWSem)
{
    /*
     * Validate handle.
     */
    RTSEMRWINTERNAL *pThis = RWSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Check the direction and take action accordingly.
     */
    uint64_t u64State    = ASMAtomicReadU64(&pThis->u64State);
    uint64_t u64OldState = u64State;
    if ((u64State & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT))
    {
#ifdef RTSEMRW_STRICT
        int rc9 = RTLockValidatorRecSharedCheckAndRelease(&pThis->ValidatorRead, NIL_RTTHREAD);
        if (RT_FAILURE(rc9))
            return rc9;
#endif
        for (;;)
        {
            uint64_t c = (u64State & RTSEMRW_CNT_RD_MASK) >> RTSEMRW_CNT_RD_SHIFT;
            AssertReturn(c > 0, VERR_NOT_OWNER);
            c--;

            if (   c > 0
                || (u64State & RTSEMRW_CNT_RD_MASK) == 0)
            {
                /* Don't change the direction. */
                u64State &= ~RTSEMRW_CNT_RD_MASK;
                u64State |= c << RTSEMRW_CNT_RD_SHIFT;
                if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                    break;
            }
            else
            {
                /* Reverse the direction and signal the reader threads. */
                u64State &= ~(RTSEMRW_CNT_RD_MASK | RTSEMRW_DIR_MASK);
                u64State |= RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT;
                if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                {
                    int rc = RTSemEventSignal(pThis->hEvtWrite);
                    AssertRC(rc);
                    break;
                }
            }

            ASMNopPause();
            u64State = ASMAtomicReadU64(&pThis->u64State);
            u64OldState = u64State;
        }
    }
    else
    {
        RTNATIVETHREAD hNativeSelf = RTThreadNativeSelf();
        RTNATIVETHREAD hNativeWriter;
        ASMAtomicReadHandle(&pThis->hNativeWriter, &hNativeWriter);
        AssertReturn(hNativeSelf == hNativeWriter, VERR_NOT_OWNER);
        AssertReturn(pThis->cWriterReads > 0, VERR_NOT_OWNER);
#ifdef RTSEMRW_STRICT
        int rc = RTLockValidatorRecExclUnwindMixed(&pThis->ValidatorWrite, &pThis->ValidatorRead.Core);
        if (RT_FAILURE(rc))
            return rc;
#endif
        ASMAtomicDecU32(&pThis->cWriterReads);
    }

    return VINF_SUCCESS;
}
RT_EXPORT_SYMBOL(RTSemRWReleaseRead);


DECL_FORCE_INLINE(int) rtSemRWRequestWrite(RTSEMRW hRWSem, unsigned cMillies, bool fInterruptible, PCRTLOCKVALSRCPOS pSrcPos)
{
    /*
     * Validate input.
     */
    RTSEMRWINTERNAL *pThis = hRWSem;
    if (pThis == NIL_RTSEMRW)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

#ifdef RTSEMRW_STRICT
    RTTHREAD hThreadSelf = NIL_RTTHREAD;
    if (cMillies)
    {
        hThreadSelf = RTThreadSelfAutoAdopt();
        int rc9 = RTLockValidatorRecExclCheckOrder(&pThis->ValidatorWrite, hThreadSelf, pSrcPos);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    /*
     * Check if we're already the owner and just recursing.
     */
    RTNATIVETHREAD hNativeSelf = RTThreadNativeSelf();
    RTNATIVETHREAD hNativeWriter;
    ASMAtomicReadHandle(&pThis->hNativeWriter, &hNativeWriter);
    if (hNativeSelf == hNativeWriter)
    {
        Assert((ASMAtomicReadU64(&pThis->u64State) & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT));
#ifdef RTSEMRW_STRICT
        int rc9 = RTLockValidatorRecExclRecursion(&pThis->ValidatorWrite, pSrcPos);
        if (RT_FAILURE(rc9))
            return rc9;
#endif
        Assert(pThis->cWriteRecursions < UINT32_MAX / 2);
        ASMAtomicIncU32(&pThis->cWriteRecursions);
        return VINF_SUCCESS;
    }

    /*
     * Get cracking.
     */
    uint64_t u64State    = ASMAtomicReadU64(&pThis->u64State);
    uint64_t u64OldState = u64State;

    for (;;)
    {
        if (   (u64State & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT)
            || (u64State & (RTSEMRW_CNT_RD_MASK | RTSEMRW_CNT_WR_MASK)) != 0)
        {
            /* It flows in the right direction, try follow it before it changes. */
            uint64_t c = (u64State & RTSEMRW_CNT_WR_MASK) >> RTSEMRW_CNT_WR_SHIFT;
            c++;
            Assert(c < RTSEMRW_CNT_MASK / 2);
            u64State &= ~RTSEMRW_CNT_WR_MASK;
            u64State |= c << RTSEMRW_CNT_WR_SHIFT;
            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                break;
        }
        else if ((u64State & (RTSEMRW_CNT_RD_MASK | RTSEMRW_CNT_WR_MASK)) == 0)
        {
            /* Wrong direction, but we're alone here and can simply try switch the direction. */
            u64State &= ~(RTSEMRW_CNT_RD_MASK | RTSEMRW_CNT_WR_MASK | RTSEMRW_DIR_MASK);
            u64State |= (UINT64_C(1) << RTSEMRW_CNT_WR_SHIFT) | (RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT);
            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                break;
        }
        else if (!cMillies)
            /* Wrong direction and we're not supposed to wait, just return. */
            return VERR_TIMEOUT;
        else
        {
            /* Add ourselves to the write count and break out to do the wait. */
            uint64_t c = (u64State & RTSEMRW_CNT_WR_MASK) >> RTSEMRW_CNT_WR_SHIFT;
            c++;
            Assert(c < RTSEMRW_CNT_MASK / 2);
            u64State &= ~RTSEMRW_CNT_WR_MASK;
            u64State |= c << RTSEMRW_CNT_WR_SHIFT;
            if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                break;
        }

        if (pThis->u32Magic != RTSEMRW_MAGIC)
            return VERR_SEM_DESTROYED;

        ASMNopPause();
        u64State = ASMAtomicReadU64(&pThis->u64State);
        u64OldState = u64State;
    }

    /*
     * If we're in write mode now try grab the ownership. Play fair if there
     * are threads already waiting.
     */
    bool fDone = (u64State & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT)
              && (  ((u64State & RTSEMRW_CNT_WR_MASK) >> RTSEMRW_CNT_WR_SHIFT) == 1
                  || cMillies == 0);
    if (fDone)
        ASMAtomicCmpXchgHandle(&pThis->hNativeWriter, hNativeSelf, NIL_RTNATIVETHREAD, fDone);
    if (!fDone)
    {
        /*
         * Wait for our turn.
         */
        for (uint32_t iLoop = 0; ; iLoop++)
        {
            int rc;
#ifdef RTSEMRW_STRICT
            if (cMillies)
            {
                if (hThreadSelf == NIL_RTTHREAD)
                    hThreadSelf = RTThreadSelfAutoAdopt();
                rc = RTLockValidatorRecExclCheckBlocking(&pThis->ValidatorWrite, hThreadSelf, pSrcPos, true,
                                                         RTTHREADSTATE_RW_WRITE, false);
            }
            else
                rc = VINF_SUCCESS;
            if (RT_SUCCESS(rc))
#else
            RTTHREAD hThreadSelf = RTThreadSelf();
            RTThreadBlocking(hThreadSelf, RTTHREADSTATE_RW_WRITE, false);
#endif
            {
                if (fInterruptible)
                    rc = RTSemEventWaitNoResume(pThis->hEvtWrite, cMillies);
                else
                    rc = RTSemEventWait(pThis->hEvtWrite, cMillies);
                RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_RW_WRITE);
                if (pThis->u32Magic != RTSEMRW_MAGIC)
                    return VERR_SEM_DESTROYED;
            }
            if (RT_FAILURE(rc))
            {
                /* Decrement the counts and return the error. */
                for (;;)
                {
                    u64OldState = u64State = ASMAtomicReadU64(&pThis->u64State);
                    uint64_t c = (u64State & RTSEMRW_CNT_WR_MASK) >> RTSEMRW_CNT_WR_SHIFT; Assert(c > 0);
                    c--;
                    u64State &= ~RTSEMRW_CNT_WR_MASK;
                    u64State |= c << RTSEMRW_CNT_WR_SHIFT;
                    if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                        break;
                }
                return rc;
            }

            u64State = ASMAtomicReadU64(&pThis->u64State);
            if ((u64State & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT))
            {
                ASMAtomicCmpXchgHandle(&pThis->hNativeWriter, hNativeSelf, NIL_RTNATIVETHREAD, fDone);
                if (fDone)
                    break;
            }
            AssertMsg(iLoop < 1000, ("%u\n", iLoop)); /* may loop a few times here... */
        }
    }

    /*
     * Got it!
     */
    Assert((ASMAtomicReadU64(&pThis->u64State) & RTSEMRW_DIR_MASK) == (RTSEMRW_DIR_WRITE << RTSEMRW_DIR_SHIFT));
    ASMAtomicWriteU32(&pThis->cWriteRecursions, 1);
    Assert(pThis->cWriterReads == 0);
#ifdef RTSEMRW_STRICT
    RTLockValidatorRecExclSetOwner(&pThis->ValidatorWrite, hThreadSelf, pSrcPos, true);
#endif

    return VINF_SUCCESS;
}


#undef RTSemRWRequestWrite
RTDECL(int) RTSemRWRequestWrite(RTSEMRW RWSem, unsigned cMillies)
{
#ifndef RTSEMRW_STRICT
    return rtSemRWRequestWrite(RWSem, cMillies, false, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtSemRWRequestWrite(RWSem, cMillies, false, &SrcPos);
#endif
}
RT_EXPORT_SYMBOL(RTSemRWRequestWrite);


RTDECL(int) RTSemRWRequestWriteDebug(RTSEMRW RWSem, unsigned cMillies, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtSemRWRequestWrite(RWSem, cMillies, false, &SrcPos);
}
RT_EXPORT_SYMBOL(RTSemRWRequestWriteDebug);


#undef RTSemRWRequestWriteNoResume
RTDECL(int) RTSemRWRequestWriteNoResume(RTSEMRW RWSem, unsigned cMillies)
{
#ifndef RTSEMRW_STRICT
    return rtSemRWRequestWrite(RWSem, cMillies, true, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtSemRWRequestWrite(RWSem, cMillies, true, &SrcPos);
#endif
}
RT_EXPORT_SYMBOL(RTSemRWRequestWriteNoResume);


RTDECL(int) RTSemRWRequestWriteNoResumeDebug(RTSEMRW RWSem, unsigned cMillies, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtSemRWRequestWrite(RWSem, cMillies, true, &SrcPos);
}
RT_EXPORT_SYMBOL(RTSemRWRequestWriteNoResumeDebug);


RTDECL(int) RTSemRWReleaseWrite(RTSEMRW RWSem)
{

    /*
     * Validate handle.
     */
    struct RTSEMRWINTERNAL *pThis = RWSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

    RTNATIVETHREAD hNativeSelf = RTThreadNativeSelf();
    RTNATIVETHREAD hNativeWriter;
    ASMAtomicReadHandle(&pThis->hNativeWriter, &hNativeWriter);
    AssertReturn(hNativeSelf == hNativeWriter, VERR_NOT_OWNER);

    /*
     * Unwind a recursion.
     */
    if (pThis->cWriteRecursions == 1)
    {
        AssertReturn(pThis->cWriterReads == 0, VERR_WRONG_ORDER); /* (must release all read recursions before the final write.) */
#ifdef RTSEMRW_STRICT
        int rc9 = RTLockValidatorRecExclReleaseOwner(&pThis->ValidatorWrite, true);
        if (RT_FAILURE(rc9))
            return rc9;
#endif
        /*
         * Update the state.
         */
        ASMAtomicWriteU32(&pThis->cWriteRecursions, 0);
        /** @todo validate order. */
        ASMAtomicWriteHandle(&pThis->hNativeWriter, NIL_RTNATIVETHREAD);

        for (;;)
        {
            uint64_t u64State    = ASMAtomicReadU64(&pThis->u64State);
            uint64_t u64OldState = u64State;

            uint64_t c = (u64State & RTSEMRW_CNT_WR_MASK) >> RTSEMRW_CNT_WR_SHIFT;
            Assert(c > 0);
            c--;

            if (   c > 0
                || (u64State & RTSEMRW_CNT_RD_MASK) == 0)
            {
                /* Don't change the direction, wait up the next writer if any. */
                u64State &= ~RTSEMRW_CNT_WR_MASK;
                u64State |= c << RTSEMRW_CNT_WR_SHIFT;
                if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                {
                    if (c > 0)
                    {
                        int rc = RTSemEventSignal(pThis->hEvtWrite);
                        AssertRC(rc);
                    }
                    break;
                }
            }
            else
            {
                /* Reverse the direction and signal the reader threads. */
                u64State &= ~(RTSEMRW_CNT_WR_MASK | RTSEMRW_DIR_MASK);
                u64State |= RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT;
                if (ASMAtomicCmpXchgU64(&pThis->u64State, u64State, u64OldState))
                {
                    Assert(!pThis->fNeedReset);
                    ASMAtomicWriteBool(&pThis->fNeedReset, true);
                    int rc = RTSemEventMultiSignal(pThis->hEvtRead);
                    AssertRC(rc);
                    break;
                }
            }

            ASMNopPause();
            if (pThis->u32Magic != RTSEMRW_MAGIC)
                return VERR_SEM_DESTROYED;
        }
    }
    else
    {
        Assert(pThis->cWriteRecursions != 0);
#ifdef RTSEMRW_STRICT
        int rc9 = RTLockValidatorRecExclUnwind(&pThis->ValidatorWrite);
        if (RT_FAILURE(rc9))
            return rc9;
#endif
        ASMAtomicDecU32(&pThis->cWriteRecursions);
    }

    return VINF_SUCCESS;
}
RT_EXPORT_SYMBOL(RTSemRWReleaseWrite);


RTDECL(bool) RTSemRWIsWriteOwner(RTSEMRW RWSem)
{
    /*
     * Validate handle.
     */
    struct RTSEMRWINTERNAL *pThis = RWSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Check ownership.
     */
    RTNATIVETHREAD hNativeSelf = RTThreadNativeSelf();
    RTNATIVETHREAD hNativeWriter;
    ASMAtomicReadHandle(&pThis->hNativeWriter, &hNativeWriter);
    return hNativeWriter == hNativeSelf;
}
RT_EXPORT_SYMBOL(RTSemRWIsWriteOwner);


RTDECL(uint32_t) RTSemRWGetWriteRecursion(RTSEMRW RWSem)
{
    /*
     * Validate handle.
     */
    struct RTSEMRWINTERNAL *pThis = RWSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Return the requested data.
     */
    return pThis->cWriteRecursions;
}
RT_EXPORT_SYMBOL(RTSemRWGetWriteRecursion);


RTDECL(uint32_t) RTSemRWGetWriterReadRecursion(RTSEMRW RWSem)
{
    /*
     * Validate handle.
     */
    struct RTSEMRWINTERNAL *pThis = RWSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMRW_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Return the requested data.
     */
    return pThis->cWriterReads;
}
RT_EXPORT_SYMBOL(RTSemRWGetWriterReadRecursion);


RTDECL(uint32_t) RTSemRWGetReadCount(RTSEMRW RWSem)
{
    /*
     * Validate input.
     */
    struct RTSEMRWINTERNAL *pThis = RWSem;
    AssertPtrReturn(pThis, 0);
    AssertMsgReturn(pThis->u32Magic == RTSEMRW_MAGIC,
                    ("pThis=%p u32Magic=%#x\n", pThis, pThis->u32Magic),
                    0);

    /*
     * Return the requested data.
     */
    uint64_t u64State = ASMAtomicReadU64(&pThis->u64State);
    if ((u64State & RTSEMRW_DIR_MASK) != (RTSEMRW_DIR_READ << RTSEMRW_DIR_SHIFT))
        return 0;
    return (u64State & RTSEMRW_CNT_RD_MASK) >> RTSEMRW_CNT_RD_SHIFT;
}
RT_EXPORT_SYMBOL(RTSemRWGetReadCount);

