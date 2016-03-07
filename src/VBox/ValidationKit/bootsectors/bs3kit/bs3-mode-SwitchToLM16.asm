; $Id$
;; @file
; BS3Kit - Bs3SwitchToLM16
;

;
; Copyright (C) 2007-2016 Oracle Corporation
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


;;
; Switch to 16-bit long mode from any other mode.
;
; @cproto   BS3_DECL(void) Bs3SwitchToLM16(void);
;
; @uses     Nothing (except possibly high 32-bit and/or upper 64-bit register parts).
;
; @remarks  Obviously returns to 16-bit mode, even if the caller was in 32-bit
;           or 64-bit mode.  It doesn't not preserve the callers ring, but
;           instead changes to ring-0.
;
; @remarks  Does not require 20h of parameter scratch space in 64-bit mode.
;
BS3_PROC_BEGIN_MODE Bs3SwitchToLM16
%ifdef TMPL_LM16
        ret

%elifdef TMPL_CMN_LM
        ;
        ; Already in long mode, just switch to 16-bit.
        ;
        extern  BS3_CMN_NM(Bs3SwitchTo16Bit)
        jmp     BS3_CMN_NM(Bs3SwitchTo16Bit)

%else
        ;
        ; Switch to LM32 and then switch to 64-bits (IDT & TSS are the same for
        ; LM16, LM32 and LM64, unlike the rest).
        ;
        ; (The long mode switching code is going via 32-bit protected mode, so
        ; Bs3SwitchToLM32 contains the actual code for switching to avoid
        ; unnecessary 32-bit -> 64-bit -> 32-bit trips.)
        ;
        extern  TMPL_NM(Bs3SwitchToLM32)
        call    TMPL_NM(Bs3SwitchToLM32)
        BS3_SET_BITS 32

        extern  _Bs3SwitchTo16Bit_c32
 %if TMPL_BITS == 16
        sub     esp, 2
        shr     dword [esp], 16
 %elif TMPL_BITS == 64
        pop     dword [esp + 4]
 %endif
        jmp     _Bs3SwitchTo16Bit_c32
%endif
BS3_PROC_END_MODE   Bs3SwitchToLM16

