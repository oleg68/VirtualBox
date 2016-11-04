/* $Id$ */
/** @file
 * DBGF - Debugger Facility, Control Flow Graph Interface (CFG).
 */

/*
 * Copyright (C) 2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/** @page pg_dbgf_cfg    DBGFR3Cfg - Control Flow Graph Interface
 *
 * The control flow graph interface provides an API to disassemble
 * guest code providing the result in a control flow graph.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGF
#include <VBox/vmm/dbgf.h>
#include "DBGFInternal.h"
#include <VBox/vmm/mm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/param.h>
#include <iprt/list.h>
#include <iprt/mem.h>
#include <iprt/sort.h>
#include <iprt/strcache.h>

/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Internal control flow graph state.
 */
typedef struct DBGFCFGINT
{
    /** Reference counter. */
    uint32_t volatile       cRefs;
    /** Internal reference counter for basic blocks. */
    uint32_t volatile       cRefsBb;
    /** List of all basic blocks. */
    RTLISTANCHOR            LstCfgBb;
    /** Number of basic blocks in this control flow graph. */
    uint32_t                cBbs;
    /** The lowest addres of a basic block. */
    DBGFADDRESS             AddrLowest;
    /** The highest address of a basic block. */
    DBGFADDRESS             AddrHighest;
    /** String cache for disassembled instructions. */
    RTSTRCACHE              hStrCacheInstr;
} DBGFCFGINT;
/** Pointer to an internal control flow graph state. */
typedef DBGFCFGINT *PDBGFCFGINT;

/**
 * Instruction record
 */
typedef struct DBGFCFGBBINSTR
{
    /** Instruction address. */
    DBGFADDRESS             AddrInstr;
    /** Size of instruction. */
    uint32_t                cbInstr;
    /** Disassembled instruction string. */
    const char              *pszInstr;
} DBGFCFGBBINSTR;
/** Pointer to an instruction record. */
typedef DBGFCFGBBINSTR *PDBGFCFGBBINSTR;

/**
 * Internal control flow graph basic block state.
 */
typedef struct DBGFCFGBBINT
{
    /** Node for the list of all basic blocks. */
    RTLISTNODE              NdCfgBb;
    /** The control flow graph the basic block belongs to. */
    PDBGFCFGINT             pCfg;
    /** Reference counter. */
    uint32_t volatile       cRefs;
    /** Basic block end type. */
    DBGFCFGBBENDTYPE        enmEndType;
    /** Start address of this basic block. */
    DBGFADDRESS             AddrStart;
    /** End address of this basic block. */
    DBGFADDRESS             AddrEnd;
    /** Address of the block succeeding.
     *  This is valid for conditional jumps
     * (the other target is referenced by AddrEnd+1) and
     * unconditional jumps (not ret, iret, etc.) except
     * if we can't infer the jump target (jmp *eax for example). */
    DBGFADDRESS             AddrTarget;
    /** Last status error code if DBGF_CFG_BB_F_INCOMPLETE_ERR is set. */
    int                     rcError;
    /** Error message if DBGF_CFG_BB_F_INCOMPLETE_ERR is set. */
    char                   *pszErr;
    /** Flags for this basic block. */
    uint32_t                fFlags;
    /** Number of instructions in this basic block. */
    uint32_t                cInstr;
    /** Maximum number of instruction records for this basic block. */
    uint32_t                cInstrMax;
    /** Instruction records, variable in size. */
    DBGFCFGBBINSTR          aInstr[1];
} DBGFCFGBBINT;
/** Pointer to an internal control flow graph basic block state. */
typedef DBGFCFGBBINT *PDBGFCFGBBINT;

/**
 * Control flow graph iterator state.
 */
typedef struct DBGFCFGITINT
{
    /** Pointer to the control flow graph (holding a reference). */
    PDBGFCFGINT             pCfg;
    /** Next basic block to return. */
    uint32_t                idxBbNext;
    /** Array of basic blocks sorted by the specified order - variable in size. */
    PDBGFCFGBBINT           apBb[1];
} DBGFCFGITINT;
/** Pointer to the internal control flow graph iterator state. */
typedef DBGFCFGITINT *PDBGFCFGITINT;

/**
 * Dumper state for a basic block.
 */
typedef struct DBGFCFGDUMPBB
{
    /** The basic block referenced. */
    PDBGFCFGBBINT           pCfgBb;
    /** Width of the basic block in chars. */
    uint32_t                cchWidth;
    /** Height of the basic block in chars. */
    uint32_t                cchHeight;
    /** X coordinate of the start. */
    uint32_t                uStartX;
    /** Y coordinate of the start. */
    uint32_t                uStartY;
} DBGFCFGDUMPBB;
/** Pointer to a basic block dumper state. */
typedef DBGFCFGDUMPBB *PDBGFCFGDUMPBB;

/**
 * Dumper ASCII screen.
 */
typedef struct DBGFCFGDUMPSCREEN
{
    /** Width of the screen. */
    uint32_t                cchWidth;
    /** Height of the screen. */
    uint32_t                cchHeight;
    /** Extra amount of characters at the end of each line (usually temrinator). */
    uint32_t                cchStride;
    /** Pointer to the char buffer. */
    char                   *pszScreen;
} DBGFCFGDUMPSCREEN;
/** Pointer to a dumper ASCII screen. */
typedef DBGFCFGDUMPSCREEN *PDBGFCFGDUMPSCREEN;

/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

static uint32_t dbgfR3CfgBbReleaseInt(PDBGFCFGBBINT pCfgBb, bool fMayDestroyCfg);

/**
 * Creates a new basic block.
 *
 * @returns Pointer to the basic block on success or NULL if out of memory.
 * @param   pThis               The control flow graph.
 * @param   pAddrStart          The start of the basic block.
 * @param   cInstrMax           Maximum number of instructions this block can hold initially.
 */
static PDBGFCFGBBINT dbgfR3CfgBbCreate(PDBGFCFGINT pThis, PDBGFADDRESS pAddrStart, uint32_t cInstrMax)
{
    PDBGFCFGBBINT pCfgBb = (PDBGFCFGBBINT)RTMemAllocZ(RT_OFFSETOF(DBGFCFGBBINT, aInstr[cInstrMax]));
    if (RT_LIKELY(pCfgBb))
    {
        RTListInit(&pCfgBb->NdCfgBb);
        pCfgBb->cRefs      = 1;
        pCfgBb->enmEndType = DBGFCFGBBENDTYPE_INVALID;
        pCfgBb->pCfg       = pThis;
        pCfgBb->fFlags     = DBGF_CFG_BB_F_EMPTY;
        pCfgBb->AddrStart  = *pAddrStart;
        pCfgBb->AddrEnd    = *pAddrStart;
        pCfgBb->rcError    = VINF_SUCCESS;
        pCfgBb->pszErr     = NULL;
        pCfgBb->cInstr     = 0;
        pCfgBb->cInstrMax  = cInstrMax;
        ASMAtomicIncU32(&pThis->cRefsBb);
    }

    return pCfgBb;
}


