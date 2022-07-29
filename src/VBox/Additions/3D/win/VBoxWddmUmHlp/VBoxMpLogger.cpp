/* $Id$ */
/** @file
 * VBox WDDM Display logger implementation
 */

/*
 * Copyright (C) 2018-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* We're unable to use standard r3 vbgl-based backdoor logging API because win8 Metro apps
 * can not do CreateFile/Read/Write by default.
 * This is why we use miniport escape functionality to issue backdoor log string to the miniport
 * and submit it to host via standard r0 backdoor logging api accordingly
 */

#define IPRT_NO_CRT_FOR_3RD_PARTY /* To get malloc and free wrappers in IPRT_NO_CRT mode. Doesn't link with IPRT in non-no-CRT mode. */
#include "UmHlpInternal.h"

#include <../../../common/wddm/VBoxMPIf.h>
#include <stdlib.h>
#ifdef IPRT_NO_CRT
# include <iprt/process.h>
# include <iprt/string.h>
#else
# include <stdio.h>
#endif

DECLCALLBACK(void) VBoxDispMpLoggerLog(const char *pszString)
{
    D3DKMTFUNCTIONS const *d3dkmt = D3DKMTFunctions();
    if (d3dkmt->pfnD3DKMTEscape == NULL)
        return;

    D3DKMT_HANDLE hAdapter;
    NTSTATUS Status = vboxDispKmtOpenAdapter(&hAdapter);
    Assert(Status == STATUS_SUCCESS);
    if (Status == 0)
    {
        uint32_t cbString = (uint32_t)strlen(pszString) + 1;
        uint32_t cbCmd = RT_UOFFSETOF_DYN(VBOXDISPIFESCAPE_DBGPRINT, aStringBuf[cbString]);
        PVBOXDISPIFESCAPE_DBGPRINT pCmd = (PVBOXDISPIFESCAPE_DBGPRINT)malloc(cbCmd);
        Assert(pCmd);
        if (pCmd)
        {
            pCmd->EscapeHdr.escapeCode = VBOXESC_DBGPRINT;
            pCmd->EscapeHdr.u32CmdSpecific = 0;
            memcpy(pCmd->aStringBuf, pszString, cbString);

            D3DKMT_ESCAPE EscapeData;
            memset(&EscapeData, 0, sizeof(EscapeData));
            EscapeData.hAdapter = hAdapter;
            // EscapeData.hDevice = NULL;
            EscapeData.Type = D3DKMT_ESCAPE_DRIVERPRIVATE;
            // EscapeData.Flags.HardwareAccess = 0;
            EscapeData.pPrivateDriverData = pCmd;
            EscapeData.PrivateDriverDataSize = cbCmd;
            // EscapeData.hContext = NULL;

            Status = d3dkmt->pfnD3DKMTEscape(&EscapeData);
            Assert(Status == STATUS_SUCCESS);

            free(pCmd);
        }

        Status = vboxDispKmtCloseAdapter(hAdapter);
        Assert(Status == STATUS_SUCCESS);
    }
}

DECLCALLBACK(void) VBoxDispMpLoggerLogF(const char *pszFormat, ...)
{
    /** @todo would make a whole lot more sense to just allocate
     *        VBOXDISPIFESCAPE_DBGPRINT here and printf into it's buffer than
     *        double buffering it like this */
    char szBuffer[4096];
    va_list va;
    va_start(va, pszFormat);
#ifdef IPRT_NO_CRT
    RTStrPrintf(szBuffer, sizeof(szBuffer), pszFormat, va);
#else
    _vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, va);
    szBuffer[sizeof(szBuffer) - 1] = '\0'; /* Don't trust the _vsnprintf function terminate the string! */
#endif
    va_end(va);

    VBoxDispMpLoggerLog(szBuffer);
}

/**
 * Prefix the output string with exe name and pid/tid.
 */
#ifndef IPRT_NO_CRT
static const char *vboxUmLogGetExeName(void)
{
    static int s_fModuleNameInited = 0;
    static char s_szModuleName[MAX_PATH];

    if (!s_fModuleNameInited)
    {
        const DWORD cchName = GetModuleFileNameA(NULL, s_szModuleName, RT_ELEMENTS(s_szModuleName));
        if (cchName == 0)
            return "<no module>";
        s_fModuleNameInited = 1;
    }
    return &s_szModuleName[0];
}
#endif

DECLCALLBACK(void) VBoxWddmUmLog(const char *pszString)
{
    /** @todo Allocate VBOXDISPIFESCAPE_DBGPRINT here and format right into it
     *        instead? That would be a lot more flexible and a little faster. */
    char szBuffer[4096];
#ifdef IPRT_NO_CRT
    /** @todo use RTProcShortName instead of RTProcExecutablePath? Will avoid
     *        chopping off log text if the executable path is too long. */
    RTStrPrintf(szBuffer, sizeof(szBuffer), "['%s' 0x%lx.0x%lx]: %s",
                RTProcExecutablePath() /* should've been initialized by nocrt-startup-dll-win.cpp already */,
                GetCurrentProcessId(), GetCurrentThreadId(), pszString);
#else
    int cch = _snprintf(szBuffer, sizeof(szBuffer), "['%s' 0x%lx.0x%lx]: %s",
                        vboxUmLogGetExeName(), GetCurrentProcessId(), GetCurrentThreadId(), pszString);
    AssertReturnVoid(cch > 0);             /* unlikely that we'll have string encoding problems, but just in case. */
    szBuffer[sizeof(szBuffer) - 1] = '\0'; /* the function doesn't necessarily terminate the buffer on overflow. */
#endif

    VBoxDispMpLoggerLog(szBuffer);
}
