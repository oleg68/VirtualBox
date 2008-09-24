/** @file
 * EM - Execution Monitor.
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

#ifndef ___VBox_em_h
#define ___VBox_em_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/trpm.h>
#include <VBox/dis.h>

__BEGIN_DECLS

/** @defgroup grp_em        The Execution Monitor API
 * @{
 */

/** Enable to allow V86 code to run in raw mode. */
#define VBOX_RAW_V86

/**
 * The Execution Manager State.
 */
typedef enum EMSTATE
{
    /** Not yet started. */
    EMSTATE_NONE = 1,
    /** Raw-mode execution. */
    EMSTATE_RAW,
    /** Hardware accelerated raw-mode execution. */
    EMSTATE_HWACC,
    /** Recompiled mode execution. */
    EMSTATE_REM,
    /** Execution is halted. (waiting for interrupt) */
    EMSTATE_HALTED,
    /** Execution is suspended. */
    EMSTATE_SUSPENDED,
    /** The VM is terminating. */
    EMSTATE_TERMINATING,
    /** Guest debug event from raw-mode is being processed. */
    EMSTATE_DEBUG_GUEST_RAW,
    /** Guest debug event from hardware accelerated mode is being processed. */
    EMSTATE_DEBUG_GUEST_HWACC,
    /** Guest debug event from recompiled-mode is being processed. */
    EMSTATE_DEBUG_GUEST_REM,
    /** Hypervisor debug event being processed. */
    EMSTATE_DEBUG_HYPER,
    /** The VM has encountered a fatal error. (And everyone is panicing....) */
    EMSTATE_GURU_MEDITATION,
    /** Just a hack to ensure that we get a 32-bit integer. */
    EMSTATE_MAKE_32BIT_HACK = 0x7fffffff
} EMSTATE;

EMDECL(EMSTATE) EMGetState(PVM pVM);

/** @name Callback handlers for instruction emulation functions.
 * These are placed here because IOM wants to use them as well.
 * @{
 */
typedef DECLCALLBACK(uint32_t)  FNEMULATEPARAM2UINT32(void *pvParam1, uint64_t val2);
typedef FNEMULATEPARAM2UINT32  *PFNEMULATEPARAM2UINT32;
typedef DECLCALLBACK(uint32_t)  FNEMULATEPARAM2(void *pvParam1, size_t val2);
typedef FNEMULATEPARAM2        *PFNEMULATEPARAM2;
typedef DECLCALLBACK(uint32_t)  FNEMULATEPARAM3(void *pvParam1, uint64_t val2, size_t val3);
typedef FNEMULATEPARAM3        *PFNEMULATEPARAM3;
typedef DECLCALLBACK(int)       FNEMULATELOCKPARAM2(void *pvParam1, uint64_t val2, RTGCUINTREG32 *pf);
typedef FNEMULATELOCKPARAM2    *PFNEMULATELOCKPARAM2;
typedef DECLCALLBACK(int)       FNEMULATELOCKPARAM3(void *pvParam1, uint64_t val2, size_t cb, RTGCUINTREG32 *pf);
typedef FNEMULATELOCKPARAM3    *PFNEMULATELOCKPARAM3;
/** @}  */


/**
 * Checks if raw ring-3 execute mode is enabled.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The VM to operate on.
 */
#define EMIsRawRing3Enabled(pVM) ((pVM)->fRawR3Enabled)

/**
 * Checks if raw ring-0 execute mode is enabled.
 *
 * @returns true if enabled.
 * @returns false if disabled.
 * @param   pVM         The VM to operate on.
 */
#define EMIsRawRing0Enabled(pVM) ((pVM)->fRawR0Enabled)

EMDECL(void)        EMSetInhibitInterruptsPC(PVM pVM, RTGCUINTPTR PC);
EMDECL(RTGCUINTPTR) EMGetInhibitInterruptsPC(PVM pVM);
EMDECL(int)         EMInterpretDisasOne(PVM pVM, PCCPUMCTXCORE pCtxCore, PDISCPUSTATE pCpu, unsigned *pcbInstr);
EMDECL(int)         EMInterpretDisasOneEx(PVM pVM, RTGCUINTPTR GCPtrInstr, PCCPUMCTXCORE pCtxCore,
                                          PDISCPUSTATE pCpu, unsigned *pcbInstr);
