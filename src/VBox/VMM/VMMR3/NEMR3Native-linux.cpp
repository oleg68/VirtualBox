/* $Id$ */
/** @file
 * NEM - Native execution manager, native ring-3 Linux backend.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_NEM
#define VMCPU_INCL_CPUM_GST_CTX
#include <VBox/vmm/nem.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pdm.h>
#include "NEMInternal.h"
#include <VBox/vmm/vmcc.h>

#include <iprt/string.h>
#include <iprt/system.h>

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <linux/kvm.h>



/**
 * Worker for nemR3NativeInit that gets the hypervisor capabilities.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pErrInfo            Where to always return error info.
 */
static int nemR3LnxInitCheckCapabilities(PVM pVM, PRTERRINFO pErrInfo)
{
    AssertReturn(pVM->nem.s.fdVm != -1, RTErrInfoSet(pErrInfo, VERR_WRONG_ORDER, "Wrong initalization order"));

    /*
     * Capabilities.
     */
    static const struct
    {
        const char *pszName;
        int         iCap;
        uint32_t    offNem      : 24;
        uint32_t    cbNem       : 3;
        uint32_t    fReqNonZero : 1;
        uint32_t    uReserved   : 4;
    } s_aCaps[] =
    {
#define CAP_ENTRY__L(a_Define)           { #a_Define, a_Define,            UINT32_C(0x00ffffff), 0, 0, 0 }
#define CAP_ENTRY__S(a_Define, a_Member) { #a_Define, a_Define, RT_UOFFSETOF(NEM, a_Member), RT_SIZEOFMEMB(NEM, a_Member), 0, 0 }
#define CAP_ENTRY_MS(a_Define, a_Member) { #a_Define, a_Define, RT_UOFFSETOF(NEM, a_Member), RT_SIZEOFMEMB(NEM, a_Member), 1, 0 }
#define CAP_ENTRY__U(a_Number)           { "KVM_CAP_" #a_Number, a_Number, UINT32_C(0x00ffffff), 0, 0, 0 }
#define CAP_ENTRY_ML(a_Number)           { "KVM_CAP_" #a_Number, a_Number, UINT32_C(0x00ffffff), 0, 1, 0 }

        CAP_ENTRY__L(KVM_CAP_IRQCHIP),                       /* 0 */
        CAP_ENTRY_ML(KVM_CAP_HLT),
        CAP_ENTRY__L(KVM_CAP_MMU_SHADOW_CACHE_CONTROL),
        CAP_ENTRY_ML(KVM_CAP_USER_MEMORY),
        CAP_ENTRY__L(KVM_CAP_SET_TSS_ADDR),
        CAP_ENTRY__U(5),
        CAP_ENTRY__L(KVM_CAP_VAPIC),
        CAP_ENTRY__L(KVM_CAP_EXT_CPUID),
        CAP_ENTRY__L(KVM_CAP_CLOCKSOURCE),
        CAP_ENTRY__L(KVM_CAP_NR_VCPUS),
        CAP_ENTRY_MS(KVM_CAP_NR_MEMSLOTS, cMaxMemSlots),     /* 10 */
        CAP_ENTRY__L(KVM_CAP_PIT),
        CAP_ENTRY__L(KVM_CAP_NOP_IO_DELAY),
        CAP_ENTRY__L(KVM_CAP_PV_MMU),
        CAP_ENTRY__L(KVM_CAP_MP_STATE),
        CAP_ENTRY__L(KVM_CAP_COALESCED_MMIO),
        CAP_ENTRY__L(KVM_CAP_SYNC_MMU),
        CAP_ENTRY__U(17),
        CAP_ENTRY__L(KVM_CAP_IOMMU),
        CAP_ENTRY__U(19), /* Buggy KVM_CAP_JOIN_MEMORY_REGIONS? */
        CAP_ENTRY__U(20), /* Mon-working KVM_CAP_DESTROY_MEMORY_REGION? */
        CAP_ENTRY__L(KVM_CAP_DESTROY_MEMORY_REGION_WORKS),   /* 21 */
        CAP_ENTRY__L(KVM_CAP_USER_NMI),
#ifdef __KVM_HAVE_GUEST_DEBUG
        CAP_ENTRY__L(KVM_CAP_SET_GUEST_DEBUG),
#endif
#ifdef __KVM_HAVE_PIT
        CAP_ENTRY__L(KVM_CAP_REINJECT_CONTROL),
#endif
        CAP_ENTRY__L(KVM_CAP_IRQ_ROUTING),
        CAP_ENTRY__L(KVM_CAP_IRQ_INJECT_STATUS),
        CAP_ENTRY__U(27),
        CAP_ENTRY__U(28),
        CAP_ENTRY__L(KVM_CAP_ASSIGN_DEV_IRQ),
        CAP_ENTRY__L(KVM_CAP_JOIN_MEMORY_REGIONS_WORKS),     /* 30 */
#ifdef __KVM_HAVE_MCE
        CAP_ENTRY__L(KVM_CAP_MCE),
#endif
        CAP_ENTRY__L(KVM_CAP_IRQFD),
#ifdef __KVM_HAVE_PIT
        CAP_ENTRY__L(KVM_CAP_PIT2),
#endif
        CAP_ENTRY__L(KVM_CAP_SET_BOOT_CPU_ID),
#ifdef __KVM_HAVE_PIT_STATE2
        CAP_ENTRY__L(KVM_CAP_PIT_STATE2),
#endif
        CAP_ENTRY__L(KVM_CAP_IOEVENTFD),
        CAP_ENTRY__L(KVM_CAP_SET_IDENTITY_MAP_ADDR),
#ifdef __KVM_HAVE_XEN_HVM
        CAP_ENTRY__L(KVM_CAP_XEN_HVM),
#endif
        CAP_ENTRY_ML(KVM_CAP_ADJUST_CLOCK),
        CAP_ENTRY__L(KVM_CAP_INTERNAL_ERROR_DATA),           /* 40 */
#ifdef __KVM_HAVE_VCPU_EVENTS
        CAP_ENTRY_ML(KVM_CAP_VCPU_EVENTS),
#else
        CAP_ENTRY_MU(41),
#endif
        CAP_ENTRY__L(KVM_CAP_S390_PSW),
        CAP_ENTRY__L(KVM_CAP_PPC_SEGSTATE),
        CAP_ENTRY__L(KVM_CAP_HYPERV),
        CAP_ENTRY__L(KVM_CAP_HYPERV_VAPIC),
        CAP_ENTRY__L(KVM_CAP_HYPERV_SPIN),
        CAP_ENTRY__L(KVM_CAP_PCI_SEGMENT),
        CAP_ENTRY__L(KVM_CAP_PPC_PAIRED_SINGLES),
        CAP_ENTRY__L(KVM_CAP_INTR_SHADOW),
#ifdef __KVM_HAVE_DEBUGREGS
        CAP_ENTRY__L(KVM_CAP_DEBUGREGS),                     /* 50 */
#endif
        CAP_ENTRY__S(KVM_CAP_X86_ROBUST_SINGLESTEP, fRobustSingleStep),
        CAP_ENTRY__L(KVM_CAP_PPC_OSI),
        CAP_ENTRY__L(KVM_CAP_PPC_UNSET_IRQ),
        CAP_ENTRY__L(KVM_CAP_ENABLE_CAP),
#ifdef __KVM_HAVE_XSAVE
        CAP_ENTRY_ML(KVM_CAP_XSAVE),
#else
        CAP_ENTRY_MU(55),
#endif
#ifdef __KVM_HAVE_XCRS
        CAP_ENTRY_ML(KVM_CAP_XCRS),
#else
        CAP_ENTRY_MU(56),
#endif
        CAP_ENTRY__L(KVM_CAP_PPC_GET_PVINFO),
        CAP_ENTRY__L(KVM_CAP_PPC_IRQ_LEVEL),
        CAP_ENTRY__L(KVM_CAP_ASYNC_PF),
        CAP_ENTRY__L(KVM_CAP_TSC_CONTROL),                   /* 60 */
        CAP_ENTRY__L(KVM_CAP_GET_TSC_KHZ),
        CAP_ENTRY__L(KVM_CAP_PPC_BOOKE_SREGS),
        CAP_ENTRY__L(KVM_CAP_SPAPR_TCE),
        CAP_ENTRY__L(KVM_CAP_PPC_SMT),
        CAP_ENTRY__L(KVM_CAP_PPC_RMA),
        CAP_ENTRY__L(KVM_CAP_MAX_VCPUS),
        CAP_ENTRY__L(KVM_CAP_PPC_HIOR),
        CAP_ENTRY__L(KVM_CAP_PPC_PAPR),
        CAP_ENTRY__L(KVM_CAP_SW_TLB),
        CAP_ENTRY__L(KVM_CAP_ONE_REG),                       /* 70 */
        CAP_ENTRY__L(KVM_CAP_S390_GMAP),
        CAP_ENTRY__L(KVM_CAP_TSC_DEADLINE_TIMER),
        CAP_ENTRY__L(KVM_CAP_S390_UCONTROL),
        CAP_ENTRY__L(KVM_CAP_SYNC_REGS),
        CAP_ENTRY__L(KVM_CAP_PCI_2_3),
        CAP_ENTRY__L(KVM_CAP_KVMCLOCK_CTRL),
        CAP_ENTRY__L(KVM_CAP_SIGNAL_MSI),
        CAP_ENTRY__L(KVM_CAP_PPC_GET_SMMU_INFO),
        CAP_ENTRY__L(KVM_CAP_S390_COW),
        CAP_ENTRY__L(KVM_CAP_PPC_ALLOC_HTAB),                /* 80 */
        CAP_ENTRY__L(KVM_CAP_READONLY_MEM),
        CAP_ENTRY__L(KVM_CAP_IRQFD_RESAMPLE),
        CAP_ENTRY__L(KVM_CAP_PPC_BOOKE_WATCHDOG),
        CAP_ENTRY__L(KVM_CAP_PPC_HTAB_FD),
        CAP_ENTRY__L(KVM_CAP_S390_CSS_SUPPORT),
        CAP_ENTRY__L(KVM_CAP_PPC_EPR),
        CAP_ENTRY__L(KVM_CAP_ARM_PSCI),
        CAP_ENTRY__L(KVM_CAP_ARM_SET_DEVICE_ADDR),
        CAP_ENTRY__L(KVM_CAP_DEVICE_CTRL),
        CAP_ENTRY__L(KVM_CAP_IRQ_MPIC),                      /* 90 */
        CAP_ENTRY__L(KVM_CAP_PPC_RTAS),
        CAP_ENTRY__L(KVM_CAP_IRQ_XICS),
        CAP_ENTRY__L(KVM_CAP_ARM_EL1_32BIT),
        CAP_ENTRY__L(KVM_CAP_SPAPR_MULTITCE),
        CAP_ENTRY__L(KVM_CAP_EXT_EMUL_CPUID),
        CAP_ENTRY__L(KVM_CAP_HYPERV_TIME),
        CAP_ENTRY__L(KVM_CAP_IOAPIC_POLARITY_IGNORED),
        CAP_ENTRY__L(KVM_CAP_ENABLE_CAP_VM),
        CAP_ENTRY__L(KVM_CAP_S390_IRQCHIP),
        CAP_ENTRY__L(KVM_CAP_IOEVENTFD_NO_LENGTH),           /* 100 */
        CAP_ENTRY__L(KVM_CAP_VM_ATTRIBUTES),
        CAP_ENTRY__L(KVM_CAP_ARM_PSCI_0_2),
        CAP_ENTRY__L(KVM_CAP_PPC_FIXUP_HCALL),
        CAP_ENTRY__L(KVM_CAP_PPC_ENABLE_HCALL),
        CAP_ENTRY__L(KVM_CAP_CHECK_EXTENSION_VM),
        CAP_ENTRY__L(KVM_CAP_S390_USER_SIGP),
        CAP_ENTRY__L(KVM_CAP_S390_VECTOR_REGISTERS),
        CAP_ENTRY__L(KVM_CAP_S390_MEM_OP),
        CAP_ENTRY__L(KVM_CAP_S390_USER_STSI),
        CAP_ENTRY__L(KVM_CAP_S390_SKEYS),                    /* 110 */
        CAP_ENTRY__L(KVM_CAP_MIPS_FPU),
        CAP_ENTRY__L(KVM_CAP_MIPS_MSA),
        CAP_ENTRY__L(KVM_CAP_S390_INJECT_IRQ),
        CAP_ENTRY__L(KVM_CAP_S390_IRQ_STATE),
        CAP_ENTRY__L(KVM_CAP_PPC_HWRNG),
        CAP_ENTRY__L(KVM_CAP_DISABLE_QUIRKS),
        CAP_ENTRY__L(KVM_CAP_X86_SMM),
        CAP_ENTRY__L(KVM_CAP_MULTI_ADDRESS_SPACE),
        CAP_ENTRY__L(KVM_CAP_GUEST_DEBUG_HW_BPS),
        CAP_ENTRY__L(KVM_CAP_GUEST_DEBUG_HW_WPS),            /* 120 */
        CAP_ENTRY__L(KVM_CAP_SPLIT_IRQCHIP),
        CAP_ENTRY__L(KVM_CAP_IOEVENTFD_ANY_LENGTH),
        CAP_ENTRY__L(KVM_CAP_HYPERV_SYNIC),
        CAP_ENTRY__L(KVM_CAP_S390_RI),
        CAP_ENTRY__L(KVM_CAP_SPAPR_TCE_64),
        CAP_ENTRY__L(KVM_CAP_ARM_PMU_V3),
        CAP_ENTRY__L(KVM_CAP_VCPU_ATTRIBUTES),
        CAP_ENTRY__L(KVM_CAP_MAX_VCPU_ID),
        CAP_ENTRY__L(KVM_CAP_X2APIC_API),
        CAP_ENTRY__L(KVM_CAP_S390_USER_INSTR0),              /* 130 */
        CAP_ENTRY__L(KVM_CAP_MSI_DEVID),
        CAP_ENTRY__L(KVM_CAP_PPC_HTM),
        CAP_ENTRY__L(KVM_CAP_SPAPR_RESIZE_HPT),
        CAP_ENTRY__L(KVM_CAP_PPC_MMU_RADIX),
        CAP_ENTRY__L(KVM_CAP_PPC_MMU_HASH_V3),
        CAP_ENTRY__L(KVM_CAP_IMMEDIATE_EXIT),
        CAP_ENTRY__L(KVM_CAP_MIPS_VZ),
        CAP_ENTRY__L(KVM_CAP_MIPS_TE),
        CAP_ENTRY__L(KVM_CAP_MIPS_64BIT),
        CAP_ENTRY__L(KVM_CAP_S390_GS),                       /* 140 */
        CAP_ENTRY__L(KVM_CAP_S390_AIS),
        CAP_ENTRY__L(KVM_CAP_SPAPR_TCE_VFIO),
        CAP_ENTRY__L(KVM_CAP_X86_DISABLE_EXITS),
        CAP_ENTRY__L(KVM_CAP_ARM_USER_IRQ),
        CAP_ENTRY__L(KVM_CAP_S390_CMMA_MIGRATION),
        CAP_ENTRY__L(KVM_CAP_PPC_FWNMI),
        CAP_ENTRY__L(KVM_CAP_PPC_SMT_POSSIBLE),
        CAP_ENTRY__L(KVM_CAP_HYPERV_SYNIC2),
        CAP_ENTRY__L(KVM_CAP_HYPERV_VP_INDEX),
        CAP_ENTRY__L(KVM_CAP_S390_AIS_MIGRATION),            /* 150 */
        CAP_ENTRY__L(KVM_CAP_PPC_GET_CPU_CHAR),
        CAP_ENTRY__L(KVM_CAP_S390_BPB),
        CAP_ENTRY__L(KVM_CAP_GET_MSR_FEATURES),
        CAP_ENTRY__L(KVM_CAP_HYPERV_EVENTFD),
        CAP_ENTRY__L(KVM_CAP_HYPERV_TLBFLUSH),
        CAP_ENTRY__L(KVM_CAP_S390_HPAGE_1M),
        CAP_ENTRY__L(KVM_CAP_NESTED_STATE),
        CAP_ENTRY__L(KVM_CAP_ARM_INJECT_SERROR_ESR),
        CAP_ENTRY__L(KVM_CAP_MSR_PLATFORM_INFO),
        CAP_ENTRY__L(KVM_CAP_PPC_NESTED_HV),                 /* 160 */
        CAP_ENTRY__L(KVM_CAP_HYPERV_SEND_IPI),
        CAP_ENTRY__L(KVM_CAP_COALESCED_PIO),
        CAP_ENTRY__L(KVM_CAP_HYPERV_ENLIGHTENED_VMCS),
        CAP_ENTRY__L(KVM_CAP_EXCEPTION_PAYLOAD),
        CAP_ENTRY__L(KVM_CAP_ARM_VM_IPA_SIZE),
        CAP_ENTRY__L(KVM_CAP_MANUAL_DIRTY_LOG_PROTECT),
        CAP_ENTRY__L(KVM_CAP_HYPERV_CPUID),
        CAP_ENTRY__L(KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2),
        CAP_ENTRY__L(KVM_CAP_PPC_IRQ_XIVE),
        CAP_ENTRY__L(KVM_CAP_ARM_SVE),                       /* 170 */
        CAP_ENTRY__L(KVM_CAP_ARM_PTRAUTH_ADDRESS),
        CAP_ENTRY__L(KVM_CAP_ARM_PTRAUTH_GENERIC),
        CAP_ENTRY__L(KVM_CAP_PMU_EVENT_FILTER),
        CAP_ENTRY__L(KVM_CAP_ARM_IRQ_LINE_LAYOUT_2),
        CAP_ENTRY__L(KVM_CAP_HYPERV_DIRECT_TLBFLUSH),
        CAP_ENTRY__L(KVM_CAP_PPC_GUEST_DEBUG_SSTEP),
        CAP_ENTRY__L(KVM_CAP_ARM_NISV_TO_USER),
        CAP_ENTRY__L(KVM_CAP_ARM_INJECT_EXT_DABT),
        CAP_ENTRY__L(KVM_CAP_S390_VCPU_RESETS),
        CAP_ENTRY__L(KVM_CAP_S390_PROTECTED),                /* 180 */
        CAP_ENTRY__L(KVM_CAP_PPC_SECURE_GUEST),
        CAP_ENTRY__L(KVM_CAP_HALT_POLL),
        CAP_ENTRY__L(KVM_CAP_ASYNC_PF_INT),
        CAP_ENTRY__L(KVM_CAP_LAST_CPU),
        CAP_ENTRY__L(KVM_CAP_SMALLER_MAXPHYADDR),
        CAP_ENTRY__L(KVM_CAP_S390_DIAG318),
        CAP_ENTRY__L(KVM_CAP_STEAL_TIME),
        CAP_ENTRY_ML(KVM_CAP_X86_USER_SPACE_MSR),            /* (since 5.10) */
        CAP_ENTRY_ML(KVM_CAP_X86_MSR_FILTER),
        CAP_ENTRY__L(KVM_CAP_ENFORCE_PV_FEATURE_CPUID),      /* 190 */
        CAP_ENTRY__L(KVM_CAP_SYS_HYPERV_CPUID),
        CAP_ENTRY__L(KVM_CAP_DIRTY_LOG_RING),
        CAP_ENTRY__L(KVM_CAP_X86_BUS_LOCK_EXIT),
        CAP_ENTRY__L(KVM_CAP_PPC_DAWR1),
        CAP_ENTRY__L(KVM_CAP_SET_GUEST_DEBUG2),
        CAP_ENTRY__L(KVM_CAP_SGX_ATTRIBUTE),
        CAP_ENTRY__L(KVM_CAP_VM_COPY_ENC_CONTEXT_FROM),
        CAP_ENTRY__L(KVM_CAP_PTP_KVM),
        CAP_ENTRY__U(199),
        CAP_ENTRY__U(200),
        CAP_ENTRY__U(201),
        CAP_ENTRY__U(202),
        CAP_ENTRY__U(203),
        CAP_ENTRY__U(204),
        CAP_ENTRY__U(205),
        CAP_ENTRY__U(206),
        CAP_ENTRY__U(207),
        CAP_ENTRY__U(208),
        CAP_ENTRY__U(209),
        CAP_ENTRY__U(210),
        CAP_ENTRY__U(211),
        CAP_ENTRY__U(212),
        CAP_ENTRY__U(213),
        CAP_ENTRY__U(214),
        CAP_ENTRY__U(215),
        CAP_ENTRY__U(216),
    };

    LogRel(("NEM: KVM capabilities (system):\n"));
    int rcRet = VINF_SUCCESS;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aCaps); i++)
    {
        int rc = ioctl(pVM->nem.s.fdVm, KVM_CHECK_EXTENSION, s_aCaps[i].iCap);
        if (rc >= 10)
            LogRel(("NEM:   %36s: %#x (%d)\n", s_aCaps[i].pszName, rc, rc));
        else if (rc >= 0)
            LogRel(("NEM:   %36s: %d\n", s_aCaps[i].pszName, rc));
        else
            LogRel(("NEM:   %s failed: %d/%d\n", s_aCaps[i].pszName, rc, errno));
        switch (s_aCaps[i].cbNem)
        {
            case 0:
                break;
            case 1:
            {
                uint8_t *puValue = (uint8_t *)&pVM->nem.padding[s_aCaps[i].offNem];
                AssertReturn(s_aCaps[i].offNem <= sizeof(NEM) - sizeof(*puValue), VERR_NEM_IPE_0);
                *puValue = (uint8_t)rc;
                AssertLogRelMsg((int)*puValue == rc, ("%s: %#x\n", s_aCaps[i].pszName, rc));
                break;
            }
            case 2:
            {
                uint16_t *puValue = (uint16_t *)&pVM->nem.padding[s_aCaps[i].offNem];
                AssertReturn(s_aCaps[i].offNem <= sizeof(NEM) - sizeof(*puValue), VERR_NEM_IPE_0);
                *puValue = (uint16_t)rc;
                AssertLogRelMsg((int)*puValue == rc, ("%s: %#x\n", s_aCaps[i].pszName, rc));
                break;
            }
            case 4:
            {
                uint32_t *puValue = (uint32_t *)&pVM->nem.padding[s_aCaps[i].offNem];
                AssertReturn(s_aCaps[i].offNem <= sizeof(NEM) - sizeof(*puValue), VERR_NEM_IPE_0);
                *puValue = (uint32_t)rc;
                AssertLogRelMsg((int)*puValue == rc, ("%s: %#x\n", s_aCaps[i].pszName, rc));
                break;
            }
            default:
                rcRet = RTErrInfoSetF(pErrInfo, VERR_NEM_IPE_0, "s_aCaps[%u] is bad: cbNem=%#x - %s",
                                      i, s_aCaps[i].pszName, s_aCaps[i].cbNem);
                AssertFailedReturn(rcRet);
        }

        /*
         * Is a require non-zero entry zero or failing?
         */
        if (s_aCaps[i].fReqNonZero && rc <= 0)
            rcRet = RTERRINFO_LOG_REL_ADD_F(pErrInfo, VERR_NEM_MISSING_FEATURE,
                                            "Required capability '%s' is missing!", s_aCaps[i].pszName);
    }

    /*
     * Get per VCpu KVM_RUN MMAP area size.
     */
    int rc = ioctl(pVM->nem.s.fdKvm, KVM_GET_VCPU_MMAP_SIZE, 0UL);
    if ((unsigned)rc < _64M)
    {
        pVM->nem.s.cbVCpuMmap = (uint32_t)rc;
        LogRel(("NEM:   %36s: %#x (%d)\n", "KVM_GET_VCPU_MMAP_SIZE", rc, rc));
    }
    else if (rc < 0)
        rcRet = RTERRINFO_LOG_REL_ADD_F(pErrInfo, VERR_NEM_MISSING_FEATURE, "KVM_GET_VCPU_MMAP_SIZE failed: %d", errno);
    else
        rcRet = RTERRINFO_LOG_REL_ADD_F(pErrInfo, VERR_NEM_INIT_FAILED, "Odd KVM_GET_VCPU_MMAP_SIZE value: %#x (%d)", rc, rc);

    /*
     * Init the slot ID bitmap.
     */
    ASMBitSet(&pVM->nem.s.bmSlotIds[0], 0);         /* don't use slot 0 */
    if (pVM->nem.s.cMaxMemSlots < _32K)
        ASMBitSetRange(&pVM->nem.s.bmSlotIds[0], pVM->nem.s.cMaxMemSlots, _32K);
    ASMBitSet(&pVM->nem.s.bmSlotIds[0], _32K - 1);  /* don't use the last slot */

    return rcRet;
}


