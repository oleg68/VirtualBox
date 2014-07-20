/* $Id$ */
/** @file
 * IPRT - Header for code using the Native NT API.
 */

/*
 * Copyright (C) 2010-2014 Oracle Corporation
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
 */

#ifndef ___iprt_nt_nt_h___
#define ___iprt_nt_nt_h___

/** @def IPRT_NT_MAP_TO_ZW
 * Map Nt calls to Zw calls.  In ring-0 the Zw calls let you pass kernel memory
 * to the APIs (takes care of the previous context checks).
 */
#ifdef DOXYGEN_RUNNING
# define IPRT_NT_MAP_TO_ZW
#endif

#ifdef IPRT_NT_MAP_TO_ZW
# define NtQueryInformationFile         ZwQueryInformationFile
# define NtQueryInformationProcess      ZwQueryInformationProcess
# define NtQueryInformationThread       ZwQueryInformationThread
# define NtQuerySystemInformation       ZwQuerySystemInformation
# define NtClose                        ZwClose
# define NtCreateFile                   ZwCreateFile
# define NtReadFile                     ZwReadFile
# define NtWriteFile                    ZwWriteFile
/** @todo this is very incomplete! */
#endif

#include <ntstatus.h>

/*
 * Hacks common to both base header sets.
 */
#define NtQueryObject              Incomplete_NtQueryObject
#define ZwQueryObject              Incomplete_ZwQueryObject
#define NtSetInformationObject     Incomplete_NtSetInformationObject
#define _OBJECT_INFORMATION_CLASS  Incomplete_OBJECT_INFORMATION_CLASS
#define OBJECT_INFORMATION_CLASS   Incomplete_OBJECT_INFORMATION_CLASS
#define ObjectBasicInformation     Incomplete_ObjectBasicInformation
#define ObjectTypeInformation      Incomplete_ObjectTypeInformation
#define _PEB                       Incomplete__PEB
#define PEB                        Incomplete_PEB
#define PPEB                       Incomplete_PPEB
#define _TEB                       Incomplete__TEB
#define TEB                        Incomplete_TEB
#define PTEB                       Incomplete_PTEB


#ifdef IPRT_NT_USE_WINTERNL
/*
 * Use Winternl.h.
 */
# define _FILE_INFORMATION_CLASS                IncompleteWinternl_FILE_INFORMATION_CLASS
# define FILE_INFORMATION_CLASS                 IncompleteWinternl_FILE_INFORMATION_CLASS
# define FileDirectoryInformation               IncompleteWinternl_FileDirectoryInformation

# define NtQueryInformationProcess              IncompleteWinternl_NtQueryInformationProcess
# define NtSetInformationProcess                IncompleteWinternl_NtSetInformationProcess
# define PROCESSINFOCLASS                       IncompleteWinternl_PROCESSINFOCLASS
# define _PROCESSINFOCLASS                      IncompleteWinternl_PROCESSINFOCLASS
# define PROCESS_BASIC_INFORMATION              IncompleteWinternl_PROCESS_BASIC_INFORMATION
# define PPROCESS_BASIC_INFORMATION             IncompleteWinternl_PPROCESS_BASIC_INFORMATION
# define _PROCESS_BASIC_INFORMATION             IncompleteWinternl_PROCESS_BASIC_INFORMATION
# define ProcessBasicInformation                IncompleteWinternl_ProcessBasicInformation
# define ProcessDebugPort                       IncompleteWinternl_ProcessDebugPort
# define ProcessWow64Information                IncompleteWinternl_ProcessWow64Information
# define ProcessImageFileName                   IncompleteWinternl_ProcessImageFileName
# define ProcessBreakOnTermination              IncompleteWinternl_ProcessBreakOnTermination

# define RTL_USER_PROCESS_PARAMETERS            IncompleteWinternl_RTL_USER_PROCESS_PARAMETERS
# define PRTL_USER_PROCESS_PARAMETERS           IncompleteWinternl_PRTL_USER_PROCESS_PARAMETERS
# define _RTL_USER_PROCESS_PARAMETERS           IncompleteWinternl__RTL_USER_PROCESS_PARAMETERS

# define NtQueryInformationThread               IncompleteWinternl_NtQueryInformationThread
# define NtSetInformationThread                 IncompleteWinternl_NtSetInformationThread
# define THREADINFOCLASS                        IncompleteWinternl_THREADINFOCLASS
# define _THREADINFOCLASS                       IncompleteWinternl_THREADINFOCLASS
# define ThreadIsIoPending                      IncompleteWinternl_ThreadIsIoPending

# define NtQuerySystemInformation               IncompleteWinternl_NtQuerySystemInformation
# define NtSetSystemInformation                 IncompleteWinternl_NtSetSystemInformation
# define SYSTEM_INFORMATION_CLASS               IncompleteWinternl_SYSTEM_INFORMATION_CLASS
# define _SYSTEM_INFORMATION_CLASS              IncompleteWinternl_SYSTEM_INFORMATION_CLASS
# define SystemBasicInformation                 IncompleteWinternl_SystemBasicInformation
# define SystemPerformanceInformation           IncompleteWinternl_SystemPerformanceInformation
# define SystemTimeOfDayInformation             IncompleteWinternl_SystemTimeOfDayInformation
# define SystemProcessInformation               IncompleteWinternl_SystemProcessInformation
# define SystemProcessorPerformanceInformation  IncompleteWinternl_SystemProcessorPerformanceInformation
# define SystemInterruptInformation             IncompleteWinternl_SystemInterruptInformation
# define SystemExceptionInformation             IncompleteWinternl_SystemExceptionInformation
# define SystemRegistryQuotaInformation         IncompleteWinternl_SystemRegistryQuotaInformation
# define SystemLookasideInformation             IncompleteWinternl_SystemLookasideInformation
# define SystemPolicyInformation                IncompleteWinternl_SystemPolicyInformation


# define WIN32_NO_STATUS
# include <windef.h>
# include <winnt.h>
# include <winternl.h>
# undef WIN32_NO_STATUS
# include <ntstatus.h>


# undef _FILE_INFORMATION_CLASS
# undef FILE_INFORMATION_CLASS
# undef FileDirectoryInformation

# undef NtQueryInformationProcess
# undef NtSetInformationProcess
# undef PROCESSINFOCLASS
# undef _PROCESSINFOCLASS
# undef PROCESS_BASIC_INFORMATION
# undef PPROCESS_BASIC_INFORMATION
# undef _PROCESS_BASIC_INFORMATION
# undef ProcessBasicInformation
# undef ProcessDebugPort
# undef ProcessWow64Information
# undef ProcessImageFileName
# undef ProcessBreakOnTermination

# undef RTL_USER_PROCESS_PARAMETERS
# undef PRTL_USER_PROCESS_PARAMETERS
# undef _RTL_USER_PROCESS_PARAMETERS

# undef NtQueryInformationThread
# undef NtSetInformationThread
# undef THREADINFOCLASS
# undef _THREADINFOCLASS
# undef ThreadIsIoPending

# undef NtQuerySystemInformation
# undef NtSetSystemInformation
# undef SYSTEM_INFORMATION_CLASS
# undef _SYSTEM_INFORMATION_CLASS
# undef SystemBasicInformation
# undef SystemPerformanceInformation
# undef SystemTimeOfDayInformation
# undef SystemProcessInformation
# undef SystemProcessorPerformanceInformation
# undef SystemInterruptInformation
# undef SystemExceptionInformation
# undef SystemRegistryQuotaInformation
# undef SystemLookasideInformation
# undef SystemPolicyInformation

#else
/*
 * Use ntifs.h and wdm.h.
 */
# ifdef RT_ARCH_X86
#  define _InterlockedAddLargeStatistic  _InterlockedAddLargeStatistic_StupidDDKVsCompilerCrap
#  pragma warning(disable : 4163)
# endif

# include <ntifs.h>
# include <wdm.h>

# ifdef RT_ARCH_X86
#  pragma warning(default : 4163)
#  undef _InterlockedAddLargeStatistic
# endif

# define IPRT_NT_NEED_API_GROUP_NTIFS
#endif

#undef NtQueryObject
#undef ZwQueryObject
#undef NtSetInformationObject
#undef _OBJECT_INFORMATION_CLASS
#undef OBJECT_INFORMATION_CLASS
#undef ObjectBasicInformation
#undef ObjectTypeInformation
#undef _PEB
#undef PEB
#undef PPEB
#undef _TEB
#undef TEB
#undef PTEB


#include <iprt/types.h>
#include <iprt/assert.h>


/** @name Useful macros
 * @{ */
/** Indicates that we're targetting native NT in the current source. */
#define RTNT_USE_NATIVE_NT              1
/** Initializes a IO_STATUS_BLOCK. */
#define RTNT_IO_STATUS_BLOCK_INITIALIZER  { STATUS_FAILED_DRIVER_ENTRY, ~(uintptr_t)42 }
/** Similar to INVALID_HANDLE_VALUE in the Windows environment. */
#define RTNT_INVALID_HANDLE_VALUE         ( (HANDLE)~(uintptr_t)0 )
/** @}  */


/** @name IPRT helper functions for NT
 * @{ */
RT_C_DECLS_BEGIN

RTDECL(int) RTNtPathOpen(const char *pszPath, ACCESS_MASK fDesiredAccess, ULONG fFileAttribs, ULONG fShareAccess,
                          ULONG fCreateDisposition, ULONG fCreateOptions, ULONG fObjAttribs,
                          PHANDLE phHandle, PULONG_PTR puDisposition);
RTDECL(int) RTNtPathOpenDir(const char *pszPath, ACCESS_MASK fDesiredAccess, ULONG fShareAccess, ULONG fCreateOptions,
                            ULONG fObjAttribs, PHANDLE phHandle, bool *pfObjDir);
RTDECL(int) RTNtPathClose(HANDLE hHandle);

RT_C_DECLS_END
/** @} */


/** @name NT API delcarations.
 * @{ */
RT_C_DECLS_BEGIN

/** @name Process access rights missing in ntddk headers
 * @{ */
#ifndef  PROCESS_TERMINATE
# define PROCESS_TERMINATE                  UINT32_C(0x00000001)
#endif
#ifndef  PROCESS_CREATE_THREAD
# define PROCESS_CREATE_THREAD              UINT32_C(0x00000002)
#endif
#ifndef  PROCESS_SET_SESSIONID
# define PROCESS_SET_SESSIONID              UINT32_C(0x00000004)
#endif
#ifndef  PROCESS_VM_OPERATION
# define PROCESS_VM_OPERATION               UINT32_C(0x00000008)
#endif
#ifndef  PROCESS_VM_READ
# define PROCESS_VM_READ                    UINT32_C(0x00000010)
#endif
#ifndef  PROCESS_VM_WRITE
# define PROCESS_VM_WRITE                   UINT32_C(0x00000020)
#endif
#ifndef  PROCESS_DUP_HANDLE
# define PROCESS_DUP_HANDLE                 UINT32_C(0x00000040)
#endif
#ifndef  PROCESS_CREATE_PROCESS
# define PROCESS_CREATE_PROCESS             UINT32_C(0x00000080)
#endif
#ifndef  PROCESS_SET_QUOTA
# define PROCESS_SET_QUOTA                  UINT32_C(0x00000100)
#endif
#ifndef  PROCESS_SET_INFORMATION
# define PROCESS_SET_INFORMATION            UINT32_C(0x00000200)
#endif
#ifndef  PROCESS_QUERY_INFORMATION
# define PROCESS_QUERY_INFORMATION          UINT32_C(0x00000400)
#endif
#ifndef  PROCESS_SUSPEND_RESUME
# define PROCESS_SUSPEND_RESUME             UINT32_C(0x00000800)
#endif
#ifndef  PROCESS_QUERY_LIMITED_INFORMATION
# define PROCESS_QUERY_LIMITED_INFORMATION  UINT32_C(0x00001000)
#endif
#ifndef  PROCESS_SET_LIMITED_INFORMATION
# define PROCESS_SET_LIMITED_INFORMATION    UINT32_C(0x00002000)
#endif
#define PROCESS_UNKNOWN_4000                UINT32_C(0x00004000)
#define PROCESS_UNKNOWN_6000                UINT32_C(0x00008000)
#ifndef PROCESS_ALL_ACCESS
# define PROCESS_ALL_ACCESS                 ( STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | UINT32_C(0x0000ffff) )
#endif
/** @} */

/** @name Thread access rights missing in ntddk headers
 * @{ */
#ifndef THREAD_QUERY_INFORMATION
# define THREAD_QUERY_INFORMATION           UINT32_C(0x00000040)
#endif
#ifndef THREAD_SET_THREAD_TOKEN
# define THREAD_SET_THREAD_TOKEN            UINT32_C(0x00000080)
#endif
#ifndef THREAD_IMPERSONATE
# define THREAD_IMPERSONATE                 UINT32_C(0x00000100)
#endif
#ifndef THREAD_DIRECT_IMPERSONATION
# define THREAD_DIRECT_IMPERSONATION        UINT32_C(0x00000200)
#endif
#ifndef THREAD_RESUME
# define THREAD_RESUME                      UINT32_C(0x00001000)
#endif
#define THREAD_UNKNOWN_2000                 UINT32_C(0x00002000)
#define THREAD_UNKNOWN_4000                 UINT32_C(0x00004000)
#define THREAD_UNKNOWN_8000                 UINT32_C(0x00008000)
/** @} */

/** @name Special handle values.
 * @{ */
#ifndef NtCurrentProcess
# define NtCurrentProcess()                 ( (HANDLE)-(intptr_t)1 )
#endif
#ifndef NtCurrentThread
# define NtCurrentThread()                  ( (HANDLE)-(intptr_t)2 )
#endif
#ifndef ZwCurrentProcess
# define ZwCurrentProcess()                 NtCurrentProcess()
#endif
#ifndef ZwCurrentThread
# define ZwCurrentThread()                  NtCurrentThread()
#endif
/** @} */


/** @name Directory object access rights.
 * @{ */
#ifndef DIRECTORY_QUERY
# define DIRECTORY_QUERY                    UINT32_C(0x00000001)
#endif
#ifndef DIRECTORY_TRAVERSE
# define DIRECTORY_TRAVERSE                 UINT32_C(0x00000002)
#endif
#ifndef DIRECTORY_CREATE_OBJECT
# define DIRECTORY_CREATE_OBJECT            UINT32_C(0x00000004)
#endif
#ifndef DIRECTORY_CREATE_SUBDIRECTORY
# define DIRECTORY_CREATE_SUBDIRECTORY      UINT32_C(0x00000008)
#endif
#ifndef DIRECTORY_ALL_ACCESS
# define DIRECTORY_ALL_ACCESS               ( STANDARD_RIGHTS_REQUIRED | UINT32_C(0x0000000f) )
#endif
/** @} */



#ifdef IPRT_NT_USE_WINTERNL
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;
#endif

/** @name Process And Thread Environment Blocks
 * @{ */