EMDECL(int)         EMInterpretInstruction(PVM pVM, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize);
EMDECL(int)         EMInterpretInstructionCPU(PVM pVM, PDISCPUSTATE pCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, uint32_t *pcbSize);
EMDECL(int)         EMInterpretCpuId(PVM pVM, PCPUMCTXCORE pRegFrame);
EMDECL(int)         EMInterpretRdtsc(PVM pVM, PCPUMCTXCORE pRegFrame);
EMDECL(int)         EMInterpretInvlpg(PVM pVM, PCPUMCTXCORE pRegFrame, RTGCPTR pAddrGC);
EMDECL(int)         EMInterpretIret(PVM pVM, PCPUMCTXCORE pRegFrame);
EMDECL(int)         EMInterpretDRxWrite(PVM pVM, PCPUMCTXCORE pRegFrame, uint32_t DestRegDrx, uint32_t SrcRegGen);
EMDECL(int)         EMInterpretDRxRead(PVM pVM, PCPUMCTXCORE pRegFrame, uint32_t DestRegGen, uint32_t SrcRegDrx);
EMDECL(int)         EMInterpretCRxWrite(PVM pVM, PCPUMCTXCORE pRegFrame, uint32_t DestRegCrx, uint32_t SrcRegGen);
EMDECL(int)         EMInterpretCRxRead(PVM pVM, PCPUMCTXCORE pRegFrame, uint32_t DestRegGen, uint32_t SrcRegCrx);
EMDECL(int)         EMInterpretLMSW(PVM pVM, uint16_t u16Data);
EMDECL(int)         EMInterpretCLTS(PVM pVM);
EMDECL(int)         EMInterpretPortIO(PVM pVM, PCPUMCTXCORE pCtxCore, PDISCPUSTATE pCpu, uint32_t cbOp);
EMDECL(int)         EMInterpretRdmsr(PVM pVM, PCPUMCTXCORE pRegFrame);
EMDECL(int)         EMInterpretWrmsr(PVM pVM, PCPUMCTXCORE pRegFrame);
EMDECL(void)        EMFlushREMTBs(PVM pVM);

/** @name Assembly routines
 * @{ */
EMDECL(uint32_t)    EMEmulateCmp(uint32_t u32Param1, uint64_t u64Param2, size_t cb);
EMDECL(uint32_t)    EMEmulateAnd(void *pvParam1, uint64_t u64Param2, size_t cb);
EMDECL(uint32_t)    EMEmulateInc(void *pvParam1, size_t cb);
EMDECL(uint32_t)    EMEmulateDec(void *pvParam1, size_t cb);
EMDECL(uint32_t)    EMEmulateOr(void *pvParam1, uint64_t u64Param2, size_t cb);
EMDECL(int)         EMEmulateLockOr(void *pvParam1, uint64_t u64Param2, size_t cbSize, RTGCUINTREG32 *pf);
EMDECL(uint32_t)    EMEmulateXor(void *pvParam1, uint64_t u64Param2, size_t cb);
EMDECL(uint32_t)    EMEmulateAdd(void *pvParam1, uint64_t u64Param2, size_t cb);
EMDECL(uint32_t)    EMEmulateSub(void *pvParam1, uint64_t u64Param2, size_t cb);
EMDECL(uint32_t)    EMEmulateAdcWithCarrySet(void *pvParam1, uint64_t u64Param2, size_t cb);
EMDECL(uint32_t)    EMEmulateBtr(void *pvParam1, uint64_t u64Param2);
EMDECL(int)         EMEmulateLockBtr(void *pvParam1, uint64_t u64Param2, RTGCUINTREG32 *pf);
EMDECL(uint32_t)    EMEmulateBts(void *pvParam1, uint64_t u64Param2);
EMDECL(uint32_t)    EMEmulateBtc(void *pvParam1, uint64_t u64Param2);
EMDECL(uint32_t)    EMEmulateCmpXchg(void *pvParam1, uint64_t *pu32Param2, uint64_t u32Param3, size_t cbSize);
EMDECL(uint32_t)    EMEmulateLockCmpXchg(void *pvParam1, uint64_t *pu64Param2, uint64_t u64Param3, size_t cbSize);
EMDECL(uint32_t)    EMEmulateCmpXchg8b32(RTHCPTR pu32Param1, uint32_t *pEAX, uint32_t *pEDX, uint32_t uEBX, uint32_t uECX);
EMDECL(uint32_t)    EMEmulateLockCmpXchg8b(RTHCPTR pu32Param1, uint32_t *pEAX, uint32_t *pEDX, uint32_t uEBX, uint32_t uECX);
/** @} */