/**
 * Does the early setup of a KVM VM.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pErrInfo            Where to always return error info.
 */
static int nemR3LnxInitSetupVm(PVM pVM, PRTERRINFO pErrInfo)
{
    AssertReturn(pVM->nem.s.fdVm != -1, RTErrInfoSet(pErrInfo, VERR_WRONG_ORDER, "Wrong initalization order"));

    /*
     * Create the VCpus.
     */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];

        /* Create it. */
        pVCpu->nem.s.fdVCpu = ioctl(pVM->nem.s.fdVm, KVM_CREATE_VCPU, (unsigned long)idCpu);
        if (pVCpu->nem.s.fdVCpu < 0)
            return VMSetError(pVM, VERR_NEM_VM_CREATE_FAILED, RT_SRC_POS,
                              "KVM_CREATE_VCPU failed for VCpu #%u: %d", idCpu, errno);

        /* Map the KVM_RUN area. */
        pVCpu->nem.s.pRun = (struct kvm_run *)mmap(NULL, pVM->nem.s.cbVCpuMmap, PROT_READ | PROT_WRITE, MAP_SHARED,
                                                   pVCpu->nem.s.fdVCpu, 0 /*offset*/);
        if ((void *)pVCpu->nem.s.pRun == MAP_FAILED)
            return VMSetError(pVM, VERR_NEM_VM_CREATE_FAILED, RT_SRC_POS, "mmap failed for VCpu #%u: %d", idCpu, errno);

        /* We want all x86 registers and events on each exit. */
        pVCpu->nem.s.pRun->kvm_valid_regs = KVM_SYNC_X86_REGS | KVM_SYNC_X86_SREGS | KVM_SYNC_X86_EVENTS;
    }
    return VINF_SUCCESS;
}