/**
 * Destroys a control flow graph.
 *
 * @returns nothing.
 * @param   pThis               The control flow graph to destroy.
 */
static void dbgfR3CfgDestroy(PDBGFCFGINT pThis)
{
    /* Defer destruction if there are still basic blocks referencing us. */
    PDBGFCFGBBINT pCfgBb = NULL;
    PDBGFCFGBBINT pCfgBbNext = NULL;
    RTListForEachSafe(&pThis->LstCfgBb, pCfgBb, pCfgBbNext, DBGFCFGBBINT, NdCfgBb)
    {
        dbgfR3CfgBbReleaseInt(pCfgBb, false /*fMayDestroyCfg*/);
    }

    Assert(!pThis->cRefs);
    if (!pThis->cRefsBb)
    {
        RTStrCacheDestroy(pThis->hStrCacheInstr);
        RTMemFree(pThis);
    }
}


/**
 * Destroys a basic block.
 *
 * @returns nothing.
 * @param   pCfgBb              The basic block to destroy.
 * @param   fMayDestroyCfg      Flag whether the control flow graph container
 *                              should be destroyed when there is nothing referencing it.
 */
static void dbgfR3CfgBbDestroy(PDBGFCFGBBINT pCfgBb, bool fMayDestroyCfg)
{
    PDBGFCFGINT pThis = pCfgBb->pCfg;

    RTListNodeRemove(&pCfgBb->NdCfgBb);
    pThis->cBbs--;
    for (uint32_t idxInstr = 0; idxInstr < pCfgBb->cInstr; idxInstr++)
        RTStrCacheRelease(pThis->hStrCacheInstr, pCfgBb->aInstr[idxInstr].pszInstr);
    uint32_t cRefsBb = ASMAtomicDecU32(&pThis->cRefsBb);
    RTMemFree(pCfgBb);

    if (!cRefsBb && !pThis->cRefs && fMayDestroyCfg)
        dbgfR3CfgDestroy(pThis);
}


/**
 * Internal basic block release worker.
 *
 * @returns New reference count of the released basic block, on 0
 *          it is destroyed.
 * @param   pCfgBb              The basic block to release.
 * @param   fMayDestroyCfg      Flag whether the control flow graph container
 *                              should be destroyed when there is nothing referencing it.
 */
static uint32_t dbgfR3CfgBbReleaseInt(PDBGFCFGBBINT pCfgBb, bool fMayDestroyCfg)
{
    uint32_t cRefs = ASMAtomicDecU32(&pCfgBb->cRefs);
    AssertMsg(cRefs < _1M, ("%#x %p %d\n", cRefs, pCfgBb, pCfgBb->enmEndType));
    if (cRefs == 0)
        dbgfR3CfgBbDestroy(pCfgBb, fMayDestroyCfg);
    return cRefs;
}


/**
 * Links the given basic block into the control flow graph.
 *
 * @returns nothing.
 * @param   pThis               The control flow graph to link into.
 * @param   pCfgBb              The basic block to link.
 */
DECLINLINE(void) dbgfR3CfgLink(PDBGFCFGINT pThis, PDBGFCFGBBINT pCfgBb)
{
    RTListAppend(&pThis->LstCfgBb, &pCfgBb->NdCfgBb);
    pThis->cBbs++;
}


/**
 * Returns the first unpopulated basic block of the given control flow graph.
 *
 * @returns The first unpopulated control flow graph or NULL if not found.
 * @param   pThis               The control flow graph.
 */
DECLINLINE(PDBGFCFGBBINT) dbgfR3CfgGetUnpopulatedBb(PDBGFCFGINT pThis)
{
    PDBGFCFGBBINT pCfgBb = NULL;
    RTListForEach(&pThis->LstCfgBb, pCfgBb, DBGFCFGBBINT, NdCfgBb)
    {
        if (pCfgBb->fFlags & DBGF_CFG_BB_F_EMPTY)
            return pCfgBb;
    }

    return NULL;
}


/**
 * Resolves the jump target address if possible from the given instruction address
 * and instruction parameter.
 *
 * @returns VBox status code.
 * @param   pUVM                The usermode VM handle.
 * @param   idCpu               CPU id for resolving the address.
 * @param   pDisParam           The parmeter from the disassembler.
 * @param   pAddrInstr          The instruction address.
 * @param   cbInstr             Size of instruction in bytes.
 * @param   fRelJmp             Flag whether this is a reltive jump.
 * @param   pAddrJmpTarget      Where to store the address to the jump target on success.
 */
static int dbgfR3CfgQueryJmpTarget(PUVM pUVM, VMCPUID idCpu, PDISOPPARAM pDisParam, PDBGFADDRESS pAddrInstr,
                                   uint32_t cbInstr, bool fRelJmp, PDBGFADDRESS pAddrJmpTarget)
{
    int rc = VINF_SUCCESS;

    /* Relative jumps are always from the beginning of the next instruction. */
    *pAddrJmpTarget = *pAddrInstr;
    DBGFR3AddrAdd(pAddrJmpTarget, cbInstr);

    if (fRelJmp)
    {
        RTGCINTPTR iRel = 0;
        if (pDisParam->fUse & DISUSE_IMMEDIATE8_REL)
            iRel = (int8_t)pDisParam->uValue;
        else if (pDisParam->fUse & DISUSE_IMMEDIATE16_REL)
            iRel = (int16_t)pDisParam->uValue;
        else if (pDisParam->fUse & DISUSE_IMMEDIATE32_REL)
            iRel = (int32_t)pDisParam->uValue;
        else if (pDisParam->fUse & DISUSE_IMMEDIATE64_REL)
            iRel = (int64_t)pDisParam->uValue;
        else
            AssertFailedStmt(rc = VERR_NOT_SUPPORTED);

        if (iRel < 0)
            DBGFR3AddrSub(pAddrJmpTarget, -iRel);
        else
            DBGFR3AddrAdd(pAddrJmpTarget, iRel);
    }
    else
    {
        if (pDisParam->fUse & (DISUSE_IMMEDIATE8 | DISUSE_IMMEDIATE16 | DISUSE_IMMEDIATE32 | DISUSE_IMMEDIATE64))
        {
            if (DBGFADDRESS_IS_FLAT(pAddrInstr))
                DBGFR3AddrFromFlat(pUVM, pAddrJmpTarget, pDisParam->uValue);
            else
                DBGFR3AddrFromSelOff(pUVM, idCpu, pAddrJmpTarget, pAddrInstr->Sel, pDisParam->uValue);
        }
        else
            AssertFailedStmt(rc = VERR_NOT_SUPPORTED);
    }

    return rc;
}


