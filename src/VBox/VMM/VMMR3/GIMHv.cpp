/* $Id$ */
/** @file
 * GIM - Guest Interface Manager, Hyper-V implementation.
 */

/*
 * Copyright (C) 2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_GIM
#include "GIMInternal.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/spinlock.h>

#include <VBox/vmm/cpum.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/version.h>

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
//#define GIMHV_HYPERCALL                 "GIMHvHypercall"
#ifdef VBOX_WITH_STATISTICS
# define GIMHV_MSRRANGE(a_uFirst, a_uLast, a_szName) \
    { (a_uFirst), (a_uLast), kCpumMsrRdFn_Gim, kCpumMsrWrFn_Gim, 0, 0, 0, 0, 0, a_szName, { 0 }, { 0 }, { 0 }, { 0 } }
#else
# define GIMHV_MSRRANGE(a_uFirst, a_uLast, a_szName) \
    { (a_uFirst), (a_uLast), kCpumMsrRdFn_Gim, kCpumMsrWrFn_Gim, 0, 0, 0, 0, 0, a_szName }
#endif

/**
 * Array of MSR ranges supported by Hyper-V.
 */
static CPUMMSRRANGE const g_aMsrRanges_HyperV[] =
{
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE0_START,  MSR_GIM_HV_RANGE0_END,  "Hyper-V range 0"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE1_START,  MSR_GIM_HV_RANGE1_END,  "Hyper-V range 1"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE2_START,  MSR_GIM_HV_RANGE2_END,  "Hyper-V range 2"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE3_START,  MSR_GIM_HV_RANGE3_END,  "Hyper-V range 3"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE4_START,  MSR_GIM_HV_RANGE4_END,  "Hyper-V range 4"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE5_START,  MSR_GIM_HV_RANGE5_END,  "Hyper-V range 5"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE6_START,  MSR_GIM_HV_RANGE6_END,  "Hyper-V range 6"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE7_START,  MSR_GIM_HV_RANGE7_END,  "Hyper-V range 7"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE8_START,  MSR_GIM_HV_RANGE8_END,  "Hyper-V range 8"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE9_START,  MSR_GIM_HV_RANGE9_END,  "Hyper-V range 9"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE10_START, MSR_GIM_HV_RANGE10_END, "Hyper-V range 10"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE11_START, MSR_GIM_HV_RANGE11_END, "Hyper-V range 11")
};
#undef GIMHV_MSR


/**
 * Initializes the Hyper-V GIM provider.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   uVersion    The interface version this VM should use.
 */