/**
 * Try initialize the native API.
 *
 * This may only do part of the job, more can be done in
 * nemR3NativeInitAfterCPUM() and nemR3NativeInitCompleted().
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   fFallback       Whether we're in fallback mode or use-NEM mode. In
 *                          the latter we'll fail if we cannot initialize.
 * @param   fForced         Whether the HMForced flag is set and we should
 *                          fail if we cannot initialize.
 */
int nemR3NativeInit(PVM pVM, bool fFallback, bool fForced)
{
    RT_NOREF(pVM, fFallback, fForced);
    /*
     * Some state init.
     */
    pVM->nem.s.fdKvm = -1;
    pVM->nem.s.fdVm  = -1;
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PNEMCPU pNemCpu = &pVM->apCpusR3[idCpu]->nem.s;
        pNemCpu->fdVCpu = -1;
    }

    /*
     * Error state.
     * The error message will be non-empty on failure and 'rc' will be set too.
     */
    RTERRINFOSTATIC ErrInfo;
    PRTERRINFO pErrInfo = RTErrInfoInitStatic(&ErrInfo);

    /*
     * Open kvm subsystem so we can issue system ioctls.
     */
    int rc;
    int fdKvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (fdKvm >= 0)
    {
        pVM->nem.s.fdKvm = fdKvm;

        /*
         * Create an empty VM since it is recommended we check capabilities on
         * the VM rather than the system descriptor.
         */
        int fdVm = ioctl(fdKvm, KVM_CREATE_VM, 0UL /* Type must be zero on x86 */);
        if (fdVm >= 0)
        {
            pVM->nem.s.fdVm = fdVm;

            /*
             * Check capabilities.
             */
            rc = nemR3LnxInitCheckCapabilities(pVM, pErrInfo);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Set up the VM (more on this later).
                 */
                rc = nemR3LnxInitSetupVm(pVM, pErrInfo);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Set ourselves as the execution engine and make config adjustments.
                     */
                    VM_SET_MAIN_EXECUTION_ENGINE(pVM, VM_EXEC_ENGINE_NATIVE_API);
                    Log(("NEM: Marked active!\n"));
                    PGMR3EnableNemMode(pVM);

                    /*
                     * Register release statistics
                     */
                    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
                    {
                        PNEMCPU pNemCpu = &pVM->apCpusR3[idCpu]->nem.s;
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportOnDemand,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of on-demand state imports",      "/NEM/CPU%u/ImportOnDemand", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportOnReturn,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of state imports on loop return", "/NEM/CPU%u/ImportOnReturn", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportOnReturnSkipped, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of skipped state imports on loop return", "/NEM/CPU%u/ImportOnReturnSkipped", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatQueryCpuTick,        STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of TSC queries",                  "/NEM/CPU%u/QueryCpuTick", idCpu);
                    }

                    /*
                     * Success.
                     */
                    return VINF_SUCCESS;
                }

                /*
                 * Bail out.
                 */
            }
            close(fdVm);
            pVM->nem.s.fdVm = -1;
        }
        else
            rc = RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "KVM_CREATE_VM failed: %u", errno);
        close(fdKvm);
        pVM->nem.s.fdKvm = -1;
    }
    else if (errno == EACCES)
        rc = RTErrInfoSet(pErrInfo, VERR_ACCESS_DENIED, "Do not have access to open /dev/kvm for reading & writing.");
    else if (errno == ENOENT)
        rc = RTErrInfoSet(pErrInfo, VERR_NOT_SUPPORTED, "KVM is not availble (/dev/kvm does not exist)");
    else
        rc = RTErrInfoSetF(pErrInfo, RTErrConvertFromErrno(errno), "Failed to open '/dev/kvm': %u", errno);

    /*
     * We only fail if in forced mode, otherwise just log the complaint and return.
     */
    Assert(RTErrInfoIsSet(pErrInfo));
    if (   (fForced || !fFallback)
        && pVM->bMainExecutionEngine != VM_EXEC_ENGINE_NATIVE_API)
        return VMSetError(pVM, RT_SUCCESS_NP(rc) ? VERR_NEM_NOT_AVAILABLE : rc, RT_SRC_POS, "%s", pErrInfo->pszMsg);
    LogRel(("NEM: Not available: %s\n", pErrInfo->pszMsg));
    return VINF_SUCCESS;
}


