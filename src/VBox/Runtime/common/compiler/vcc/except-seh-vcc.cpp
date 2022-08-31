/* $Id$ */
/** @file
 * IPRT - Visual C++ Compiler - SEH exception handler (__try/__except/__finally).
 */

/*
 * Copyright (C) 2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/nocrt.h"

#include "except-vcc.h"


#if !defined(RT_ARCH_AMD64)
# error "This file is for AMD64 (and probably ARM, but needs porting)"
#endif



/**
 * Call exception filters, handlers and unwind code.
 *
 * This is called for windows' structured exception handling (SEH), i.e. the
 * __try/__except/__finally stuff in Visual C++.  The compiler generate scope
 * records for the __try/__except blocks as well as unwind records for __finally
 * and probably C++ stack object destructors.
 *
 * @returns Exception disposition.
 * @param   pXcptRec    The exception record.
 * @param   pXcptRegRec The exception registration record, taken to be the frame
 *                      address.
 * @param   pCpuCtx     The CPU context for the exception.
 * @param   pDispCtx    Dispatcher context.
 */
EXCEPTION_DISPOSITION __C_specific_handler(PEXCEPTION_RECORD pXcptRec, PEXCEPTION_REGISTRATION_RECORD pXcptRegRec,
                                           PCONTEXT pCpuCtx, PDISPATCHER_CONTEXT pDispCtx)
{
    /*
     * This function works the scope table, starting at ScopeIndex
     * from the dispatcher context.
     */
    SCOPE_TABLE const * const   pScopeTab   = (SCOPE_TABLE const *)pDispCtx->HandlerData;
    uint32_t const              cScopes     = pScopeTab->Count;
    uint32_t                    idxScope    = pDispCtx->ScopeIndex;

    /*
     * The table addresses are RVAs, so convert the program counter (PC) to an RVA.
     */
    uint32_t const              uRvaPc      = pDispCtx->ControlPc - pDispCtx->ImageBase;

    /*
     * Before we get any further, there are two types of scope records:
     *      1. Unwind (aka termination) handler (JumpTarget == 0).
     *      2. Exception filter & handler (JumpTarget != 0).
     */

    if (IS_DISPATCHING(pXcptRec->ExceptionFlags))
    {
        /*
         * Call exception filter functions when dispatching.
         */
        for (; idxScope < cScopes; idxScope++)
        {
            /* Skip unwind entries (exception handler set to zero). */
            uint32_t const uXcptHandler = pScopeTab->ScopeRecord[idxScope].JumpTarget;
            if (uXcptHandler != 0)
            {
                uint32_t const uBegin  = pScopeTab->ScopeRecord[idxScope].BeginAddress;
                uint32_t const uEnd    = pScopeTab->ScopeRecord[idxScope].EndAddress;
                uint32_t const cbScope = uEnd - uBegin;
                if (   uRvaPc - uBegin < cbScope
                    && uBegin < uEnd /* paranoia */)
                {
                    /* The special HandlerAddress value 1 translates to a
                       EXCEPTION_EXECUTE_HANDLER filter return value. */
                    LONG lRet = EXCEPTION_EXECUTE_HANDLER;
                    uint32_t const uFltTermHandler = pScopeTab->ScopeRecord[idxScope].HandlerAddress;
                    if (uFltTermHandler != 1)
                    {
                        PEXCEPTION_FILTER  const pfnFilter = (PEXCEPTION_FILTER)(pDispCtx->ImageBase + uFltTermHandler);
                        EXCEPTION_POINTERS       XcptPtrs  = { pXcptRec, pCpuCtx };
                        /** @todo shouldn't we do a guard check on this call? */
                        lRet = pfnFilter(&XcptPtrs, pXcptRegRec);

                        AssertCompile(EXCEPTION_CONTINUE_SEARCH == 0);
                        if (lRet == EXCEPTION_CONTINUE_SEARCH)
                            continue;
                    }

                    /* Return if we're supposed to continue execution (the convension
                       it to match negative values rather than the exact defined value):  */
                    AssertCompile(EXCEPTION_CONTINUE_EXECUTION == -1);
                    AssertCompile(EXCEPTION_EXECUTE_HANDLER == 1);
                    if (lRet <= EXCEPTION_CONTINUE_EXECUTION)
                        return ExceptionContinueExecution;

                    /* Execute the handler (lRet >= EXCEPTION_EXECUTE_HANDLER). */
                    uintptr_t const uPtrXcptHandler = uXcptHandler + pDispCtx->ImageBase;
                    /** @todo shouldn't we do a guard check on this call? */

                    /// @todo _NLG_Notify(uPtrXcptHandler, pXcptRegRec, 1); - debugger notification.

                    RtlUnwindEx(pXcptRegRec, (void *)uPtrXcptHandler, pXcptRec,
                                (PVOID)(uintptr_t)pXcptRec->ExceptionCode, pCpuCtx, pDispCtx->HistoryTable);

                    /// @todo _NLG_Return2(); - debugger notification.
                }
            }
        }
    }
    else
    {
        /*
         * Do unwinding.
         */

        /* Convert the target unwind address to an RVA up front for efficiency.
           (I think target unwinding is what the RtlUnwindEx call above does.) */
        uint32_t const uTargetPc = pXcptRec->ExceptionFlags & EXCEPTION_TARGET_UNWIND
                                 ? pDispCtx->TargetIp - pDispCtx->ImageBase
                                 : UINT32_MAX;

        for (; idxScope < cScopes; idxScope++)
        {
            uint32_t const uBegin  = pScopeTab->ScopeRecord[idxScope].BeginAddress;
            uint32_t const uEnd    = pScopeTab->ScopeRecord[idxScope].EndAddress;
            uint32_t const cbScope = uEnd - uBegin;
            if (   uRvaPc - uBegin < cbScope
                && uBegin < uEnd /* paranoia */)
            {
                uint32_t const uFltTermHandler = pScopeTab->ScopeRecord[idxScope].HandlerAddress;
                uint32_t const uXcptHandler    = pScopeTab->ScopeRecord[idxScope].JumpTarget;

                /* Target unwind requires us to stop if the target PC is in the same
                   scope as the control PC.  Happens for goto out of inner __try scope
                   or when longjmp'ing into a __try scope. */
                if (pXcptRec->ExceptionFlags & EXCEPTION_TARGET_UNWIND)
                {
                    /* The scope same-ness is identified by the same handler and jump target rva values. */
                    for (uint32_t idxTgtScope = 0; idxTgtScope < cScopes; idxTgtScope++)
                        if (   pScopeTab->ScopeRecord[idxTgtScope].JumpTarget     == uXcptHandler
                            && pScopeTab->ScopeRecord[idxTgtScope].HandlerAddress == uFltTermHandler)
                        {
                            uint32_t const uTgtBegin  = pScopeTab->ScopeRecord[idxTgtScope].BeginAddress;
                            uint32_t const uTgtEnd    = pScopeTab->ScopeRecord[idxTgtScope].EndAddress;
                            uint32_t const cbTgtScope = uTgtEnd - uTgtBegin;
                            if (   uTargetPc - uTgtBegin < uTgtBegin
                                && uTgtBegin < uTgtEnd /* paranoia */)
                                return ExceptionContinueSearch;
                        }
                }

                /* The unwind handlers are what we're here for. */
                if (uXcptHandler == 0)
                {
                    PTERMINATION_HANDLER const pfnTermHandler = (PTERMINATION_HANDLER)(pDispCtx->ImageBase + uFltTermHandler);
                    pDispCtx->ScopeIndex = idxScope + 1;
                    /** @todo shouldn't we do a guard check on this call? */
                    pfnTermHandler(TRUE /*fAbend*/, pXcptRegRec);
                }
                /* Exception filter & handler entries are skipped, unless the exception
                   handler is being targeted by the unwind, in which case we're done
                   unwinding and the caller should transfer control there. */
                else if (   uXcptHandler == uTargetPc
                         && (pXcptRec->ExceptionFlags & EXCEPTION_TARGET_UNWIND))
                    return ExceptionContinueSearch;
            }
        }
    }

    return ExceptionContinueSearch;
}