VMMR3_INT_DECL(int) GIMR3HvInit(PVM pVM)
{
    AssertReturn(pVM, VERR_INVALID_PARAMETER);
    AssertReturn(pVM->gim.s.enmProviderId == GIMPROVIDERID_HYPERV, VERR_INTERNAL_ERROR_5);

    int rc;
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    /*
     * Determine interface capabilities based on the version.
     */
    if (!pVM->gim.s.u32Version)
    {
        pHv->uBaseFeat = 0
                       //| GIM_HV_BASE_FEAT_VP_RUNTIME_MSR
                       | GIM_HV_BASE_FEAT_PART_TIME_REF_COUNT_MSR
                       //| GIM_HV_BASE_FEAT_BASIC_SYNTH_IC
                       //| GIM_HV_BASE_FEAT_SYNTH_TIMER_MSRS
                       | GIM_HV_BASE_FEAT_APIC_ACCESS_MSRS
                       | GIM_HV_BASE_FEAT_HYPERCALL_MSRS
                       | GIM_HV_BASE_FEAT_VP_ID_MSR
                       | GIM_HV_BASE_FEAT_VIRT_SYS_RESET_MSR
                       //| GIM_HV_BASE_FEAT_STAT_PAGES_MSR
                       | GIM_HV_BASE_FEAT_PART_REF_TSC_MSR
                       //| GIM_HV_BASE_FEAT_GUEST_IDLE_STATE_MSR
                       | GIM_HV_BASE_FEAT_TIMER_FREQ_MSRS
                       //| GIM_HV_BASE_FEAT_DEBUG_MSRS
                       ;

        pHv->uMiscFeat = GIM_HV_MISC_FEAT_TIMER_FREQ;

        pHv->uHyperHints = GIM_HV_HINT_MSR_FOR_SYS_RESET;
    }

    /*
     * Populate the required fields in MMIO2 region records for registering.
     */
    AssertCompile(GIM_HV_PAGE_SIZE == PAGE_SIZE);
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    pRegion->iRegion    = GIM_HV_HYPERCALL_PAGE_REGION_IDX;
    pRegion->fRCMapping = false;
    pRegion->cbRegion   = PAGE_SIZE;
    pRegion->GCPhysPage = NIL_RTGCPHYS;
    RTStrCopy(pRegion->szDescription, sizeof(pRegion->szDescription), "Hyper-V hypercall page");

    pRegion = &pHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    pRegion->iRegion    = GIM_HV_REF_TSC_PAGE_REGION_IDX;
    pRegion->fRCMapping = false;
    pRegion->cbRegion   = PAGE_SIZE;
    pRegion->GCPhysPage = NIL_RTGCPHYS;
    RTStrCopy(pRegion->szDescription, sizeof(pRegion->szDescription), "Hyper-V TSC page");

    /*
     * Make sure the CPU ID bit are in accordance to the Hyper-V
     * requirement and other paranoia checks.
     * See "Requirements for implementing the Microsoft hypervisor interface" spec.
     */
    Assert(!(pHv->uPartFlags & (  GIM_HV_PART_FLAGS_CREATE_PART
                                | GIM_HV_PART_FLAGS_ACCESS_MEMORY_POOL
                                | GIM_HV_PART_FLAGS_ACCESS_PART_ID
                                | GIM_HV_PART_FLAGS_ADJUST_MSG_BUFFERS
                                | GIM_HV_PART_FLAGS_CREATE_PORT
                                | GIM_HV_PART_FLAGS_ACCESS_STATS
                                | GIM_HV_PART_FLAGS_CPU_MGMT
                                | GIM_HV_PART_FLAGS_CPU_PROFILER)));
    Assert((pHv->uBaseFeat & (GIM_HV_BASE_FEAT_HYPERCALL_MSRS | GIM_HV_BASE_FEAT_VP_ID_MSR))
                          == (GIM_HV_BASE_FEAT_HYPERCALL_MSRS | GIM_HV_BASE_FEAT_VP_ID_MSR));
    for (unsigned i = 0; i < RT_ELEMENTS(pHv->aMmio2Regions); i++)
    {
        PCGIMMMIO2REGION pcCur = &pHv->aMmio2Regions[i];
        Assert(!pcCur->fRCMapping);
        Assert(!pcCur->fMapped);
        Assert(pcCur->GCPhysPage == NIL_RTGCPHYS);
    }

    /*
     * Expose HVP (Hypervisor Present) bit to the guest.
     */
    CPUMSetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_HVP);

    /*
     * Modify the standard hypervisor leaves for Hyper-V.
     */
    CPUMCPUIDLEAF HyperLeaf;
    RT_ZERO(HyperLeaf);
    HyperLeaf.uLeaf        = UINT32_C(0x40000000);
    HyperLeaf.uEax         = UINT32_C(0x40000005); /* Minimum value for Hyper-V */
    HyperLeaf.uEbx         = 0x7263694D;           /* 'Micr' */
    HyperLeaf.uEcx         = 0x666F736F;           /* 'osof' */
    HyperLeaf.uEdx         = 0x76482074;           /* 't Hv' */
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000001);
    HyperLeaf.uEax         = 0x31237648;           /* 'Hv#1' */
    HyperLeaf.uEbx         = 0;                    /* Reserved */
    HyperLeaf.uEcx         = 0;                    /* Reserved */
    HyperLeaf.uEdx         = 0;                    /* Reserved */
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Add Hyper-V specific leaves.
     */
    HyperLeaf.uLeaf        = UINT32_C(0x40000002); /* MBZ until MSR_GIM_HV_GUEST_OS_ID is set by the guest. */
    HyperLeaf.uEax         = 0;
    HyperLeaf.uEbx         = 0;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000003);
    HyperLeaf.uEax         = pHv->uBaseFeat;
    HyperLeaf.uEbx         = pHv->uPartFlags;
    HyperLeaf.uEcx         = pHv->uPowMgmtFeat;
    HyperLeaf.uEdx         = pHv->uMiscFeat;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000004);
    HyperLeaf.uEax         = pHv->uHyperHints;
    HyperLeaf.uEbx         = 0xffffffff;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Insert all MSR ranges of Hyper-V.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(g_aMsrRanges_HyperV); i++)
    {
        rc = CPUMR3MsrRangesInsert(pVM, &g_aMsrRanges_HyperV[i]);
        AssertLogRelRCReturn(rc, rc);
    }

    return VINF_SUCCESS;
}


