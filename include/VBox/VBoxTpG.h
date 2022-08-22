/* $Id$ */
/** @file
 * VBox Tracepoint Generator Structures.
 */

/*
 * Copyright (C) 2012-2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_VBoxTpG_h
#define VBOX_INCLUDED_VBoxTpG_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assert.h>

RT_C_DECLS_BEGIN

/**
 * 32-bit probe location.
 */
typedef struct VTGPROBELOC32
{
    uint32_t    uLine    : 31;
    uint32_t    fEnabled : 1;
    uint32_t    idProbe;
    uint32_t    pszFunction;
    uint32_t    pProbe;
} VTGPROBELOC32;
AssertCompileSize(VTGPROBELOC32, 16);
/** Pointer to a 32-bit probe location. */
typedef VTGPROBELOC32 *PVTGPROBELOC32;
/** Pointer to a const 32-bit probe location. */
typedef VTGPROBELOC32 const *PCVTGPROBELOC32;

/**
 * 64-bit probe location.
 */
typedef struct VTGPROBELOC64
{
    uint32_t    uLine    : 31;
    uint32_t    fEnabled : 1;
    uint32_t    idProbe;
    uint64_t    pszFunction;
    uint64_t    pProbe;
    uint64_t    uAlignment;
} VTGPROBELOC64;
AssertCompileSize(VTGPROBELOC64, 32);
/** Pointer to a 64-bit probe location. */
typedef VTGPROBELOC64 *PVTGPROBELOC64;
/** Pointer to a const 64-bit probe location. */
typedef VTGPROBELOC64 const *PCVTGPROBELOC64;


/**
 * Probe location.
 */
typedef struct VTGPROBELOC
{
    uint32_t                uLine    : 31;
    uint32_t                fEnabled : 1;
    uint32_t                idProbe;
    const char             *pszFunction;
    struct VTGDESCPROBE    *pProbe;
#if ARCH_BITS == 64
    uintptr_t               uAlignment;
#endif
} VTGPROBELOC;
AssertCompileSizeAlignment(VTGPROBELOC, 16);
/** Pointer to a probe location. */
typedef VTGPROBELOC *PVTGPROBELOC;
/** Pointer to a const probe location. */
typedef VTGPROBELOC const *PCVTGPROBELOC;

/** @def VTG_OBJ_SECT
 * The name of the section containing the other probe data provided by the
 * assembly / object generated by VBoxTpG. */
/** @def VTG_LOC_SECT
 * The name of the section containing the VTGPROBELOC structures.  This is
 * filled by the probe macros, @see VTG_DECL_VTGPROBELOC. */
/** @def VTG_DECL_VTGPROBELOC
 * Declares a static variable, @a a_VarName, of type VTGPROBELOC in the section
 * indicated by VTG_LOC_SECT.  */
#if defined(RT_OS_WINDOWS)
# define VTG_OBJ_SECT       "VTGObj"
# define VTG_LOC_SECT       "VTGPrLc.Data"
# ifdef _MSC_VER
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    __declspec(allocate(VTG_LOC_SECT)) static VTGPROBELOC a_VarName
# elif defined(__GNUC__) || defined(DOXYGEN_RUNNING)
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC __attribute__((section(VTG_LOC_SECT))) a_VarName
# else
#  error "Unsupported Windows compiler!"
# endif

#elif defined(RT_OS_DARWIN)
# define VTG_OBJ_SECT       "__VTGObj"
# define VTG_LOC_SECT       "__VTGPrLc"
# define VTG_LOC_SEG        "__VTG"
# if defined(__GNUC__) || defined(DOXYGEN_RUNNING)
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC __attribute__((section(VTG_LOC_SEG "," VTG_LOC_SECT ",regular")/*, aligned(16)*/)) a_VarName
# else
#  error "Unsupported Darwin compiler!"
# endif

