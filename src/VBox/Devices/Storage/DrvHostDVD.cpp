/* $Id$ */
/** @file
 * DrvHostDVD - Host DVD block driver.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DRV_HOST_DVD
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#ifdef RT_OS_DARWIN
# include <mach/mach.h>
# include <Carbon/Carbon.h>
# include <IOKit/IOKitLib.h>
# include <IOKit/IOCFPlugIn.h>
# include <IOKit/scsi/SCSITaskLib.h>
# include <IOKit/scsi/SCSICommandOperationCodes.h>
# include <IOKit/storage/IOStorageDeviceCharacteristics.h>
# include <mach/mach_error.h>
# define USE_MEDIA_POLLING

#elif defined RT_OS_LINUX
# include <sys/ioctl.h>
# include <linux/version.h>
/* All the following crap is apparently not necessary anymore since Linux
 * version 2.6.29. */
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
/* This is a hack to work around conflicts between these linux kernel headers
 * and the GLIBC tcpip headers. They have different declarations of the 4
 * standard byte order functions. */
#  define _LINUX_BYTEORDER_GENERIC_H
/* This is another hack for not bothering with C++ unfriendly byteswap macros. */
/* Those macros that are needed are defined in the header below. */
#  include "swab.h"
# endif
# include <linux/cdrom.h>
# include <sys/fcntl.h>
# include <errno.h>
# include <limits.h>
# include <iprt/mem.h>
# define USE_MEDIA_POLLING

#elif defined(RT_OS_SOLARIS)
# include <stropts.h>
# include <fcntl.h>
# include <errno.h>
# include <pwd.h>
# include <unistd.h>
# include <syslog.h>
# ifdef VBOX_WITH_SUID_WRAPPER
#  include <auth_attr.h>
# endif
# include <sys/dkio.h>
# include <sys/sockio.h>
# include <sys/scsi/scsi.h>
# define USE_MEDIA_POLLING

#elif defined(RT_OS_WINDOWS)
# pragma warning(disable : 4163)
# define _interlockedbittestandset      they_messed_it_up_in_winnt_h_this_time_sigh__interlockedbittestandset
# define _interlockedbittestandreset    they_messed_it_up_in_winnt_h_this_time_sigh__interlockedbittestandreset
# define _interlockedbittestandset64    they_messed_it_up_in_winnt_h_this_time_sigh__interlockedbittestandset64
# define _interlockedbittestandreset64  they_messed_it_up_in_winnt_h_this_time_sigh__interlockedbittestandreset64
# include <iprt/win/windows.h>
# include <winioctl.h>
# include <ntddscsi.h>
# pragma warning(default : 4163)
# undef _interlockedbittestandset
# undef _interlockedbittestandreset
# undef _interlockedbittestandset64
# undef _interlockedbittestandreset64
# undef USE_MEDIA_POLLING

#elif defined(RT_OS_FREEBSD)
# include <sys/cdefs.h>
# include <sys/param.h>
# include <stdio.h>
# include <cam/cam.h>
# include <cam/cam_ccb.h>
# define USE_MEDIA_POLLING

#else
# error "Unsupported Platform."
#endif

#include <iprt/asm.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmstorageifs.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/critsect.h>
#include <VBox/scsi.h>

#include "VBoxDD.h"
#include "DrvHostBase.h"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int) drvHostDvdDoLock(PDRVHOSTBASE pThis, bool fLock);

#ifdef VBOX_WITH_SUID_WRAPPER
/**
 * Checks if the current user is authorized using Solaris' role-based access control.
 * Made as a separate function with so that it need not be invoked each time we need
 * to gain root access.
 *
 * @returns VBox error code.
 */
static int solarisCheckUserAuth()
{
    /* Uses Solaris' role-based access control (RBAC).*/
    struct passwd *pPass = getpwuid(getuid());
    if (pPass == NULL || chkauthattr("solaris.device.cdrw", pPass->pw_name) == 0)
        return VERR_PERMISSION_DENIED;

    return VINF_SUCCESS;
}
#endif