/**
 * Checks whether both addresses are equal.
 *
 * @returns true if both addresses point to the same location, false otherwise.
 * @param   pAddr1              First address.
 * @param   pAddr2              Second address.
 */
static bool dbgfR3CfgBbAddrEqual(PDBGFADDRESS pAddr1, PDBGFADDRESS pAddr2)
{
    return    pAddr1->Sel == pAddr2->Sel
           && pAddr1->off == pAddr2->off;
}


/**
 * Checks whether the first given address is lower than the second one.
 *
 * @returns true if both addresses point to the same location, false otherwise.
 * @param   pAddr1              First address.
 * @param   pAddr2              Second address.
 */
static bool dbgfR3CfgBbAddrLower(PDBGFADDRESS pAddr1, PDBGFADDRESS pAddr2)
{
    return    pAddr1->Sel == pAddr2->Sel
           && pAddr1->off < pAddr2->off;
}


/**
 * Checks whether the given basic block and address intersect.
 *
 * @returns true if they intersect, false otherwise.
 * @param   pCfgBb              The basic block to check.
 * @param   pAddr               The address to check for.
 */
static bool dbgfR3CfgBbAddrIntersect(PDBGFCFGBBINT pCfgBb, PDBGFADDRESS pAddr)
{
    return    (pCfgBb->AddrStart.Sel == pAddr->Sel)
           && (pCfgBb->AddrStart.off <= pAddr->off)
           && (pCfgBb->AddrEnd.off >= pAddr->off);
}


/**
 * Checks whether the given control flow graph contains a basic block
 * with the given start address.
 *
 * @returns true if there is a basic block with the start address, false otherwise.
 * @param   pThis               The control flow graph.
 * @param   pAddr               The address to check for.
 */
static bool dbgfR3CfgHasBbWithStartAddr(PDBGFCFGINT pThis, PDBGFADDRESS pAddr)
{
    PDBGFCFGBBINT pCfgBb = NULL;
    RTListForEach(&pThis->LstCfgBb, pCfgBb, DBGFCFGBBINT, NdCfgBb)
    {
        if (dbgfR3CfgBbAddrEqual(&pCfgBb->AddrStart, pAddr))
            return true;
    }
    return false;
}

/**
 * Splits a given basic block into two at the given address.
 *
 * @returns VBox status code.
 * @param   pThis               The control flow graph.
 * @param   pCfgBb              The basic block to split.
 * @param   pAddr               The address to split at.
 */