#ifdef IN_RING3
/** @defgroup grp_em_r3     The EM Host Context Ring-3 API
 * @ingroup grp_em
 * @{
 */
EMR3DECL(int)       EMR3Init(PVM pVM);
EMR3DECL(void)      EMR3Relocate(PVM pVM);
EMR3DECL(void)      EMR3Reset(PVM pVM);
EMR3DECL(int)       EMR3Term(PVM pVM);
EMR3DECL(DECLNORETURN(void)) EMR3FatalError(PVM pVM, int rc);
EMR3DECL(int)       EMR3ExecuteVM(PVM pVM);
EMR3DECL(int)       EMR3CheckRawForcedActions(PVM pVM);
EMR3DECL(int)       EMR3Interpret(PVM pVM);

/**
 * Command argument for EMR3RawSetMode().
 *
 * It's possible to extend this interface to change several
 * execution modes at once should the need arise.
 */
typedef enum EMRAWMODE
{
    /** No raw execution. */
    EMRAW_NONE = 0,
    /** Enable Only ring-3 raw execution. */
    EMRAW_RING3_ENABLE,
    /** Only ring-3 raw execution. */
    EMRAW_RING3_DISABLE,
    /** Enable raw ring-0 execution. */
    EMRAW_RING0_ENABLE,
    /** Disable raw ring-0 execution. */
    EMRAW_RING0_DISABLE,
    EMRAW_END
} EMRAWMODE;

EMR3DECL(int)       EMR3RawSetMode(PVM pVM, EMRAWMODE enmMode);
/** @} */
#endif /* IN_RING3 */


#ifdef IN_GC
/** @defgroup grp_em_gc     The EM Guest Context API
 * @ingroup grp_em
 * @{
 */
EMGCDECL(int)       EMGCTrap(PVM pVM, unsigned uTrap, PCPUMCTXCORE pRegFrame);
EMGCDECL(uint32_t)  EMGCEmulateLockCmpXchg(RTRCPTR pu32Param1, uint32_t *pu32Param2, uint32_t u32Param3, size_t cbSize, uint32_t *pEflags);
EMGCDECL(uint32_t)  EMGCEmulateCmpXchg(RTRCPTR pu32Param1, uint32_t *pu32Param2, uint32_t u32Param3, size_t cbSize, uint32_t *pEflags);
EMGCDECL(uint32_t)  EMGCEmulateLockCmpXchg8b(RTRCPTR pu32Param1, uint32_t *pEAX, uint32_t *pEDX, uint32_t uEBX, uint32_t uECX, uint32_t *pEflags);
EMGCDECL(uint32_t)  EMGCEmulateCmpXchg8b(RTRCPTR pu32Param1, uint32_t *pEAX, uint32_t *pEDX, uint32_t uEBX, uint32_t uECX, uint32_t *pEflags);
EMGCDECL(uint32_t)  EMGCEmulateLockXAdd(RTRCPTR pu32Param1, uint32_t *pu32Param2, size_t cbSize, uint32_t *pEflags);
EMGCDECL(uint32_t)  EMGCEmulateXAdd(RTRCPTR pu32Param1, uint32_t *pu32Param2, size_t cbSize, uint32_t *pEflags);
/** @} */
#endif /* IN_GC */

/** @} */

__END_DECLS

#endif