/** @interface_method_impl{PDMIMOUNT,pfnUnmount} */
static DECLCALLBACK(int) drvHostDvdUnmount(PPDMIMOUNT pInterface, bool fForce, bool fEject)
{
    PDRVHOSTBASE pThis = PDMIMOUNT_2_DRVHOSTBASE(pInterface);
    RTCritSectEnter(&pThis->CritSect);

    /*
     * Validate state.
     */
    int rc = VINF_SUCCESS;
    if (!pThis->fLocked || fForce)
    {
        /* Unlock drive if necessary. */
        if (pThis->fLocked)
            drvHostDvdDoLock(pThis, false);

        if (fEject)
        {
            /*
             * Eject the disc.
             */
#if defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
            uint8_t abCmd[16] =
            {
                SCSI_START_STOP_UNIT, 0, 0, 0, 2 /*eject+stop*/, 0,
                0,0,0,0,0,0,0,0,0,0
            };
            rc = drvHostBaseScsiCmdOs(pThis, abCmd, 6, PDMMEDIATXDIR_NONE, NULL, NULL, NULL, 0, 0);

#elif defined(RT_OS_LINUX)
            rc = ioctl(RTFileToNative(pThis->hFileDevice), CDROMEJECT, 0);
            if (rc < 0)
            {
                if (errno == EBUSY)
                    rc = VERR_PDM_MEDIA_LOCKED;
                else if (errno == ENOSYS)
                    rc = VERR_NOT_SUPPORTED;
                else
                    rc = RTErrConvertFromErrno(errno);
            }

#elif defined(RT_OS_SOLARIS)
            rc = ioctl(RTFileToNative(pThis->hFileRawDevice), DKIOCEJECT, 0);
            if (rc < 0)
            {
                if (errno == EBUSY)
                    rc = VERR_PDM_MEDIA_LOCKED;
                else if (errno == ENOSYS || errno == ENOTSUP)
                    rc = VERR_NOT_SUPPORTED;
                else if (errno == ENODEV)
                    rc = VERR_PDM_MEDIA_NOT_MOUNTED;
                else
                    rc = RTErrConvertFromErrno(errno);
            }

#elif defined(RT_OS_WINDOWS)
            RTFILE hFileDevice = pThis->hFileDevice;
            if (hFileDevice == NIL_RTFILE) /* obsolete crap */
                rc = RTFileOpen(&hFileDevice, pThis->pszDeviceOpen, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
            if (RT_SUCCESS(rc))
            {
                /* do ioctl */
                DWORD cbReturned;
                if (DeviceIoControl((HANDLE)RTFileToNative(hFileDevice), IOCTL_STORAGE_EJECT_MEDIA,
                                    NULL, 0,
                                    NULL, 0, &cbReturned,
                                    NULL))
                    rc = VINF_SUCCESS;
                else
                    rc = RTErrConvertFromWin32(GetLastError());

                /* clean up handle */
                if (hFileDevice != pThis->hFileDevice)
                    RTFileClose(hFileDevice);
            }
            else
                AssertMsgFailed(("Failed to open '%s' for ejecting this tray.\n",  rc));


#else
            AssertMsgFailed(("Eject is not implemented!\n"));
            rc = VINF_SUCCESS;
#endif
        }

        /*
         * Media is no longer present.
         */
        DRVHostBaseMediaNotPresent(pThis);  /** @todo This isn't thread safe! */
    }
    else
    {
        Log(("drvHostDvdUnmount: Locked\n"));
        rc = VERR_PDM_MEDIA_LOCKED;
    }

    RTCritSectLeave(&pThis->CritSect);
    LogFlow(("drvHostDvdUnmount: returns %Rrc\n", rc));
    return rc;
}


/**
 * Locks or unlocks the drive.
 *
 * @returns VBox status code.
 * @param   pThis       The instance data.
 * @param   fLock       True if the request is to lock the drive, false if to unlock.
 */
static DECLCALLBACK(int) drvHostDvdDoLock(PDRVHOSTBASE pThis, bool fLock)
{
#if defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
    uint8_t abCmd[16] =
    {
        SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL, 0, 0, 0, fLock, 0,
        0,0,0,0,0,0,0,0,0,0
    };
    int rc = drvHostBaseScsiCmdOs(pThis, abCmd, 6, PDMMEDIATXDIR_NONE, NULL, NULL, NULL, 0, 0);

#elif defined(RT_OS_LINUX)
    int rc = ioctl(RTFileToNative(pThis->hFileDevice), CDROM_LOCKDOOR, (int)fLock);
    if (rc < 0)
    {
        if (errno == EBUSY)
            rc = VERR_ACCESS_DENIED;
        else if (errno == EDRIVE_CANT_DO_THIS)
            rc = VERR_NOT_SUPPORTED;
        else
            rc = RTErrConvertFromErrno(errno);
    }

#elif defined(RT_OS_SOLARIS)
    int rc = ioctl(RTFileToNative(pThis->hFileRawDevice), fLock ? DKIOCLOCK : DKIOCUNLOCK, 0);
    if (rc < 0)
    {
        if (errno == EBUSY)
            rc = VERR_ACCESS_DENIED;
        else if (errno == ENOTSUP || errno == ENOSYS)
            rc = VERR_NOT_SUPPORTED;
        else
            rc = RTErrConvertFromErrno(errno);
    }

#elif defined(RT_OS_WINDOWS)

    PREVENT_MEDIA_REMOVAL PreventMediaRemoval = {fLock};
    DWORD cbReturned;
    int rc;
    if (DeviceIoControl((HANDLE)RTFileToNative(pThis->hFileDevice), IOCTL_STORAGE_MEDIA_REMOVAL,
                        &PreventMediaRemoval, sizeof(PreventMediaRemoval),
                        NULL, 0, &cbReturned,
                        NULL))
        rc = VINF_SUCCESS;
    else
        /** @todo figure out the return codes for already locked. */
        rc = RTErrConvertFromWin32(GetLastError());

#else
    AssertMsgFailed(("Lock/Unlock is not implemented!\n"));
    int rc = VINF_SUCCESS;

#endif

    LogFlow(("drvHostDvdDoLock(, fLock=%RTbool): returns %Rrc\n", fLock, rc));
    return rc;
}



#ifdef RT_OS_LINUX
/**
 * Get the media size.
 *
 * @returns VBox status code.
 * @param   pThis   The instance data.
 * @param   pcb     Where to store the size.
 */
static DECLCALLBACK(int) drvHostDvdGetMediaSize(PDRVHOSTBASE pThis, uint64_t *pcb)
{
    /*
     * Query the media size.
     */
    /* Clear the media-changed-since-last-call-thingy just to be on the safe side. */
    ioctl(RTFileToNative(pThis->hFileDevice), CDROM_MEDIA_CHANGED, CDSL_CURRENT);
    return RTFileSeek(pThis->hFileDevice, 0, RTFILE_SEEK_END, pcb);

}
#endif /* RT_OS_LINUX */


#ifdef USE_MEDIA_POLLING
/**
 * Do media change polling.
 */
static DECLCALLBACK(int) drvHostDvdPoll(PDRVHOSTBASE pThis)
{
    /*
     * Poll for media change.
     */
#if defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
#ifdef RT_OS_DARWIN
    AssertReturn(pThis->ppScsiTaskDI, VERR_INTERNAL_ERROR);
#endif

    /*
     * Issue a TEST UNIT READY request.
     */
    bool fMediaChanged = false;
    bool fMediaPresent = false;
    uint8_t abCmd[16] = { SCSI_TEST_UNIT_READY, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    uint8_t abSense[32];
    int rc2 = drvHostBaseScsiCmdOs(pThis, abCmd, 6, PDMMEDIATXDIR_NONE, NULL, NULL, abSense, sizeof(abSense), 0);
    if (RT_SUCCESS(rc2))
        fMediaPresent = true;
    else if (   rc2 == VERR_UNRESOLVED_ERROR
             && abSense[2] == 6 /* unit attention */
             && (   (abSense[12] == 0x29 && abSense[13] < 5 /* reset */)
                 || (abSense[12] == 0x2a && abSense[13] == 0 /* parameters changed */)                        //???
                 || (abSense[12] == 0x3f && abSense[13] == 0 /* target operating conditions have changed */)  //???
                 || (abSense[12] == 0x3f && abSense[13] == 2 /* changed operating definition */)              //???
                 || (abSense[12] == 0x3f && abSense[13] == 3 /* inquiry parameters changed */)
                 || (abSense[12] == 0x3f && abSense[13] == 5 /* device identifier changed */)
                 )
            )
    {
        fMediaPresent = false;
        fMediaChanged = true;
        /** @todo check this media change stuff on Darwin. */
    }

#elif defined(RT_OS_LINUX)
    bool fMediaPresent = ioctl(RTFileToNative(pThis->hFileDevice), CDROM_DRIVE_STATUS, CDSL_CURRENT) == CDS_DISC_OK;
    bool fMediaChanged = false;
    if (pThis->fMediaPresent != fMediaPresent)
        fMediaChanged = ioctl(RTFileToNative(pThis->hFileDevice), CDROM_MEDIA_CHANGED, CDSL_CURRENT) == 1;

#elif defined(RT_OS_SOLARIS)
    bool fMediaPresent = false;
    bool fMediaChanged = false;

    /* Need to pass the previous state and DKIO_NONE for the first time. */
    static dkio_state s_DeviceState = DKIO_NONE;
    dkio_state PreviousState = s_DeviceState;
    int rc2 = ioctl(RTFileToNative(pThis->hFileRawDevice), DKIOCSTATE, &s_DeviceState);
    if (rc2 == 0)
    {
        fMediaPresent = (s_DeviceState == DKIO_INSERTED);
        if (PreviousState != s_DeviceState)
            fMediaChanged = true;
    }

#else
# error "Unsupported platform."
#endif

    RTCritSectEnter(&pThis->CritSect);

    int rc = VINF_SUCCESS;
    if (pThis->fMediaPresent != fMediaPresent)
    {
        LogFlow(("drvHostDvdPoll: %d -> %d\n", pThis->fMediaPresent, fMediaPresent));
        pThis->fMediaPresent = false;
        if (fMediaPresent)
            rc = DRVHostBaseMediaPresent(pThis);
        else
            DRVHostBaseMediaNotPresent(pThis);
    }
    else if (fMediaPresent)
    {
        /*
         * Poll for media change.
         */
        if (fMediaChanged)
        {
            LogFlow(("drvHostDVDMediaThread: Media changed!\n"));
            DRVHostBaseMediaNotPresent(pThis);
            rc = DRVHostBaseMediaPresent(pThis);
        }
    }

    RTCritSectLeave(&pThis->CritSect);
    return rc;
}
#endif /* USE_MEDIA_POLLING */


/** @interface_method_impl{PDMIMEDIA,pfnSendCmd} */
static DECLCALLBACK(int) drvHostDvdSendCmd(PPDMIMEDIA pInterface, const uint8_t *pbCmd,
                                           PDMMEDIATXDIR enmTxDir, void *pvBuf, uint32_t *pcbBuf,
                                           uint8_t *pabSense, size_t cbSense, uint32_t cTimeoutMillies)
{
    RT_NOREF(cbSense);
    PDRVHOSTBASE pThis = PDMIMEDIA_2_DRVHOSTBASE(pInterface);
    int rc;
    LogFlow(("%s: cmd[0]=%#04x txdir=%d pcbBuf=%d timeout=%d\n", __FUNCTION__, pbCmd[0], enmTxDir, *pcbBuf, cTimeoutMillies));

    /*
     * Pass the request on to the internal scsi command interface.
     * The command seems to be 12 bytes long, the docs a bit copy&pasty on the command length point...
     */
    if (enmTxDir == PDMMEDIATXDIR_FROM_DEVICE)
        memset(pvBuf, '\0', *pcbBuf); /* we got read size, but zero it anyway. */
    rc = drvHostBaseScsiCmdOs(pThis, pbCmd, 12, enmTxDir, pvBuf, pcbBuf, pabSense, cbSense, cTimeoutMillies);
    if (rc == VERR_UNRESOLVED_ERROR)
        /* sense information set */
        rc = VERR_DEV_IO_ERROR;

    if (pbCmd[0] == SCSI_GET_EVENT_STATUS_NOTIFICATION)
    {
        uint8_t *pbBuf = (uint8_t*)pvBuf;
        Log2(("Event Status Notification class=%#02x supported classes=%#02x\n", pbBuf[2], pbBuf[3]));
        if (RT_BE2H_U16(*(uint16_t*)pbBuf) >= 6)
            Log2(("  event %#02x %#02x %#02x %#02x\n", pbBuf[4], pbBuf[5], pbBuf[6], pbBuf[7]));
    }

    LogFlow(("%s: rc=%Rrc\n", __FUNCTION__, rc));
    return rc;
}


/* -=-=-=-=- driver interface -=-=-=-=- */


/** @copydoc FNPDMDRVDESTRUCT */
static DECLCALLBACK(void) drvHostDvdDestruct(PPDMDRVINS pDrvIns)
{
#ifdef RT_OS_LINUX
    PDRVHOSTBASE pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTBASE);

    if (pThis->pbDoubleBuffer)
    {
        RTMemFree(pThis->pbDoubleBuffer);
        pThis->pbDoubleBuffer = NULL;
    }
#endif
    return DRVHostBaseDestruct(pDrvIns);
}


/**
 * Construct a host dvd drive driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostDvdConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDRVHOSTBASE pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTBASE);
    LogFlow(("drvHostDvdConstruct: iInstance=%d\n", pDrvIns->iInstance));

    /*
     * Init instance data.
     */
    int rc = DRVHostBaseInitData(pDrvIns, pCfg, PDMMEDIATYPE_DVD);
    if (RT_SUCCESS(rc))
    {
        /*
         * Validate configuration.
         */
        if (CFGMR3AreValuesValid(pCfg, "Path\0Interval\0Locked\0BIOSVisible\0AttachFailError\0Passthrough\0"))
        {
            /*
             * Override stuff.
             */
#ifdef RT_OS_LINUX
            pThis->pbDoubleBuffer = (uint8_t *)RTMemAlloc(SCSI_MAX_BUFFER_SIZE);
            if (!pThis->pbDoubleBuffer)
                return VERR_NO_MEMORY;
#endif

            bool fPassthrough;
            rc = CFGMR3QueryBool(pCfg, "Passthrough", &fPassthrough);
            if (RT_SUCCESS(rc) && fPassthrough)
            {
                pThis->IMedia.pfnSendCmd = drvHostDvdSendCmd;
                /* Passthrough requires opening the device in R/W mode. */
                pThis->fReadOnlyConfig = false;
#ifdef VBOX_WITH_SUID_WRAPPER  /* Solaris setuid for Passthrough mode. */
                rc = solarisCheckUserAuth();
                if (RT_FAILURE(rc))
                {
                    Log(("DVD: solarisCheckUserAuth failed. Permission denied!\n"));
                    return rc;
                }
#endif /* VBOX_WITH_SUID_WRAPPER */
            }

            pThis->IMount.pfnUnmount = drvHostDvdUnmount;
            pThis->pfnDoLock         = drvHostDvdDoLock;
#ifdef USE_MEDIA_POLLING
            if (!fPassthrough)
                pThis->pfnPoll       = drvHostDvdPoll;
            else
                pThis->pfnPoll       = NULL;
#endif
#ifdef RT_OS_LINUX
            pThis->pfnGetMediaSize   = drvHostDvdGetMediaSize;
#endif

            /*
             * 2nd init part.
             */
            rc = DRVHostBaseInitFinish(pThis);
        }
        else
        {
            pThis->fAttachFailError = true;
            rc = VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;
        }
    }
    if (RT_FAILURE(rc))
    {
        if (!pThis->fAttachFailError)
        {
            /* Suppressing the attach failure error must not affect the normal
             * DRVHostBaseDestruct, so reset this flag below before leaving. */
            pThis->fKeepInstance = true;
            rc = VINF_SUCCESS;
        }
        DRVHostBaseDestruct(pDrvIns);
        pThis->fKeepInstance = false;
    }

    LogFlow(("drvHostDvdConstruct: returns %Rrc\n", rc));
    return rc;
}


/**
 * Block driver registration record.
 */
const PDMDRVREG g_DrvHostDVD =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "HostDVD",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Host DVD Block Driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_BLOCK,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTBASE),
    /* pfnConstruct */
    drvHostDvdConstruct,
    /* pfnDestruct */
    drvHostDvdDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