static int dbgfR3CfgBbSplit(PDBGFCFGINT pThis, PDBGFCFGBBINT pCfgBb, PDBGFADDRESS pAddr)
{
    int rc = VINF_SUCCESS;
    uint32_t idxInstrSplit;

    /* If the block is empty it will get populated later so there is nothing to split,
     * same if the start address equals. */
    if (   pCfgBb->fFlags & DBGF_CFG_BB_F_EMPTY
        || dbgfR3CfgBbAddrEqual(&pCfgBb->AddrStart, pAddr))
        return VINF_SUCCESS;

    /* Find the instruction to split at. */
    for (idxInstrSplit = 1; idxInstrSplit < pCfgBb->cInstr; idxInstrSplit++)
        if (dbgfR3CfgBbAddrEqual(&pCfgBb->aInstr[idxInstrSplit].AddrInstr, pAddr))
            break;

    Assert(idxInstrSplit > 0);

    /*
     * Given address might not be on instruction boundary, this is not supported
     * so far and results in an error.
     */
    if (idxInstrSplit < pCfgBb->cInstr)
    {
        /* Create new basic block. */
        uint32_t cInstrNew = pCfgBb->cInstr - idxInstrSplit;
        PDBGFCFGBBINT pCfgBbNew = dbgfR3CfgBbCreate(pThis, &pCfgBb->aInstr[idxInstrSplit].AddrInstr,
                                                    cInstrNew);
        if (pCfgBbNew)
        {
            /* Move instructions over. */
            pCfgBbNew->cInstr     = cInstrNew;
            pCfgBbNew->AddrEnd    = pCfgBb->AddrEnd;
            pCfgBbNew->enmEndType = pCfgBb->enmEndType;
            pCfgBbNew->fFlags     = pCfgBb->fFlags & ~DBGF_CFG_BB_F_ENTRY;

            /* Move any error to the new basic block and clear them in the old basic block. */
            pCfgBbNew->rcError    = pCfgBb->rcError;
            pCfgBbNew->pszErr     = pCfgBb->pszErr;
            pCfgBb->rcError       = VINF_SUCCESS;
            pCfgBb->pszErr        = NULL;
            pCfgBb->fFlags       &= ~DBGF_CFG_BB_F_INCOMPLETE_ERR;

            memcpy(&pCfgBbNew->aInstr[0], &pCfgBb->aInstr[idxInstrSplit], cInstrNew * sizeof(DBGFCFGBBINSTR));
            pCfgBb->cInstr     = idxInstrSplit;
            pCfgBb->enmEndType = DBGFCFGBBENDTYPE_UNCOND;
            pCfgBb->AddrEnd    = pCfgBb->aInstr[idxInstrSplit-1].AddrInstr;
            pCfgBb->AddrTarget = pCfgBbNew->AddrStart;
            DBGFR3AddrAdd(&pCfgBb->AddrEnd, pCfgBb->aInstr[idxInstrSplit-1].cbInstr - 1);
            RT_BZERO(&pCfgBb->aInstr[idxInstrSplit], cInstrNew * sizeof(DBGFCFGBBINSTR));

            dbgfR3CfgLink(pThis, pCfgBbNew);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        AssertFailedStmt(rc = VERR_INVALID_STATE); /** @todo: Proper status code. */

    return rc;
}


/**
 * Makes sure there is an successor at the given address splitting already existing
 * basic blocks if they intersect.
 *
 * @returns VBox status code.
 * @param   pThis               The control flow graph.
 * @param   pAddrSucc           The guest address the new successor should start at.
 */
static int dbgfR3CfgBbSuccessorAdd(PDBGFCFGINT pThis, PDBGFADDRESS pAddrSucc)
{
    PDBGFCFGBBINT pCfgBb = NULL;
    RTListForEach(&pThis->LstCfgBb, pCfgBb, DBGFCFGBBINT, NdCfgBb)
    {
        /*
         * The basic block must be split if it intersects with the given address
         * and the start address does not equal the given one.
         */
        if (dbgfR3CfgBbAddrIntersect(pCfgBb, pAddrSucc))
            return dbgfR3CfgBbSplit(pThis, pCfgBb, pAddrSucc);
    }

    int rc = VINF_SUCCESS;
    pCfgBb = dbgfR3CfgBbCreate(pThis, pAddrSucc, 10);
    if (pCfgBb)
        dbgfR3CfgLink(pThis, pCfgBb);
    else
        rc = VERR_NO_MEMORY;

    return rc;
}


/**
 * Sets the given error status for the basic block.
 *
 * @returns nothing.
 * @param   pCfgBb              The basic block causing the error.
 * @param   rcError             The error to set.
 * @param   pszFmt              Format string of the error description.
 * @param   ...                 Arguments for the format string.
 */
static void dbgfR3CfgBbSetError(PDBGFCFGBBINT pCfgBb, int rcError, const char *pszFmt, ...)
{
    va_list va;
    va_start(va, pszFmt);

    Assert(!(pCfgBb->fFlags & DBGF_CFG_BB_F_INCOMPLETE_ERR));
    pCfgBb->fFlags |= DBGF_CFG_BB_F_INCOMPLETE_ERR;
    pCfgBb->fFlags &= ~DBGF_CFG_BB_F_EMPTY;
    pCfgBb->rcError = rcError;
    pCfgBb->pszErr = RTStrAPrintf2V(pszFmt, va);
    va_end(va);
}


/**
 * Processes and fills one basic block.
 *
 * @returns VBox status code.
 * @param   pUVM                The user mode VM handle.
 * @param   idCpu               CPU id for disassembling.
 * @param   pThis               The control flow graph to populate.
 * @param   pCfgBb              The basic block to fill.
 * @param   cbDisasmMax         The maximum amount to disassemble.
 * @param   fFlags              Combination of DBGF_DISAS_FLAGS_*.
 */
static int dbgfR3CfgBbProcess(PUVM pUVM, VMCPUID idCpu, PDBGFCFGINT pThis, PDBGFCFGBBINT pCfgBb,
                              uint32_t cbDisasmMax, uint32_t fFlags)
{
    int rc = VINF_SUCCESS;
    uint32_t cbDisasmLeft = cbDisasmMax ? cbDisasmMax : UINT32_MAX;
    DBGFADDRESS AddrDisasm = pCfgBb->AddrEnd;

    Assert(pCfgBb->fFlags & DBGF_CFG_BB_F_EMPTY);

    /*
     * Disassemble instruction by instruction until we get a conditional or
     * unconditional jump or some sort of return.
     */
    while (   cbDisasmLeft
           && RT_SUCCESS(rc))
    {
        DBGFDISSTATE DisState;
        char szOutput[_4K];

        /*
         * Before disassembling we have to check whether the address belongs
         * to another basic block and stop here.
         */
        if (   !(pCfgBb->fFlags & DBGF_CFG_BB_F_EMPTY)
            && dbgfR3CfgHasBbWithStartAddr(pThis, &AddrDisasm))
        {
            pCfgBb->AddrTarget = AddrDisasm;
            pCfgBb->enmEndType = DBGFCFGBBENDTYPE_UNCOND;
            break;
        }

        pCfgBb->fFlags &= ~DBGF_CFG_BB_F_EMPTY;

        rc = dbgfR3DisasInstrStateEx(pUVM, idCpu, &AddrDisasm, fFlags,
                                     &szOutput[0], sizeof(szOutput), &DisState);
        if (RT_SUCCESS(rc))
        {
            cbDisasmLeft -= DisState.cbInstr;

            if (pCfgBb->cInstr == pCfgBb->cInstrMax)
            {
                /* Reallocate. */
                RTListNodeRemove(&pCfgBb->NdCfgBb);
                PDBGFCFGBBINT pCfgBbNew = (PDBGFCFGBBINT)RTMemRealloc(pCfgBb, RT_OFFSETOF(DBGFCFGBBINT, aInstr[pCfgBb->cInstrMax + 10]));
                if (pCfgBbNew)
                {
                    pCfgBbNew->cInstrMax += 10;
                    pCfgBb = pCfgBbNew;
                }
                else
                    rc = VERR_NO_MEMORY;
                RTListAppend(&pThis->LstCfgBb, &pCfgBb->NdCfgBb);
            }

            if (RT_SUCCESS(rc))
            {
                PDBGFCFGBBINSTR pInstr = &pCfgBb->aInstr[pCfgBb->cInstr];

                pInstr->AddrInstr = AddrDisasm;
                pInstr->cbInstr   = DisState.cbInstr;
                pInstr->pszInstr  = RTStrCacheEnter(pThis->hStrCacheInstr, &szOutput[0]);
                pCfgBb->cInstr++;

                pCfgBb->AddrEnd = AddrDisasm;
                DBGFR3AddrAdd(&pCfgBb->AddrEnd, pInstr->cbInstr - 1);
                DBGFR3AddrAdd(&AddrDisasm, pInstr->cbInstr);

                /*
                 * Check control flow instructions and create new basic blocks
                 * marking the current one as complete.
                 */
                if (DisState.pCurInstr->fOpType & DISOPTYPE_CONTROLFLOW)
                {
                    uint16_t uOpc = DisState.pCurInstr->uOpcode;

                    if (   uOpc == OP_RETN || uOpc == OP_RETF || uOpc == OP_IRET
                        || uOpc == OP_SYSEXIT || uOpc == OP_SYSRET)
                        pCfgBb->enmEndType = DBGFCFGBBENDTYPE_EXIT;
                    else if (uOpc == OP_JMP)
                    {
                        Assert(DisState.pCurInstr->fOpType & DISOPTYPE_UNCOND_CONTROLFLOW);
                        pCfgBb->enmEndType = DBGFCFGBBENDTYPE_UNCOND_JMP;

                        /* Create one new basic block with the jump target address. */
                        rc = dbgfR3CfgQueryJmpTarget(pUVM, idCpu, &DisState.Param1, &pInstr->AddrInstr, pInstr->cbInstr,
                                                     RT_BOOL(DisState.pCurInstr->fOpType & DISOPTYPE_RELATIVE_CONTROLFLOW),
                                                     &pCfgBb->AddrTarget);
                        if (RT_SUCCESS(rc))
                            rc = dbgfR3CfgBbSuccessorAdd(pThis, &pCfgBb->AddrTarget);
                    }
                    else if (uOpc != OP_CALL)
                    {
                        Assert(DisState.pCurInstr->fOpType & DISOPTYPE_COND_CONTROLFLOW);
                        pCfgBb->enmEndType = DBGFCFGBBENDTYPE_COND;

                        /*
                         * Create two new basic blocks, one with the jump target address
                         * and one starting after the current instruction.
                         */
                        rc = dbgfR3CfgBbSuccessorAdd(pThis, &AddrDisasm);
                        if (RT_SUCCESS(rc))
                        {
                            rc = dbgfR3CfgQueryJmpTarget(pUVM, idCpu, &DisState.Param1, &pInstr->AddrInstr, pInstr->cbInstr, 
                                                         RT_BOOL(DisState.pCurInstr->fOpType & DISOPTYPE_RELATIVE_CONTROLFLOW),
                                                         &pCfgBb->AddrTarget);
                            if (RT_SUCCESS(rc))
                                rc = dbgfR3CfgBbSuccessorAdd(pThis, &pCfgBb->AddrTarget);
                        }
                    }

                    if (RT_FAILURE(rc))
                        dbgfR3CfgBbSetError(pCfgBb, rc, "Adding successor blocks failed with %Rrc", rc);

                    /* Quit disassembling. */
                    if (   uOpc != OP_CALL
                        || RT_FAILURE(rc))
                        break;
                }
            }
            else
                dbgfR3CfgBbSetError(pCfgBb, rc, "Increasing basic block failed with %Rrc", rc);
        }
        else
            dbgfR3CfgBbSetError(pCfgBb, rc, "Disassembling the instruction failed with %Rrc", rc);
    }

    return VINF_SUCCESS;
}

/**
 * Populate all empty basic blocks.
 *
 * @returns VBox status code.
 * @param   pUVM                The user mode VM handle.
 * @param   idCpu               CPU id for disassembling.
 * @param   pThis               The control flow graph to populate.
 * @param   pAddrStart          The start address to disassemble at.
 * @param   cbDisasmMax         The maximum amount to disassemble.
 * @param   fFlags              Combination of DBGF_DISAS_FLAGS_*.
 */
static int dbgfR3CfgPopulate(PUVM pUVM, VMCPUID idCpu, PDBGFCFGINT pThis, PDBGFADDRESS pAddrStart,
                             uint32_t cbDisasmMax, uint32_t fFlags)
{
    int rc = VINF_SUCCESS;
    PDBGFCFGBBINT pCfgBb = dbgfR3CfgGetUnpopulatedBb(pThis);
    DBGFADDRESS AddrEnd = *pAddrStart;
    DBGFR3AddrAdd(&AddrEnd, cbDisasmMax);

    while (VALID_PTR(pCfgBb))
    {
        rc = dbgfR3CfgBbProcess(pUVM, idCpu, pThis, pCfgBb, cbDisasmMax, fFlags);
        if (RT_FAILURE(rc))
            break;

        pCfgBb = dbgfR3CfgGetUnpopulatedBb(pThis);
    }

    return rc;
}

/**
 * Creates a new control flow graph from the given start address.
 *
 * @returns VBox status code.
 * @param   pUVM                The user mode VM handle.
 * @param   idCpu               CPU id for disassembling.
 * @param   pAddressStart       Where to start creating the control flow graph.
 * @param   cbDisasmMax         Limit the amount of bytes to disassemble, 0 for no limit.
 * @param   fFlags              Combination of DBGF_DISAS_FLAGS_*.
 * @param   phCfg               Where to store the handle to the control flow graph on success.
 */
VMMR3DECL(int) DBGFR3CfgCreate(PUVM pUVM, VMCPUID idCpu, PDBGFADDRESS pAddressStart, uint32_t cbDisasmMax,
                               uint32_t fFlags, PDBGFCFG phCfg)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(idCpu < pUVM->cCpus, VERR_INVALID_CPU_ID);
    AssertPtrReturn(pAddressStart, VERR_INVALID_POINTER);
    AssertReturn(!(fFlags & ~DBGF_DISAS_FLAGS_VALID_MASK), VERR_INVALID_PARAMETER);
    AssertReturn((fFlags & DBGF_DISAS_FLAGS_MODE_MASK) <= DBGF_DISAS_FLAGS_64BIT_MODE, VERR_INVALID_PARAMETER);

    /* Create the control flow graph container. */
    int rc = VINF_SUCCESS;
    PDBGFCFGINT pThis = (PDBGFCFGINT)RTMemAllocZ(sizeof(DBGFCFGINT));
    if (RT_LIKELY(pThis))
    {
        rc = RTStrCacheCreate(&pThis->hStrCacheInstr, "DBGFCFG");
        if (RT_SUCCESS(rc))
        {
            pThis->cRefs   = 1;
            pThis->cRefsBb = 0;
            pThis->cBbs    = 0;
            RTListInit(&pThis->LstCfgBb);
            /* Create the entry basic block and start the work. */

            PDBGFCFGBBINT pCfgBb = dbgfR3CfgBbCreate(pThis, pAddressStart, 10);
            if (RT_LIKELY(pCfgBb))
            {
                pCfgBb->fFlags |= DBGF_CFG_BB_F_ENTRY;
                dbgfR3CfgLink(pThis, pCfgBb);
                rc = dbgfR3CfgPopulate(pUVM, idCpu, pThis, pAddressStart, cbDisasmMax, fFlags);
                if (RT_SUCCESS(rc))
                {
                    *phCfg = pThis;
                    return VINF_SUCCESS;
                }
            }
            else
                rc = VERR_NO_MEMORY;
        }

        ASMAtomicDecU32(&pThis->cRefs);
        dbgfR3CfgDestroy(pThis);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}


/**
 * Retains the control flow graph handle.
 *
 * @returns Current reference count.
 * @param   hCfg                The control flow graph handle to retain.
 */
VMMR3DECL(uint32_t) DBGFR3CfgRetain(DBGFCFG hCfg)
{
    PDBGFCFGINT pThis = hCfg;
    AssertPtrReturn(pThis, UINT32_MAX);

    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1M, ("%#x %p\n", cRefs, pThis));
    return cRefs;
}


/**
 * Releases the control flow graph handle.
 *
 * @returns Current reference count, on 0 the control flow graph will be destroyed.
 * @param   hCfg                The control flow graph handle to release.
 */
VMMR3DECL(uint32_t) DBGFR3CfgRelease(DBGFCFG hCfg)
{
    PDBGFCFGINT pThis = hCfg;
    if (!pThis)
        return 0;
    AssertPtrReturn(pThis, UINT32_MAX);

    uint32_t cRefs = ASMAtomicDecU32(&pThis->cRefs);
    AssertMsg(cRefs < _1M, ("%#x %p\n", cRefs, pThis));
    if (cRefs == 0)
        dbgfR3CfgDestroy(pThis);
    return cRefs;
}


/**
 * Queries the basic block denoting the entry point into the control flow graph.
 *
 * @returns VBox status code.
 * @param   hCfg                The control flow graph handle.
 * @param   phCfgBb             Where to store the basic block handle on success.
 */
VMMR3DECL(int) DBGFR3CfgQueryStartBb(DBGFCFG hCfg, PDBGFCFGBB phCfgBb)
{
    PDBGFCFGINT pThis = hCfg;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);

    PDBGFCFGBBINT pCfgBb = NULL;
    RTListForEach(&pThis->LstCfgBb, pCfgBb, DBGFCFGBBINT, NdCfgBb)
    {
        if (pCfgBb->fFlags & DBGF_CFG_BB_F_ENTRY)
        {
            *phCfgBb = pCfgBb;
            return VINF_SUCCESS;
        }
    }

    AssertFailed(); /* Should never get here. */
    return VERR_INTERNAL_ERROR;
}


