/* $Id$ */
/** @file
 * Validation Kit audio driver.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 */

#include <iprt/alloc.h>
#include <iprt/uuid.h> /* For PDMIBASE_2_PDMDRV. */

#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"
#include "VBoxDD.h"


/**
 * Structure for keeping a debug input/output stream.
 */
typedef struct VAKITAUDIOSTREAM
{
    /** Note: Always must come first! */
    PDMAUDIOSTREAM     Stream;
    /** Audio file to dump output to or read input from. */
    PDMAUDIOFILE       File;
    union
    {
        struct
        {
            /** Timestamp of last captured samples. */
            uint64_t   tsLastCaptured;
        } In;
        struct
        {
            /** Timestamp of last played samples. */
            uint64_t   tsLastPlayed;
            uint64_t   cMaxSamplesInPlayBuffer;
            uint8_t   *pu8PlayBuffer;
        } Out;
    };

} VAKITAUDIOSTREAM, *PVAKITAUDIOSTREAM;

/**
 * Validation Kit audio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTVAKITAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS    pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO IHostAudio;
} DRVHOSTVAKITAUDIO, *PDRVHOSTVAKITAUDIO;

/*******************************************PDM_AUDIO_DRIVER******************************/


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostVaKitAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    NOREF(pInterface);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    pBackendCfg->cbStreamOut    = sizeof(VAKITAUDIOSTREAM);
    pBackendCfg->cbStreamIn     = sizeof(VAKITAUDIOSTREAM);

    pBackendCfg->cMaxStreamsOut = 1; /* Output */
    pBackendCfg->cMaxStreamsIn  = 2; /* Line input + microphone input. */

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostVaKitAudioInit(PPDMIHOSTAUDIO pInterface)
{
    NOREF(pInterface);

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostVaKitAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    NOREF(pInterface);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostVaKitAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


static int debugCreateStreamIn(PPDMIHOSTAUDIO pInterface,
                               PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    RT_NOREF(pInterface, pStream);

    /* Just adopt the wanted stream configuration. */
    PDMAUDIOPCMPROPS Props;
    int rc = DrvAudioHlpStreamCfgToProps(pCfgReq, &Props);
    if (RT_SUCCESS(rc))
    {
        if (pCfgAcq)
            pCfgAcq->cSampleBufferSize = _1K;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int debugCreateStreamOut(PPDMIHOSTAUDIO pInterface,
                                PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    NOREF(pInterface);

    PVAKITAUDIOSTREAM pDbgStream = (PVAKITAUDIOSTREAM)pStream;

    /* Just adopt the wanted stream configuration. */
    PDMAUDIOPCMPROPS Props;
    int rc = DrvAudioHlpStreamCfgToProps(pCfgReq, &Props);
    if (RT_SUCCESS(rc))
    {
        pDbgStream->Out.tsLastPlayed            = 0;
        pDbgStream->Out.cMaxSamplesInPlayBuffer = _1K;
        pDbgStream->Out.pu8PlayBuffer           = (uint8_t *)RTMemAlloc(pDbgStream->Out.cMaxSamplesInPlayBuffer << Props.cShift);
        if (!pDbgStream->Out.pu8PlayBuffer)
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        char szTemp[RTPATH_MAX];
        rc = RTPathTemp(szTemp, sizeof(szTemp));
        if (RT_SUCCESS(rc))
        {
            char szFile[RTPATH_MAX];
            rc = DrvAudioHlpGetFileName(szFile, RT_ELEMENTS(szFile), szTemp, NULL, PDMAUDIOFILETYPE_WAV);
            if (RT_SUCCESS(rc))
            {
                LogFlowFunc(("%s\n", szFile));
                rc = DrvAudioHlpWAVFileOpen(&pDbgStream->File, szFile,
                                            RTFILE_O_WRITE | RTFILE_O_DENY_WRITE | RTFILE_O_CREATE_REPLACE,
                                            &Props, PDMAUDIOFILEFLAG_NONE);
                if (RT_FAILURE(rc))
                    LogRel(("DebugAudio: Creating output file '%s' failed with %Rrc\n", szFile, rc));
            }
            else
                LogRel(("DebugAudio: Unable to build file name for temp dir '%s': %Rrc\n", szTemp, rc));
        }
        else
            LogRel(("DebugAudio: Unable to retrieve temp dir: %Rrc\n", rc));
    }

    if (RT_SUCCESS(rc))
    {
        if (pCfgAcq)
            pCfgAcq->cSampleBufferSize = pDbgStream->Out.cMaxSamplesInPlayBuffer;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostVaKitAudioStreamCreate(PPDMIHOSTAUDIO pInterface,
                                                       PPDMAUDIOSTREAM pStream,
                                                       PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = debugCreateStreamIn( pInterface, pStream, pCfgReq, pCfgAcq);
    else
        rc = debugCreateStreamOut(pInterface, pStream, pCfgReq, pCfgAcq);

    LogFlowFunc(("%s: rc=%Rrc\n", pStream->szName, rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvHostVaKitAudioStreamPlay(PPDMIHOSTAUDIO pInterface,
                                                     PPDMAUDIOSTREAM pStream, const void *pvBuf, uint32_t cbBuf,
                                                     uint32_t *pcbWritten)
{
    RT_NOREF(pvBuf, cbBuf);

    PDRVHOSTVAKITAUDIO pDrv       = RT_FROM_MEMBER(pInterface, DRVHOSTVAKITAUDIO, IHostAudio);
    PVAKITAUDIOSTREAM  pDbgStream = (PVAKITAUDIOSTREAM)pStream;

    /* Consume as many samples as would be played at the current frequency since last call. */
    /*uint32_t cLive           = AudioMixBufLive(&pStream->MixBuf);*/

    uint64_t u64TicksNow     = PDMDrvHlpTMGetVirtualTime(pDrv->pDrvIns);
   // uint64_t u64TicksElapsed = u64TicksNow  - pDbgStream->Out.tsLastPlayed;
   // uint64_t u64TicksFreq    = PDMDrvHlpTMGetVirtualFreq(pDrv->pDrvIns);

    /*
     * Minimize the rounding error by adding 0.5: samples = int((u64TicksElapsed * samplesFreq) / u64TicksFreq + 0.5).
     * If rounding is not taken into account then the playback rate will be consistently lower that expected.
     */
   // uint64_t cSamplesPlayed = (2 * u64TicksElapsed * pStream->Props.uHz + u64TicksFreq) / u64TicksFreq / 2;

    /* Don't play more than available. */
    /*if (cSamplesPlayed > cLive)
        cSamplesPlayed = cLive;*/

    uint32_t cSamplesPlayed = 0;
    uint32_t cSamplesAvail  = RT_MIN(AudioMixBufUsed(&pStream->MixBuf), pDbgStream->Out.cMaxSamplesInPlayBuffer);
    while (cSamplesAvail)
    {
        uint32_t cSamplesRead = 0;
        int rc2 = AudioMixBufReadCirc(&pStream->MixBuf, pDbgStream->Out.pu8PlayBuffer,
                                      AUDIOMIXBUF_S2B(&pStream->MixBuf, cSamplesAvail), &cSamplesRead);

        if (RT_FAILURE(rc2))
            LogRel(("DebugAudio: Reading output failed with %Rrc\n", rc2));

        if (!cSamplesRead)
            break;
#if 0
        RTFILE fh;
        RTFileOpen(&fh, "/tmp/AudioDebug-Output.pcm",
                   RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
        RTFileWrite(fh, pDbgStream->Out.pu8PlayBuffer, AUDIOMIXBUF_S2B(&pStream->MixBuf, cSamplesRead), NULL);
        RTFileClose(fh);
#endif
        rc2 = DrvAudioHlpWAVFileWrite(&pDbgStream->File,
                                      pDbgStream->Out.pu8PlayBuffer, AUDIOMIXBUF_S2B(&pStream->MixBuf, cSamplesRead),
                                      0 /* fFlags */);
        if (RT_FAILURE(rc2))
            LogRel(("DebugAudio: Writing output failed with %Rrc\n", rc2));

        AudioMixBufFinish(&pStream->MixBuf, cSamplesRead);

        Assert(cSamplesAvail >= cSamplesRead);
        cSamplesAvail -= cSamplesRead;

        cSamplesPlayed += cSamplesRead;
    }

    /* Remember when samples were consumed. */
    pDbgStream->Out.tsLastPlayed = u64TicksNow;

    if (pcbWritten)
        *pcbWritten = cSamplesPlayed;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostVaKitAudioStreamCapture(PPDMIHOSTAUDIO pInterface,
                                                        PPDMAUDIOSTREAM pStream, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead)
{
    RT_NOREF(pInterface, pStream, pvBuf, cbBuf);

    /* Never capture anything. */
    if (pcbRead)
        *pcbRead = 0;

    return VINF_SUCCESS;
}


static int debugDestroyStreamIn(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    RT_NOREF(pInterface, pStream);
    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}


static int debugDestroyStreamOut(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    RT_NOREF(pInterface);
    PVAKITAUDIOSTREAM pDbgStream = (PVAKITAUDIOSTREAM)pStream;
    if (   pDbgStream
        && pDbgStream->Out.pu8PlayBuffer)
    {
        RTMemFree(pDbgStream->Out.pu8PlayBuffer);
        pDbgStream->Out.pu8PlayBuffer = NULL;
    }

    size_t cbDataSize = DrvAudioHlpWAVFileGetDataSize(&pDbgStream->File);

    int rc = DrvAudioHlpWAVFileClose(&pDbgStream->File);
    if (RT_SUCCESS(rc))
    {
        /* Delete the file again if nothing but the header was written to it. */
        bool fDeleteEmptyFiles = true; /** @todo Make deletion configurable? */

        if (   !cbDataSize
            && fDeleteEmptyFiles)
        {
            rc = RTFileDelete(pDbgStream->File.szName);
        }
        else
            LogRel(("DebugAudio: Created output file '%s' (%zu bytes)\n", pDbgStream->File.szName, cbDataSize));
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static DECLCALLBACK(int) drvHostVaKitAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    int rc;
    if (pStream->enmDir == PDMAUDIODIR_IN)
        rc = debugDestroyStreamIn(pInterface,  pStream);
    else
        rc = debugDestroyStreamOut(pInterface, pStream);

    return rc;
}

static DECLCALLBACK(int) drvHostVaKitAudioStreamControl(PPDMIHOSTAUDIO pInterface,
                                                        PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    RT_NOREF(enmStreamCmd);

    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    Assert(pStream->enmCtx == PDMAUDIOSTREAMCTX_HOST);

    return VINF_SUCCESS;
}

static DECLCALLBACK(PDMAUDIOSTRMSTS) drvHostVaKitAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    NOREF(pInterface);
    NOREF(pStream);

    return (  PDMAUDIOSTRMSTS_FLAG_INITIALIZED | PDMAUDIOSTRMSTS_FLAG_ENABLED
            | PDMAUDIOSTRMSTS_FLAG_DATA_READABLE | PDMAUDIOSTRMSTS_FLAG_DATA_WRITABLE);
}

static DECLCALLBACK(int) drvHostVaKitAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    NOREF(pInterface);
    NOREF(pStream);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostVaKitAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS         pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTVAKITAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTVAKITAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);
    return NULL;
}


/**
 * Constructs a Null audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostVaKitAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTVAKITAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTVAKITAUDIO);
    LogRel(("Audio: Initializing ValidationKit driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostVaKitAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostVaKitAudio);

    return VINF_SUCCESS;
}

/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostValidationKitAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "ValidationKitAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "ValidationKit audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DRVHOSTVAKITAUDIO),
    /* pfnConstruct */
    drvHostVaKitAudioConstruct,
    /* pfnDestruct */
    NULL,
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

