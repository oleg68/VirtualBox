; $Id$
;; @file
; BS3Kit - Bs3SwitchToPE16_V86
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
; Switch to 16-bit unpaged protected mode from any other mode.
;
; @cproto   BS3_DECL(void) Bs3SwitchToPE16(void);
;
; @uses     Nothing (except high 32-bit register parts).
;
; @remarks  Obviously returns to 16-bit mode, even if the caller was
;           in 32-bit or 64-bit mode.
;
; @remarks  Does not require 20h of parameter scratch space in 64-bit mode.
;
BS3_PROC_BEGIN_MODE Bs3SwitchToPE16_V86
%ifdef TMPL_PE16_V86
        ret

%else
        ;
        ; Convert the return address and jump to the 16-bit code segment.
        ;
 %if TMPL_BITS != 16
        shl     [xSP], TMPL_BITS - 16
        add     xSP, (TMPL_BITS - 16) / 8
        jmp     .sixteen_bit_segment
BS3_BEGIN_TEXT16
        BS3_SET_BITS TMPL_BITS
.sixteen_bit_segment:
 %endif

        ;
        ; Switch to 16-bit PE16 and from there to V8086.
        ;
        extern  TMPL_NM(Bs3SwitchToPE16)
        call    TMPL_NM(Bs3SwitchToPE16)
        BS3_SET_BITS 16

        ;
        ; Switch to v8086 mode.
        ;
        extern  TMPL_NM(Bs3SwitchToV86)
        jmp     TMPL_NM(Bs3SwitchToV86)
%endif
BS3_PROC_END_MODE   Bs3SwitchToPE16_V86