/**
 * Queries a basic block in the given control flow graph which covers the given
 * address.
 *
 * @returns VBox status code.
 * @retval  VERR_NOT_FOUND if there is no basic block intersecting with the address.
 * @param   hCfg                The control flow graph handle.
 * @param   pAddr               The address to look for.
 * @param   phCfgBb             Where to store the basic block handle on success.
 */
VMMR3DECL(int) DBGFR3CfgQueryBbByAddress(DBGFCFG hCfg, PDBGFADDRESS pAddr, PDBGFCFGBB phCfgBb)
{
    PDBGFCFGINT pThis = hCfg;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertPtrReturn(phCfgBb, VERR_INVALID_POINTER);

    PDBGFCFGBBINT pCfgBb = NULL;
    RTListForEach(&pThis->LstCfgBb, pCfgBb, DBGFCFGBBINT, NdCfgBb)
    {
        if (dbgfR3CfgBbAddrIntersect(pCfgBb, pAddr))
        {
            DBGFR3CfgBbRetain(pCfgBb);
            *phCfgBb = pCfgBb;
            return VINF_SUCCESS;
        }
    }

    return VERR_NOT_FOUND;
}


/**
 * Returns the number of basic blcoks inside the control flow graph.
 *
 * @returns Number of basic blocks.
 * @param   hCfg                The control flow graph handle.
 */