#if 0
/* Windows 8.1 PEB: */
typedef struct _PEB_W81
{
    BOOLEAN InheritedAddressSpace;                                          /**< 0x000 / 0x000 */
    BOOLEAN ReadImageFileExecOptions;                                       /**< 0x001 / 0x001 */
    BOOLEAN BeingDebugged;                                                  /**< 0x002 / 0x002 */
    union
    {
        uint8_t BitField;                                                   /**< 0x003 / 0x003 */
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W80 */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W80 */
            uint8_t IsPackagedProcess : 1;                                  /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W80 */
            uint8_t IsAppContainer : 1;                                     /**< 0x003 / 0x003 : Pos 5, 1 Bit - Differs from W80 */
            uint8_t IsProtectedProcessLight : 1;                            /**< 0x003 / 0x003 : Pos 6, 1 Bit - Differs from W80 */
            uint8_t SpareBits : 1;                                          /**< 0x003 / 0x003 : Pos 7, 1 Bit */
        };
    };
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x004 / NA */
#endif
    HANDLE Mutant;                                                          /**< 0x008 / 0x004 */
    PVOID ImageBaseAddress;                                                 /**< 0x010 / 0x008 */
    struct _PEB_LDR_DATA *Ldr;                                              /**< 0x018 / 0x00c */
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /**< 0x020 / 0x010 */
    PVOID SubSystemData;                                                    /**< 0x028 / 0x014 */
    HANDLE ProcessHeap;                                                     /**< 0x030 / 0x018 */
    struct _RTL_CRITICAL_SECTION *FastPebLock;                              /**< 0x038 / 0x01c */
    PVOID AtlThunkSListPtr;                                                 /**< 0x040 / 0x020 */
    PVOID IFEOKey;                                                          /**< 0x048 / 0x024 */
    union
    {
        ULONG CrossProcessFlags;                                            /**< 0x050 / 0x028 */
        struct
        {
            uint32_t ProcessInJob : 1;                                      /**< 0x050 / 0x028: Pos 0, 1 Bit */
            uint32_t ProcessInitializing : 1;                               /**< 0x050 / 0x028: Pos 1, 1 Bit */
            uint32_t ProcessUsingVEH : 1;                                   /**< 0x050 / 0x028: Pos 2, 1 Bit */
            uint32_t ProcessUsingVCH : 1;                                   /**< 0x050 / 0x028: Pos 3, 1 Bit */
            uint32_t ProcessUsingFTH : 1;                                   /**< 0x050 / 0x028: Pos 4, 1 Bit */
            uint32_t ReservedBits0 : 27;                                    /**< 0x050 / 0x028: Pos 5, 27 Bits */
        };
    };
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x054 / */
#endif
    union
    {
        PVOID KernelCallbackTable;                                          /**< 0x058 / 0x02c */
        PVOID UserSharedInfoPtr;                                            /**< 0x058 / 0x02c */
    };
    uint32_t SystemReserved;                                                /**< 0x060 / 0x030 */
    uint32_t AtlThunkSListPtr32;                                            /**< 0x064 / 0x034 */
    PVOID ApiSetMap;                                                        /**< 0x068 / 0x038 */
    uint32_t TlsExpansionCounter;                                           /**< 0x070 / 0x03c */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x074 / NA */
#endif
    struct _RTL_BITMAP *TlsBitmap;                                          /**< 0x078 / 0x040 */
    uint32_t TlsBitmapBits[2];                                              /**< 0x080 / 0x044 */
    PVOID ReadOnlySharedMemoryBase;                                         /**< 0x088 / 0x04c */
    PVOID SparePvoid0;                                                      /**< 0x090 / 0x050 - HotpatchInformation in W80. */
    PVOID *ReadOnlyStaticServerData;                                        /**< 0x098 / 0x054 */
    PVOID AnsiCodePageData;                                                 /**< 0x0a0 / 0x058 */
    PVOID OemCodePageData;                                                  /**< 0x0a8 / 0x05c */
    PVOID UnicodeCaseTableData;                                             /**< 0x0b0 / 0x060 */
    uint32_t NumberOfProcessors;                                            /**< 0x0b8 / 0x064 */
    uint32_t NtGlobalFlag;                                                  /**< 0x0bc / 0x068 */
    LARGE_INTEGER CriticalSectionTimeout;                                   /**< 0x0c0 / 0x070 */
    SIZE_T HeapSegmentReserve;                                              /**< 0x0c8 / 0x078 */
    SIZE_T HeapSegmentCommit;                                               /**< 0x0d0 / 0x07c */
    SIZE_T HeapDeCommitTotalFreeThreshold;                                  /**< 0x0d8 / 0x080 */
    SIZE_T HeapDeCommitFreeBlockThreshold;                                  /**< 0x0e0 / 0x084 */
    uint32_t NumberOfHeaps;                                                 /**< 0x0e8 / 0x088 */
    uint32_t MaximumNumberOfHeaps;                                          /**< 0x0ec / 0x08c */
    PVOID *ProcessHeaps;                                                    /**< 0x0f0 / 0x090 */
    PVOID GdiSharedHandleTable;                                             /**< 0x0f8 / 0x094 */
    PVOID ProcessStarterHelper;                                             /**< 0x100 / 0x098 */
    uint32_t GdiDCAttributeList;                                            /**< 0x108 / 0x09c */
#if ARCH_BITS == 64
    uint32_t Padding3;                                                      /**< 0x10c / NA */
#endif
    struct _RTL_CRITICAL_SECTION *LoaderLock;                               /**< 0x110 / 0x0a0 */
    uint32_t OSMajorVersion;                                                /**< 0x118 / 0x0a4 */
    uint32_t OSMinorVersion;                                                /**< 0x11c / 0x0a8 */
    uint16_t OSBuildNumber;                                                 /**< 0x120 / 0x0ac */
    uint16_t OSCSDVersion;                                                  /**< 0x122 / 0x0ae */
    uint32_t OSPlatformId;                                                  /**< 0x124 / 0x0b0 */
    uint32_t ImageSubsystem;                                                /**< 0x128 / 0x0b4 */
    uint32_t ImageSubsystemMajorVersion;                                    /**< 0x12c / 0x0b8 */
    uint32_t ImageSubsystemMinorVersion;                                    /**< 0x130 / 0x0bc */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x134 / NA */
#endif
    SIZE_T ActiveProcessAffinityMask;                                       /**< 0x138 / 0x0c0 */
    uint32_t GdiHandleBuffer[ARCH_BITS == 64 ? 60 : 34];                    /**< 0x140 / 0x0c4 */
    PVOID PostProcessInitRoutine;                                           /**< 0x230 / 0x14c */
    PVOID TlsExpansionBitmap;                                               /**< 0x238 / 0x150 */
    uint32_t TlsExpansionBitmapBits[32];                                    /**< 0x240 / 0x154 */
    uint32_t SessionId;                                                     /**< 0x2c0 / 0x1d4 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x2c4 / NA */
#endif
    ULARGE_INTEGER AppCompatFlags;                                          /**< 0x2c8 / 0x1d8 */
    ULARGE_INTEGER AppCompatFlagsUser;                                      /**< 0x2d0 / 0x1e0 */
    PVOID pShimData;                                                        /**< 0x2d8 / 0x1e8 */
    PVOID AppCompatInfo;                                                    /**< 0x2e0 / 0x1ec */
    UNICODE_STRING CSDVersion;                                              /**< 0x2e8 / 0x1f0 */
    struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;                 /**< 0x2f8 / 0x1f8 */
    struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;                /**< 0x300 / 0x1fc */
    struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData;    /**< 0x308 / 0x200 */
    struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;                 /**< 0x310 / 0x204 */
    SIZE_T MinimumStackCommit;                                              /**< 0x318 / 0x208 */
    struct _FLS_CALLBACK_INFO *FlsCallback;                                 /**< 0x320 / 0x20c */
    LIST_ENTRY FlsListHead;                                                 /**< 0x328 / 0x210 */
    PVOID FlsBitmap;                                                        /**< 0x338 / 0x218 */
    uint32_t FlsBitmapBits[4];                                              /**< 0x340 / 0x21c */
    uint32_t FlsHighIndex;                                                  /**< 0x350 / 0x22c */
    PVOID WerRegistrationData;                                              /**< 0x358 / 0x230 */
    PVOID WerShipAssertPtr;                                                 /**< 0x360 / 0x234 */
    PVOID pUnused;                                                          /**< 0x368 / 0x238 - Was pContextData in W7. */
    PVOID pImageHeaderHash;                                                 /**< 0x370 / 0x23c */
    union
    {
        uint32_t TracingFlags;                                              /**< 0x378 / 0x240 */
        uint32_t HeapTracingEnabled : 1;                                    /**< 0x378 / 0x240 : Pos 0, 1 Bit */
        uint32_t CritSecTracingEnabled : 1;                                 /**< 0x378 / 0x240 : Pos 1, 1 Bit */
        uint32_t LibLoaderTracingEnabled : 1;                               /**< 0x378 / 0x240 : Pos 2, 1 Bit */
        uint32_t SpareTracingBits : 29;                                     /**< 0x378 / 0x240 : Pos 3, 29 Bits */
    };
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x37c / NA */
#endif
    uint64_t CsrServerReadOnlySharedMemoryBase;                             /**< 0x380 / 0x248 */
} PEB_W81;
typedef PEB_W81 *PPEB_W81;
AssertCompileMemberOffset(PEB_W81, NtGlobalFlag,   ARCH_BITS == 64 ?  0xbc :  0x68);
AssertCompileMemberOffset(PEB_W81, LoaderLock,     ARCH_BITS == 64 ? 0x110 :  0xa0);
AssertCompileMemberOffset(PEB_W81, ActiveProcessAffinityMask, ARCH_BITS == 64 ? 0x138 :  0xc0);
AssertCompileMemberOffset(PEB_W81, PostProcessInitRoutine,    ARCH_BITS == 64 ? 0x230 : 0x14c);
AssertCompileMemberOffset(PEB_W81, AppCompatFlags, ARCH_BITS == 64 ? 0x2c8 : 0x1d8);
AssertCompileSize(PEB_W81, ARCH_BITS == 64 ? 0x388 : 0x250);

/* Windows 8.0 PEB: */
typedef struct _PEB_W80
{
    BOOLEAN InheritedAddressSpace;                                          /**< 0x000 / 0x000 */
    BOOLEAN ReadImageFileExecOptions;                                       /**< 0x001 / 0x001 */
    BOOLEAN BeingDebugged;                                                  /**< 0x002 / 0x002 */
    union
    {
        uint8_t BitField;                                                   /**< 0x003 / 0x003 */
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81 */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81 */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W81 */
            uint8_t IsPackagedProcess : 1;                                  /**< 0x003 / 0x003 : Pos 5, 1 Bit - Differs from W81 */
            uint8_t IsAppContainer : 1;                                     /**< 0x003 / 0x003 : Pos 6, 1 Bit - Differs from W81 */
            uint8_t SpareBits : 1;                                          /**< 0x003 / 0x003 : Pos 7, 1 Bit */
        };
    };
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x004 / NA */
#endif
    HANDLE Mutant;                                                          /**< 0x008 / 0x004 */
    PVOID ImageBaseAddress;                                                 /**< 0x010 / 0x008 */
    struct _PEB_LDR_DATA *Ldr;                                              /**< 0x018 / 0x00c */
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /**< 0x020 / 0x010 */
    PVOID SubSystemData;                                                    /**< 0x028 / 0x014 */
    HANDLE ProcessHeap;                                                     /**< 0x030 / 0x018 */
    struct _RTL_CRITICAL_SECTION *FastPebLock;                              /**< 0x038 / 0x01c */
    PVOID AtlThunkSListPtr;                                                 /**< 0x040 / 0x020 */
    PVOID IFEOKey;                                                          /**< 0x048 / 0x024 */
    union
    {
        ULONG CrossProcessFlags;                                            /**< 0x050 / 0x028 */
        struct
        {
            uint32_t ProcessInJob : 1;                                      /**< 0x050 / 0x028: Pos 0, 1 Bit */
            uint32_t ProcessInitializing : 1;                               /**< 0x050 / 0x028: Pos 1, 1 Bit */
            uint32_t ProcessUsingVEH : 1;                                   /**< 0x050 / 0x028: Pos 2, 1 Bit */
            uint32_t ProcessUsingVCH : 1;                                   /**< 0x050 / 0x028: Pos 3, 1 Bit */
            uint32_t ProcessUsingFTH : 1;                                   /**< 0x050 / 0x028: Pos 4, 1 Bit */
            uint32_t ReservedBits0 : 27;                                    /**< 0x050 / 0x028: Pos 5, 27 Bits */
        };
    };
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x054 / */
#endif
    union
    {
        PVOID KernelCallbackTable;                                          /**< 0x058 / 0x02c */
        PVOID UserSharedInfoPtr;                                            /**< 0x058 / 0x02c */
    };
    uint32_t SystemReserved;                                                /**< 0x060 / 0x030 */
    uint32_t AtlThunkSListPtr32;                                            /**< 0x064 / 0x034 */
    PVOID ApiSetMap;                                                        /**< 0x068 / 0x038 */
    uint32_t TlsExpansionCounter;                                           /**< 0x070 / 0x03c */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x074 / NA */
#endif
    struct _RTL_BITMAP *TlsBitmap;                                          /**< 0x078 / 0x040 */
    uint32_t TlsBitmapBits[2];                                              /**< 0x080 / 0x044 */
    PVOID ReadOnlySharedMemoryBase;                                         /**< 0x088 / 0x04c */
    PVOID HotpatchInformation;                                              /**< 0x090 / 0x050 - Retired in W81. */
    PVOID *ReadOnlyStaticServerData;                                        /**< 0x098 / 0x054 */
    PVOID AnsiCodePageData;                                                 /**< 0x0a0 / 0x058 */
    PVOID OemCodePageData;                                                  /**< 0x0a8 / 0x05c */
    PVOID UnicodeCaseTableData;                                             /**< 0x0b0 / 0x060 */
    uint32_t NumberOfProcessors;                                            /**< 0x0b8 / 0x064 */
    uint32_t NtGlobalFlag;                                                  /**< 0x0bc / 0x068 */
    LARGE_INTEGER CriticalSectionTimeout;                                   /**< 0x0c0 / 0x070 */
    SIZE_T HeapSegmentReserve;                                              /**< 0x0c8 / 0x078 */
    SIZE_T HeapSegmentCommit;                                               /**< 0x0d0 / 0x07c */
    SIZE_T HeapDeCommitTotalFreeThreshold;                                  /**< 0x0d8 / 0x080 */
    SIZE_T HeapDeCommitFreeBlockThreshold;                                  /**< 0x0e0 / 0x084 */
    uint32_t NumberOfHeaps;                                                 /**< 0x0e8 / 0x088 */
    uint32_t MaximumNumberOfHeaps;                                          /**< 0x0ec / 0x08c */
    PVOID *ProcessHeaps;                                                    /**< 0x0f0 / 0x090 */
    PVOID GdiSharedHandleTable;                                             /**< 0x0f8 / 0x094 */
    PVOID ProcessStarterHelper;                                             /**< 0x100 / 0x098 */
    uint32_t GdiDCAttributeList;                                            /**< 0x108 / 0x09c */
#if ARCH_BITS == 64
    uint32_t Padding3;                                                      /**< 0x10c / NA */
#endif
    struct _RTL_CRITICAL_SECTION *LoaderLock;                               /**< 0x110 / 0x0a0 */
    uint32_t OSMajorVersion;                                                /**< 0x118 / 0x0a4 */
    uint32_t OSMinorVersion;                                                /**< 0x11c / 0x0a8 */
    uint16_t OSBuildNumber;                                                 /**< 0x120 / 0x0ac */
    uint16_t OSCSDVersion;                                                  /**< 0x122 / 0x0ae */
    uint32_t OSPlatformId;                                                  /**< 0x124 / 0x0b0 */
    uint32_t ImageSubsystem;                                                /**< 0x128 / 0x0b4 */
    uint32_t ImageSubsystemMajorVersion;                                    /**< 0x12c / 0x0b8 */
    uint32_t ImageSubsystemMinorVersion;                                    /**< 0x130 / 0x0bc */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x134 / NA */