/**
 * This is called after CPUMR3Init is done.
 *
 * @returns VBox status code.
 * @param   pVM                 The VM handle..
 */
int nemR3NativeInitAfterCPUM(PVM pVM)
{
    /*
     * Validate sanity.
     */
    AssertReturn(pVM->nem.s.fdKvm >= 0, VERR_WRONG_ORDER);
    AssertReturn(pVM->nem.s.fdVm >= 0, VERR_WRONG_ORDER);
    AssertReturn(pVM->bMainExecutionEngine == VM_EXEC_ENGINE_NATIVE_API, VERR_WRONG_ORDER);

    /** @todo */

    return VINF_SUCCESS;
}


int nemR3NativeInitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
    RT_NOREF(pVM, enmWhat);
    return VINF_SUCCESS;
}


int nemR3NativeTerm(PVM pVM)
{
    /*
     * Per-cpu data
     */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];

        if (pVCpu->nem.s.fdVCpu != -1)
        {
            close(pVCpu->nem.s.fdVCpu);
            pVCpu->nem.s.fdVCpu = -1;
        }
        if (pVCpu->nem.s.pRun)
        {
            munmap(pVCpu->nem.s.pRun, pVM->nem.s.cbVCpuMmap);
            pVCpu->nem.s.pRun = NULL;
        }
    }

    /*
     * Global data.
     */
    if (pVM->nem.s.fdVm != -1)
    {
        close(pVM->nem.s.fdVm);
        pVM->nem.s.fdVm = -1;
    }

    if (pVM->nem.s.fdKvm != -1)
    {
        close(pVM->nem.s.fdKvm);
        pVM->nem.s.fdKvm = -1;
    }
    return VINF_SUCCESS;
}


/**
 * VM reset notification.
 *
 * @param   pVM         The cross context VM structure.
 */
void nemR3NativeReset(PVM pVM)
{
    RT_NOREF(pVM);
}


/**
 * Reset CPU due to INIT IPI or hot (un)plugging.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the CPU being
 *                      reset.
 * @param   fInitIpi    Whether this is the INIT IPI or hot (un)plugging case.
 */
void nemR3NativeResetCpu(PVMCPU pVCpu, bool fInitIpi)
{
    RT_NOREF(pVCpu, fInitIpi);
}


/*********************************************************************************************************************************
*   Memory management                                                                                                            *
*********************************************************************************************************************************/


/**
 * Allocates a memory slot ID.
 *
 * @returns Slot ID on success, UINT16_MAX on failure.
 */
static uint16_t nemR3LnxMemSlotIdAlloc(PVM pVM)
{
    /* Use the hint first. */
    uint16_t idHint = pVM->nem.s.idPrevSlot;
    if (idHint < _32K - 1)
    {
        int32_t idx = ASMBitNextClear(&pVM->nem.s.bmSlotIds, _32K, idHint);
        Assert(idx < _32K);
        if (idx > 0 && !ASMAtomicBitTestAndSet(&pVM->nem.s.bmSlotIds, idx))
            return pVM->nem.s.idPrevSlot = (uint16_t)idx;
    }

    /*
     * Search the whole map from the start.
     */
    int32_t idx = ASMBitFirstClear(&pVM->nem.s.bmSlotIds, _32K);
    Assert(idx < _32K);
    if (idx > 0 && !ASMAtomicBitTestAndSet(&pVM->nem.s.bmSlotIds, idx))
        return pVM->nem.s.idPrevSlot = (uint16_t)idx;

    Assert(idx < 0 /*shouldn't trigger unless there is a race */);
    return UINT16_MAX; /* caller is expected to assert. */
}


/**
 * Frees a memory slot ID
 */
static void nemR3LnxMemSlotIdFree(PVM pVM, uint16_t idSlot)
{
    if (RT_LIKELY(idSlot < _32K && ASMAtomicBitTestAndClear(&pVM->nem.s.bmSlotIds, idSlot)))
    { /*likely*/ }
    else
        AssertMsgFailed(("idSlot=%u (%#x)\n", idSlot, idSlot));
}



VMMR3_INT_DECL(int) NEMR3NotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvR3,
                                               uint8_t *pu2State, uint32_t *puNemRange)
{
    uint16_t idSlot = nemR3LnxMemSlotIdAlloc(pVM);
    AssertLogRelReturn(idSlot < _32K, VERR_NEM_MAP_PAGES_FAILED);

    Log5(("NEMR3NotifyPhysRamRegister: %RGp LB %RGp, pvR3=%p pu2State=%p (%d) puNemRange=%p (%d) - idSlot=%#x\n",
          GCPhys, cb, pvR3, pu2State, pu2State, puNemRange, *puNemRange, idSlot));

    struct kvm_userspace_memory_region Region;
    Region.slot             = idSlot;
    Region.flags            = 0;
    Region.guest_phys_addr  = GCPhys;
    Region.memory_size      = cb;
    Region.userspace_addr   = (uintptr_t)pvR3;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
    if (rc == 0)
    {
        *pu2State   = 0;
        *puNemRange = idSlot;
        return VINF_SUCCESS;
    }

    LogRel(("NEMR3NotifyPhysRamRegister: %RGp LB %RGp, pvR3=%p, idSlot=%#x failed: %u/%u\n", GCPhys, cb, pvR3, idSlot, rc, errno));
    nemR3LnxMemSlotIdFree(pVM, idSlot);
    return VERR_NEM_MAP_PAGES_FAILED;
}


VMMR3_INT_DECL(bool) NEMR3IsMmio2DirtyPageTrackingSupported(PVM pVM)
{
    RT_NOREF(pVM);
    return true;
}