VMMR3DECL(uint32_t) DBGFR3CfgGetBbCount(DBGFCFG hCfg)
{
    PDBGFCFGINT pThis = hCfg;
    AssertPtrReturn(pThis, 0);

    return pThis->cBbs;
}


/**
 * Retains the basic block handle.
 *
 * @returns Current reference count.
 * @param   hCfgBb              The basic block handle to retain.
 */
VMMR3DECL(uint32_t) DBGFR3CfgBbRetain(DBGFCFGBB hCfgBb)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, UINT32_MAX);

    uint32_t cRefs = ASMAtomicIncU32(&pCfgBb->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1M, ("%#x %p %d\n", cRefs, pCfgBb, pCfgBb->enmEndType));
    return cRefs;
}


/**
 * Releases the basic block handle.
 *
 * @returns Current reference count, on 0 the basic block will be destroyed.
 * @param   hCfgBb              The basic block handle to release.
 */
VMMR3DECL(uint32_t) DBGFR3CfgBbRelease(DBGFCFGBB hCfgBb)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    if (!pCfgBb)
        return 0;

    return dbgfR3CfgBbReleaseInt(pCfgBb, true /* fMayDestroyCfg */);
}


/**
 * Returns the start address of the basic block.
 *
 * @returns Pointer to DBGF adress containing the start address of the basic block.
 * @param   hCfgBb              The basic block handle.
 * @param   pAddrStart          Where to store the start address of the basic block.
 */
VMMR3DECL(PDBGFADDRESS) DBGFR3CfgBbGetStartAddress(DBGFCFGBB hCfgBb, PDBGFADDRESS pAddrStart)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, NULL);
    AssertPtrReturn(pAddrStart, NULL);

    *pAddrStart = pCfgBb->AddrStart;
    return pAddrStart;
}


/**
 * Returns the end address of the basic block (inclusive).
 *
 * @returns Pointer to DBGF adress containing the end address of the basic block.
 * @param   hCfgBb              The basic block handle.
 * @param   pAddrEnd            Where to store the end address of the basic block.
 */
VMMR3DECL(PDBGFADDRESS) DBGFR3CfgBbGetEndAddress(DBGFCFGBB hCfgBb, PDBGFADDRESS pAddrEnd)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, NULL);
    AssertPtrReturn(pAddrEnd, NULL);

    *pAddrEnd = pCfgBb->AddrEnd;
    return pAddrEnd;
}


/**
 * Returns the address the last instruction in the basic block branches to.
 *
 * @returns Pointer to DBGF adress containing the branch address of the basic block.
 * @param   hCfgBb              The basic block handle.
 * @param   pAddrTarget         Where to store the branch address of the basic block.
 *
 * @note This is only valid for unconditional or conditional branches and will assert
 *       for every other basic block type.
 */
VMMR3DECL(PDBGFADDRESS) DBGFR3CfgBbGetBranchAddress(DBGFCFGBB hCfgBb, PDBGFADDRESS pAddrTarget)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, NULL);
    AssertPtrReturn(pAddrTarget, NULL);
    AssertReturn(   pCfgBb->enmEndType == DBGFCFGBBENDTYPE_UNCOND_JMP
                 || pCfgBb->enmEndType == DBGFCFGBBENDTYPE_COND,
                 NULL);

    *pAddrTarget = pCfgBb->AddrTarget;
    return pAddrTarget;
}


/**
 * Returns the address of the next block following this one in the instruction stream.
 * (usually end address + 1).
 *
 * @returns Pointer to DBGF adress containing the following address of the basic block.
 * @param   hCfgBb              The basic block handle.
 * @param   pAddrFollow         Where to store the following address of the basic block.
 *
 * @note This is only valid for conditional branches and if the last instruction in the
 *       given basic block doesn't change the control flow but the blocks were split
 *       because the successor is referenced by multiple other blocks as an entry point.
 */