#elif defined(RT_OS_OS2) /** @todo This doesn't actually work, but it makes the code compile. */
# define VTG_OBJ_SECT       "__DATA"
# define VTG_LOC_SECT       "__VTGPrLc"
# define VTG_LOC_SET        "__VTGPrLcSet"
# if defined(__GNUC__) || defined(DOXYGEN_RUNNING)
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC a_VarName; \
    __asm__ (".stabs \"__VTGPrLcSet\",  23, 0, 0, _" #a_VarName );

# else
#  error "Unsupported Darwin compiler!"
# endif

#else /* Assume the rest uses ELF. */
# define VTG_OBJ_SECT       ".VTGObj"
# define VTG_LOC_SECT       ".VTGPrLc"
# if defined(__GNUC__) || defined(DOXYGEN_RUNNING)
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC __attribute__((section(VTG_LOC_SECT))) a_VarName
# else
#  error "Unsupported compiler!"
# endif
#endif

/** VTG string table offset. */
typedef uint32_t VTGSTROFF;


/** @name VTG type flags
 * @{ */
/** Masking out the fixed size if given. */
#define VTG_TYPE_SIZE_MASK      UINT32_C(0x000000ff)
/** Indicates that VTG_TYPE_SIZE_MASK can be applied, UNSIGNED or SIGNED is
 * usually set as well, so may PHYS. */
#define VTG_TYPE_FIXED_SIZED    RT_BIT_32(8)
/** It's a pointer type, the size is given by the context the probe fired in. */
#define VTG_TYPE_POINTER        RT_BIT_32(9)
/** A context specfic pointer or address, consult VTG_TYPE_CTX_XXX. */
#define VTG_TYPE_CTX_POINTER    RT_BIT_32(10)
/** The type has the same size as the host architecture. */
#define VTG_TYPE_HC_ARCH_SIZED  RT_BIT_32(11)
/** Const char pointer, requires casting in wrapper headers. */
#define VTG_TYPE_CONST_CHAR_PTR RT_BIT_32(12)
/** The type applies to ring-3 context. */
#define VTG_TYPE_CTX_R3         RT_BIT_32(24)
/** The type applies to ring-0 context. */
#define VTG_TYPE_CTX_R0         RT_BIT_32(25)
/** The type applies to raw-mode context. */
#define VTG_TYPE_CTX_RC         RT_BIT_32(26)
/** The type applies to guest context. */
#define VTG_TYPE_CTX_GST        RT_BIT_32(27)
/** The type context mask. */
#define VTG_TYPE_CTX_MASK       UINT32_C(0x0f000000)
/** The type is automatically converted to a ring-0 pointer. */
#define VTG_TYPE_AUTO_CONV_PTR  RT_BIT_32(28)
/** The type is a physical address. */
#define VTG_TYPE_PHYS           RT_BIT_32(29)
/** The type is unsigned. */
#define VTG_TYPE_UNSIGNED       RT_BIT_32(30)
/** The type is signed. */
#define VTG_TYPE_SIGNED         RT_BIT_32(31)
/** Mask of valid bits (for simple validation). */
#define VTG_TYPE_VALID_MASK     UINT32_C(0xff001fff)
/** @} */

/**
 * Checks if the VTG type flags indicates a large fixed size argument.
 */
#define VTG_TYPE_IS_LARGE(a_fType) \
    ( ((a_fType) & VTG_TYPE_SIZE_MASK) > 4 && ((a_fType) & VTG_TYPE_FIXED_SIZED) )


/**
 * VTG argument descriptor.
 */
typedef struct VTGDESCARG
{
    VTGSTROFF       offType;
    uint32_t        fType;
} VTGDESCARG;
/** Pointer to an argument descriptor. */
typedef VTGDESCARG         *PVTGDESCARG;
/** Pointer to a const argument descriptor. */
typedef VTGDESCARG const *PCVTGDESCARG;


/**
 * VTG argument list descriptor.
 */
typedef struct VTGDESCARGLIST
{
    uint8_t         cArgs;
    uint8_t         fHaveLargeArgs;
    uint8_t         abReserved[2];
    VTGDESCARG      aArgs[1];
} VTGDESCARGLIST;
/** Pointer to a VTG argument list descriptor. */
typedef VTGDESCARGLIST     *PVTGDESCARGLIST;
/** Pointer to a const VTG argument list descriptor. */
typedef VTGDESCARGLIST const *PCVTGDESCARGLIST;


/**
 * VTG probe descriptor.
 */
typedef struct VTGDESCPROBE
{
    VTGSTROFF       offName;
    uint32_t        offArgList;
    uint16_t        idxEnabled;
    uint16_t        idxProvider;
    /** The distance from this structure to the VTG object header. */
    int32_t         offObjHdr;
} VTGDESCPROBE;
AssertCompileSize(VTGDESCPROBE, 16);
/** Pointer to a VTG probe descriptor. */
typedef VTGDESCPROBE       *PVTGDESCPROBE;
/** Pointer to a const VTG probe descriptor. */
typedef VTGDESCPROBE const *PCVTGDESCPROBE;


/**
 * Code/data stability.
 */
typedef enum kVTGStability
{
    kVTGStability_Invalid = 0,
    kVTGStability_Internal,
    kVTGStability_Private,
    kVTGStability_Obsolete,
    kVTGStability_External,
    kVTGStability_Unstable,
    kVTGStability_Evolving,
    kVTGStability_Stable,
    kVTGStability_Standard,
    kVTGStability_End
} kVTGStability;

/**
 * Data dependency.
 */
typedef enum kVTGClass
{
    kVTGClass_Invalid = 0,
    kVTGClass_Unknown,
    kVTGClass_Cpu,
    kVTGClass_Platform,
    kVTGClass_Group,
    kVTGClass_Isa,
    kVTGClass_Common,
    kVTGClass_End
} kVTGClass;


/**
 * VTG attributes.
 */
typedef struct VTGDESCATTR
{
    uint8_t         u8Code;
    uint8_t         u8Data;
    uint8_t         u8DataDep;
} VTGDESCATTR;
AssertCompileSize(VTGDESCATTR, 3);
/** Pointer to a const VTG attribute. */
typedef VTGDESCATTR const *PCVTGDESCATTR;


/**
 * VTG provider descriptor.
 */
typedef struct VTGDESCPROVIDER
{
    VTGSTROFF           offName;
    uint16_t            iFirstProbe;
    uint16_t            cProbes;
    VTGDESCATTR         AttrSelf;
    VTGDESCATTR         AttrModules;
    VTGDESCATTR         AttrFunctions;
    VTGDESCATTR         AttrNames;
    VTGDESCATTR         AttrArguments;
    uint8_t             bReserved;
    uint32_t volatile   cProbesEnabled;
    /** This increases every time a probe is enabled or disabled.
     * Can be used in non-ring-3 context via PROVIDER_GET_SETTINGS_SEQ_NO() in
     * order to only configure probes related stuff when actually required.  */
    uint32_t volatile   uSettingsSerialNo;
} VTGDESCPROVIDER;
AssertCompileSize(VTGDESCPROVIDER, 32);
/** Pointer to a VTG provider descriptor. */
typedef VTGDESCPROVIDER    *PVTGDESCPROVIDER;
/** Pointer to a const VTG provider descriptor. */
typedef VTGDESCPROVIDER const *PCVTGDESCPROVIDER;


/**
 * VTG data object header.
 */
typedef struct VTGOBJHDR
{
    /** Magic value (VTGOBJHDR_MAGIC). */
    char                szMagic[24];
    /** The bitness of the structures.
     * This only affects the probe location pointers and structures. */
    uint32_t            cBits;
    /** The size of the VTG object. This excludes the probe locations. */
    uint32_t            cbObj;

    /** @name Area Descriptors
     * @remarks The offsets are relative to the header.  The members are
     *          ordered by ascending offset (maybe with the exception of the
     *          probe locations).  No overlaps, though there might be zero
     *          filled gaps between them due to alignment.
     * @{ */
    /* 32: */
    /** Offset of the string table (char) relative to this header. */
    uint32_t            offStrTab;
    /** The size of the string table, in bytes. */
    uint32_t            cbStrTab;
    /** Offset of the argument lists (VTGDESCARGLIST - variable size) relative
     * to this header. */
    uint32_t            offArgLists;
    /** The size of the argument lists, in bytes. */
    uint32_t            cbArgLists;
    /* 48: */
    /** Offset of the probe array (VTGDESCPROBE) relative to this header. */
    uint32_t            offProbes;
    /** The size of the probe array, in bytes. */
    uint32_t            cbProbes;
    /** Offset of the provider array (VTGDESCPROVIDER) relative to this
     * header. */
    uint32_t            offProviders;
    /** The size of the provider array, in bytes. */
    uint32_t            cbProviders;
    /* 64: */
    /** Offset of the probe-enabled array (uint32_t) relative to this
     * header. */
    uint32_t            offProbeEnabled;
    /** The size of the probe-enabled array, in bytes. */
    uint32_t            cbProbeEnabled;
    /** Offset of the probe location array (VTGPROBELOC) relative to this
     * header.
     * @remarks This is filled in by the first VTG user using uProbeLocs. */
    int32_t             offProbeLocs;
    /** The size of the probe location array, in bytes.
     * @remarks This is filled in by the first VTG user using uProbeLocs. */
    uint32_t            cbProbeLocs;
    /** @}  */
    /* 80: */
    /**
     * The probe location array is generated by C code and lives in a
     * different section/subsection/segment than the rest of the data.
     *
     * The assembler cannot generate offsets across sections for most (if not
     * all) object formats, so we have to store pointers here.  The first user
     * of the data will convert these two members into offset and size and fill
     * in the offProbeLocs and cbProbeLocs members above.
     *
     * @remarks Converting these members to offset+size and reusing the members
     *          to store the converted values isn't possible because of
     *          raw-mode context modules having relocations associated with the
     *          fields.
     */
    union
    {
        PVTGPROBELOC    p;
        uintptr_t       uPtr;
        uint32_t        u32;
        uint64_t        u64;
    }
    /** Pointer to the probe location array. */
                        uProbeLocs,
    /** Pointer to the end of the probe location array. */
                        uProbeLocsEnd;
    /** UUID for making sharing ring-0 structures for the same ring-3
     * modules easier. */
    RTUUID              Uuid;
    /** Mac 10.6.x load workaround.
     * The linker or/and load messes up the uProbeLocs and uProbeLocsEnd fields
     * so that they will be link addresses instead of load addresses.  To be
     * able to work around it we store the start address of the __VTGObj section
     * here and uses it to validate the probe location addresses. */
    uint64_t            u64VtgObjSectionStart;
    /** Reserved / alignment. */
    uint32_t            au32Reserved1[2];
} VTGOBJHDR;
AssertCompileSize(VTGOBJHDR, 128);
AssertCompileMemberAlignment(VTGOBJHDR, uProbeLocs, 8);
AssertCompileMemberAlignment(VTGOBJHDR, uProbeLocsEnd, 8);
/** Pointer to a VTG data object header. */
typedef VTGOBJHDR          *PVTGOBJHDR;
/** Pointer to a const VTG data object header. */
typedef VTGOBJHDR const    *PCVTGOBJHDR;

/** The current VTGOBJHDR::szMagic value. */
#define VTGOBJHDR_MAGIC     "VTG Object Header v1.7\0"

/** The name of the VTG data object header symbol in the object file. */
extern VTGOBJHDR            g_VTGObjHeader;


/** @name Macros for converting typical pointer arguments to ring-0 pointers.
 * @{ */
#ifdef IN_RING0
# define VTG_VM_TO_R0(a_pVM)                     (a_pVM)
# define VTG_VMCPU_TO_R0(a_pVCpu)                (a_pVCpu)
# define VTG_CPUMCTX_TO_R0(a_pVCpu, a_pCtx)      (a_pCtx)
#else
# define VTG_VM_TO_R0(a_pVM)                     ((a_pVM)   ? (a_pVM)->pVMR0ForCall  : NIL_RTR0PTR)
# define VTG_VMCPU_TO_R0(a_pVCpu)                ((a_pVCpu) ? (a_pVCpu)->pVCpuR0ForVtg : NIL_RTR0PTR)
# define VTG_CPUMCTX_TO_R0(a_pVCpu, a_pCtx)      ((a_pVCpu) ? (a_pVCpu)->pVCpuR0ForVtg + ((uintptr_t)(a_pCtx) - (uintptr_t)(a_pVCpu)) : NIL_RTR0PTR)
#endif
/** @} */


RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_VBoxTpG_h */

