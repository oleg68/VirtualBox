/* $Id$ */
/** @file
 * IPRT - No-CRT - Windows EXE startup code.
 *
 * @note Does not run static constructors and destructors!
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
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
#include "internal/nocrt.h"
#include "internal/process.h"

#include <iprt/nt/nt-and-windows.h>
#include <iprt/getopt.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/utf16.h>

#include "internal/compiler-vcc.h"


/*********************************************************************************************************************************
*   External Symbols                                                                                                             *
*********************************************************************************************************************************/
extern int main(int argc, char **argv, char **envp);    /* in program */
#ifndef IPRT_NO_CRT
extern DECLHIDDEN(void) InitStdHandles(PRTL_USER_PROCESS_PARAMETERS pParams); /* nocrt-streams-win.cpp */ /** @todo put in header */
#endif


static int rtTerminateProcess(int32_t rcExit, bool fDoAtExit)
{
#ifdef IPRT_NO_CRT
    /*
     * Run atexit callback in reverse order.
     */
    if (fDoAtExit)
    {
        rtVccTermRunAtExit();
        rtVccInitializersRunTerm();
    }
#else
    RT_NOREF(fDoAtExit);
#endif

    /*
     * Terminate.
     */
    for (;;)
        NtTerminateProcess(NtCurrentProcess(), rcExit);
}


DECLASM(void) CustomMainEntrypoint(PPEB pPeb)
{
    /*
     * Initialize stuff.
     */
#ifdef IPRT_NO_CRT
    rtVccInitSecurityCookie();
#else
    InitStdHandles(pPeb->ProcessParameters);
#endif
    rtVccWinInitProcExecPath();

    RTEXITCODE rcExit;
#ifdef IPRT_NO_CRT
    AssertCompile(sizeof(rcExit) == sizeof(int));
    rcExit = (RTEXITCODE)rtVccInitializersRunInit();
    if (rcExit == RTEXITCODE_SUCCESS)
#endif
    {
        /*
         * Get and convert the command line to argc/argv format.
         */
        rcExit = RTEXITCODE_INIT;
        UNICODE_STRING const *pCmdLine = pPeb->ProcessParameters ? &pPeb->ProcessParameters->CommandLine : NULL;
        if (pCmdLine)
        {
            char *pszCmdLine = NULL;
            int rc = RTUtf16ToUtf8Ex(pCmdLine->Buffer, pCmdLine->Length / sizeof(WCHAR), &pszCmdLine, 0, NULL);
            if (RT_SUCCESS(rc))
            {
                char **papszArgv;
                int    cArgs = 0;
                rc = RTGetOptArgvFromString(&papszArgv, &cArgs, pszCmdLine,
                                            RTGETOPTARGV_CNV_MODIFY_INPUT | RTGETOPTARGV_CNV_QUOTE_MS_CRT,  NULL);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Call the main function.
                     */
                    AssertCompile(sizeof(rcExit) == sizeof(int));
                    rcExit = (RTEXITCODE)main(cArgs, papszArgv, NULL /*envp*/);
                }
                else
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
                    RTMsgError("Error parsing command line: %Rrc\n", rc);
#else
                    rtNoCrtFatalMsgWithRc(RT_STR_TUPLE("Error parsing command line: "), rc);
#endif
            }
            else
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
                RTMsgError("Failed to convert command line to UTF-8: %Rrc\n", rc);
#else
                rtNoCrtFatalMsgWithRc(RT_STR_TUPLE("Failed to convert command line to UTF-8: "), rc);
#endif
        }
        else
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
            RTMsgError("No command line\n");
#else
            rtNoCrtFatalMsg(RT_STR_TUPLE("No command line\r\n"));
#endif
        rtTerminateProcess(rcExit, true /*fDoAtExit*/);
    }
#ifdef IPRT_NO_CRT
    else
    {
# ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
        RTMsgError("A C static initializor failed (%d)\n", rcExit);
# else
        rtNoCrtFatalWriteBegin(RT_STR_TUPLE("A C static initializor failed ("));
        rtNoCrtFatalWriteWinRc(rcExit);
        rtNoCrtFatalWriteEnd(RT_STR_TUPLE("\r\n"));
# endif
        rtTerminateProcess(rcExit, false /*fDoAtExit*/);
    }
#endif
}