VMMR3DECL(PDBGFADDRESS) DBGFR3CfgBbGetFollowingAddress(DBGFCFGBB hCfgBb, PDBGFADDRESS pAddrFollow)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, NULL);
    AssertPtrReturn(pAddrFollow, NULL);
    AssertReturn(   pCfgBb->enmEndType == DBGFCFGBBENDTYPE_UNCOND
                 || pCfgBb->enmEndType == DBGFCFGBBENDTYPE_COND,
                 NULL);

    *pAddrFollow = pCfgBb->AddrEnd;
    DBGFR3AddrAdd(pAddrFollow, 1);
    return pAddrFollow;
}


/**
 * Returns the type of the last instruction in the basic block.
 *
 * @returns Last instruction type.
 * @param   hCfgBb              The basic block handle.
 */
VMMR3DECL(DBGFCFGBBENDTYPE) DBGFR3CfgBbGetType(DBGFCFGBB hCfgBb)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, DBGFCFGBBENDTYPE_INVALID);

    return pCfgBb->enmEndType;
}


/**
 * Get the number of instructions contained in the basic block.
 *
 * @returns Number of instructions in the basic block.
 * @param   hCfgBb              The basic block handle.
 */
VMMR3DECL(uint32_t) DBGFR3CfgBbGetInstrCount(DBGFCFGBB hCfgBb)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, 0);

    return pCfgBb->cInstr;
}


/**
 * Get flags for the given basic block.
 *
 * @returns Combination of DBGF_CFG_BB_F_*
 * @param   hCfgBb              The basic block handle.
 */
VMMR3DECL(uint32_t) DBGFR3CfgBbGetFlags(DBGFCFGBB hCfgBb)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, 0);

    return pCfgBb->fFlags;
}


/**
 * Returns the error status and message if the given basic block has an error.
 *
 * @returns VBox status code of the error for the basic block.
 * @param   hCfgBb              The basic block handle.
 * @param   ppszErr             Where to store the pointer to the error message - optional.
 */
VMMR3DECL(int) DBGFR3CfgBbQueryError(DBGFCFGBB hCfgBb, const char **ppszErr)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, VERR_INVALID_HANDLE);

    if (ppszErr)
        *ppszErr = pCfgBb->pszErr;

    return pCfgBb->rcError;
}


/**
 * Store the disassembled instruction as a string in the given output buffer.
 *
 * @returns VBox status code.
 * @param   hCfgBb              The basic block handle.
 * @param   idxInstr            The instruction to query.
 * @param   pAddrInstr          Where to store the guest instruction address on success, optional.
 * @param   pcbInstr            Where to store the instruction size on success, optional.
 * @param   ppszInstr           Where to store the pointer to the disassembled instruction string, optional.
 */
VMMR3DECL(int) DBGFR3CfgBbQueryInstr(DBGFCFGBB hCfgBb, uint32_t idxInstr, PDBGFADDRESS pAddrInstr,
                                     uint32_t *pcbInstr, const char **ppszInstr)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, VERR_INVALID_POINTER);
    AssertReturn(idxInstr < pCfgBb->cInstr, VERR_INVALID_PARAMETER);

    if (pAddrInstr)
        *pAddrInstr = pCfgBb->aInstr[idxInstr].AddrInstr;
    if (pcbInstr)
        *pcbInstr = pCfgBb->aInstr[idxInstr].cbInstr;
    if (ppszInstr)
        *ppszInstr = pCfgBb->aInstr[idxInstr].pszInstr;

    return VINF_SUCCESS;
}


/**
 * Queries the successors of the basic block.
 *
 * @returns VBox status code.
 * @param   hCfgBb              The basic block handle.
 * @param   phCfgBbFollow       Where to store the handle to the basic block following
 *                              this one (optional).
 * @param   phCfgBbTarget       Where to store the handle to the basic block being the
 *                              branch target for this one (optional).
 */
VMMR3DECL(int) DBGFR3CfgBbQuerySuccessors(DBGFCFGBB hCfgBb, PDBGFCFGBB phCfgBbFollow, PDBGFCFGBB phCfgBbTarget)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, VERR_INVALID_POINTER);

    if (   phCfgBbFollow
        && (   pCfgBb->enmEndType == DBGFCFGBBENDTYPE_UNCOND
            || pCfgBb->enmEndType == DBGFCFGBBENDTYPE_COND))
    {
        DBGFADDRESS AddrStart = pCfgBb->AddrEnd;
        DBGFR3AddrAdd(&AddrStart, 1);
        int rc = DBGFR3CfgQueryBbByAddress(pCfgBb->pCfg, &AddrStart, phCfgBbFollow);
        AssertRC(rc);
    }

    if (   phCfgBbTarget
        && (   pCfgBb->enmEndType == DBGFCFGBBENDTYPE_UNCOND_JMP
            || pCfgBb->enmEndType == DBGFCFGBBENDTYPE_COND))
    {
        int rc = DBGFR3CfgQueryBbByAddress(pCfgBb->pCfg, &pCfgBb->AddrTarget, phCfgBbTarget);
        AssertRC(rc);
    }

    return VINF_SUCCESS;
}


/**
 * Returns the number of basic blocks referencing this basic block as a target.
 *
 * @returns Number of other basic blocks referencing this one.
 * @param   hCfgBb              The basic block handle.
 *
 * @note If the given basic block references itself (loop, etc.) this will be counted as well.
 */
VMMR3DECL(uint32_t) DBGFR3CfgBbGetRefBbCount(DBGFCFGBB hCfgBb)
{
    PDBGFCFGBBINT pCfgBb = hCfgBb;
    AssertPtrReturn(pCfgBb, 0);

    uint32_t cRefsBb = 0;
    PDBGFCFGBBINT pCfgBbCur = NULL;
    RTListForEach(&pCfgBb->pCfg->LstCfgBb, pCfgBbCur, DBGFCFGBBINT, NdCfgBb)
    {
        if (pCfgBbCur->fFlags & DBGF_CFG_BB_F_INCOMPLETE_ERR)
            continue;

        if (   pCfgBbCur->enmEndType == DBGFCFGBBENDTYPE_UNCOND
            || pCfgBbCur->enmEndType == DBGFCFGBBENDTYPE_COND)
        {
            DBGFADDRESS AddrStart = pCfgBb->AddrEnd;
            DBGFR3AddrAdd(&AddrStart, 1);
            if (dbgfR3CfgBbAddrEqual(&pCfgBbCur->AddrStart, &AddrStart))
                cRefsBb++;
        }

        if (   (   pCfgBbCur->enmEndType == DBGFCFGBBENDTYPE_UNCOND_JMP
                || pCfgBbCur->enmEndType == DBGFCFGBBENDTYPE_COND)
            && dbgfR3CfgBbAddrEqual(&pCfgBbCur->AddrStart, &pCfgBb->AddrTarget))
            cRefsBb++;
    }
    return cRefsBb;
}


