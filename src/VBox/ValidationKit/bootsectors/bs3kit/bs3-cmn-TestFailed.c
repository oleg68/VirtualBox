/* $Id$ */
/** @file
 * BS3Kit - Bs3TestFailed, Bs3TestFailedF, Bs3TestFailedV.
 */

/*
 * Copyright (C) 2007-2016 Oracle Corporation
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
#include "bs3kit-template-header.h"
#include "bs3-cmn-test.h"
#include <iprt/asm-amd64-x86.h>


/**
 * @impl_callback_method{FNBS3STRFORMATOUTPUT,
 *      Used by Bs3TestFailedV and Bs3TestSkippedV.}
 */
BS3_DECL_CALLBACK(size_t) bs3TestFailedStrOutput(char ch, void BS3_FAR *pvUser)
{
    PBS3TESTFAILEDBUF pBuf = (PBS3TESTFAILEDBUF)pvUser;

    /*
     * VMMDev first.  We postpone newline processing here so we can strip one
     * trailing newline.
     */
    if (g_fbBs3VMMDevTesting)
    {
        if (pBuf->fNewLine && ch != '\0')
            ASMOutU8(VMMDEV_TESTING_IOPORT_DATA, '\n');
        pBuf->fNewLine = ch == '\n';
        if (ch != '\n')
            ASMOutU8(VMMDEV_TESTING_IOPORT_DATA, ch);
    }

    /*
     * Console next.
     */
    if (ch != '\0')
    {
        BS3_ASSERT(pBuf->cchBuf < RT_ELEMENTS(pBuf->achBuf));
        pBuf->achBuf[pBuf->cchBuf++] = ch;

        /* Whether to flush the buffer.  We do line flushing here to avoid
           dropping too much info when the formatter crashes on bad input. */
        if (   pBuf->cchBuf < RT_ELEMENTS(pBuf->achBuf)
            && ch != '\n')
        {
            pBuf->fNewLine = false;
            return 1;
        }
        pBuf->fNewLine = '\n';
    }
    /* Try fit missing newline into the buffer. */
    else if (!pBuf->fNewLine && pBuf->cchBuf < RT_ELEMENTS(pBuf->achBuf))
    {
        pBuf->fNewLine = true;
        pBuf->achBuf[pBuf->cchBuf++] = '\n';
    }

    BS3_ASSERT(pBuf->cchBuf <= RT_ELEMENTS(pBuf->achBuf));
    Bs3PrintStrN(&pBuf->achBuf[0], pBuf->cchBuf);
    pBuf->cchBuf = 0;

    /* In case we failed to add trailing new line, print one separately.  */
    if (!pBuf->fNewLine)
        Bs3PrintChr('\n');

    return ch != '\0';
}


/**
 * Equivalent to RTTestIFailedV.
 */
BS3_DECL(void) Bs3TestFailedV(const char *pszFormat, va_list va)
{
    BS3TESTFAILEDBUF Buf;

    if (!++g_cusBs3TestErrors)
        g_cusBs3TestErrors++;

    if (g_fbBs3VMMDevTesting)
#if ARCH_BITS == 16
        ASMOutU16(VMMDEV_TESTING_IOPORT_CMD, (uint16_t)VMMDEV_TESTING_CMD_FAILED);
#else
        ASMOutU32(VMMDEV_TESTING_IOPORT_CMD, VMMDEV_TESTING_CMD_FAILED);
#endif

    Buf.fNewLine = false;
    Buf.cchBuf   = 0;
    Bs3StrFormatV(pszFormat, va, bs3TestFailedStrOutput, &Buf);
}


/**
 * Equivalent to RTTestIFailedF.
 */
BS3_DECL(void) Bs3TestFailedF(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    Bs3TestFailedV(pszFormat, va);
    va_end(va);
}


/**
 * Equivalent to RTTestIFailed.
 */
BS3_DECL(void) Bs3TestFailed(const char *pszMessage)
{
    Bs3TestFailedF("%s", pszMessage);
}