#if 0
VMMR3_INT_DECL(int) GIMR3HvInitFinalize(PVM pVM)
{
    pVM->gim.s.pfnHypercallR3 = &GIMHvHypercall;
    if (!HMIsEnabled(pVM))
    {
        rc = PDMR3LdrGetSymbolRC(pVM, NULL /* pszModule */, GIMHV_HYPERCALL, &pVM->gim.s.pfnHypercallRC);
        AssertRCReturn(rc, rc);
    }
    rc = PDMR3LdrGetSymbolR0(pVM, NULL /* pszModule */, GIMHV_HYPERCALL, &pVM->gim.s.pfnHypercallR0);
    AssertRCReturn(rc, rc);
}
#endif


VMMR3_INT_DECL(int) GIMR3HvTerm(PVM pVM)
{
    GIMR3HvReset(pVM);
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(void) GIMR3HvRelocate(PVM pVM, RTGCINTPTR offDelta)
{
#if 0
    int rc = PDMR3LdrGetSymbolRC(pVM, NULL /* pszModule */, GIMHV_HYPERCALL, &pVM->gim.s.pfnHypercallRC);
    AssertFatalRC(rc);
#endif
}


/**
 * The VM is being reset. This resets Hyper-V provider MSRs and unmaps whatever
 * Hyper-V regions that the guest may have mapped.
 *
 * @param   pVM     Pointer to the VM.
 */
VMMR3_INT_DECL(void) GIMR3HvReset(PVM pVM)
{
    /*
     * Unmap MMIO2 pages that the guest may have setup.
     */
    LogRel(("GIM: HyperV: Resetting Hyper-V MMIO2 regions and MSRs\n"));
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    for (unsigned i = 0; i < RT_ELEMENTS(pHv->aMmio2Regions); i++)
    {
        PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[i];
        GIMR3Mmio2Unmap(pVM, pRegion);
    }

    /*
     * Reset MSRs.
     */
    pHv->u64GuestOsIdMsr = 0;
    pHv->u64HypercallMsr = 0;
    pHv->u64TscPageMsr   = 0;
}


/**
 * Returns a pointer to the MMIO2 regions supported by Hyper-V.
 *
 * @returns Pointer to an array of MMIO2 regions.
 * @param   pVM         Pointer to the VM.
 * @param   pcRegions   Where to store the number of regions in the array.
 */
VMMR3_INT_DECL(PGIMMMIO2REGION) GIMR3HvGetMmio2Regions(PVM pVM, uint32_t *pcRegions)
{
    Assert(GIMIsEnabled(pVM));
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    *pcRegions = RT_ELEMENTS(pHv->aMmio2Regions);
    Assert(*pcRegions <= UINT8_MAX);    /* See PGMR3PhysMMIO2Register(). */
    return pHv->aMmio2Regions;
}


/**
 * Hyper-V state-save operation.
 *
 * @returns VBox status code.
 * @param   pVM     Pointer to the VM.
 * @param   pSSM    Pointer to the SSM handle.
 */
VMMR3_INT_DECL(int) GIMR3HvSave(PVM pVM, PSSMHANDLE pSSM)
{
    PCGIMHV pcHv = &pVM->gim.s.u.Hv;

    /** @todo Save per-VCPU data. */

    /*
     * Save per-VM MSRs.
     */
    int rc = SSMR3PutU64(pSSM, pcHv->u64GuestOsIdMsr);      AssertRCReturn(rc, rc);
    rc = SSMR3PutU64(pSSM, pcHv->u64HypercallMsr);          AssertRCReturn(rc, rc);
    rc = SSMR3PutU64(pSSM, pcHv->u64TscPageMsr);            AssertRCReturn(rc, rc);

    /*
     * Save Hyper-V features / capabilities.
     */
    rc = SSMR3PutU32(pSSM, pcHv->uBaseFeat);                AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pcHv->uPartFlags);               AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pcHv->uPowMgmtFeat);             AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pcHv->uMiscFeat);                AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pcHv->uHyperHints);              AssertRCReturn(rc, rc);

    /*
     * Save the Hypercall region.
     */
    PCGIMMMIO2REGION pcRegion = &pcHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    rc = SSMR3PutU8(pSSM,     pcRegion->iRegion);           AssertRCReturn(rc, rc);
    rc = SSMR3PutBool(pSSM,   pcRegion->fRCMapping);        AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM,    pcRegion->cbRegion);          AssertRCReturn(rc, rc);
    rc = SSMR3PutGCPhys(pSSM, pcRegion->GCPhysPage);        AssertRCReturn(rc, rc);
    rc = SSMR3PutStrZ(pSSM,   pcRegion->szDescription);     AssertRCReturn(rc, rc);

    /*
     * Save the reference TSC region.
     */
    pcRegion = &pcHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    rc = SSMR3PutU8(pSSM,     pcRegion->iRegion);           AssertRCReturn(rc, rc);
    rc = SSMR3PutBool(pSSM,   pcRegion->fRCMapping);        AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM,    pcRegion->cbRegion);          AssertRCReturn(rc, rc);
    rc = SSMR3PutGCPhys(pSSM, pcRegion->GCPhysPage);        AssertRCReturn(rc, rc);
    rc = SSMR3PutStrZ(pSSM,   pcRegion->szDescription);     AssertRCReturn(rc, rc);
    /* Save the TSC sequence so we can bump it on restore (as the CPU frequency/offset may change). */
    uint32_t uTscSequence = 0;
    if (   pcRegion->fMapped
        && MSR_GIM_HV_REF_TSC_IS_ENABLED(pcHv->u64TscPageMsr))
    {
        PCGIMHVREFTSC pcRefTsc = (PCGIMHVREFTSC)pcRegion->pvPageR3;
        uTscSequence = pcRefTsc->u32TscSequence;
    }
    rc = SSMR3PutU32(pSSM,    uTscSequence);                AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}


