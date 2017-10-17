; $Id$
;; @file
; BS3Kit - Bs3SelFlatDataToProtFar16.
;

;
; Copyright (C) 2007-2017 Oracle Corporation
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


;*********************************************************************************************************************************
;*      Header Files                                                                                                             *
;*********************************************************************************************************************************
%include "bs3kit-template-header.mac"


;*********************************************************************************************************************************
;*  External Symbols                                                                                                             *
;*********************************************************************************************************************************
%ifdef BS3_STRICT
BS3_EXTERN_CMN Bs3Panic
%endif

TMPL_BEGIN_TEXT
%if TMPL_BITS == 16
CPU 8086
%endif


;;
; @cproto   BS3_CMN_PROTO_NOSB(uint32_t, Bs3SelFlatDataToProtFar16,(uint32_t uFlatAddr));
;
; @uses     Only return registers (ax:dx, eax, eax)
; @remarks  No 20h scratch area requirements.
;
BS3_PROC_BEGIN_CMN Bs3SelFlatDataToProtFar16, BS3_PBC_NEAR      ; Far stub generated by the makefile/bs3kit.h.
        push    xBP
        mov     xBP, xSP

        ;
        ; Check if we can use the protected mode stack or data selector.
        ; The latter ensures the usability of this function for setting SS.
        ;
%if TMPL_BITS == 16
        mov     ax, [xBP + xCB + cbCurRetAddr]
        mov     dx, [xBP + xCB + cbCurRetAddr + 2]
        test    dx, dx
        jnz     .not_stack
        mov     dx, BS3_SEL_R0_SS16
%else
TNOT64  mov     eax, [xBP + xCB + cbCurRetAddr]
TONLY64 mov     eax, ecx
        test    eax, 0ffff0000h
        jnz     .not_stack
        or      eax, BS3_SEL_R0_SS16 << 16
%endif
        jmp     .return

.not_stack:
%if TMPL_BITS == 16
        sub     ax, BS3_ADDR_BS3DATA16 & 0xffff
        sbb     dx, BS3_ADDR_BS3DATA16 >> 16
        jnz     .do_tiled
        mov     dx, BS3_SEL_R0_DS16
%else
        sub     eax, BS3_ADDR_BS3DATA16
        test    eax, 0ffff0000h
        jnz     .do_tiled
        or      eax, BS3_SEL_R0_DS16 << 16
%endif
        jmp     .return

        ;
        ; Just translate the address to tiled.
        ;
.do_tiled:
%if TMPL_BITS == 16
        ; Convert upper 16-bit to a tiled selector.
        mov     ax, cx                  ; save cx
        mov     dx, [xBP + xCB + cbCurRetAddr + 2]
 %ifdef BS3_STRICT
        cmp     dx, BS3_SEL_TILED_AREA_SIZE >> 16
        jb      .address_ok
        call    Bs3Panic
.address_ok:
 %endif
        mov     cl, X86_SEL_SHIFT
        shl     dx, cl
        add     dx, BS3_SEL_TILED
        mov     cx, ax                  ; restore cx

        ; Load segment offset and return.
        mov     ax, [xBP + xCB + cbCurRetAddr]

%else
        ; Convert upper 16-bit to tiled selector.
TNOT64  mov     eax, [xBP + xCB + cbCurRetAddr]
TONLY64 mov     rax, rcx
 %ifdef BS3_STRICT
        cmp     xAX, BS3_SEL_TILED_AREA_SIZE
        jb      .address_ok
        call    Bs3Panic
.address_ok:
 %endif
        ror     eax, 16
        shl     ax, X86_SEL_SHIFT
        add     ax, BS3_SEL_TILED
        rol     eax, 16
%endif

.return:
        pop     xBP
        BS3_HYBRID_RET
BS3_PROC_END_CMN   Bs3SelFlatDataToProtFar16