#endif
    SIZE_T ActiveProcessAffinityMask;                                       /**< 0x138 / 0x0c0 */
    uint32_t GdiHandleBuffer[ARCH_BITS == 64 ? 60 : 34];                    /**< 0x140 / 0x0c4 */
    PVOID PostProcessInitRoutine;                                           /**< 0x230 / 0x14c */
    PVOID TlsExpansionBitmap;                                               /**< 0x238 / 0x150 */
    uint32_t TlsExpansionBitmapBits[32];                                    /**< 0x240 / 0x154 */
    uint32_t SessionId;                                                     /**< 0x2c0 / 0x1d4 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x2c4 / NA */
#endif
    ULARGE_INTEGER AppCompatFlags;                                          /**< 0x2c8 / 0x1d8 */
    ULARGE_INTEGER AppCompatFlagsUser;                                      /**< 0x2d0 / 0x1e0 */
    PVOID pShimData;                                                        /**< 0x2d8 / 0x1e8 */
    PVOID AppCompatInfo;                                                    /**< 0x2e0 / 0x1ec */
    UNICODE_STRING CSDVersion;                                              /**< 0x2e8 / 0x1f0 */
    struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;                 /**< 0x2f8 / 0x1f8 */
    struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;                /**< 0x300 / 0x1fc */
    struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData;    /**< 0x308 / 0x200 */
    struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;                 /**< 0x310 / 0x204 */
    SIZE_T MinimumStackCommit;                                              /**< 0x318 / 0x208 */
    struct _FLS_CALLBACK_INFO *FlsCallback;                                 /**< 0x320 / 0x20c */
    LIST_ENTRY FlsListHead;                                                 /**< 0x328 / 0x210 */
    PVOID FlsBitmap;                                                        /**< 0x338 / 0x218 */
    uint32_t FlsBitmapBits[4];                                              /**< 0x340 / 0x21c */
    uint32_t FlsHighIndex;                                                  /**< 0x350 / 0x22c */
    PVOID WerRegistrationData;                                              /**< 0x358 / 0x230 */
    PVOID WerShipAssertPtr;                                                 /**< 0x360 / 0x234 */
    PVOID pUnused;                                                          /**< 0x368 / 0x238 - Was pContextData in W7. */
    PVOID pImageHeaderHash;                                                 /**< 0x370 / 0x23c */
    union
    {
        uint32_t TracingFlags;                                              /**< 0x378 / 0x240 */
        uint32_t HeapTracingEnabled : 1;                                    /**< 0x378 / 0x240 : Pos 0, 1 Bit */
        uint32_t CritSecTracingEnabled : 1;                                 /**< 0x378 / 0x240 : Pos 1, 1 Bit */
        uint32_t LibLoaderTracingEnabled : 1;                               /**< 0x378 / 0x240 : Pos 2, 1 Bit */
        uint32_t SpareTracingBits : 29;                                     /**< 0x378 / 0x240 : Pos 3, 29 Bits */
    };
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x37c / NA */
#endif
    uint64_t CsrServerReadOnlySharedMemoryBase;                             /**< 0x380 / 0x248 */
} PEB_W80;
typedef PEB_W80 *PPEB_W80;
AssertCompileMemberOffset(PEB_W80, NtGlobalFlag,   ARCH_BITS == 64 ?  0xbc :  0x68);
AssertCompileMemberOffset(PEB_W80, LoaderLock,     ARCH_BITS == 64 ? 0x110 :  0xa0);
AssertCompileMemberOffset(PEB_W80, ActiveProcessAffinityMask, ARCH_BITS == 64 ? 0x138 :  0xc0);
AssertCompileMemberOffset(PEB_W80, PostProcessInitRoutine,    ARCH_BITS == 64 ? 0x230 : 0x14c);
AssertCompileMemberOffset(PEB_W80, AppCompatFlags, ARCH_BITS == 64 ? 0x2c8 : 0x1d8);
AssertCompileSize(PEB_W80, ARCH_BITS == 64 ? 0x388 : 0x250);


/* Windows 7 PEB: */
typedef struct _PEB_W7
{
    BOOLEAN InheritedAddressSpace;                                          /**< 0x000 / 0x000 */
    BOOLEAN ReadImageFileExecOptions;                                       /**< 0x001 / 0x001 */
    BOOLEAN BeingDebugged;                                                  /**< 0x002 / 0x002 */
    union
    {
        uint8_t BitField;                                                   /**< 0x003 / 0x003 */
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81, same as W80. */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81, same as W80. */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W81, same as W80. */
            uint8_t SpareBits : 3;                                          /**< 0x003 / 0x003 : Pos 7, 3 Bit - Differs from W81 & W80, more spare bits. */
        };
    };
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x004 / NA */
#endif
    HANDLE Mutant;                                                          /**< 0x008 / 0x004 */
    PVOID ImageBaseAddress;                                                 /**< 0x010 / 0x008 */
    struct _PEB_LDR_DATA *Ldr;                                              /**< 0x018 / 0x00c */
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /**< 0x020 / 0x010 */
    PVOID SubSystemData;                                                    /**< 0x028 / 0x014 */
    HANDLE ProcessHeap;                                                     /**< 0x030 / 0x018 */
    struct _RTL_CRITICAL_SECTION *FastPebLock;                              /**< 0x038 / 0x01c */
    PVOID AtlThunkSListPtr;                                                 /**< 0x040 / 0x020 */
    PVOID IFEOKey;                                                          /**< 0x048 / 0x024 */
    union
    {
        ULONG CrossProcessFlags;                                            /**< 0x050 / 0x028 */
        struct
        {
            uint32_t ProcessInJob : 1;                                      /**< 0x050 / 0x028: Pos 0, 1 Bit */
            uint32_t ProcessInitializing : 1;                               /**< 0x050 / 0x028: Pos 1, 1 Bit */
            uint32_t ProcessUsingVEH : 1;                                   /**< 0x050 / 0x028: Pos 2, 1 Bit */
            uint32_t ProcessUsingVCH : 1;                                   /**< 0x050 / 0x028: Pos 3, 1 Bit */
            uint32_t ProcessUsingFTH : 1;                                   /**< 0x050 / 0x028: Pos 4, 1 Bit */
            uint32_t ReservedBits0 : 27;                                    /**< 0x050 / 0x028: Pos 5, 27 Bits */
        };
    };
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x054 / */
#endif
    union
    {
        PVOID KernelCallbackTable;                                          /**< 0x058 / 0x02c */
        PVOID UserSharedInfoPtr;                                            /**< 0x058 / 0x02c */
    };
    uint32_t SystemReserved;                                                /**< 0x060 / 0x030 */
    uint32_t AtlThunkSListPtr32;                                            /**< 0x064 / 0x034 */
    PVOID ApiSetMap;                                                        /**< 0x068 / 0x038 */
    uint32_t TlsExpansionCounter;                                           /**< 0x070 / 0x03c */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x074 / NA */
#endif
    struct _RTL_BITMAP *TlsBitmap;                                          /**< 0x078 / 0x040 */
    uint32_t TlsBitmapBits[2];                                              /**< 0x080 / 0x044 */
    PVOID ReadOnlySharedMemoryBase;                                         /**< 0x088 / 0x04c */
    PVOID HotpatchInformation;                                              /**< 0x090 / 0x050 - Retired in W81. */
    PVOID *ReadOnlyStaticServerData;                                        /**< 0x098 / 0x054 */
    PVOID AnsiCodePageData;                                                 /**< 0x0a0 / 0x058 */
    PVOID OemCodePageData;                                                  /**< 0x0a8 / 0x05c */
    PVOID UnicodeCaseTableData;                                             /**< 0x0b0 / 0x060 */
    uint32_t NumberOfProcessors;                                            /**< 0x0b8 / 0x064 */
    uint32_t NtGlobalFlag;                                                  /**< 0x0bc / 0x068 */
    LARGE_INTEGER CriticalSectionTimeout;                                   /**< 0x0c0 / 0x070 */
    SIZE_T HeapSegmentReserve;                                              /**< 0x0c8 / 0x078 */
    SIZE_T HeapSegmentCommit;                                               /**< 0x0d0 / 0x07c */
    SIZE_T HeapDeCommitTotalFreeThreshold;                                  /**< 0x0d8 / 0x080 */
    SIZE_T HeapDeCommitFreeBlockThreshold;                                  /**< 0x0e0 / 0x084 */
    uint32_t NumberOfHeaps;                                                 /**< 0x0e8 / 0x088 */
    uint32_t MaximumNumberOfHeaps;                                          /**< 0x0ec / 0x08c */
    PVOID *ProcessHeaps;                                                    /**< 0x0f0 / 0x090 */
    PVOID GdiSharedHandleTable;                                             /**< 0x0f8 / 0x094 */
    PVOID ProcessStarterHelper;                                             /**< 0x100 / 0x098 */
    uint32_t GdiDCAttributeList;                                            /**< 0x108 / 0x09c */
#if ARCH_BITS == 64
    uint32_t Padding3;                                                      /**< 0x10c / NA */
#endif
    struct _RTL_CRITICAL_SECTION *LoaderLock;                               /**< 0x110 / 0x0a0 */
    uint32_t OSMajorVersion;                                                /**< 0x118 / 0x0a4 */
    uint32_t OSMinorVersion;                                                /**< 0x11c / 0x0a8 */
    uint16_t OSBuildNumber;                                                 /**< 0x120 / 0x0ac */
    uint16_t OSCSDVersion;                                                  /**< 0x122 / 0x0ae */
    uint32_t OSPlatformId;                                                  /**< 0x124 / 0x0b0 */
    uint32_t ImageSubsystem;                                                /**< 0x128 / 0x0b4 */
    uint32_t ImageSubsystemMajorVersion;                                    /**< 0x12c / 0x0b8 */
    uint32_t ImageSubsystemMinorVersion;                                    /**< 0x130 / 0x0bc */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x134 / NA */
#endif
    SIZE_T ActiveProcessAffinityMask;                                       /**< 0x138 / 0x0c0 */
    uint32_t GdiHandleBuffer[ARCH_BITS == 64 ? 60 : 34];                    /**< 0x140 / 0x0c4 */
    PVOID PostProcessInitRoutine;                                           /**< 0x230 / 0x14c */
    PVOID TlsExpansionBitmap;                                               /**< 0x238 / 0x150 */
    uint32_t TlsExpansionBitmapBits[32];                                    /**< 0x240 / 0x154 */
    uint32_t SessionId;                                                     /**< 0x2c0 / 0x1d4 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x2c4 / NA */
#endif
    ULARGE_INTEGER AppCompatFlags;                                          /**< 0x2c8 / 0x1d8 */
    ULARGE_INTEGER AppCompatFlagsUser;                                      /**< 0x2d0 / 0x1e0 */
    PVOID pShimData;                                                        /**< 0x2d8 / 0x1e8 */
    PVOID AppCompatInfo;                                                    /**< 0x2e0 / 0x1ec */
    UNICODE_STRING CSDVersion;                                              /**< 0x2e8 / 0x1f0 */
    struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;                 /**< 0x2f8 / 0x1f8 */
    struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;                /**< 0x300 / 0x1fc */
    struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData;    /**< 0x308 / 0x200 */
    struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;                 /**< 0x310 / 0x204 */
    SIZE_T MinimumStackCommit;                                              /**< 0x318 / 0x208 */
    struct _FLS_CALLBACK_INFO *FlsCallback;                                 /**< 0x320 / 0x20c */
    LIST_ENTRY FlsListHead;                                                 /**< 0x328 / 0x210 */
    PVOID FlsBitmap;                                                        /**< 0x338 / 0x218 */
    uint32_t FlsBitmapBits[4];                                              /**< 0x340 / 0x21c */
    uint32_t FlsHighIndex;                                                  /**< 0x350 / 0x22c */
    PVOID WerRegistrationData;                                              /**< 0x358 / 0x230 */
    PVOID WerShipAssertPtr;                                                 /**< 0x360 / 0x234 */
    PVOID pContextData;                                                     /**< 0x368 / 0x238 - Retired in W80. */
    PVOID pImageHeaderHash;                                                 /**< 0x370 / 0x23c */
    union
    {
        uint32_t TracingFlags;                                              /**< 0x378 / 0x240 */
        uint32_t HeapTracingEnabled : 1;                                    /**< 0x378 / 0x240 : Pos 0, 1 Bit */
        uint32_t CritSecTracingEnabled : 1;                                 /**< 0x378 / 0x240 : Pos 1, 1 Bit */
        uint32_t SpareTracingBits : 30;                                     /**< 0x378 / 0x240 : Pos 3, 30 Bits - One bit more than W80 */
    };
    uint32_t Padding6;                                                      /**< 0x37c / 0x244 */
} PEB_W7;
typedef PEB_W7 *PPEB_W7;
AssertCompileMemberOffset(PEB_W7, NtGlobalFlag,   ARCH_BITS == 64 ?  0xbc :  0x68);
AssertCompileMemberOffset(PEB_W7, LoaderLock,     ARCH_BITS == 64 ? 0x110 :  0xa0);
AssertCompileMemberOffset(PEB_W7, ActiveProcessAffinityMask, ARCH_BITS == 64 ? 0x138 :  0xc0);
AssertCompileMemberOffset(PEB_W7, PostProcessInitRoutine,    ARCH_BITS == 64 ? 0x230 : 0x14c);
AssertCompileMemberOffset(PEB_W7, AppCompatFlags, ARCH_BITS == 64 ? 0x2c8 : 0x1d8);
AssertCompileSize(PEB_W7, ARCH_BITS == 64 ? 0x380 : 0x248);
#endif /* Just use PEB_COMMON for now. */