VMMR3_INT_DECL(int) NEMR3NotifyPhysMmioExMapEarly(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                  void *pvRam, void *pvMmio2, uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysMmioExMapEarly: %RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p pu2State=%p (%d) puNemRange=%p (%#x)\n",
          GCPhys, cb, fFlags, pvRam, pvMmio2, pu2State, *pu2State, puNemRange, puNemRange ? *puNemRange : UINT32_MAX));
    RT_NOREF(pvRam);

    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE)
    {
        /** @todo implement splitting and whatnot of ranges if we want to be 100%
         *        conforming (just modify RAM registrations in MM.cpp to test). */
        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p\n", GCPhys, cb, fFlags, pvRam, pvMmio2),
                                    VERR_NEM_MAP_PAGES_FAILED);
    }

    /*
     * Register MMIO2.
     */
    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2)
    {
        AssertReturn(pvMmio2, VERR_NEM_MAP_PAGES_FAILED);
        AssertReturn(puNemRange, VERR_NEM_MAP_PAGES_FAILED);

        uint16_t idSlot = nemR3LnxMemSlotIdAlloc(pVM);
        AssertLogRelReturn(idSlot < _32K, VERR_NEM_MAP_PAGES_FAILED);

        struct kvm_userspace_memory_region Region;
        Region.slot             = idSlot;
        Region.flags            = fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES ? KVM_MEM_LOG_DIRTY_PAGES : 0;
        Region.guest_phys_addr  = GCPhys;
        Region.memory_size      = cb;
        Region.userspace_addr   = (uintptr_t)pvMmio2;

        int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
        if (rc == 0)
        {
            *pu2State   = 0;
            *puNemRange = idSlot;
            Log5(("NEMR3NotifyPhysMmioExMapEarly: %RGp LB %RGp fFlags=%#x pvMmio2=%p - idSlot=%#x\n",
                  GCPhys, cb, fFlags, pvMmio2, idSlot));
            return VINF_SUCCESS;
        }

        nemR3LnxMemSlotIdFree(pVM, idSlot);
        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x, pvMmio2=%p, idSlot=%#x failed: %u/%u\n",
                                     GCPhys, cb, fFlags, pvMmio2, idSlot, errno, rc),
                                    VERR_NEM_MAP_PAGES_FAILED);
    }

    /* MMIO, don't care. */
    *pu2State   = 0;
    *puNemRange = UINT32_MAX;
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3NotifyPhysMmioExMapLate(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                 void *pvRam, void *pvMmio2, uint32_t *puNemRange)
{
    RT_NOREF(pVM, GCPhys, cb, fFlags, pvRam, pvMmio2, puNemRange);
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3NotifyPhysMmioExUnmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags, void *pvRam,
                                               void *pvMmio2, uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysMmioExUnmap: %RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p pu2State=%p puNemRange=%p (%#x)\n",
          GCPhys, cb, fFlags, pvRam, pvMmio2, pu2State, puNemRange, *puNemRange));
    RT_NOREF(pVM, GCPhys, cb, fFlags, pvRam, pvMmio2, pu2State);

    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE)
    {
        /** @todo implement splitting and whatnot of ranges if we want to be 100%
         *        conforming (just modify RAM registrations in MM.cpp to test). */
        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p\n", GCPhys, cb, fFlags, pvRam, pvMmio2),
                                    VERR_NEM_UNMAP_PAGES_FAILED);
    }

    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2)
    {
        uint32_t const idSlot = *puNemRange;
        AssertReturn(idSlot > 0 && idSlot < _32K, VERR_NEM_IPE_4);
        AssertReturn(ASMBitTest(pVM->nem.s.bmSlotIds, idSlot), VERR_NEM_IPE_4);

        struct kvm_userspace_memory_region Region;
        Region.slot             = idSlot;
        Region.flags            = 0;
        Region.guest_phys_addr  = GCPhys;
        Region.memory_size      = 0;    /* this deregisters it. */
        Region.userspace_addr   = (uintptr_t)pvMmio2;

        int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
        if (rc == 0)
        {
            if (pu2State)
                *pu2State = 0;
            *puNemRange = UINT32_MAX;
            nemR3LnxMemSlotIdFree(pVM, idSlot);
            return VINF_SUCCESS;
        }

        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x, pvMmio2=%p, idSlot=%#x failed: %u/%u\n",
                                     GCPhys, cb, fFlags, pvMmio2, idSlot, errno, rc),
                                    VERR_NEM_UNMAP_PAGES_FAILED);
    }

    if (pu2State)
        *pu2State = UINT8_MAX;
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3PhysMmio2QueryAndResetDirtyBitmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t uNemRange,
                                                           void *pvBitmap, size_t cbBitmap)
{
    AssertReturn(uNemRange > 0 && uNemRange < _32K, VERR_NEM_IPE_4);
    AssertReturn(ASMBitTest(pVM->nem.s.bmSlotIds, uNemRange), VERR_NEM_IPE_4);

    RT_NOREF(GCPhys, cbBitmap);

    struct kvm_dirty_log DirtyLog;
    DirtyLog.slot         = uNemRange;
    DirtyLog.padding1     = 0;
    DirtyLog.dirty_bitmap = pvBitmap;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_GET_DIRTY_LOG, &DirtyLog);
    AssertLogRelMsgReturn(rc == 0, ("%RGp LB %RGp idSlot=%#x failed: %u/%u\n", GCPhys, cb, uNemRange, errno, rc),
                          VERR_NEM_QUERY_DIRTY_BITMAP_FAILED);

    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int)  NEMR3NotifyPhysRomRegisterEarly(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvPages, uint32_t fFlags,
                                                     uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysRomRegisterEarly: %RGp LB %RGp pvPages=%p fFlags=%#x\n", GCPhys, cb, pvPages, fFlags));
    *pu2State = UINT8_MAX;

    /* Don't support puttint ROM where there is already RAM.  For
       now just shuffle the registrations till it works... */
    AssertLogRelMsgReturn(!(fFlags & NEM_NOTIFY_PHYS_ROM_F_REPLACE), ("%RGp LB %RGp fFlags=%#x\n", GCPhys, cb, fFlags),
                          VERR_NEM_MAP_PAGES_FAILED);

    /** @todo figure out how to do shadow ROMs.   */

    /*
     * We only allocate a slot number here in case we need to use it to
     * fend of physical handler fun.
     */
    uint16_t idSlot = nemR3LnxMemSlotIdAlloc(pVM);
    AssertLogRelReturn(idSlot < _32K, VERR_NEM_MAP_PAGES_FAILED);

    *pu2State   = 0;
    *puNemRange = idSlot;
    Log5(("NEMR3NotifyPhysRomRegisterEarly: %RGp LB %RGp fFlags=%#x pvPages=%p - idSlot=%#x\n",
          GCPhys, cb, fFlags, pvPages, idSlot));
    RT_NOREF(GCPhys, cb, fFlags, pvPages);
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int)  NEMR3NotifyPhysRomRegisterLate(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvPages,
                                                    uint32_t fFlags, uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysRomRegisterLate: %RGp LB %RGp pvPages=%p fFlags=%#x pu2State=%p (%d) puNemRange=%p (%#x)\n",
          GCPhys, cb, pvPages, fFlags, pu2State, *pu2State, puNemRange, *puNemRange));

    AssertPtrReturn(pvPages, VERR_NEM_IPE_5);

    uint32_t const idSlot = *puNemRange;
    AssertReturn(idSlot > 0 && idSlot < _32K, VERR_NEM_IPE_4);
    AssertReturn(ASMBitTest(pVM->nem.s.bmSlotIds, idSlot), VERR_NEM_IPE_4);

    *pu2State = UINT8_MAX;

    /*
     * Do the actual setting of the user pages here now that we've
     * got a valid pvPages (typically isn't available during the early
     * notification, unless we're replacing RAM).
     */
    struct kvm_userspace_memory_region Region;
    Region.slot             = idSlot;
    Region.flags            = 0;
    Region.guest_phys_addr  = GCPhys;
    Region.memory_size      = cb;
    Region.userspace_addr   = (uintptr_t)pvPages;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
    if (rc == 0)
    {
        *pu2State   = 0;
        Log5(("NEMR3NotifyPhysRomRegisterEarly: %RGp LB %RGp fFlags=%#x pvPages=%p - idSlot=%#x\n",
              GCPhys, cb, fFlags, pvPages, idSlot));
        return VINF_SUCCESS;
    }
    AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x, pvPages=%p, idSlot=%#x failed: %u/%u\n",
                                 GCPhys, cb, fFlags, pvPages, idSlot, errno, rc),
                                VERR_NEM_MAP_PAGES_FAILED);
}


/**
 * Called when the A20 state changes.
 *
 * @param   pVCpu           The CPU the A20 state changed on.
 * @param   fEnabled        Whether it was enabled (true) or disabled.
 */
VMMR3_INT_DECL(void) NEMR3NotifySetA20(PVMCPU pVCpu, bool fEnabled)
{
    Log(("nemR3NativeNotifySetA20: fEnabled=%RTbool\n", fEnabled));
    Assert(VM_IS_NEM_ENABLED(pVCpu->CTX_SUFF(pVM)));
    RT_NOREF(pVCpu, fEnabled);
}


VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalDeregister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                                        RTR3PTR pvMemR3, uint8_t *pu2State)
{
    Log5(("NEMHCNotifyHandlerPhysicalDeregister: %RGp LB %RGp enmKind=%d pvMemR3=%p pu2State=%p (%d)\n",
          GCPhys, cb, enmKind, pvMemR3, pu2State, *pu2State));

    *pu2State = UINT8_MAX;
    RT_NOREF(pVM, enmKind, GCPhys, cb, pvMemR3);
}


void nemHCNativeNotifyHandlerPhysicalRegister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalRegister: %RGp LB %RGp enmKind=%d\n", GCPhys, cb, enmKind));
    RT_NOREF(pVM, enmKind, GCPhys, cb);
}


void nemHCNativeNotifyHandlerPhysicalModify(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld,
                                            RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fRestoreAsRAM)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalModify: %RGp LB %RGp -> %RGp enmKind=%d fRestoreAsRAM=%d\n",
          GCPhysOld, cb, GCPhysNew, enmKind, fRestoreAsRAM));
    RT_NOREF(pVM, enmKind, GCPhysOld, GCPhysNew, cb, fRestoreAsRAM);
}


int nemHCNativeNotifyPhysPageAllocated(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint32_t fPageProt,
                                       PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageAllocated: %RGp HCPhys=%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhys, fPageProt, enmType, *pu2State));
    RT_NOREF(pVM, GCPhys, HCPhys, fPageProt, enmType, pu2State);
    return VINF_SUCCESS;
}


VMM_INT_DECL(void) NEMHCNotifyPhysPageProtChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, RTR3PTR pvR3, uint32_t fPageProt,
                                                  PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("NEMHCNotifyPhysPageProtChanged: %RGp HCPhys=%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhys, fPageProt, enmType, *pu2State));
    Assert(VM_IS_NEM_ENABLED(pVM));
    RT_NOREF(pVM, GCPhys, HCPhys, pvR3, fPageProt, enmType, pu2State);

}


VMM_INT_DECL(void) NEMHCNotifyPhysPageChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhysPrev, RTHCPHYS HCPhysNew,
                                              RTR3PTR pvNewR3, uint32_t fPageProt, PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageChanged: %RGp HCPhys=%RHp->%RHp pvNewR3=%p fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhysPrev, HCPhysNew, pvNewR3, fPageProt, enmType, *pu2State));
    Assert(VM_IS_NEM_ENABLED(pVM));
    RT_NOREF(pVM, GCPhys, HCPhysPrev, HCPhysNew, pvNewR3, fPageProt, enmType, pu2State);
}


/*********************************************************************************************************************************
*   CPU State                                                                                                                    *
*********************************************************************************************************************************/

/**
 * Worker that imports selected state from KVM.
 */
static int nemHCLnxImportState(PVMCPUCC pVCpu, uint64_t fWhat, struct kvm_run *pRun)
{
    RT_NOREF(pVCpu, fWhat, pRun);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Interface for importing state on demand (used by IEM).
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   fWhat       What to import, CPUMCTX_EXTRN_XXX.
 */
VMM_INT_DECL(int) NEMImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnDemand);

    RT_NOREF(pVCpu, fWhat);
    return nemHCLnxImportState(pVCpu, fWhat, pVCpu->nem.s.pRun);
}


/**
 * Exports state to KVM.
 */