/**
 * Hyper-V state-load operation, final pass.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the VM.
 * @param   pSSM            Pointer to the SSM handle.
 * @param   uSSMVersion     The saved-state version.
 */
VMMR3_INT_DECL(int) GIMR3HvLoad(PVM pVM, PSSMHANDLE pSSM, uint32_t uSSMVersion)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    /** @todo Load per-VCPU data. */

    /*
     * Load per-VM MSRs.
     */
    int rc = SSMR3GetU64(pSSM, &pHv->u64GuestOsIdMsr);      AssertRCReturn(rc, rc);
    rc = SSMR3GetU64(pSSM, &pHv->u64HypercallMsr);          AssertRCReturn(rc, rc);
    rc = SSMR3GetU64(pSSM, &pHv->u64TscPageMsr);            AssertRCReturn(rc, rc);

    /*
     * Save Hyper-V features / capabilities.
     */
    rc = SSMR3GetU32(pSSM, &pHv->uBaseFeat);                AssertRCReturn(rc, rc);
    rc = SSMR3GetU32(pSSM, &pHv->uPartFlags);               AssertRCReturn(rc, rc);
    rc = SSMR3GetU32(pSSM, &pHv->uPowMgmtFeat);             AssertRCReturn(rc, rc);
    rc = SSMR3GetU32(pSSM, &pHv->uMiscFeat);                AssertRCReturn(rc, rc);
    rc = SSMR3GetU32(pSSM, &pHv->uHyperHints);              AssertRCReturn(rc, rc);

    /*
     * Load and enable the Hypercall region.
     */
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    rc = SSMR3GetU8(pSSM,     &pRegion->iRegion);           AssertRCReturn(rc, rc);
    rc = SSMR3GetBool(pSSM,   &pRegion->fRCMapping);        AssertRCReturn(rc, rc);
    rc = SSMR3GetU32(pSSM,    &pRegion->cbRegion);          AssertRCReturn(rc, rc);
    rc = SSMR3GetGCPhys(pSSM, &pRegion->GCPhysPage);        AssertRCReturn(rc, rc);
    rc = SSMR3GetStrZ(pSSM,    pRegion->szDescription, sizeof(pRegion->szDescription));
    AssertRCReturn(rc, rc);
    if (MSR_GIM_HV_HYPERCALL_IS_ENABLED(pHv->u64HypercallMsr))
    {
        Assert(pRegion->GCPhysPage != NIL_RTGCPHYS);
        if (RT_LIKELY(pRegion->fRegistered))
        {
            rc = GIMR3HvEnableHypercallPage(pVM, pRegion->GCPhysPage);
            if (RT_FAILURE(rc))
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Failed to enable the hypercall page. GCPhys=%#RGp rc=%Rrc"),
                                        pRegion->GCPhysPage, rc);
        }
        else
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Hypercall MMIO2 region not registered. Missing GIM device?!"));
    }

    /*
     * Load and enable the reference TSC region.
     */
    uint32_t uTscSequence;
    pRegion = &pHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    rc = SSMR3GetU8(pSSM,     &pRegion->iRegion);           AssertRCReturn(rc, rc);
    rc = SSMR3GetBool(pSSM,   &pRegion->fRCMapping);        AssertRCReturn(rc, rc);
    rc = SSMR3GetU32(pSSM,    &pRegion->cbRegion);          AssertRCReturn(rc, rc);
    rc = SSMR3GetGCPhys(pSSM, &pRegion->GCPhysPage);        AssertRCReturn(rc, rc);
    rc = SSMR3GetStrZ(pSSM,    pRegion->szDescription, sizeof(pRegion->szDescription));
    rc = SSMR3GetU32(pSSM,    &uTscSequence);               AssertRCReturn(rc, rc);
    AssertRCReturn(rc, rc);
    if (MSR_GIM_HV_REF_TSC_IS_ENABLED(pHv->u64TscPageMsr))
    {
        Assert(pRegion->GCPhysPage != NIL_RTGCPHYS);
        if (pRegion->fRegistered)
        {
            rc = GIMR3HvEnableTscPage(pVM, pRegion->GCPhysPage, true /* fUseThisTscSeq */, uTscSequence);
            if (RT_FAILURE(rc))
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Failed to enable the TSC page. GCPhys=%#RGp rc=%Rrc"),
                                        pRegion->GCPhysPage, rc);
        }
        else
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("TSC-page MMIO2 region not registered. Missing GIM device?!"));
    }

    return rc;
}


