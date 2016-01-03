; $Id$
;; @file
; BS3Kit - Bs3PagingGetRootForPP32
;

;
; Copyright (C) 2007-2015 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL) only, as it comes in the "COPYING.CDDL" file of the
; VirtualBox OSE distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;

%include "bs3kit-template-header.mac"


%ifdef TMPL_RM
extern TMPL_NM(Bs3SwitchToPE16)
extern NAME(Bs3SwitchToRM_pe16)
%elifdef TMPL_CMN_V86
extern TMPL_NM(Bs3SwitchToRing0)
extern TMPL_NM(Bs3SwitchToV86)
%endif

BS3_EXTERN_CMN Bs3PagingInitRootForPP

BS3_EXTERN_DATA16 g_PhysPagingRootPP
TMPL_BEGIN_TEXT


;;
; @cproto   BS3_DECL(uint32_t) Bs3PagingGetRootForPP32(void)
;
; @returns  eax
;
; @uses     ax
;
; @remarks  returns value in EAX, not dx:ax!
;
BS3_PROC_BEGIN_MODE Bs3PagingGetRootForPP32
        mov     eax, [g_PhysPagingRootPP]
        cmp     eax, 0ffffffffh
        je      .init_root
        ret

.init_root:
        push    xBP
        mov     xBP, xSP

%ifdef TMPL_RM
        ;
        ; We don't want to be restricted to real mode addressing, so
        ; temporarily switch to 16-bit protected mode.
        ;
        call    TMPL_NM(Bs3SwitchToPE16)
        call    Bs3PagingInitRootForPP
        call    NAME(Bs3SwitchToRM_pe16)
%elifdef TMPL_CMN_V86
        ;
        ; V8086 mode uses real mode addressing too.  Unlikly that we'll
        ; ever end up here though.
        ;
        call    TMPL_NM(Bs3SwitchToRing0)
        call    Bs3PagingInitRootForPP
        call    TMPL_NM(Bs3SwitchToV86)
%else
        ;
        ; Not a problematic addressing mode.
        ;
        call    Bs3PagingInitRootForPP
%endif

        mov     eax, [g_PhysPagingRootPP]
        leave
        ret
BS3_PROC_END_MODE   Bs3PagingGetRootForPP32