typedef struct _PEB_COMMON
{
    BOOLEAN InheritedAddressSpace;                                          /**< 0x000 / 0x000 */
    BOOLEAN ReadImageFileExecOptions;                                       /**< 0x001 / 0x001 */
    BOOLEAN BeingDebugged;                                                  /**< 0x002 / 0x002 */
    union
    {
        uint8_t BitField;                                                   /**< 0x003 / 0x003 */
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
        } Common;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W80 */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W80 */
            uint8_t IsPackagedProcess : 1;                                  /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W80 */
            uint8_t IsAppContainer : 1;                                     /**< 0x003 / 0x003 : Pos 5, 1 Bit - Differs from W80 */
            uint8_t IsProtectedProcessLight : 1;                            /**< 0x003 / 0x003 : Pos 6, 1 Bit - Differs from W80 */
            uint8_t SpareBits : 1;                                          /**< 0x003 / 0x003 : Pos 7, 1 Bit */
        } W81;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81 */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81 */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W81 */
            uint8_t IsPackagedProcess : 1;                                  /**< 0x003 / 0x003 : Pos 5, 1 Bit - Differs from W81 */
            uint8_t IsAppContainer : 1;                                     /**< 0x003 / 0x003 : Pos 6, 1 Bit - Differs from W81 */
            uint8_t SpareBits : 1;                                          /**< 0x003 / 0x003 : Pos 7, 1 Bit */
        } W80;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81, same as W80 & W6. */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81, same as W80 & W6. */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 4, 1 Bit - Added in W7; Differs from W81, same as W80. */
            uint8_t SpareBits : 3;                                          /**< 0x003 / 0x003 : Pos 5, 3 Bit - Differs from W81 & W80, more spare bits. */
        } W7;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81, same as W80 & W7. */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81, same as W80 & W7. */
            uint8_t SpareBits : 4;                                          /**< 0x003 / 0x003 : Pos 4, 4 Bit - Differs from W81, W80, & W7, more spare bits. */
        } W6;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t SpareBits : 7;                                          /**< 0x003 / 0x003 : Pos 1, 7 Bit - Differs from W81, W80, & W7, more spare bits. */
        } W52;
        struct
        {
            BOOLEAN SpareBool;
        } W51;
    } Diff0;
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x004 / NA */
#endif
    HANDLE Mutant;                                                          /**< 0x008 / 0x004 */
    PVOID ImageBaseAddress;                                                 /**< 0x010 / 0x008 */
    struct _PEB_LDR_DATA *Ldr;                                              /**< 0x018 / 0x00c */
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /**< 0x020 / 0x010 */
    PVOID SubSystemData;                                                    /**< 0x028 / 0x014 */
    HANDLE ProcessHeap;                                                     /**< 0x030 / 0x018 */
    struct _RTL_CRITICAL_SECTION *FastPebLock;                              /**< 0x038 / 0x01c */
    union
    {
        struct
        {
            PVOID AtlThunkSListPtr;                                         /**< 0x040 / 0x020 */
            PVOID IFEOKey;                                                  /**< 0x048 / 0x024 */
            union
            {
                ULONG CrossProcessFlags;                                    /**< 0x050 / 0x028 */
                struct
                {
                    uint32_t ProcessInJob : 1;                              /**< 0x050 / 0x028: Pos 0, 1 Bit */
                    uint32_t ProcessInitializing : 1;                       /**< 0x050 / 0x028: Pos 1, 1 Bit */
                    uint32_t ProcessUsingVEH : 1;                           /**< 0x050 / 0x028: Pos 2, 1 Bit */
                    uint32_t ProcessUsingVCH : 1;                           /**< 0x050 / 0x028: Pos 3, 1 Bit */
                    uint32_t ProcessUsingFTH : 1;                           /**< 0x050 / 0x028: Pos 4, 1 Bit */
                    uint32_t ReservedBits0 : 1;                             /**< 0x050 / 0x028: Pos 5, 27 Bits */
                } W7, W8, W80, W81;
                struct
                {
                    uint32_t ProcessInJob : 1;                              /**< 0x050 / 0x028: Pos 0, 1 Bit */
                    uint32_t ProcessInitializing : 1;                       /**< 0x050 / 0x028: Pos 1, 1 Bit */
                    uint32_t ReservedBits0 : 30;                            /**< 0x050 / 0x028: Pos 2, 30 Bits */
                } W6;
            };
#if ARCH_BITS == 64
            uint32_t Padding1;                                              /**< 0x054 / */
#endif
        } W6, W7, W8, W80, W81;
        struct
        {
            PVOID AtlThunkSListPtr;                                         /**< 0x040 / 0x020 */
            PVOID SparePtr2;                                                /**< 0x048 / 0x024 */
            uint32_t EnvironmentUpdateCount;                                /**< 0x050 / 0x028 */
#if ARCH_BITS == 64
            uint32_t Padding1;                                              /**< 0x054 / */
#endif
        } W52;
        struct
        {
            PVOID FastPebLockRoutine;                                       /**< NA / 0x020 */
            PVOID FastPebUnlockRoutine;                                     /**< NA / 0x024 */
            uint32_t EnvironmentUpdateCount;                                /**< NA / 0x028 */
        } W51;
    } Diff1;
    union
    {
        PVOID KernelCallbackTable;                                          /**< 0x058 / 0x02c */
        PVOID UserSharedInfoPtr;                                            /**< 0x058 / 0x02c - Alternative use in W6.*/
    };
    uint32_t SystemReserved;                                                /**< 0x060 / 0x030 */
    union
    {
        struct
        {
            uint32_t AtlThunkSListPtr32;                                    /**< 0x064 / 0x034 */
        } W7, W8, W80, W81;
        struct
        {
            uint32_t SpareUlong;                                            /**< 0x064 / 0x034 */
        } W52, W6;
        struct
        {
            uint32_t ExecuteOptions : 2;                                    /**< NA / 0x034: Pos 0, 2 Bits */
            uint32_t SpareBits : 30;                                        /**< NA / 0x034: Pos 2, 30 Bits */
        } W51;
    } Diff2;
    union
    {
        struct
        {
            PVOID ApiSetMap;                                                /**< 0x068 / 0x038 */
        } W7, W8, W80, W81;
        struct
        {
            struct _PEB_FREE_BLOCK *FreeList;                               /**< 0x068 / 0x038 */
        } W52, W6;
        struct
        {
            struct _PEB_FREE_BLOCK *FreeList;                               /**< NA / 0x038 */
        } W51;
    } Diff3;
    uint32_t TlsExpansionCounter;                                           /**< 0x070 / 0x03c */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x074 / NA */
#endif
    struct _RTL_BITMAP *TlsBitmap;                                          /**< 0x078 / 0x040 */
    uint32_t TlsBitmapBits[2];                                              /**< 0x080 / 0x044 */
    PVOID ReadOnlySharedMemoryBase;                                         /**< 0x088 / 0x04c */
    union
    {
        struct
        {
            PVOID SparePvoid0;                                              /**< 0x090 / 0x050 - HotpatchInformation before W81. */
        } W81;
        struct
        {
            PVOID HotpatchInformation;                                      /**< 0x090 / 0x050 - Retired in W81. */
        } W6, W7, W80;
        struct
        {
            PVOID ReadOnlySharedMemoryHeap;
        } W52;
    } Diff4;
    PVOID *ReadOnlyStaticServerData;                                        /**< 0x098 / 0x054 */
    PVOID AnsiCodePageData;                                                 /**< 0x0a0 / 0x058 */
    PVOID OemCodePageData;                                                  /**< 0x0a8 / 0x05c */
    PVOID UnicodeCaseTableData;                                             /**< 0x0b0 / 0x060 */
    uint32_t NumberOfProcessors;                                            /**< 0x0b8 / 0x064 */
    uint32_t NtGlobalFlag;                                                  /**< 0x0bc / 0x068 */
    LARGE_INTEGER CriticalSectionTimeout;                                   /**< 0x0c0 / 0x070 */
    SIZE_T HeapSegmentReserve;                                              /**< 0x0c8 / 0x078 */
    SIZE_T HeapSegmentCommit;                                               /**< 0x0d0 / 0x07c */
    SIZE_T HeapDeCommitTotalFreeThreshold;                                  /**< 0x0d8 / 0x080 */
    SIZE_T HeapDeCommitFreeBlockThreshold;                                  /**< 0x0e0 / 0x084 */
    uint32_t NumberOfHeaps;                                                 /**< 0x0e8 / 0x088 */
    uint32_t MaximumNumberOfHeaps;                                          /**< 0x0ec / 0x08c */
    PVOID *ProcessHeaps;                                                    /**< 0x0f0 / 0x090 */
    PVOID GdiSharedHandleTable;                                             /**< 0x0f8 / 0x094 */
    PVOID ProcessStarterHelper;                                             /**< 0x100 / 0x098 */
    uint32_t GdiDCAttributeList;                                            /**< 0x108 / 0x09c */
#if ARCH_BITS == 64
    uint32_t Padding3;                                                      /**< 0x10c / NA */
#endif
    struct _RTL_CRITICAL_SECTION *LoaderLock;                               /**< 0x110 / 0x0a0 */
    uint32_t OSMajorVersion;                                                /**< 0x118 / 0x0a4 */
    uint32_t OSMinorVersion;                                                /**< 0x11c / 0x0a8 */
    uint16_t OSBuildNumber;                                                 /**< 0x120 / 0x0ac */
    uint16_t OSCSDVersion;                                                  /**< 0x122 / 0x0ae */
    uint32_t OSPlatformId;                                                  /**< 0x124 / 0x0b0 */
    uint32_t ImageSubsystem;                                                /**< 0x128 / 0x0b4 */
    uint32_t ImageSubsystemMajorVersion;                                    /**< 0x12c / 0x0b8 */
    uint32_t ImageSubsystemMinorVersion;                                    /**< 0x130 / 0x0bc */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x134 / NA */
#endif
    union
    {
        struct
        {
            SIZE_T ActiveProcessAffinityMask;                               /**< 0x138 / 0x0c0 */
        } W7, W8, W80, W81;
        struct
        {
            SIZE_T ImageProcessAffinityMask;                                /**< 0x138 / 0x0c0 */
        } W52, W6;
    } Diff5;
    uint32_t GdiHandleBuffer[ARCH_BITS == 64 ? 60 : 34];                    /**< 0x140 / 0x0c4 */
    PVOID PostProcessInitRoutine;                                           /**< 0x230 / 0x14c */
    PVOID TlsExpansionBitmap;                                               /**< 0x238 / 0x150 */
    uint32_t TlsExpansionBitmapBits[32];                                    /**< 0x240 / 0x154 */
    uint32_t SessionId;                                                     /**< 0x2c0 / 0x1d4 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x2c4 / NA */
#endif
    ULARGE_INTEGER AppCompatFlags;                                          /**< 0x2c8 / 0x1d8 */
    ULARGE_INTEGER AppCompatFlagsUser;                                      /**< 0x2d0 / 0x1e0 */
    PVOID pShimData;                                                        /**< 0x2d8 / 0x1e8 */
    PVOID AppCompatInfo;                                                    /**< 0x2e0 / 0x1ec */
    UNICODE_STRING CSDVersion;                                              /**< 0x2e8 / 0x1f0 */
    struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;                 /**< 0x2f8 / 0x1f8 */
    struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;                /**< 0x300 / 0x1fc */
    struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData;    /**< 0x308 / 0x200 */
    struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;                 /**< 0x310 / 0x204 */
    SIZE_T MinimumStackCommit;                                              /**< 0x318 / 0x208 */
    /* End of PEB in W52 (Windows XP (RTM))! */
    struct _FLS_CALLBACK_INFO *FlsCallback;                                 /**< 0x320 / 0x20c */
    LIST_ENTRY FlsListHead;                                                 /**< 0x328 / 0x210 */
    PVOID FlsBitmap;                                                        /**< 0x338 / 0x218 */
    uint32_t FlsBitmapBits[4];                                              /**< 0x340 / 0x21c */
    uint32_t FlsHighIndex;                                                  /**< 0x350 / 0x22c */
    /* End of PEB in W52 (Windows Server 2003)! */
    PVOID WerRegistrationData;                                              /**< 0x358 / 0x230 */
    PVOID WerShipAssertPtr;                                                 /**< 0x360 / 0x234 */
    /* End of PEB in W6 (windows Vista)! */
    union
    {
        struct
        {
            PVOID pUnused;                                                  /**< 0x368 / 0x238 - Was pContextData in W7. */
        } W8, W80, W81;
        struct
        {
            PVOID pContextData;                                             /**< 0x368 / 0x238 - Retired in W80. */
        } W7;
    } Diff6;
    PVOID pImageHeaderHash;                                                 /**< 0x370 / 0x23c */
    union
    {
        uint32_t TracingFlags;                                              /**< 0x378 / 0x240 */
        struct
        {
            uint32_t HeapTracingEnabled : 1;                                /**< 0x378 / 0x240 : Pos 0, 1 Bit */
            uint32_t CritSecTracingEnabled : 1;                             /**< 0x378 / 0x240 : Pos 1, 1 Bit */
            uint32_t LibLoaderTracingEnabled : 1;                           /**< 0x378 / 0x240 : Pos 2, 1 Bit */
            uint32_t SpareTracingBits : 29;                                 /**< 0x378 / 0x240 : Pos 3, 29 Bits */
        } W8, W80, W81;
        struct
        {
            uint32_t HeapTracingEnabled : 1;                                /**< 0x378 / 0x240 : Pos 0, 1 Bit */
            uint32_t CritSecTracingEnabled : 1;                             /**< 0x378 / 0x240 : Pos 1, 1 Bit */
            uint32_t SpareTracingBits : 30;                                 /**< 0x378 / 0x240 : Pos 3, 30 Bits - One bit more than W80 */
        } W7;
    } Diff7;
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x37c / NA */
#endif
    uint64_t CsrServerReadOnlySharedMemoryBase;                             /**< 0x380 / 0x248 */
} PEB_COMMON;
typedef PEB_COMMON *PPEB_COMMON;

AssertCompileMemberOffset(PEB_COMMON, ProcessHeap,    ARCH_BITS == 64 ?  0x30 :  0x18);
AssertCompileMemberOffset(PEB_COMMON, SystemReserved, ARCH_BITS == 64 ?  0x60 :  0x30);
AssertCompileMemberOffset(PEB_COMMON, TlsExpansionCounter,   ARCH_BITS == 64 ?  0x70 :  0x3c);
AssertCompileMemberOffset(PEB_COMMON, NtGlobalFlag,   ARCH_BITS == 64 ?  0xbc :  0x68);
AssertCompileMemberOffset(PEB_COMMON, LoaderLock,     ARCH_BITS == 64 ? 0x110 :  0xa0);
AssertCompileMemberOffset(PEB_COMMON, Diff5.W52.ImageProcessAffinityMask, ARCH_BITS == 64 ? 0x138 :  0xc0);
AssertCompileMemberOffset(PEB_COMMON, PostProcessInitRoutine,    ARCH_BITS == 64 ? 0x230 : 0x14c);
AssertCompileMemberOffset(PEB_COMMON, AppCompatFlags, ARCH_BITS == 64 ? 0x2c8 : 0x1d8);
AssertCompileSize(PEB_COMMON, ARCH_BITS == 64 ? 0x388 : 0x250);

/** The size of the windows 8.1 PEB structure.  */
#define PEB_SIZE_W81    sizeof(PEB_COMMON)
/** The size of the windows 8.0 PEB structure.  */
#define PEB_SIZE_W80    sizeof(PEB_COMMON)
/** The size of the windows 7 PEB structure.  */
#define PEB_SIZE_W7     RT_UOFFSETOF(PEB_COMMON, CsrServerReadOnlySharedMemoryBase)
/** The size of the windows vista PEB structure.  */
#define PEB_SIZE_W6     RT_UOFFSETOF(PEB_COMMON, Diff3)
/** The size of the windows server 2003 PEB structure.  */
#define PEB_SIZE_W52    RT_UOFFSETOF(PEB_COMMON, WerRegistrationData)
/** The size of the windows XP PEB structure.  */
#define PEB_SIZE_W51    RT_UOFFSETOF(PEB_COMMON, FlsCallback)