static int nemHCLnxExportState(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, struct kvm_run *pRun)
{
    uint64_t const fExtrn = pCtx->fExtrn;
    Assert((fExtrn & CPUMCTX_EXTRN_ALL) != CPUMCTX_EXTRN_ALL);

    /*
     * Stuff that goes into kvm_run::s.regs.regs:
     */
    if (   (fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_GPRS_MASK))
        !=           (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_GPRS_MASK))
    {
        if (!(fExtrn & CPUMCTX_EXTRN_RIP))
            pRun->s.regs.regs.rip    = pCtx->rip;
        if (!(fExtrn & CPUMCTX_EXTRN_RFLAGS))
            pRun->s.regs.regs.rflags = pCtx->rflags.u;

        if (!(fExtrn & CPUMCTX_EXTRN_RAX))
            pRun->s.regs.regs.rax    = pCtx->rax;
        if (!(fExtrn & CPUMCTX_EXTRN_RCX))
            pRun->s.regs.regs.rcx    = pCtx->rcx;
        if (!(fExtrn & CPUMCTX_EXTRN_RDX))
            pRun->s.regs.regs.rdx    = pCtx->rdx;
        if (!(fExtrn & CPUMCTX_EXTRN_RBX))
            pRun->s.regs.regs.rbx    = pCtx->rbx;
        if (!(fExtrn & CPUMCTX_EXTRN_RSP))
            pRun->s.regs.regs.rsp    = pCtx->rsp;
        if (!(fExtrn & CPUMCTX_EXTRN_RBP))
            pRun->s.regs.regs.rbp    = pCtx->rbp;
        if (!(fExtrn & CPUMCTX_EXTRN_RSI))
            pRun->s.regs.regs.rsi    = pCtx->rsi;
        if (!(fExtrn & CPUMCTX_EXTRN_RDI))
            pRun->s.regs.regs.rdi    = pCtx->rdi;
        if (!(fExtrn & CPUMCTX_EXTRN_R8_R15))
        {
            pRun->s.regs.regs.r8     = pCtx->r8;
            pRun->s.regs.regs.r9     = pCtx->r9;
            pRun->s.regs.regs.r10    = pCtx->r10;
            pRun->s.regs.regs.r11    = pCtx->r11;
            pRun->s.regs.regs.r12    = pCtx->r12;
            pRun->s.regs.regs.r13    = pCtx->r13;
            pRun->s.regs.regs.r14    = pCtx->r14;
            pRun->s.regs.regs.r15    = pCtx->r15;
        }
        pRun->kvm_dirty_regs |= KVM_SYNC_X86_REGS;
    }

    /*
     * Stuff that goes into kvm_run::s.regs.sregs:
     */
    /** @todo apic_base   */
    if (   (fExtrn & (CPUMCTX_EXTRN_SREG_MASK | CPUMCTX_EXTRN_TABLE_MASK | CPUMCTX_EXTRN_CR_MASK | CPUMCTX_EXTRN_EFER | CPUMCTX_EXTRN_APIC_TPR))
        !=           (CPUMCTX_EXTRN_SREG_MASK | CPUMCTX_EXTRN_TABLE_MASK | CPUMCTX_EXTRN_CR_MASK | CPUMCTX_EXTRN_EFER | CPUMCTX_EXTRN_APIC_TPR))
    {
#define NEM_LNX_EXPORT_SEG(a_KvmSeg, a_CtxSeg) do { \
            (a_KvmSeg).base     = (a_CtxSeg).u64Base; \
            (a_KvmSeg).limit    = (a_CtxSeg).u32Limit; \
            (a_KvmSeg).selector = (a_CtxSeg).Sel; \
            (a_KvmSeg).type     = (a_CtxSeg).Attr.n.u4Type; \
            (a_KvmSeg).s        = (a_CtxSeg).Attr.n.u1DescType; \
            (a_KvmSeg).dpl      = (a_CtxSeg).Attr.n.u2Dpl; \
            (a_KvmSeg).present  = (a_CtxSeg).Attr.n.u1Present; \
            (a_KvmSeg).avl      = (a_CtxSeg).Attr.n.u1Available; \
            (a_KvmSeg).l        = (a_CtxSeg).Attr.n.u1Long; \
            (a_KvmSeg).db       = (a_CtxSeg).Attr.n.u1DefBig; \
            (a_KvmSeg).g        = (a_CtxSeg).Attr.n.u1Granularity; \
            (a_KvmSeg).unusable = (a_CtxSeg).Attr.n.u1Unusable; \
            (a_KvmSeg).padding  = 0; \
        } while (0)

        if ((fExtrn & CPUMCTX_EXTRN_SREG_MASK) != CPUMCTX_EXTRN_SREG_MASK)
        {
            if (!(fExtrn & CPUMCTX_EXTRN_ES))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.es, pCtx->es);
            if (!(fExtrn & CPUMCTX_EXTRN_CS))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.cs, pCtx->cs);
            if (!(fExtrn & CPUMCTX_EXTRN_SS))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.ss, pCtx->ss);
            if (!(fExtrn & CPUMCTX_EXTRN_DS))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.ds, pCtx->ds);
            if (!(fExtrn & CPUMCTX_EXTRN_FS))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.fs, pCtx->fs);
            if (!(fExtrn & CPUMCTX_EXTRN_GS))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.gs, pCtx->gs);
        }
        if ((fExtrn & CPUMCTX_EXTRN_TABLE_MASK) != CPUMCTX_EXTRN_TABLE_MASK)
        {
            if (!(fExtrn & CPUMCTX_EXTRN_GDTR))
            {
                pRun->s.regs.sregs.gdt.base  = pCtx->gdtr.pGdt;
                pRun->s.regs.sregs.gdt.limit = pCtx->gdtr.cbGdt;
                pRun->s.regs.sregs.gdt.padding[0] = 0;
                pRun->s.regs.sregs.gdt.padding[1] = 0;
                pRun->s.regs.sregs.gdt.padding[2] = 0;
            }
            if (!(fExtrn & CPUMCTX_EXTRN_IDTR))
            {
                pRun->s.regs.sregs.idt.base  = pCtx->idtr.pIdt;
                pRun->s.regs.sregs.idt.limit = pCtx->idtr.cbIdt;
                pRun->s.regs.sregs.idt.padding[0] = 0;
                pRun->s.regs.sregs.idt.padding[1] = 0;
                pRun->s.regs.sregs.idt.padding[2] = 0;
            }
            if (!(fExtrn & CPUMCTX_EXTRN_LDTR))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.ldt, pCtx->ldtr);
            if (!(fExtrn & CPUMCTX_EXTRN_TR))
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.tr, pCtx->tr);
        }
        if ((fExtrn & CPUMCTX_EXTRN_CR_MASK) != CPUMCTX_EXTRN_CR_MASK)
        {
            if (!(fExtrn & CPUMCTX_EXTRN_CR0))
                pRun->s.regs.sregs.cr0   = pCtx->cr0;
            if (!(fExtrn & CPUMCTX_EXTRN_CR2))
                pRun->s.regs.sregs.cr2   = pCtx->cr2;
            if (!(fExtrn & CPUMCTX_EXTRN_CR3))
                pRun->s.regs.sregs.cr3   = pCtx->cr3;
            if (!(fExtrn & CPUMCTX_EXTRN_CR4))
                pRun->s.regs.sregs.cr4   = pCtx->cr4;
        }
        if (!(fExtrn & CPUMCTX_EXTRN_APIC_TPR))
            pRun->s.regs.sregs.cr8    = CPUMGetGuestCR8(pVCpu);
        if (!(fExtrn & CPUMCTX_EXTRN_EFER))
            pRun->s.regs.sregs.efer   = pCtx->msrEFER;

        /** @todo apic_base   */
        /** @todo interrupt_bitmap - IRQ injection?  */
        pRun->kvm_dirty_regs |= KVM_SYNC_X86_SREGS;
    }

    /*
     * Debug registers.
     */
    if ((fExtrn & CPUMCTX_EXTRN_DR_MASK) != CPUMCTX_EXTRN_DR_MASK)
    {
        struct kvm_debugregs DbgRegs = {{0}};

        if (fExtrn & CPUMCTX_EXTRN_DR_MASK)
        {
            /* Partial debug state, we must get DbgRegs first so we can merge: */
            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_DEBUGREGS, &DbgRegs);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }

        if (!(fExtrn & CPUMCTX_EXTRN_DR0_DR3))
        {
            DbgRegs.db[0] = pCtx->dr[0];
            DbgRegs.db[1] = pCtx->dr[1];
            DbgRegs.db[2] = pCtx->dr[2];
            DbgRegs.db[3] = pCtx->dr[3];
        }
        if (!(fExtrn & CPUMCTX_EXTRN_DR6))
            DbgRegs.dr6 = pCtx->dr[6];
        if (!(fExtrn & CPUMCTX_EXTRN_DR7))
            DbgRegs.dr7 = pCtx->dr[7];

        int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_DEBUGREGS, &DbgRegs);
        AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
    }

    /*
     * FPU, SSE, AVX, ++.
     */
    if (   (fExtrn & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx))
        !=           (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx))
    {
        if (   (fExtrn & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE))
            !=           (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE))
        {
            if (fExtrn & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE))
            {
                /* Partial state is annoying as we have to do merging - is this possible at all? */
                struct kvm_xsave XSave;
                int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_XSAVE, &XSave);
                AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);

                if (!(fExtrn & CPUMCTX_EXTRN_X87))
                    memcpy(&pCtx->XState.x87, &XSave, sizeof(pCtx->XState.x87));
                if (!(fExtrn & CPUMCTX_EXTRN_SSE_AVX))
                {
                    /** @todo    */
                }
                if (!(fExtrn & CPUMCTX_EXTRN_OTHER_XSAVE))
                {
                    /** @todo   */
                }
            }

            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_XSAVE, &pCtx->XState);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }

        if (!(fExtrn & CPUMCTX_EXTRN_XCRx))
        {
            struct kvm_xcrs Xcrs =
            {   /*.nr_xcrs = */ 2,
                /*.flags = */   0,
                /*.xcrs= */ {
                    { /*.xcr =*/ 0, /*.reserved=*/ 0, /*.value=*/ pCtx->aXcr[0] },
                    { /*.xcr =*/ 1, /*.reserved=*/ 0, /*.value=*/ pCtx->aXcr[1] },
                }
            };

            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_XCRS, &Xcrs);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }
    }

    /*
     * MSRs.
     */
    if (   (fExtrn & (CPUMCTX_EXTRN_KERNEL_GS_BASE | CPUMCTX_EXTRN_SYSCALL_MSRS | CPUMCTX_EXTRN_SYSENTER_MSRS | CPUMCTX_EXTRN_TSC_AUX | CPUMCTX_EXTRN_OTHER_MSRS))
        !=           (CPUMCTX_EXTRN_KERNEL_GS_BASE | CPUMCTX_EXTRN_SYSCALL_MSRS | CPUMCTX_EXTRN_SYSENTER_MSRS | CPUMCTX_EXTRN_TSC_AUX | CPUMCTX_EXTRN_OTHER_MSRS))
    {
        union
        {
            struct kvm_msrs Core;
            uint64_t padding[2 + sizeof(struct kvm_msr_entry) * 32];
        }                   uBuf;
        uint32_t            iMsr     = 0;
        PCPUMCTXMSRS const  pCtxMsrs = CPUMQueryGuestCtxMsrsPtr(pVCpu);

#define ADD_MSR(a_Msr, a_uValue) do { \
            Assert(iMsr < 32); \
            uBuf.Core.entries[iMsr].index    = (a_Msr); \
            uBuf.Core.entries[iMsr].reserved = 0; \
            uBuf.Core.entries[iMsr].data     = (a_uValue); \
            iMsr += 1; \
        } while (0)

        if (!(fExtrn & CPUMCTX_EXTRN_KERNEL_GS_BASE))
            ADD_MSR(MSR_K8_KERNEL_GS_BASE, pCtx->msrKERNELGSBASE);
        if (!(fExtrn & CPUMCTX_EXTRN_SYSCALL_MSRS))
        {
            ADD_MSR(MSR_K6_STAR,    pCtx->msrSTAR);
            ADD_MSR(MSR_K8_LSTAR,   pCtx->msrLSTAR);
            ADD_MSR(MSR_K8_CSTAR,   pCtx->msrCSTAR);
            ADD_MSR(MSR_K8_SF_MASK, pCtx->msrSFMASK);
        }
        if (!(fExtrn & CPUMCTX_EXTRN_SYSENTER_MSRS))
        {
            ADD_MSR(MSR_IA32_SYSENTER_CS,  pCtx->SysEnter.cs);
            ADD_MSR(MSR_IA32_SYSENTER_EIP, pCtx->SysEnter.eip);
            ADD_MSR(MSR_IA32_SYSENTER_ESP, pCtx->SysEnter.esp);
        }
        if (!(fExtrn & CPUMCTX_EXTRN_TSC_AUX))
            ADD_MSR(MSR_K8_TSC_AUX, pCtxMsrs->msr.TscAux);
        if (!(fExtrn & CPUMCTX_EXTRN_OTHER_MSRS))
        {
            ADD_MSR(MSR_IA32_CR_PAT, pCtx->msrPAT);
            /** @todo What do we _have_ to add here?
             * We also have: Mttr*, MiscEnable, FeatureControl. */
        }

        uBuf.Core.pad   = 0;
        uBuf.Core.nmsrs = iMsr;
        int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_MSRS, &uBuf);
        AssertMsgReturn(rc == (int)iMsr,
                        ("rc=%d iMsr=%d (->%#x) errno=%d\n",
                         rc, iMsr, (uint32_t)rc < iMsr ? uBuf.Core.entries[rc].index : 0, errno),
                        VERR_NEM_IPE_3);
    }

    /*
     * KVM now owns all the state.
     */
    pCtx->fExtrn = (fExtrn & ~CPUMCTX_EXTRN_KEEPER_MASK) | CPUMCTX_EXTRN_KEEPER_NEM | CPUMCTX_EXTRN_ALL;

    RT_NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * Query the CPU tick counter and optionally the TSC_AUX MSR value.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   pcTicks     Where to return the CPU tick count.
 * @param   puAux       Where to return the TSC_AUX register value.
 */
VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, uint64_t *pcTicks, uint32_t *puAux)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatQueryCpuTick);
    // KVM_GET_CLOCK?
    RT_NOREF(pVCpu, pcTicks, puAux);
    return VINF_SUCCESS;
}


/**
 * Resumes CPU clock (TSC) on all virtual CPUs.
 *
 * This is called by TM when the VM is started, restored, resumed or similar.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context CPU structure of the calling EMT.
 * @param   uPausedTscValue The TSC value at the time of pausing.
 */
VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu, uint64_t uPausedTscValue)
{
    // KVM_SET_CLOCK?
    RT_NOREF(pVM, pVCpu, uPausedTscValue);
    return VINF_SUCCESS;
}


VMM_INT_DECL(uint32_t) NEMHCGetFeatures(PVMCC pVM)
{
    RT_NOREF(pVM);
    return NEM_FEAT_F_NESTED_PAGING
         | NEM_FEAT_F_FULL_GST_EXEC
         | NEM_FEAT_F_XSAVE_XRSTOR;
}



/*********************************************************************************************************************************
*   Execution                                                                                                                    *
*********************************************************************************************************************************/


VMMR3_INT_DECL(bool) NEMR3CanExecuteGuest(PVM pVM, PVMCPU pVCpu)
{
    /*
     * Only execute when the A20 gate is enabled as I cannot immediately
     * spot any A20 support in KVM.
     */
    RT_NOREF(pVM);
    Assert(VM_IS_NEM_ENABLED(pVM));
    return PGMPhysIsA20Enabled(pVCpu);
}


bool nemR3NativeSetSingleInstruction(PVM pVM, PVMCPU pVCpu, bool fEnable)
{
    NOREF(pVM); NOREF(pVCpu); NOREF(fEnable);
    return false;
}


/**
 * Forced flag notification call from VMEmt.cpp.
 *
 * This is only called when pVCpu is in the VMCPUSTATE_STARTED_EXEC_NEM state.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the CPU
 *                          to be notified.
 * @param   fFlags          Notification flags, VMNOTIFYFF_FLAGS_XXX.
 */
void nemR3NativeNotifyFF(PVM pVM, PVMCPU pVCpu, uint32_t fFlags)
{
    RT_NOREF(pVM, pVCpu, fFlags);
}


static VBOXSTRICTRC nemHCLnxHandleInterruptFF(PVM pVM, PVMCPU pVCpu)
{
    RT_NOREF(pVM, pVCpu);
    return VINF_SUCCESS;
}