/**
 * Enables the Hyper-V TSC page.
 *
 * @returns VBox status code.
 * @param   pVM                     Pointer to the VM.
 * @param   GCPhysTscPage           Where to map the TSC page.
 * @param   fUseThisTscSequence     Whether to set the TSC sequence number to
 *                                  the one specified in @a uTscSequence.
 * @param   uTscSequence            The TSC sequence value to use. Ignored if @a
 *                                  fUseThisTscSequence is false.
                                                                    */
VMMR3_INT_DECL(int) GIMR3HvEnableTscPage(PVM pVM, RTGCPHYS GCPhysTscPage, bool fUseThisTscSequence, uint32_t uTscSequence)
{
    PPDMDEVINSR3    pDevIns = pVM->gim.s.pDevInsR3;
    PGIMMMIO2REGION pRegion = &pVM->gim.s.u.Hv.aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    AssertPtrReturn(pDevIns, VERR_GIM_DEVICE_NOT_REGISTERED);

    int rc;
    if (pRegion->fMapped)
    {
        /*
         * Is it already enabled at the given guest-address?
         */
        if (pRegion->GCPhysPage == GCPhysTscPage)
            return VINF_SUCCESS;

        /*
         * If it's mapped at a different address, unmap the previous address.
         */
        rc = GIMR3HvDisableTscPage(pVM);
        AssertRC(rc);
    }

    /*
     * Map the TSC-page at the specified address.
     */
    Assert(!pRegion->fMapped);
    rc = GIMR3Mmio2Map(pVM, pRegion, GCPhysTscPage);
    if (RT_SUCCESS(rc))
    {
        Assert(pRegion->GCPhysPage == GCPhysTscPage);

        /*
         * Update the TSC scale. Windows guests expect a non-zero TSC sequence, otherwise
         * they fallback to using the reference count MSR which is not ideal in terms of VM-exits.
         *
         * Also, Hyper-V normalizes the time in 10 MHz, see:
         * http://technet.microsoft.com/it-it/sysinternals/dn553408%28v=vs.110%29
         */
        PGIMHVREFTSC pRefTsc = (PGIMHVREFTSC)pRegion->pvPageR3;
        Assert(pRefTsc);

        uint64_t const u64TscKHz = TMCpuTicksPerSecond(pVM) / UINT64_C(1000);
        uint32_t       u32TscSeq = 1;
        if (   fUseThisTscSequence
            && uTscSequence < UINT32_C(0xfffffffe))
        {
            u32TscSeq = uTscSequence + 1;
        }
        pRefTsc->u32TscSequence  = u32TscSeq;
        pRefTsc->u64TscScale     = ((INT64_C(10000) << 32) / u64TscKHz) << 32;

        LogRel(("GIM: HyperV: Enabled TSC page at %#RGp - u64TscScale=%#RX64 u64TscKHz=%#RX64 (%'RU64)\n", GCPhysTscPage,
                pRefTsc->u64TscScale, u64TscKHz, u64TscKHz));
        return VINF_SUCCESS;
    }
    else
        LogRelFunc(("GIMR3Mmio2Map failed. rc=%Rrc\n", rc));

    return VERR_GIM_OPERATION_FAILED;
}