#if 0
#if 0
typedef struct _NT_TIB
{
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union
    {
        PVOID FiberData;
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB;
typedef NT_TIB *PNT_TIB;
#endif

/* Windows 8.0 and Windows 8.1 TEB: */
typedef struct _TEB_W8
{
    NT_TIB NtTib;                                                           /**< 0x000 / 0x000 */
    PVOID EnvironmentPointer;                                               /**< 0x038 / 0x01c */
    CLIENT_ID ClientId;                                                     /**< 0x040 / 0x020 */
    PVOID ActiveRpcHandle;                                                  /**< 0x050 / 0x028 */
    PVOID ThreadLocalStoragePointer;                                        /**< 0x058 / 0x02c */
    PPEB_W81 ProcessEnvironmentBlock;                                       /**< 0x060 / 0x030 */
    uint32_t LastErrorValue;                                                /**< 0x068 / 0x034 */
    uint32_t CountOfOwnedCriticalSections;                                  /**< 0x06c / 0x038 */
    PVOID CsrClientThread;                                                  /**< 0x070 / 0x03c */
    PVOID Win32ThreadInfo;                                                  /**< 0x078 / 0x040 */
    uint32_t User32Reserved[26];                                            /**< 0x080 / 0x044 */
    uint32_t UserReserved[5];                                               /**< 0x0e8 / 0x0ac */
    PVOID WOW32Reserved;                                                    /**< 0x100 / 0x0c0 */
    uint32_t CurrentLocale;                                                 /**< 0x108 / 0x0c4 */
    uint32_t FpSoftwareStatusRegister;                                      /**< 0x10c / 0x0c8 */
    PVOID SystemReserved1[54];                                              /**< 0x110 / 0x0cc */
    uint32_t ExceptionCode;                                                 /**< 0x2c0 / 0x1a4 */
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x2c4 / NA */
#endif
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;        /**< 0x2c8 / 0x1a8 */
    uint8_t SpareBytes[ARCH_BITS == 64 ? 24 : 36];                          /**< 0x2d0 / 0x1ac */
    uint32_t TxFsContext;                                                   /**< 0x2e8 / 0x1d0 */
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x2ec / NA */
#endif
    /*_GDI_TEB_BATCH*/ uint8_t GdiTebBatch[ARCH_BITS == 64 ? 0x4e8 :0x4e0]; /**< 0x2f0 / 0x1d4 */
    CLIENT_ID RealClientId;                                                 /**< 0x7d8 / 0x6b4 */
    HANDLE GdiCachedProcessHandle;                                          /**< 0x7e8 / 0x6bc */
    uint32_t GdiClientPID;                                                  /**< 0x7f0 / 0x6c0 */
    uint32_t GdiClientTID;                                                  /**< 0x7f4 / 0x6c4 */
    PVOID GdiThreadLocalInfo;                                               /**< 0x7f8 / 0x6c8 */
    SIZE_T Win32ClientInfo[62];                                             /**< 0x800 / 0x6cc */
    PVOID glDispatchTable[233];                                             /**< 0x9f0 / 0x7c4 */
    SIZE_T glReserved1[29];                                                 /**< 0x1138 / 0xb68 */
    PVOID glReserved2;                                                      /**< 0x1220 / 0xbdc */
    PVOID glSectionInfo;                                                    /**< 0x1228 / 0xbe0 */
    PVOID glSection;                                                        /**< 0x1230 / 0xbe4 */
    PVOID glTable;                                                          /**< 0x1238 / 0xbe8 */
    PVOID glCurrentRC;                                                      /**< 0x1240 / 0xbec */
    PVOID glContext;                                                        /**< 0x1248 / 0xbf0 */
    NTSTATUS LastStatusValue;                                               /**< 0x1250 / 0xbf4 */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x1254 / NA */
#endif
    UNICODE_STRING StaticUnicodeString;                                     /**< 0x1258 / 0xbf8 */
    WCHAR StaticUnicodeBuffer[261];                                         /**< 0x1268 / 0xc00 */
#if ARCH_BITS == 64
    WCHAR Padding3[3];                                                      /**< 0x1472 / NA */
#endif
    PVOID DeallocationStack;                                                /**< 0x1478 / 0xe0c */
    PVOID TlsSlots[64];                                                     /**< 0x1480 / 0xe10 */
    LIST_ENTRY TlsLinks;                                                    /**< 0x1680 / 0xf10 */
    PVOID Vdm;                                                              /**< 0x1690 / 0xf18 */
    PVOID ReservedForNtRpc;                                                 /**< 0x1698 / 0xf1c */
    PVOID DbgSsReserved[2];                                                 /**< 0x16a0 / 0xf20 */
    uint32_t HardErrorMode;                                                 /**< 0x16b0 / 0xf28 */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x16b4 / NA */
#endif
    PVOID Instrumentation[ARCH_BITS == 64 ? 11 : 9];                        /**< 0x16b8 / 0xf2c */
    GUID ActivityId;                                                        /**< 0x1710 / 0xf50 */
    PVOID SubProcessTag;                                                    /**< 0x1720 / 0xf60 */
    PVOID PerflibData;                                                      /**< 0x1728 / 0xf64 */
    PVOID EtwTraceData;                                                     /**< 0x1730 / 0xf68 */
    PVOID WinSockData;                                                      /**< 0x1738 / 0xf6c */
    uint32_t GdiBatchCount;                                                 /**< 0x1740 / 0xf70 */
    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;                             /**< 0x1744 / 0xf74 */
        uint32_t IdealProcessorValue;                                       /**< 0x1744 / 0xf74 */
        struct
        {
            uint8_t ReservedPad1;                                           /**< 0x1744 / 0xf74 */
            uint8_t ReservedPad2;                                           /**< 0x1745 / 0xf75 */
            uint8_t ReservedPad3;                                           /**< 0x1746 / 0xf76 */
            uint8_t IdealProcessor;                                         /**< 0x1747 / 0xf77 */
        };
    };
    uint32_t GuaranteedStackBytes;                                          /**< 0x1748 / 0xf78 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x174c / NA */
#endif
    PVOID ReservedForPerf;                                                  /**< 0x1750 / 0xf7c */
    PVOID ReservedForOle;                                                   /**< 0x1758 / 0xf80 */
    uint32_t WaitingOnLoaderLock;                                           /**< 0x1760 / 0xf84 */
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x1764 / NA */
#endif
    PVOID SavedPriorityState;                                               /**< 0x1768 / 0xf88 */
    SIZE_T ReservedForCodeCoverage;                                         /**< 0x1770 / 0xf8c */
    PVOID ThreadPoolData;                                                   /**< 0x1778 / 0xf90 */
    PVOID TlsExpansionSlots;                                                /**< 0x1780 / 0xf94 */
#if ARCH_BITS == 64
    PVOID DallocationBStore;                                                /**< 0x1788 / NA */
    PVOID BStoreLimit;                                                      /**< 0x1790 / NA */
#endif
    uint32_t MuiGeneration;                                                 /**< 0x1798 / 0xf98 */
    uint32_t IsImpersonating;                                               /**< 0x179c / 0xf9c */
    PVOID NlsCache;                                                         /**< 0x17a0 / 0xfa0 */
    PVOID pShimData;                                                        /**< 0x17a8 / 0xfa4 */
    uint16_t HeapVirtualAffinity;                                           /**< 0x17b0 / 0xfa8 */
    uint16_t LowFragHeapDataSlot;                                           /**< 0x17b2 / 0xfaa */
#if ARCH_BITS == 64
    uint32_t Padding7;                                                      /**< 0x17b4 / NA */
#endif
    HANDLE CurrentTransactionHandle;                                        /**< 0x17b8 / 0xfac */
    struct _TEB_ACTIVE_FRAME *ActiveFrame;                                  /**< 0x17c0 / 0xfb0 */
    PVOID FlsData;                                                          /**< 0x17c8 / 0xfb4 */
    PVOID PreferredLanguages;                                               /**< 0x17d0 / 0xfb8 */
    PVOID UserPrefLanguages;                                                /**< 0x17d8 / 0xfbc */
    PVOID MergedPrefLanguages;                                              /**< 0x17e0 / 0xfc0 */
    uint32_t MuiImpersonation;                                              /**< 0x17e8 / 0xfc4 */
    union
    {
        uint16_t CrossTebFlags;                                             /**< 0x17ec / 0xfc8 */
        struct
        {
            uint16_t SpareCrossTebBits : 16;                                /**< 0x17ec / 0xfc8 : Pos 0, 16 Bits */
        };
    };
    union
    {
        uint16_t SameTebFlags;                                              /**< 0x17ee / 0xfca */
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t DisableUserStackWalk : 1;                              /**< 0x17ee / 0xfca : Pos 8, 1 Bit */
            uint16_t RtlExceptionAttached : 1;                              /**< 0x17ee / 0xfca : Pos 9, 1 Bit */
            uint16_t InitialThread : 1;                                     /**< 0x17ee / 0xfca : Pos 10, 1 Bit */
            uint16_t SessionAware : 1;                                      /**< 0x17ee / 0xfca : Pos 11, 1 Bit - New Since W7. */
            uint16_t SpareSameTebBits : 4;                                  /**< 0x17ee / 0xfca : Pos 12, 4 Bits */
        };
    };
    PVOID TxnScopeEnterCallback;                                            /**< 0x17f0 / 0xfcc */
    PVOID TxnScopeExitCallback;                                             /**< 0x17f8 / 0xfd0 */
    PVOID TxnScopeContext;                                                  /**< 0x1800 / 0xfd4 */
    uint32_t LockCount;                                                     /**< 0x1808 / 0xfd8 */
    uint32_t SpareUlong0;                                                   /**< 0x180c / 0xfdc */
    PVOID ResourceRetValue;                                                 /**< 0x1810 / 0xfe0 */
    PVOID ReservedForWdf;                                                   /**< 0x1818 / 0xfe4 - New Since W7. */
} TEB_W8;
typedef TEB_W8 *PTEB_W8;
AssertCompileMemberOffset(TEB_W8, ExceptionCode,        ARCH_BITS == 64 ?  0x2c0 : 0x1a4);
AssertCompileMemberOffset(TEB_W8, LastStatusValue,      ARCH_BITS == 64 ? 0x1250 : 0xbf4);
AssertCompileMemberOffset(TEB_W8, DeallocationStack,    ARCH_BITS == 64 ? 0x1478 : 0xe0c);
AssertCompileMemberOffset(TEB_W8, ReservedForNtRpc,     ARCH_BITS == 64 ? 0x1698 : 0xf1c);
AssertCompileMemberOffset(TEB_W8, Instrumentation,      ARCH_BITS == 64 ? 0x16b8 : 0xf2c);
AssertCompileMemberOffset(TEB_W8, GuaranteedStackBytes, ARCH_BITS == 64 ? 0x1748 : 0xf78);
AssertCompileMemberOffset(TEB_W8, MuiImpersonation,     ARCH_BITS == 64 ? 0x17e8 : 0xfc4);
AssertCompileMemberOffset(TEB_W8, LockCount,            ARCH_BITS == 64 ? 0x1808 : 0xfd8);
AssertCompileSize(TEB_W8, ARCH_BITS == 64 ? 0x1820 : 0xfe8);

typedef TEB_W8  TEB_W81;
typedef PTEB_W8 PTEB_W81;
typedef TEB_W8  TEB_W80;
typedef PTEB_W8 PTEB_W80;

/* Windows 7 TEB: */
typedef struct _TEB_W7
{
    NT_TIB NtTib;                                                           /**< 0x000 / 0x000 */
    PVOID EnvironmentPointer;                                               /**< 0x038 / 0x01c */
    CLIENT_ID ClientId;                                                     /**< 0x040 / 0x020 */
    PVOID ActiveRpcHandle;                                                  /**< 0x050 / 0x028 */
    PVOID ThreadLocalStoragePointer;                                        /**< 0x058 / 0x02c */
    PPEB_W7 ProcessEnvironmentBlock;                                        /**< 0x060 / 0x030 */
    uint32_t LastErrorValue;                                                /**< 0x068 / 0x034 */
    uint32_t CountOfOwnedCriticalSections;                                  /**< 0x06c / 0x038 */
    PVOID CsrClientThread;                                                  /**< 0x070 / 0x03c */
    PVOID Win32ThreadInfo;                                                  /**< 0x078 / 0x040 */
    uint32_t User32Reserved[26];                                            /**< 0x080 / 0x044 */
    uint32_t UserReserved[5];                                               /**< 0x0e8 / 0x0ac */
    PVOID WOW32Reserved;                                                    /**< 0x100 / 0x0c0 */
    uint32_t CurrentLocale;                                                 /**< 0x108 / 0x0c4 */
    uint32_t FpSoftwareStatusRegister;                                      /**< 0x10c / 0x0c8 */
    PVOID SystemReserved1[54];                                              /**< 0x110 / 0x0cc */
    uint32_t ExceptionCode;                                                 /**< 0x2c0 / 0x1a4 */
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x2c4 / NA */
#endif
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;        /**< 0x2c8 / 0x1a8 */
    uint8_t SpareBytes[ARCH_BITS == 64 ? 24 : 36];                          /**< 0x2d0 / 0x1ac */
    uint32_t TxFsContext;                                                   /**< 0x2e8 / 0x1d0 */
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x2ec / NA */
#endif
    /*_GDI_TEB_BATCH*/ uint8_t GdiTebBatch[ARCH_BITS == 64 ? 0x4e8 :0x4e0]; /**< 0x2f0 / 0x1d4 */
    CLIENT_ID RealClientId;                                                 /**< 0x7d8 / 0x6b4 */
    HANDLE GdiCachedProcessHandle;                                          /**< 0x7e8 / 0x6bc */
    uint32_t GdiClientPID;                                                  /**< 0x7f0 / 0x6c0 */
    uint32_t GdiClientTID;                                                  /**< 0x7f4 / 0x6c4 */
    PVOID GdiThreadLocalInfo;                                               /**< 0x7f8 / 0x6c8 */
    SIZE_T Win32ClientInfo[62];                                             /**< 0x800 / 0x6cc */
    PVOID glDispatchTable[233];                                             /**< 0x9f0 / 0x7c4 */
    SIZE_T glReserved1[29];                                                 /**< 0x1138 / 0xb68 */
    PVOID glReserved2;                                                      /**< 0x1220 / 0xbdc */
    PVOID glSectionInfo;                                                    /**< 0x1228 / 0xbe0 */
    PVOID glSection;                                                        /**< 0x1230 / 0xbe4 */
    PVOID glTable;                                                          /**< 0x1238 / 0xbe8 */
    PVOID glCurrentRC;                                                      /**< 0x1240 / 0xbec */
    PVOID glContext;                                                        /**< 0x1248 / 0xbf0 */
    NTSTATUS LastStatusValue;                                               /**< 0x1250 / 0xbf4 */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x1254 / NA */
#endif
    UNICODE_STRING StaticUnicodeString;                                     /**< 0x1258 / 0xbf8 */
    WCHAR StaticUnicodeBuffer[261];                                         /**< 0x1268 / 0xc00 */
#if ARCH_BITS == 64
    WCHAR Padding3[3];                                                      /**< 0x1472 / NA */
#endif
    PVOID DeallocationStack;                                                /**< 0x1478 / 0xe0c */
    PVOID TlsSlots[64];                                                     /**< 0x1480 / 0xe10 */
    LIST_ENTRY TlsLinks;                                                    /**< 0x1680 / 0xf10 */
    PVOID Vdm;                                                              /**< 0x1690 / 0xf18 */
    PVOID ReservedForNtRpc;                                                 /**< 0x1698 / 0xf1c */
    PVOID DbgSsReserved[2];                                                 /**< 0x16a0 / 0xf20 */
    uint32_t HardErrorMode;                                                 /**< 0x16b0 / 0xf28 */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x16b4 / NA */
#endif
    PVOID Instrumentation[ARCH_BITS == 64 ? 11 : 9];                        /**< 0x16b8 / 0xf2c */
    GUID ActivityId;                                                        /**< 0x1710 / 0xf50 */
    PVOID SubProcessTag;                                                    /**< 0x1720 / 0xf60 */
    PVOID EtwLocalData;                                                     /**< 0x1728 / 0xf64 */
    PVOID EtwTraceData;                                                     /**< 0x1730 / 0xf68 */
    PVOID WinSockData;                                                      /**< 0x1738 / 0xf6c */
    uint32_t GdiBatchCount;                                                 /**< 0x1740 / 0xf70 */
    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;                             /**< 0x1744 / 0xf74 */
        uint32_t IdealProcessorValue;                                       /**< 0x1744 / 0xf74 */
        struct
        {
            uint8_t ReservedPad1;                                           /**< 0x1744 / 0xf74 */
            uint8_t ReservedPad2;                                           /**< 0x1745 / 0xf75 */
            uint8_t ReservedPad3;                                           /**< 0x1746 / 0xf76 */
            uint8_t IdealProcessor;                                         /**< 0x1747 / 0xf77 */
        };
    };
    uint32_t GuaranteedStackBytes;                                          /**< 0x1748 / 0xf78 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x174c / NA */