static VBOXSTRICTRC nemHCLnxHandleExitIo(PVMCC pVM, PVMCPUCC pVCpu, struct kvm_run *pRun)
{
    /*
     * Input validation.
     */
    Assert(pRun->io.count > 0);
    Assert(pRun->io.size == 1 || pRun->io.size == 2 || pRun->io.size == 4);
    Assert(pRun->io.direction == KVM_EXIT_IO_IN || pRun->io.direction == KVM_EXIT_IO_OUT);
    Assert(pRun->io.data_offset < pVM->nem.s.cbVCpuMmap);
    Assert(pRun->io.data_offset + pRun->io.size * pRun->io.count <= pVM->nem.s.cbVCpuMmap);

    /*
     * Do the requested job.
     */
    VBOXSTRICTRC    rcStrict;
    RTPTRUNION      uPtrData;
    uPtrData.pu8 = (uint8_t *)pRun + pRun->io.data_offset;
    if (pRun->io.count == 1)
    {
        if (pRun->io.direction == KVM_EXIT_IO_IN)
        {
            uint32_t uValue = 0;
            rcStrict = IOMIOPortRead(pVM, pVCpu, pRun->io.port, &uValue, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: IN %#x LB %u -> %#x, rcStrict=%Rrc\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, pRun->io.size, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
            if (IOM_SUCCESS(rcStrict))
            {
                if (pRun->io.size == 4)
                    *uPtrData.pu32 = uValue;
                else if (pRun->io.size == 2)
                    *uPtrData.pu16 = (uint16_t)uValue;
                else
                    *uPtrData.pu8  = (uint8_t)uValue;
            }
        }
        else
        {
            uint32_t const uValue = pRun->io.size == 4 ? *uPtrData.pu32
                                  : pRun->io.size == 2 ? *uPtrData.pu16
                                  :                      *uPtrData.pu8;
            rcStrict = IOMIOPortWrite(pVM, pVCpu, pRun->io.port, uValue, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: OUT %#x, %#x LB %u rcStrict=%Rrc\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, uValue, pRun->io.size, VBOXSTRICTRC_VAL(rcStrict) ));
        }
    }
    else
    {
        uint32_t cTransfers = pRun->io.count;
        if (pRun->io.direction == KVM_EXIT_IO_IN)
        {
            rcStrict = IOMIOPortReadString(pVM, pVCpu, pRun->io.port, uPtrData.pv, &cTransfers, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: REP INS %#x LB %u * %#x times -> rcStrict=%Rrc cTransfers=%d\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, pRun->io.size, pRun->io.count, VBOXSTRICTRC_VAL(rcStrict), cTransfers ));
        }
        else
        {
            rcStrict = IOMIOPortWriteString(pVM, pVCpu, pRun->io.port, uPtrData.pv, &cTransfers, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: REP OUTS %#x LB %u * %#x times -> rcStrict=%Rrc cTransfers=%d\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, pRun->io.size, pRun->io.count, VBOXSTRICTRC_VAL(rcStrict), cTransfers ));
        }
        Assert(cTransfers == 0);
    }
    return rcStrict;
}


static VBOXSTRICTRC nemHCLnxHandleExit(PVMCC pVM, PVMCPUCC pVCpu, struct kvm_run *pRun)
{
    switch (pRun->exit_reason)
    {
        case KVM_EXIT_EXCEPTION:
            AssertFailed();
            break;

        case KVM_EXIT_IO:
            return nemHCLnxHandleExitIo(pVM, pVCpu, pRun);

        case KVM_EXIT_HYPERCALL:
            AssertFailed();
            break;

        case KVM_EXIT_DEBUG:
            AssertFailed();
            break;

        case KVM_EXIT_HLT:
            AssertFailed();
            break;

        case KVM_EXIT_MMIO:
            AssertFailed();
            break;

        case KVM_EXIT_IRQ_WINDOW_OPEN:
            AssertFailed();
            break;

        case KVM_EXIT_X86_RDMSR:
            AssertFailed();
            break;

        case KVM_EXIT_X86_WRMSR:
            AssertFailed();
            break;

        case KVM_EXIT_INTR: /* EINTR */
            return VINF_SUCCESS;

        case KVM_EXIT_SET_TPR:
            AssertFailed();
            break;
        case KVM_EXIT_TPR_ACCESS:
            AssertFailed();
            break;
        case KVM_EXIT_NMI:
            AssertFailed();
            break;

        case KVM_EXIT_SYSTEM_EVENT:
            AssertFailed();
            break;
        case KVM_EXIT_IOAPIC_EOI:
            AssertFailed();
            break;
        case KVM_EXIT_HYPERV:
            AssertFailed();
            break;

        case KVM_EXIT_DIRTY_RING_FULL:
            AssertFailed();
            break;
        case KVM_EXIT_AP_RESET_HOLD:
            AssertFailed();
            break;
        case KVM_EXIT_X86_BUS_LOCK:
            AssertFailed();
            break;


        case KVM_EXIT_SHUTDOWN:
            AssertFailed();
            break;

        case KVM_EXIT_FAIL_ENTRY:
            AssertFailed();
            break;
        case KVM_EXIT_INTERNAL_ERROR:
            AssertFailed();
            break;

        /*
         * Foreign and unknowns.
         */
        case KVM_EXIT_EPR:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_EPR on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_WATCHDOG:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_WATCHDOG on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_ARM_NISV:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_ARM_NISV on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_STSI:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_STSI on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_TSCH:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_TSCH on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_OSI:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_OSI on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_PAPR_HCALL:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_PAPR_HCALL on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_UCONTROL:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_UCONTROL on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_DCR:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_DCR on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_SIEIC:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_SIEIC on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_RESET:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_RESET on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_UNKNOWN:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_UNKNOWN on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_XEN:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_XEN on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        default:
            AssertLogRelMsgFailedReturn(("Unknown exit reason %u on VCpu #%u at %04x:%RX64!\n", pRun->exit_reason, pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
    }

    RT_NOREF(pVM, pVCpu, pRun);
    return VERR_NOT_IMPLEMENTED;
}


VBOXSTRICTRC nemR3NativeRunGC(PVM pVM, PVMCPU pVCpu)
{
    /*
     * Try switch to NEM runloop state.
     */
    if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED))
    { /* likely */ }
    else
    {
        VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED);
        LogFlow(("NEM/%u: returning immediately because canceled\n", pVCpu->idCpu));
        return VINF_SUCCESS;
    }

    /*
     * The run loop.
     */
    struct kvm_run * const  pRun                = pVCpu->nem.s.pRun;
    const bool              fSingleStepping     = DBGFIsStepping(pVCpu);
    VBOXSTRICTRC            rcStrict            = VINF_SUCCESS;
    for (unsigned iLoop = 0;; iLoop++)
    {
        /*
         * Pending interrupts or such?  Need to check and deal with this prior
         * to the state syncing.
         */
        if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_PIC
                                     | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI))
        {
            /* Try inject interrupt. */
            rcStrict = nemHCLnxHandleInterruptFF(pVM, pVCpu);
            if (rcStrict == VINF_SUCCESS)
            { /* likely */ }
            else
            {
                LogFlow(("NEM/%u: breaking: nemHCLnxHandleInterruptFF -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                break;
            }
        }

        /*
         * Do not execute in KVM if the A20 isn't enabled.
         */
        if (PGMPhysIsA20Enabled(pVCpu))
        { /* likely */ }
        else
        {
            rcStrict = VINF_EM_RESCHEDULE_REM;
            LogFlow(("NEM/%u: breaking: A20 disabled\n", pVCpu->idCpu));
            break;
        }

        /*
         * Ensure KVM has the whole state.
         */
        if (   (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL)
            !=                              CPUMCTX_EXTRN_ALL)
        {
            int rc2 = nemHCLnxExportState(pVM, pVCpu, &pVCpu->cpum.GstCtx, pRun);
            AssertRCReturn(rc2, rc2);
        }

        /*
         * Poll timers and run for a bit.
         *
         * With the VID approach (ring-0 or ring-3) we can specify a timeout here,
         * so we take the time of the next timer event and uses that as a deadline.
         * The rounding heuristics are "tuned" so that rhel5 (1K timer) will boot fine.
         */
        /** @todo See if we cannot optimize this TMTimerPollGIP by only redoing
         *        the whole polling job when timers have changed... */
        uint64_t       offDeltaIgnored;
        uint64_t const nsNextTimerEvt = TMTimerPollGIP(pVM, pVCpu, &offDeltaIgnored); NOREF(nsNextTimerEvt);
        if (   !VM_FF_IS_ANY_SET(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_TM_VIRTUAL_SYNC)
            && !VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
        {
            if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM_WAIT, VMCPUSTATE_STARTED_EXEC_NEM))
            {
                LogFlow(("NEM/%u: Entry @ %04x:%08RX64 IF=%d EFL=%#RX64 SS:RSP=%04x:%08RX64 cr0=%RX64\n",
                         pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                         !!(pRun->s.regs.regs.rflags & X86_EFL_IF), pRun->s.regs.regs.rflags,
                         pRun->s.regs.sregs.ss.selector, pRun->s.regs.regs.rsp, pRun->s.regs.sregs.cr0));
                TMNotifyStartOfExecution(pVM, pVCpu);

                int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_RUN, 0UL);

                VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_WAIT);
                TMNotifyEndOfExecution(pVM, pVCpu, ASMReadTSC());

                LogFlow(("NEM/%u: Exit  @ %04x:%08RX64 IF=%d EFL=%#RX64 CR8=%#x Reason=%#x IrqReady=%d Flags=%#x\n", pVCpu->idCpu,
                         pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip, pRun->if_flag,
                         pRun->s.regs.regs.rflags, pRun->s.regs.sregs.cr8, pRun->exit_reason,
                         pRun->ready_for_interrupt_injection, pRun->flags));
                if (RT_LIKELY(rcLnx == 0 || errno == EINTR))
                {
                    /*
                     * Deal with the message.
                     */
                    rcStrict = nemHCLnxHandleExit(pVM, pVCpu, pRun);
                    if (rcStrict == VINF_SUCCESS)
                    { /* hopefully likely */ }
                    else
                    {
                        LogFlow(("NEM/%u: breaking: nemHCLnxHandleExit -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                        break;
                    }
                }
                else
                {
                    int rc2 = RTErrConvertFromErrno(errno);
                    AssertLogRelMsgFailedReturn(("KVM_RUN failed: rcLnx=%d errno=%u rc=%Rrc\n", rcLnx, errno, rc2), rc2);
                }

                /*
                 * If no relevant FFs are pending, loop.
                 */
                if (   !VM_FF_IS_ANY_SET(   pVM,   !fSingleStepping ? VM_FF_HP_R0_PRE_HM_MASK    : VM_FF_HP_R0_PRE_HM_STEP_MASK)
                    && !VMCPU_FF_IS_ANY_SET(pVCpu, !fSingleStepping ? VMCPU_FF_HP_R0_PRE_HM_MASK : VMCPU_FF_HP_R0_PRE_HM_STEP_MASK) )
                    continue;

                /** @todo Try handle pending flags, not just return to EM loops.  Take care
                 *        not to set important RCs here unless we've handled an exit. */
                LogFlow(("NEM/%u: breaking: pending FF (%#x / %#RX64)\n",
                         pVCpu->idCpu, pVM->fGlobalForcedActions, (uint64_t)pVCpu->fLocalForcedActions));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnFFPost);
            }
            else
            {
                LogFlow(("NEM/%u: breaking: canceled %d (pre exec)\n", pVCpu->idCpu, VMCPU_GET_STATE(pVCpu) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnCancel);
            }
        }
        else
        {
            LogFlow(("NEM/%u: breaking: pending FF (pre exec)\n", pVCpu->idCpu));
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnFFPre);
        }
        break;
    } /* the run loop */


    /*
     * If the CPU is running, make sure to stop it before we try sync back the
     * state and return to EM.  We don't sync back the whole state if we can help it.
     */
    if (!VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_NEM))
        VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED);

    if (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL)
    {
        /* Try anticipate what we might need. */
        uint64_t fImport = IEM_CPUMCTX_EXTRN_MUST_MASK;
        if (   (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST)
            || RT_FAILURE(rcStrict))
            fImport = CPUMCTX_EXTRN_ALL;
# ifdef IN_RING0 /* Ring-3 I/O port access optimizations: */
        else if (   rcStrict == VINF_IOM_R3_IOPORT_COMMIT_WRITE
                 || rcStrict == VINF_EM_PENDING_R3_IOPORT_WRITE)
            fImport = CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS;
        else if (rcStrict == VINF_EM_PENDING_R3_IOPORT_READ)
            fImport = CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS;
# endif
        else if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_INTERRUPT_APIC
                                          | VMCPU_FF_INTERRUPT_NMI | VMCPU_FF_INTERRUPT_SMI))
            fImport |= IEM_CPUMCTX_EXTRN_XCPT_MASK;

        if (pVCpu->cpum.GstCtx.fExtrn & fImport)
        {
            int rc2 = nemHCLnxImportState(pVCpu, fImport, pRun);
            if (RT_SUCCESS(rc2))
                pVCpu->cpum.GstCtx.fExtrn &= ~fImport;
            else if (RT_SUCCESS(rcStrict))
                rcStrict = rc2;
            if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL))
                pVCpu->cpum.GstCtx.fExtrn = 0;
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturn);
        }
        else
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturnSkipped);
    }
    else
    {
        pVCpu->cpum.GstCtx.fExtrn = 0;
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturnSkipped);
    }

    LogFlow(("NEM/%u: %04x:%08RX64 efl=%#08RX64 => %Rrc\n", pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip,
             pVCpu->cpum.GstCtx.rflags, VBOXSTRICTRC_VAL(rcStrict) ));
    return rcStrict;
}


/** @page pg_nem_linux NEM/linux - Native Execution Manager, Linux.
 *
 * This is using KVM.
 *
 */