/**
 * Disables the Hyper-V TSC page.
 *
 * @returns VBox status code.
 * @param   pVM     Pointer to the VM.
 */
VMMR3_INT_DECL(int) GIMR3HvDisableTscPage(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    if (pRegion->fMapped)
    {
        GIMR3Mmio2Unmap(pVM, pRegion);
        Assert(!pRegion->fMapped);
        LogRel(("GIM: HyperV: Disabled TSC-page\n"));
        return VINF_SUCCESS;
    }
    return VERR_GIM_PVTSC_NOT_ENABLED;
}


/**
 * Disables the Hyper-V Hypercall page.
 *
 * @returns VBox status code.
 */
VMMR3_INT_DECL(int) GIMR3HvDisableHypercallPage(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    if (pRegion->fMapped)
    {
        GIMR3Mmio2Unmap(pVM, pRegion);
        Assert(!pRegion->fMapped);
        LogRel(("GIM: HyperV: Disabled Hypercall-page\n"));
        return VINF_SUCCESS;
    }
    return VERR_GIM_HYPERCALLS_NOT_ENABLED;
}


/**
 * Enables the Hyper-V Hypercall page.
 *
 * @returns VBox status code.
 * @param   pVM                     Pointer to the VM.
 * @param   GCPhysHypercallPage     Where to map the hypercall page.
 */
VMMR3_INT_DECL(int) GIMR3HvEnableHypercallPage(PVM pVM, RTGCPHYS GCPhysHypercallPage)
{
    PPDMDEVINSR3    pDevIns = pVM->gim.s.pDevInsR3;
    PGIMMMIO2REGION pRegion = &pVM->gim.s.u.Hv.aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    AssertPtrReturn(pDevIns, VERR_GIM_DEVICE_NOT_REGISTERED);

    if (pRegion->fMapped)
    {
        /*
         * Is it already enabled at the given guest-address?
         */
        if (pRegion->GCPhysPage == GCPhysHypercallPage)
            return VINF_SUCCESS;

        /*
         * If it's mapped at a different address, unmap the previous address.
         */
        int rc2 = GIMR3HvDisableHypercallPage(pVM);
        AssertRC(rc2);
    }

    /*
     * Map the hypercall-page at the specified address.
     */
    Assert(!pRegion->fMapped);
    int rc = GIMR3Mmio2Map(pVM, pRegion, GCPhysHypercallPage);
    if (RT_SUCCESS(rc))
    {
        Assert(pRegion->GCPhysPage == GCPhysHypercallPage);

        /*
         * Patch the hypercall-page.
         */
        if (HMIsEnabled(pVM))
        {
            size_t cbWritten = 0;
            rc = HMPatchHypercall(pVM, pRegion->pvPageR3, PAGE_SIZE, &cbWritten);
            if (   RT_SUCCESS(rc)
                && cbWritten < PAGE_SIZE)
            {
                uint8_t *pbLast = (uint8_t *)pRegion->pvPageR3 + cbWritten;
                *pbLast = 0xc3;  /* RET */

                LogRel(("GIM: HyperV: Enabled hypercalls at %#RGp\n", GCPhysHypercallPage));
                return VINF_SUCCESS;
            }
            else
            {
                if (rc == VINF_SUCCESS)
                    rc = VERR_GIM_OPERATION_FAILED;
                LogRelFunc(("HMPatchHypercall failed. rc=%Rrc cbWritten=%u\n", rc, cbWritten));
            }
        }
        else
        {
            /** @todo Handle raw-mode hypercall page patching. */
            LogRel(("GIM: HyperV: Raw-mode not yet implemented!\n"));
        }
        GIMR3Mmio2Unmap(pVM, pRegion);
    }
    else
        LogRelFunc(("GIMR3Mmio2Map failed. rc=%Rrc\n", rc));

    return rc;
}