#endif
    PVOID ReservedForPerf;                                                  /**< 0x1750 / 0xf7c */
    PVOID ReservedForOle;                                                   /**< 0x1758 / 0xf80 */
    uint32_t WaitingOnLoaderLock;                                           /**< 0x1760 / 0xf84 */
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x1764 / NA */
#endif
    PVOID SavedPriorityState;                                               /**< 0x1768 / 0xf88 */
    SIZE_T SoftPatchPtr1;                                                   /**< 0x1770 / 0xf8c */
    PVOID ThreadPoolData;                                                   /**< 0x1778 / 0xf90 */
    PVOID TlsExpansionSlots;                                                /**< 0x1780 / 0xf94 */
#if ARCH_BITS == 64
    PVOID DallocationBStore;                                                /**< 0x1788 / NA */
    PVOID BStoreLimit;                                                      /**< 0x1790 / NA */
#endif
    uint32_t MuiGeneration;                                                 /**< 0x1798 / 0xf98 */
    uint32_t IsImpersonating;                                               /**< 0x179c / 0xf9c */
    PVOID NlsCache;                                                         /**< 0x17a0 / 0xfa0 */
    PVOID pShimData;                                                        /**< 0x17a8 / 0xfa4 */
    uint32_t HeapVirtualAffinity;                                           /**< 0x17b0 / 0xfa8 */
#if ARCH_BITS == 64
    uint32_t Padding7;                                                      /**< 0x17b4 / NA */
#endif
    HANDLE CurrentTransactionHandle;                                        /**< 0x17b8 / 0xfac */
    struct _TEB_ACTIVE_FRAME *ActiveFrame;                                  /**< 0x17c0 / 0xfb0 */
    PVOID FlsData;                                                          /**< 0x17c8 / 0xfb4 */
    PVOID PreferredLanguages;                                               /**< 0x17d0 / 0xfb8 */
    PVOID UserPrefLanguages;                                                /**< 0x17d8 / 0xfbc */
    PVOID MergedPrefLanguages;                                              /**< 0x17e0 / 0xfc0 */
    uint32_t MuiImpersonation;                                              /**< 0x17e8 / 0xfc4 */
    union
    {
        uint16_t CrossTebFlags;                                             /**< 0x17ec / 0xfc8 */
        struct
        {
            uint16_t SpareCrossTebBits : 16;                                /**< 0x17ec / 0xfc8 : Pos 0, 16 Bits */
        };
    };
    union
    {
        uint16_t SameTebFlags;                                              /**< 0x17ee / 0xfca */
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t DisableUserStackWalk : 1;                              /**< 0x17ee / 0xfca : Pos 8, 1 Bit */
            uint16_t RtlExceptionAttached : 1;                              /**< 0x17ee / 0xfca : Pos 9, 1 Bit */
            uint16_t InitialThread : 1;                                     /**< 0x17ee / 0xfca : Pos 10, 1 Bit */
            uint16_t SpareSameTebBits : 5;                                  /**< 0x17ee / 0xfca : Pos 12, 4 Bits */
        };
    };
    PVOID TxnScopeEnterCallback;                                            /**< 0x17f0 / 0xfcc */
    PVOID TxnScopeExitCallback;                                             /**< 0x17f8 / 0xfd0 */
    PVOID TxnScopeContext;                                                  /**< 0x1800 / 0xfd4 */
    uint32_t LockCount;                                                     /**< 0x1808 / 0xfd8 */
    uint32_t SpareUlong0;                                                   /**< 0x180c / 0xfdc */
    PVOID ResourceRetValue;                                                 /**< 0x1810 / 0xfe0 */
} TEB_W7;
typedef TEB_W7 *PTEB_W7;
AssertCompileMemberOffset(TEB_W7, ExceptionCode,        ARCH_BITS == 64 ?  0x2c0 : 0x1a4);
AssertCompileMemberOffset(TEB_W7, LastStatusValue,      ARCH_BITS == 64 ? 0x1250 : 0xbf4);
AssertCompileMemberOffset(TEB_W7, DeallocationStack,    ARCH_BITS == 64 ? 0x1478 : 0xe0c);
AssertCompileMemberOffset(TEB_W7, ReservedForNtRpc,     ARCH_BITS == 64 ? 0x1698 : 0xf1c);
AssertCompileMemberOffset(TEB_W7, Instrumentation,      ARCH_BITS == 64 ? 0x16b8 : 0xf2c);
AssertCompileMemberOffset(TEB_W7, GuaranteedStackBytes, ARCH_BITS == 64 ? 0x1748 : 0xf78);
AssertCompileMemberOffset(TEB_W7, MuiImpersonation,     ARCH_BITS == 64 ? 0x17e8 : 0xfc4);
AssertCompileMemberOffset(TEB_W7, LockCount,            ARCH_BITS == 64 ? 0x1808 : 0xfd8);
AssertCompileSize(TEB_W7, ARCH_BITS == 64 ? 0x1818 : 0xfe4);
#endif


typedef struct _ACTIVATION_CONTEXT_STACK
{
   uint32_t Flags;
   uint32_t NextCookieSequenceNumber;
   PVOID ActiveFrame;
   LIST_ENTRY FrameListCache;
} ACTIVATION_CONTEXT_STACK;


/* Common TEB. */
typedef struct _TEB_COMMON
{
    NT_TIB NtTib;                                                           /**< 0x000 / 0x000 */
    PVOID EnvironmentPointer;                                               /**< 0x038 / 0x01c */
    CLIENT_ID ClientId;                                                     /**< 0x040 / 0x020 */
    PVOID ActiveRpcHandle;                                                  /**< 0x050 / 0x028 */
    PVOID ThreadLocalStoragePointer;                                        /**< 0x058 / 0x02c */
    PPEB_COMMON ProcessEnvironmentBlock;                                    /**< 0x060 / 0x030 */
    uint32_t LastErrorValue;                                                /**< 0x068 / 0x034 */
    uint32_t CountOfOwnedCriticalSections;                                  /**< 0x06c / 0x038 */
    PVOID CsrClientThread;                                                  /**< 0x070 / 0x03c */
    PVOID Win32ThreadInfo;                                                  /**< 0x078 / 0x040 */
    uint32_t User32Reserved[26];                                            /**< 0x080 / 0x044 */
    uint32_t UserReserved[5];                                               /**< 0x0e8 / 0x0ac */
    PVOID WOW32Reserved;                                                    /**< 0x100 / 0x0c0 */
    uint32_t CurrentLocale;                                                 /**< 0x108 / 0x0c4 */
    uint32_t FpSoftwareStatusRegister;                                      /**< 0x10c / 0x0c8 */
    PVOID SystemReserved1[54];                                              /**< 0x110 / 0x0cc */
    uint32_t ExceptionCode;                                                 /**< 0x2c0 / 0x1a4 */
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x2c4 / NA */
#endif
    union
    {
        struct
        {
            struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;/**< 0x2c8 / 0x1a8 */
            uint8_t SpareBytes[ARCH_BITS == 64 ? 24 : 36];                  /**< 0x2d0 / 0x1ac */
        } W52, W6, W7, W8, W80, W81;
#if ARCH_BITS == 32
        struct
        {
            ACTIVATION_CONTEXT_STACK ActivationContextStack;                /**< NA / 0x1a8 */
            uint8_t SpareBytes[20];                                         /**< NA / 0x1bc */
        } W51;
#endif
    } Diff0;
    union
    {
        struct
        {
            uint32_t TxFsContext;                                           /**< 0x2e8 / 0x1d0 */
        } W6, W7, W8, W80, W81;
        struct
        {
            uint32_t SpareBytesContinues;                                   /**< 0x2e8 / 0x1d0 */
        } W52;
    } Diff1;
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x2ec / NA */
#endif
    /*_GDI_TEB_BATCH*/ uint8_t GdiTebBatch[ARCH_BITS == 64 ? 0x4e8 :0x4e0]; /**< 0x2f0 / 0x1d4 */
    CLIENT_ID RealClientId;                                                 /**< 0x7d8 / 0x6b4 */
    HANDLE GdiCachedProcessHandle;                                          /**< 0x7e8 / 0x6bc */
    uint32_t GdiClientPID;                                                  /**< 0x7f0 / 0x6c0 */
    uint32_t GdiClientTID;                                                  /**< 0x7f4 / 0x6c4 */
    PVOID GdiThreadLocalInfo;                                               /**< 0x7f8 / 0x6c8 */
    SIZE_T Win32ClientInfo[62];                                             /**< 0x800 / 0x6cc */
    PVOID glDispatchTable[233];                                             /**< 0x9f0 / 0x7c4 */
    SIZE_T glReserved1[29];                                                 /**< 0x1138 / 0xb68 */
    PVOID glReserved2;                                                      /**< 0x1220 / 0xbdc */
    PVOID glSectionInfo;                                                    /**< 0x1228 / 0xbe0 */
    PVOID glSection;                                                        /**< 0x1230 / 0xbe4 */
    PVOID glTable;                                                          /**< 0x1238 / 0xbe8 */
    PVOID glCurrentRC;                                                      /**< 0x1240 / 0xbec */
    PVOID glContext;                                                        /**< 0x1248 / 0xbf0 */
    NTSTATUS LastStatusValue;                                               /**< 0x1250 / 0xbf4 */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x1254 / NA */
#endif
    UNICODE_STRING StaticUnicodeString;                                     /**< 0x1258 / 0xbf8 */
    WCHAR StaticUnicodeBuffer[261];                                         /**< 0x1268 / 0xc00 */
#if ARCH_BITS == 64
    WCHAR Padding3[3];                                                      /**< 0x1472 / NA */
#endif
    PVOID DeallocationStack;                                                /**< 0x1478 / 0xe0c */
    PVOID TlsSlots[64];                                                     /**< 0x1480 / 0xe10 */
    LIST_ENTRY TlsLinks;                                                    /**< 0x1680 / 0xf10 */
    PVOID Vdm;                                                              /**< 0x1690 / 0xf18 */
    PVOID ReservedForNtRpc;                                                 /**< 0x1698 / 0xf1c */
    PVOID DbgSsReserved[2];                                                 /**< 0x16a0 / 0xf20 */
    uint32_t HardErrorMode;                                                 /**< 0x16b0 / 0xf28 - Called HardErrorsAreDisabled in W51. */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x16b4 / NA */
#endif
    PVOID Instrumentation[ARCH_BITS == 64 ? 11 : 9];                        /**< 0x16b8 / 0xf2c */
    union
    {
        struct
        {
            GUID ActivityId;                                                /**< 0x1710 / 0xf50 */
            PVOID SubProcessTag;                                            /**< 0x1720 / 0xf60 */
        } W6, W7, W8, W80, W81;
        struct
        {
            PVOID InstrumentationContinues[ARCH_BITS == 64 ? 3 : 5];        /**< 0x1710 / 0xf50 */
        } W52;
    } Diff2;
    union                                                                   /**< 0x1728 / 0xf64 */
    {
        struct
        {
            PVOID PerflibData;                                              /**< 0x1728 / 0xf64 */
        } W8, W80, W81;
        struct
        {
            PVOID EtwLocalData;                                             /**< 0x1728 / 0xf64 */
        } W7, W6;
        struct
        {
            PVOID SubProcessTag;                                            /**< 0x1728 / 0xf64 */
        } W52;
        struct
        {
            PVOID InstrumentationContinues[1];                              /**< 0x1728 / 0xf64 */
        } W51;
    } Diff3;
    union
    {
        struct
        {
            PVOID EtwTraceData;                                             /**< 0x1730 / 0xf68 */
        } W52, W6, W7, W8, W80, W81;
        struct
        {
            PVOID InstrumentationContinues[1];                              /**< 0x1730 / 0xf68 */
        } W51;
    } Diff4;
    PVOID WinSockData;                                                      /**< 0x1738 / 0xf6c */
    uint32_t GdiBatchCount;                                                 /**< 0x1740 / 0xf70 */
    union
    {
        union
        {
            PROCESSOR_NUMBER CurrentIdealProcessor;                         /**< 0x1744 / 0xf74 - W7+ */
            uint32_t IdealProcessorValue;                                   /**< 0x1744 / 0xf74 - W7+ */
            struct
            {
                uint8_t ReservedPad1;                                       /**< 0x1744 / 0xf74 - Called SpareBool0 in W6 */
                uint8_t ReservedPad2;                                       /**< 0x1745 / 0xf75 - Called SpareBool0 in W6 */
                uint8_t ReservedPad3;                                       /**< 0x1746 / 0xf76 - Called SpareBool0 in W6 */
                uint8_t IdealProcessor;                                     /**< 0x1747 / 0xf77 */
            };
        } W6, W7, W8, W80, W81;
        struct
        {
            BOOLEAN InDbgPrint;                                             /**< 0x1744 / 0xf74 */
            BOOLEAN FreeStackOnTermination;                                 /**< 0x1745 / 0xf75 */
            BOOLEAN HasFiberData;                                           /**< 0x1746 / 0xf76 */
            uint8_t IdealProcessor;                                         /**< 0x1747 / 0xf77 */
        } W51, W52;
    } Diff5;
    uint32_t GuaranteedStackBytes;                                          /**< 0x1748 / 0xf78 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x174c / NA */
#endif
    PVOID ReservedForPerf;                                                  /**< 0x1750 / 0xf7c */
    PVOID ReservedForOle;                                                   /**< 0x1758 / 0xf80 */
    uint32_t WaitingOnLoaderLock;                                           /**< 0x1760 / 0xf84 */
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x1764 / NA */
#endif
    union                                                                   /**< 0x1770 / 0xf8c */
    {
        struct
        {
            PVOID SavedPriorityState;                                       /**< 0x1768 / 0xf88 */
            SIZE_T ReservedForCodeCoverage;                                 /**< 0x1770 / 0xf8c */
            PVOID ThreadPoolData;                                           /**< 0x1778 / 0xf90 */
        } W8, W80, W81;
        struct
        {
            PVOID SavedPriorityState;                                       /**< 0x1768 / 0xf88 */
            SIZE_T SoftPatchPtr1;                                           /**< 0x1770 / 0xf8c */
            PVOID ThreadPoolData;                                           /**< 0x1778 / 0xf90 */
        } W6, W7;
        struct
        {
            PVOID SparePointer1;                                            /**< 0x1768 / 0xf88 */
            SIZE_T SoftPatchPtr1;                                           /**< 0x1770 / 0xf8c */
            PVOID SoftPatchPtr2;                                            /**< 0x1778 / 0xf90 */
        } W52;
#if ARCH_BITS == 32
        struct _Wx86ThreadState
        {
            PVOID CallBx86Eip;                                            /**< NA / 0xf88 */
            PVOID DeallocationCpu;                                        /**< NA / 0xf8c */
            BOOLEAN UseKnownWx86Dll;                                      /**< NA / 0xf90 */
            int8_t OleStubInvoked;                                        /**< NA / 0xf91 */
        } W51;
#endif
    } Diff6;
    PVOID TlsExpansionSlots;                                                /**< 0x1780 / 0xf94 */
#if ARCH_BITS == 64
    PVOID DallocationBStore;                                                /**< 0x1788 / NA */
    PVOID BStoreLimit;                                                      /**< 0x1790 / NA */
#endif
    union
    {
        struct
        {
            uint32_t MuiGeneration;                                                 /**< 0x1798 / 0xf98 */
        } W7, W8, W80, W81;
        struct
        {
            uint32_t ImpersonationLocale;
        } W6;
    } Diff7;
    uint32_t IsImpersonating;                                               /**< 0x179c / 0xf9c */
    PVOID NlsCache;                                                         /**< 0x17a0 / 0xfa0 */
    PVOID pShimData;                                                        /**< 0x17a8 / 0xfa4 */
    union                                                                   /**< 0x17b0 / 0xfa8 */
    {
        struct
        {
            uint16_t HeapVirtualAffinity;                                   /**< 0x17b0 / 0xfa8 */
            uint16_t LowFragHeapDataSlot;                                   /**< 0x17b2 / 0xfaa */
        } W8, W80, W81;
        struct
        {
            uint32_t HeapVirtualAffinity;                                   /**< 0x17b0 / 0xfa8 */
        } W7;
    } Diff8;
#if ARCH_BITS == 64
    uint32_t Padding7;                                                      /**< 0x17b4 / NA */
#endif
    HANDLE CurrentTransactionHandle;                                        /**< 0x17b8 / 0xfac */
    struct _TEB_ACTIVE_FRAME *ActiveFrame;                                  /**< 0x17c0 / 0xfb0 */
    /* End of TEB in W51 (Windows XP)! */
    PVOID FlsData;                                                          /**< 0x17c8 / 0xfb4 */
    union
    {
        struct
        {
            PVOID PreferredLanguages;                                       /**< 0x17d0 / 0xfb8 */
        } W6, W7, W8, W80, W81;
        struct
        {
            BOOLEAN SafeThunkCall;                                          /**< 0x17d0 / 0xfb8 */
            uint8_t BooleanSpare[3];                                        /**< 0x17d1 / 0xfb9 */
            /* End of TEB in W52 (Windows server 2003)! */
        } W52;
    } Diff9;
    PVOID UserPrefLanguages;                                                /**< 0x17d8 / 0xfbc */
    PVOID MergedPrefLanguages;                                              /**< 0x17e0 / 0xfc0 */
    uint32_t MuiImpersonation;                                              /**< 0x17e8 / 0xfc4 */
    union
    {
        uint16_t CrossTebFlags;                                             /**< 0x17ec / 0xfc8 */
        struct
        {
            uint16_t SpareCrossTebBits : 16;                                /**< 0x17ec / 0xfc8 : Pos 0, 16 Bits */
        };
    };
    union
    {
        uint16_t SameTebFlags;                                              /**< 0x17ee / 0xfca */
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
        } Common;
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t DisableUserStackWalk : 1;                              /**< 0x17ee / 0xfca : Pos 8, 1 Bit */
            uint16_t RtlExceptionAttached : 1;                              /**< 0x17ee / 0xfca : Pos 9, 1 Bit */
            uint16_t InitialThread : 1;                                     /**< 0x17ee / 0xfca : Pos 10, 1 Bit */
            uint16_t SessionAware : 1;                                      /**< 0x17ee / 0xfca : Pos 11, 1 Bit - New Since W7. */
            uint16_t SpareSameTebBits : 4;                                  /**< 0x17ee / 0xfca : Pos 12, 4 Bits */
        } W8, W80, W81;
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t DisableUserStackWalk : 1;                              /**< 0x17ee / 0xfca : Pos 8, 1 Bit */
            uint16_t RtlExceptionAttached : 1;                              /**< 0x17ee / 0xfca : Pos 9, 1 Bit */
            uint16_t InitialThread : 1;                                     /**< 0x17ee / 0xfca : Pos 10, 1 Bit */
            uint16_t SpareSameTebBits : 5;                                  /**< 0x17ee / 0xfca : Pos 12, 4 Bits */
        } W7;
        struct
        {
            uint16_t DbgSafeThunkCall : 1;                                  /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t DbgInDebugPrint : 1;                                   /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t DbgHasFiberData : 1;                                   /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t DbgSkipThreadAttach : 1;                               /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t DbgWerInShipAssertCode : 1;                            /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t DbgRanProcessInit : 1;                                 /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t DbgClonedThread : 1;                                   /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t DbgSuppressDebugMsg : 1;                               /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t SpareSameTebBits : 8;                                  /**< 0x17ee / 0xfca : Pos 8, 8 Bits */
        } W6;
    } Diff10;
    PVOID TxnScopeEnterCallback;                                            /**< 0x17f0 / 0xfcc */
    PVOID TxnScopeExitCallback;                                             /**< 0x17f8 / 0xfd0 */
    PVOID TxnScopeContext;                                                  /**< 0x1800 / 0xfd4 */
    uint32_t LockCount;                                                     /**< 0x1808 / 0xfd8 */
    union
    {
        struct
        {
            uint32_t SpareUlong0;                                           /**< 0x180c / 0xfdc */
        } W7, W8, W80, W81;
        struct
        {
            uint32_t ProcessRundown;
        } W6;
    } Diff11;
    union
    {
        struct
        {
            PVOID ResourceRetValue;                                        /**< 0x1810 / 0xfe0 */
            /* End of TEB in W7 (windows 7)! */
            PVOID ReservedForWdf;                                          /**< 0x1818 / 0xfe4 - New Since W7. */
            /* End of TEB in W8 (windows 8.0 & 8.1)! */
        } W8, W80, W81;
        struct
        {
            PVOID ResourceRetValue;                                        /**< 0x1810 / 0xfe0 */
        } W7;
        struct
        {
            uint64_t LastSwitchTime;                                       /**< 0x1810 / 0xfe0 */
            uint64_t TotalSwitchOutTime;                                   /**< 0x1818 / 0xfe8 */
            LARGE_INTEGER WaitReasonBitMap;                                /**< 0x1820 / 0xff0 */
            /* End of TEB in W6 (windows Vista)! */
        } W6;
    } Diff12;
} TEB_COMMON;
typedef TEB_COMMON *PTEB_COMMON;
AssertCompileMemberOffset(TEB_COMMON, ExceptionCode,        ARCH_BITS == 64 ?  0x2c0 : 0x1a4);
AssertCompileMemberOffset(TEB_COMMON, LastStatusValue,      ARCH_BITS == 64 ? 0x1250 : 0xbf4);
AssertCompileMemberOffset(TEB_COMMON, DeallocationStack,    ARCH_BITS == 64 ? 0x1478 : 0xe0c);
AssertCompileMemberOffset(TEB_COMMON, ReservedForNtRpc,     ARCH_BITS == 64 ? 0x1698 : 0xf1c);
AssertCompileMemberOffset(TEB_COMMON, Instrumentation,      ARCH_BITS == 64 ? 0x16b8 : 0xf2c);
AssertCompileMemberOffset(TEB_COMMON, Diff2,                ARCH_BITS == 64 ? 0x1710 : 0xf50);
AssertCompileMemberOffset(TEB_COMMON, Diff3,                ARCH_BITS == 64 ? 0x1728 : 0xf64);
AssertCompileMemberOffset(TEB_COMMON, Diff4,                ARCH_BITS == 64 ? 0x1730 : 0xf68);
AssertCompileMemberOffset(TEB_COMMON, WinSockData,          ARCH_BITS == 64 ? 0x1738 : 0xf6c);
AssertCompileMemberOffset(TEB_COMMON, GuaranteedStackBytes, ARCH_BITS == 64 ? 0x1748 : 0xf78);
AssertCompileMemberOffset(TEB_COMMON, MuiImpersonation,     ARCH_BITS == 64 ? 0x17e8 : 0xfc4);
AssertCompileMemberOffset(TEB_COMMON, LockCount,            ARCH_BITS == 64 ? 0x1808 : 0xfd8);
AssertCompileSize(TEB_COMMON, ARCH_BITS == 64 ? 0x1828 : 0xff8);