/**
 * Returns the basic block handles referencing the given basic block.
 *
 * @returns VBox status code.
 * @retval  VERR_BUFFER_OVERFLOW if the array can't hold all the basic blocks.
 * @param   hCfgBb              The basic block handle.
 * @param   paCfgBbRef          Pointer to the array containing the referencing basic block handles on success.
 * @param   cRef                Number of entries in the given array.
 */
VMMR3DECL(int) DBGFR3CfgBbGetRefBb(DBGFCFGBB hCfgBb, PDBGFCFGBB paCfgBbRef, uint32_t cRef)
{
    RT_NOREF3(hCfgBb, paCfgBbRef, cRef);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @callback_method_impl{FNRTSORTCMP}
 */
static DECLCALLBACK(int) dbgfR3CfgItSortCmp(void const *pvElement1, void const *pvElement2, void *pvUser)
{
    PDBGFCFGITORDER penmOrder = (PDBGFCFGITORDER)pvUser;
    PDBGFCFGBBINT pCfgBb1 = *(PDBGFCFGBBINT *)pvElement1;
    PDBGFCFGBBINT pCfgBb2 = *(PDBGFCFGBBINT *)pvElement2;

    if (dbgfR3CfgBbAddrEqual(&pCfgBb1->AddrStart, &pCfgBb2->AddrStart))
        return 0;

    if (*penmOrder == DBGFCFGITORDER_BY_ADDR_LOWEST_FIRST)
    {
        if (dbgfR3CfgBbAddrLower(&pCfgBb1->AddrStart, &pCfgBb2->AddrStart))
            return -1;
        else
            return 1;
    }
    else
    {
        if (dbgfR3CfgBbAddrLower(&pCfgBb1->AddrStart, &pCfgBb2->AddrStart))
            return 1;
        else
            return -1;
    }

    AssertFailed();
}


/**
 * Creates a new iterator for the given control flow graph.
 *
 * @returns VBox status code.
 * @param   hCfg                The control flow graph handle.
 * @param   enmOrder            The order in which the basic blocks are enumerated.
 * @param   phCfgIt             Where to store the handle to the iterator on success.
 */
VMMR3DECL(int) DBGFR3CfgItCreate(DBGFCFG hCfg, DBGFCFGITORDER enmOrder, PDBGFCFGIT phCfgIt)
{
    int rc = VINF_SUCCESS;
    PDBGFCFGINT pCfg = hCfg;
    AssertPtrReturn(pCfg, VERR_INVALID_POINTER);
    AssertPtrReturn(phCfgIt, VERR_INVALID_POINTER);
    AssertReturn(enmOrder > DBGFCFGITORDER_INVALID && enmOrder < DBGFCFGITORDER_BREADTH_FIRST,
                 VERR_INVALID_PARAMETER);
    AssertReturn(enmOrder < DBGFCFGITORDER_DEPTH_FRIST, VERR_NOT_IMPLEMENTED); /** @todo */

    PDBGFCFGITINT pIt = (PDBGFCFGITINT)RTMemAllocZ(RT_OFFSETOF(DBGFCFGITINT, apBb[pCfg->cBbs]));
    if (RT_LIKELY(pIt))
    {
        DBGFR3CfgRetain(hCfg);
        pIt->pCfg      = pCfg;
        pIt->idxBbNext = 0;
        /* Fill the list and then sort. */
        PDBGFCFGBBINT pCfgBb;
        uint32_t idxBb = 0;
        RTListForEach(&pCfg->LstCfgBb, pCfgBb, DBGFCFGBBINT, NdCfgBb)
        {
            DBGFR3CfgBbRetain(pCfgBb);
            pIt->apBb[idxBb++] = pCfgBb;
        }

        /* Sort the blocks by address. */
        RTSortShell(&pIt->apBb[0], pCfg->cBbs, sizeof(PDBGFCFGBBINT), dbgfR3CfgItSortCmp, &enmOrder);

        *phCfgIt = pIt;
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}


/**
 * Destroys a given control flow graph iterator.
 *
 * @returns nothing.
 * @param   hCfgIt              The control flow graph iterator handle.
 */
VMMR3DECL(void) DBGFR3CfgItDestroy(DBGFCFGIT hCfgIt)
{
    PDBGFCFGITINT pIt = hCfgIt;
    AssertPtrReturnVoid(pIt);

    for (unsigned i = 0; i < pIt->pCfg->cBbs; i++)
        DBGFR3CfgBbRelease(pIt->apBb[i]);

    DBGFR3CfgRelease(pIt->pCfg);
    RTMemFree(pIt);
}


/**
 * Returns the next basic block in the iterator or NULL if there is no
 * basic block left.
 *
 * @returns Handle to the next basic block in the iterator or NULL if the end
 *          was reached.
 * @param   hCfgIt              The iterator handle.
 *
 * @note If a valid handle is returned it must be release with DBGFR3CfgBbRelease()
 *       when not required anymore.
 */
VMMR3DECL(DBGFCFGBB) DBGFR3CfgItNext(DBGFCFGIT hCfgIt)
{
    PDBGFCFGITINT pIt = hCfgIt;
    AssertPtrReturn(pIt, NULL);

    PDBGFCFGBBINT pCfgBb = NULL;
    if (pIt->idxBbNext < pIt->pCfg->cBbs)
    {
        pCfgBb = pIt->apBb[pIt->idxBbNext++];
        DBGFR3CfgBbRetain(pCfgBb);
    }

    return pCfgBb;
}


/**
 * Resets the given iterator to the beginning.
 *
 * @returns VBox status code.
 * @param   hCfgIt              The iterator handle.
 */
VMMR3DECL(int) DBGFR3CfgItReset(DBGFCFGIT hCfgIt)
{
    PDBGFCFGITINT pIt = hCfgIt;
    AssertPtrReturn(pIt, VERR_INVALID_HANDLE);

    pIt->idxBbNext = 0;
    return VINF_SUCCESS;
}