/** The size of the windows 8.1 PEB structure.  */
#define TEB_SIZE_W81    ( RT_UOFFSETOF(TEB_COMMON, Diff12.W8.ReservedForWdf) + sizeof(PVOID) )
/** The size of the windows 8.0 PEB structure.  */
#define TEB_SIZE_W80    ( RT_UOFFSETOF(TEB_COMMON, Diff12.W8.ReservedForWdf) + sizeof(PVOID) )
/** The size of the windows 7 PEB structure.  */
#define TEB_SIZE_W7     RT_UOFFSETOF(TEB_COMMON, Diff12.W8.ReservedForWdf)
/** The size of the windows vista PEB structure.  */
#define TEB_SIZE_W6     ( RT_UOFFSETOF(TEB_COMMON, Diff12.W6.WaitReasonBitMap) + sizeof(LARGE_INTEGER) )
/** The size of the windows server 2003 PEB structure.  */
#define TEB_SIZE_W52    RT_ALIGN_Z(RT_UOFFSETOF(TEB_COMMON, Diff9.W52.BooleanSpare), sizeof(PVOID))
/** The size of the windows XP PEB structure.  */
#define TEB_SIZE_W51    RT_UOFFSETOF(TEB_COMMON, FlsData)



#define _PEB        _PEB_COMMON
typedef PEB_COMMON  PEB;
typedef PPEB_COMMON *PPEB;

#define _TEB        _TEB_COMMON
typedef TEB_COMMON  TEB;
typedef PTEB_COMMON PTEB;

#define NtCurrentPeb()  (((PTEB)NtCurrentTeb())->ProcessEnvironmentBlock)

/** @} */


#ifdef IPRT_NT_USE_WINTERNL
NTSYSAPI NTSTATUS NTAPI NtCreateSection(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PLARGE_INTEGER, ULONG, ULONG, HANDLE);

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION
{
    ULONG   FileSystemAttributes;
    LONG    MaximumComponentNameLength;
    ULONG   FileSystemNameLength;
    WCHAR   FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION;
typedef FILE_FS_ATTRIBUTE_INFORMATION *PFILE_FS_ATTRIBUTE_INFORMATION;

NTSYSAPI NTSTATUS NTAPI NtOpenProcessToken(HANDLE, ACCESS_MASK, PHANDLE);
NTSYSAPI NTSTATUS NTAPI NtOpenThreadToken(HANDLE, ACCESS_MASK, BOOLEAN, PHANDLE);

typedef enum _FSINFOCLASS
{
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsDriverPathInformation,
    FileFsVolumeFlagsInformation,
    FileFsSectorSizeInformation,
    FileFsDataCopyInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS;
typedef FS_INFORMATION_CLASS *PFS_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryVolumeInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FS_INFORMATION_CLASS);

typedef struct _FILE_BOTH_DIR_INFORMATION
{
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    ULONG           EaSize;
    CCHAR           ShortNameLength;
    WCHAR           ShortName[12];
    WCHAR           FileName[1];
} FILE_BOTH_DIR_INFORMATION;
typedef FILE_BOTH_DIR_INFORMATION *PFILE_BOTH_DIR_INFORMATION;
typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   EndOfFile;
    ULONG           NumberOfLinks;
    BOOLEAN         DeletePending;
    BOOLEAN         Directory;
} FILE_STANDARD_INFORMATION;
typedef FILE_STANDARD_INFORMATION *PFILE_STANDARD_INFORMATION;
typedef struct _FILE_NAME_INFORMATION
{
    ULONG           FileNameLength;
    WCHAR           FileName[1];
} FILE_NAME_INFORMATION;
typedef FILE_NAME_INFORMATION *PFILE_NAME_INFORMATION;
typedef enum _FILE_INFORMATION_CLASS
{
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileIdBothDirectoryInformation,
    FileIdFullDirectoryInformation,
    FileValidDataLengthInformation,
    FileShortNameInformation,
    FileIoCompletionNotificationInformation,
    FileIoStatusBlockRangeInformation,
    FileIoPriorityHintInformation,
    FileSfioReserveInformation,
    FileSfioVolumeInformation,
    FileHardLinkInformation,
    FileProcessIdsUsingFileInformation,
    FileNormalizedNameInformation,
    FileNetworkPhysicalNameInformation,
    FileIdGlobalTxDirectoryInformation,
    FileIsRemoteDeviceInformation,
    FileUnusedInformation,
    FileNumaNodeInformation,
    FileStandardLinkInformation,
    FileRemoteProtocolInformation,
    FileRenameInformationBypassAccessCheck,
    FileLinkInformationBypassAccessCheck,
    FileVolumeNameInformation,
    FileIdInformation,
    FileIdExtdDirectoryInformation,
    FileReplaceCompletionInformation,
    FileHardLinkFullIdInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS;
typedef FILE_INFORMATION_CLASS *PFILE_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
NTSYSAPI NTSTATUS NTAPI NtQueryDirectoryFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG,
                                             FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);

typedef struct _MEMORY_SECTION_NAME
{
    UNICODE_STRING  SectionFileName;
    WCHAR           NameBuffer[1];
} MEMORY_SECTION_NAME;

#ifdef IPRT_NT_USE_WINTERNL
typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    int32_t BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;
#endif

typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    ProcessIoPriority,
    ProcessExecuteFlags,
    ProcessTlsInformation,
    ProcessCookie,
    ProcessImageInformation,
    ProcessCycleTime,
    ProcessPagePriority,
    ProcessInstrumentationCallbak,
    ProcessThreadStackAllocation,
    ProcessWorkingSetWatchEx,
    ProcessImageFileNameWin32,
    ProcessImageFileMapping,
    ProcessAffinityUpdateMode,
    ProcessMemoryAllocationMode,
    ProcessGroupInformation,
    ProcessTokenVirtualizationEnabled,
    ProcessConsoleHostProcess,
    ProcessWindowsInformation,
    MaxProcessInfoClass
} PROCESSINFOCLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryInformationProcess(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation = 0,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context,
    ThreadGroupInformation,
    ThreadUmsInformation,
    ThreadCounterProfiling,
    ThreadIdealProcessorEx,
    ThreadCpuAccountingInformation,
    MaxThreadInfoClass
} THREADINFOCLASS;
NTSYSAPI NTSTATUS NTAPI NtSetInformationThread(HANDLE, THREADINFOCLASS, LPCVOID, ULONG);

NTSYSAPI NTSTATUS NTAPI NtQueryInformationToken(HANDLE, TOKEN_INFORMATION_CLASS, PVOID, ULONG, PULONG);

NTSYSAPI NTSTATUS NTAPI NtReadFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
NTSYSAPI NTSTATUS NTAPI NtWriteFile(HANDLE, HANDLE, PIO_APC_ROUTINE, void const *, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);

NTSYSAPI NTSTATUS NTAPI NtReadVirtualMemory(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
NTSYSAPI NTSTATUS NTAPI NtWriteVirtualMemory(HANDLE, PVOID, void const *, SIZE_T, PSIZE_T);

NTSYSAPI NTSTATUS NTAPI RtlAddAccessAllowedAce(PACL, ULONG, ULONG, PSID);
NTSYSAPI NTSTATUS NTAPI RtlCopySid(ULONG, PSID, PSID);
NTSYSAPI NTSTATUS NTAPI RtlCreateAcl(PACL, ULONG, ULONG);
NTSYSAPI NTSTATUS NTAPI RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR, ULONG);
NTSYSAPI NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW);
NTSYSAPI NTSTATUS NTAPI RtlInitializeSid(PSID, PSID_IDENTIFIER_AUTHORITY, UCHAR);
NTSYSAPI NTSTATUS NTAPI RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR, BOOLEAN, PACL, BOOLEAN);
NTSYSAPI PULONG   NTAPI RtlSubAuthoritySid(PSID, ULONG);

#endif /* IPRT_NT_USE_WINTERNL */

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation = 0,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
} OBJECT_INFORMATION_CLASS;
typedef OBJECT_INFORMATION_CLASS *POBJECT_INFORMATION_CLASS;
#ifdef IN_RING0
# define NtQueryObject ZwQueryObject
#endif
NTSYSAPI NTSTATUS NTAPI NtQueryObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
NTSYSAPI NTSTATUS NTAPI NtSetInformationObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG);
NTSYSAPI NTSTATUS NTAPI NtDuplicateObject(HANDLE, HANDLE, HANDLE, PHANDLE, ACCESS_MASK, ULONG, ULONG);

NTSYSAPI NTSTATUS NTAPI NtOpenDirectoryObject(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION;
typedef OBJECT_DIRECTORY_INFORMATION *POBJECT_DIRECTORY_INFORMATION;
NTSYSAPI NTSTATUS NTAPI NtQueryDirectoryObject(HANDLE, PVOID, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG);

NTSYSAPI NTSTATUS NTAPI NtSuspendProcess(HANDLE);
NTSYSAPI NTSTATUS NTAPI NtResumeProcess(HANDLE);
/** @name ProcessDefaultHardErrorMode bit definitions.
 * @{ */
#define PROCESS_HARDERR_CRITICAL_ERROR              UINT32_C(0x00000001) /**< Inverted from the win32 define. */
#define PROCESS_HARDERR_NO_GP_FAULT_ERROR           UINT32_C(0x00000002)
#define PROCESS_HARDERR_NO_ALIGNMENT_FAULT_ERROR    UINT32_C(0x00000004)
#define PROCESS_HARDERR_NO_OPEN_FILE_ERROR          UINT32_C(0x00008000)
/** @} */
NTSYSAPI NTSTATUS NTAPI NtSetInformationProcess(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);

/** Retured by ProcessImageInformation as well as NtQuerySection. */
typedef struct _SECTION_IMAGE_INFORMATION
{
    PVOID TransferAddress;
    ULONG ZeroBits;
    SIZE_T MaximumStackSize;
    SIZE_T CommittedStackSize;
    ULONG SubSystemType;
    union
    {
        struct
        {
            USHORT SubSystemMinorVersion;
            USHORT SubSystemMajorVersion;
        };
        ULONG SubSystemVersion;
    };
    ULONG GpValue;
    USHORT ImageCharacteristics;
    USHORT DllCharacteristics;
    USHORT Machine;
    BOOLEAN ImageContainsCode;
    union /**< Since Vista, used to be a spare BOOLEAN. */
    {
        struct
        {
            UCHAR ComPlusNativeRead : 1;
            UCHAR ComPlusILOnly : 1;
            UCHAR ImageDynamicallyRelocated : 1;
            UCHAR ImageMAppedFlat : 1;
            UCHAR Reserved : 4;
        };
        UCHAR ImageFlags;
    };
    ULONG LoaderFlags;
    ULONG ImageFileSize; /**< Since XP? */
    ULONG CheckSum; /**< Since Vista, Used to be a reserved/spare ULONG. */
} SECTION_IMAGE_INFORMATION;
typedef SECTION_IMAGE_INFORMATION *PSECTION_IMAGE_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation = 0,
    SectionImageInformation,
    MaxSectionInfoClass
} SECTION_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQuerySection(HANDLE, SECTION_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);

NTSYSAPI NTSTATUS NTAPI NtQueryInformationThread(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
NTSYSAPI NTSTATUS NTAPI NtResumeThread(HANDLE, PULONG);
NTSYSAPI NTSTATUS NTAPI NtSuspendThread(HANDLE, PULONG);

#ifndef SEC_FILE
# define SEC_FILE               UINT32_C(0x00800000)
#endif
#ifndef SEC_IMAGE
# define SEC_IMAGE              UINT32_C(0x01000000)
#endif
#ifndef SEC_PROTECTED_IMAGE
# define SEC_PROTECTED_IMAGE    UINT32_C(0x02000000)
#endif
#ifndef SEC_NOCACHE
# define SEC_NOCACHE            UINT32_C(0x10000000)
#endif
#ifndef MEM_ROTATE
# define MEM_ROTATE             UINT32_C(0x00800000)
#endif
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation = 0,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;
#ifdef IN_RING0
typedef struct _MEMORY_BASIC_INFORMATION
{
    PVOID BaseAddress;
    PVOID AllocationBase;
    ULONG AllocationProtect;
    SIZE_T RegionSize;
    ULONG State;
    ULONG Protect;
    ULONG Type;
} MEMORY_BASIC_INFORMATION;
typedef MEMORY_BASIC_INFORMATION *PMEMORY_BASIC_INFORMATION;
# define NtQueryVirtualMemory ZwQueryVirtualMemory
#endif
NTSYSAPI NTSTATUS NTAPI NtQueryVirtualMemory(HANDLE, void const *, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation = 0,
    SystemCpuInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemInformation_Unknown_4,
    SystemProcessInformation,
    SystemInformation_Unknown_6,
    SystemInformation_Unknown_7,
    SystemProcessorPerformanceInformation,
    SystemInformation_Unknown_9,
    SystemInformation_Unknown_10,
    SystemModuleInformation,
    SystemInformation_Unknown_12,
    SystemInformation_Unknown_13,
    SystemInformation_Unknown_14,
    SystemInformation_Unknown_15,
    SystemHandleInformation,
    SystemInformation_Unknown_17,
    SystemPageFileInformation,
    SystemInformation_Unknown_19,
    SystemInformation_Unknown_20,
    SystemCacheInformation,
    SystemInformation_Unknown_22,
    SystemInterruptInformation,
    SystemDpcBehaviourInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation, /* 26 */
    SystemUnloadGdiDriverInformation, /* 27 */
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemInformation_Unknown_30,
    SystemInformation_Unknown_31,
    SystemInformation_Unknown_32,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemInformation_Unknown_38,
    SystemInformation_Unknown_39,
    SystemInformation_Unknown_40,
    SystemInformation_Unknown_41,
    SystemInformation_Unknown_42,
    SystemInformation_Unknown_43,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemSetTimeSlipEvent,
    SystemCreateSession,
    SystemDeleteSession,
    SystemInformation_Unknown_49,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemInformation_Unknown_52,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpaceInformation, /* 54 */
    SystemInformation_Unknown_55,
    SystemInformation_Unknown_56,
    SystemExtendedProcessInformation,
    SystemInformation_Unknown_58,
    SystemInformation_Unknown_59,
    SystemInformation_Unknown_60,
    SystemInformation_Unknown_61,
    SystemInformation_Unknown_62,
    SystemInformation_Unknown_63,
    SystemExtendedHandleInformation, /* 64 */

    /** @todo fill gap. they've added a whole bunch of things  */
    SystemPolicyInformation = 134,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;

#ifdef IPRT_NT_USE_WINTERNL
typedef struct _VM_COUNTERS
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;
#endif

#if 0
typedef struct _IO_COUNTERS
{
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;
typedef IO_COUNTERS *PIO_COUNTERS;
#endif

typedef struct _RTNT_SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;          /**< 0x00 / 0x00 */
    ULONG NumberOfThreads;          /**< 0x04 / 0x04 */
    LARGE_INTEGER Reserved1[3];     /**< 0x08 / 0x08 */
    LARGE_INTEGER CreationTime;     /**< 0x20 / 0x20 */
    LARGE_INTEGER UserTime;         /**< 0x28 / 0x28 */
    LARGE_INTEGER KernelTime;       /**< 0x30 / 0x30 */
    UNICODE_STRING ProcessName;     /**< 0x38 / 0x38 Clean unicode encoding? */
    int32_t BasePriority;           /**< 0x40 / 0x48 */
    HANDLE UniqueProcessId;         /**< 0x44 / 0x50 */
    HANDLE ParentProcessId;         /**< 0x48 / 0x58 */
    ULONG HandleCount;              /**< 0x4c / 0x60 */
    ULONG Reserved2;                /**< 0x50 / 0x64 Session ID? */
    ULONG_PTR Reserved3;            /**< 0x54 / 0x68 */
    VM_COUNTERS VmCounters;         /**< 0x58 / 0x70 */
    IO_COUNTERS IoCounters;         /**< 0x88 / 0xd0 Might not be present in earlier windows versions. */
    /* After this follows the threads, then the ProcessName.Buffer. */
} RTNT_SYSTEM_PROCESS_INFORMATION;
typedef RTNT_SYSTEM_PROCESS_INFORMATION *PRTNT_SYSTEM_PROCESS_INFORMATION;
#ifndef IPRT_NT_USE_WINTERNL
typedef RTNT_SYSTEM_PROCESS_INFORMATION SYSTEM_PROCESS_INFORMATION ;
typedef SYSTEM_PROCESS_INFORMATION *PSYSTEM_PROCESS_INFORMATION;
#endif

typedef struct _SYSTEM_HANDLE_ENTRY_INFO
{
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT HandleValue;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_ENTRY_INFO;
typedef SYSTEM_HANDLE_ENTRY_INFO *PSYSTEM_HANDLE_ENTRY_INFO;

/** Returned by SystemHandleInformation  */
typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION;
typedef SYSTEM_HANDLE_INFORMATION *PSYSTEM_HANDLE_INFORMATION;

/** Extended handle information entry.
 * @remarks 3 x PVOID + 4 x ULONG = 28 bytes on 32-bit / 40 bytes on 64-bit  */
typedef struct _SYSTEM_HANDLE_ENTRY_INFO_EX
{
    PVOID Object;
    HANDLE UniqueProcessId;
    HANDLE HandleValue;
    ACCESS_MASK GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_ENTRY_INFO_EX;
typedef SYSTEM_HANDLE_ENTRY_INFO_EX *PSYSTEM_HANDLE_ENTRY_INFO_EX;

/** Returned by SystemExtendedHandleInformation.  */
typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX;
typedef SYSTEM_HANDLE_INFORMATION_EX *PSYSTEM_HANDLE_INFORMATION_EX;

/** Input to SystemSessionProcessInformation. */
typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
    ULONG SessionId;
    ULONG BufferLength;
    /** Return buffer, SYSTEM_PROCESS_INFORMATION entries. */
    PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION;
typedef SYSTEM_SESSION_PROCESS_INFORMATION *PSYSTEM_SESSION_PROCESS_INFORMATION;

NTSYSAPI NTSTATUS NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

NTSYSAPI NTSTATUS NTAPI NtDelayExecution(BOOLEAN, PLARGE_INTEGER);
NTSYSAPI NTSTATUS NTAPI NtYieldExecution(void);

NTSYSAPI NTSTATUS NTAPI RtlAddAccessDeniedAce(PACL, ULONG, ULONG, PSID);


typedef struct _CURDIR
{
    UNICODE_STRING  DosPath;
    HANDLE          Handle;
} CURDIR;
typedef CURDIR *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT          Flags;
    USHORT          Length;
    ULONG           TimeStamp;
    STRING          DosPath; /**< Yeah, it's STRING according to dt ntdll!_RTL_DRIVE_LETTER_CURDIR. */
} RTL_DRIVE_LETTER_CURDIR;
typedef RTL_DRIVE_LETTER_CURDIR *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG           MaximumLength;
    ULONG           Length;
    ULONG           Flags;
    ULONG           DebugFlags;
    HANDLE          ConsoleHandle;
    ULONG           ConsoleFlags;
    HANDLE          StandardInput;
    HANDLE          StandardOutput;
    HANDLE          StandardError;
    CURDIR          CurrentDirectory;
    UNICODE_STRING  DllPath;
    UNICODE_STRING  ImagePathName;
    UNICODE_STRING  CommandLine;
    PWSTR           Environment;
    ULONG           StartingX;
    ULONG           StartingY;
    ULONG           CountX;
    ULONG           CountY;
    ULONG           CountCharsX;
    ULONG           CountCharsY;
    ULONG           FillAttribute;
    ULONG           WindowFlags;
    ULONG           ShowWindowFlags;
    UNICODE_STRING  WindowTitle;
    UNICODE_STRING  DesktopInfo;
    UNICODE_STRING  ShellInfo;
    UNICODE_STRING  RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR  CurrentDirectories[0x20];
    SIZE_T          EnvironmentSize;        /**< Added in Vista */
    SIZE_T          EnvironmentVersion;     /**< Added in Windows 7. */
    PVOID           PackageDependencyData;  /**< Added Windows 8? */
    ULONG           ProcessGroupId;         /**< Added Windows 8? */
} RTL_USER_PROCESS_PARAMETERS;
typedef RTL_USER_PROCESS_PARAMETERS *PRTL_USER_PROCESS_PARAMETERS;
#define RTL_USER_PROCESS_PARAMS_FLAG_NORMALIZED     1

typedef struct _RTL_USER_PROCESS_INFORMATION
{
    ULONG           Size;
    HANDLE          ProcessHandle;
    HANDLE          ThreadHandle;
    CLIENT_ID       ClientId;
    SECTION_IMAGE_INFORMATION  ImageInformation;
} RTL_USER_PROCESS_INFORMATION;
typedef RTL_USER_PROCESS_INFORMATION *PRTL_USER_PROCESS_INFORMATION;


NTSYSAPI NTSTATUS NTAPI RtlCreateUserProcess(PUNICODE_STRING, ULONG, PRTL_USER_PROCESS_PARAMETERS, PSECURITY_DESCRIPTOR,
                                             PSECURITY_DESCRIPTOR, HANDLE, BOOLEAN, HANDLE, HANDLE, PRTL_USER_PROCESS_INFORMATION);
NTSYSAPI NTSTATUS NTAPI RtlCreateProcessParameters(PRTL_USER_PROCESS_PARAMETERS *, PUNICODE_STRING ImagePathName,
                                                   PUNICODE_STRING DllPath, PUNICODE_STRING CurrentDirectory,
                                                   PUNICODE_STRING CommandLine, PUNICODE_STRING Environment,
                                                   PUNICODE_STRING WindowTitle, PUNICODE_STRING DesktopInfo,
                                                   PUNICODE_STRING ShellInfo, PUNICODE_STRING RuntimeInfo);
NTSYSAPI VOID     NTAPI RtlDestroyProcessParameters(PRTL_USER_PROCESS_PARAMETERS);

RT_C_DECLS_END
/** @} */


#if defined(IN_RING0) || defined(DOXYGEN_RUNNING)
/** @name NT Kernel APIs
 * @{ */
NTSYSAPI BOOLEAN  NTAPI ObFindHandleForObject(PEPROCESS pProcess, PVOID pvObject, POBJECT_TYPE pObjectType,
                                              PVOID pvOptionalConditions, PHANDLE phFound);
NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(PUNICODE_STRING pObjectPath, ULONG fAttributes, PACCESS_STATE pAccessState,
                                                ACCESS_MASK fDesiredAccess, POBJECT_TYPE pObjectType,
                                                KPROCESSOR_MODE enmAccessMode, PVOID pvParseContext, PVOID *ppvObject);
NTSYSAPI HANDLE   NTAPI PsGetProcessInheritedFromUniqueProcessId(PEPROCESS);
NTSYSAPI UCHAR *  NTAPI PsGetProcessImageFileName(PEPROCESS);
NTSYSAPI BOOLEAN  NTAPI PsIsProcessBeingDebugged(PEPROCESS);
NTSYSAPI ULONG    NTAPI PsGetProcessSessionId(PEPROCESS);
extern DECLIMPORT(POBJECT_TYPE *) LpcPortObjectType;            /**< In vista+ this is the ALPC port object type. */
extern DECLIMPORT(POBJECT_TYPE *) LpcWaitablePortObjectType;    /**< In vista+ this is the ALPC port object type. */

/** @ */
#endif /* IN_RING0 */

#endif

