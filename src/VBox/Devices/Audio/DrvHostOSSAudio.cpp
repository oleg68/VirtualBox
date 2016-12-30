/* $Id$ */
/** @file
 * OSS (Open Sound System) host audio backend.
 */

/*
 * Copyright (C) 2014-2016 Oracle Corporation
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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/soundcard.h>
#include <unistd.h>

#include <iprt/alloc.h>
#include <iprt/uuid.h> /* For PDMIBASE_2_PDMDRV. */

#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>
#include <VBox/vmm/pdmaudioifs.h>

#include "DrvAudio.h"
#include "AudioMixBuffer.h"

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

#if ((SOUND_VERSION > 360) && (defined(OSS_SYSINFO)))
/* OSS > 3.6 has a new syscall available for querying a bit more detailed information
 * about OSS' audio capabilities. This is handy for e.g. Solaris. */
# define VBOX_WITH_AUDIO_OSS_SYSINFO 1
#endif

/** Makes DRVHOSTOSSAUDIO out of PDMIHOSTAUDIO. */
#define PDMIHOSTAUDIO_2_DRVHOSTOSSAUDIO(pInterface) \
    ( (PDRVHOSTOSSAUDIO)((uintptr_t)pInterface - RT_OFFSETOF(DRVHOSTOSSAUDIO, IHostAudio)) )


/*********************************************************************************************************************************
*   Structures                                                                                                                   *
*********************************************************************************************************************************/

/**
 * OSS host audio driver instance data.
 * @implements PDMIAUDIOCONNECTOR
 */
typedef struct DRVHOSTOSSAUDIO
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS         pDrvIns;
    /** Pointer to host audio interface. */
    PDMIHOSTAUDIO      IHostAudio;
    /** Error count for not flooding the release log.
     *  UINT32_MAX for unlimited logging. */
    uint32_t           cLogErrors;
} DRVHOSTOSSAUDIO, *PDRVHOSTOSSAUDIO;

typedef struct OSSAUDIOSTREAMCFG
{
    PDMAUDIOFMT       enmFormat;
    PDMAUDIOENDIANNESS enmENDIANNESS;
    uint16_t          uFreq;
    uint8_t           cChannels;
    uint16_t          cFragments;
    uint32_t          cbFragmentSize;
} OSSAUDIOSTREAMCFG, *POSSAUDIOSTREAMCFG;

typedef struct OSSAUDIOSTREAMIN
{
    /** Note: Always must come first! */
    PDMAUDIOSTREAM     pStreamIn;
    /** The PCM properties of this stream. */
    PDMAUDIOPCMPROPS   Props;
    int                hFile;
    int                cFragments;
    int                cbFragmentSize;
    /** Own PCM buffer. */
    void              *pvBuf;
    /** Size (in bytes) of own PCM buffer. */
    size_t             cbBuf;
    int                old_optr;
} OSSAUDIOSTREAMIN, *POSSAUDIOSTREAMIN;

typedef struct OSSAUDIOSTREAMOUT
{
    /** Note: Always must come first! */
    PDMAUDIOSTREAM      pStreamOut;
    /** The PCM properties of this stream. */
    PDMAUDIOPCMPROPS    Props;
    int                 hFile;
    int                 cFragments;
    int                 cbFragmentSize;
#ifndef RT_OS_L4
    /** Whether we use a memory mapped file instead of our
     *  own allocated PCM buffer below. */
    /** @todo The memory mapped code seems to be utterly broken.
     *        Needs investigation! */
    bool                fMMIO;
#endif
    /** Own PCM buffer in case memory mapping is unavailable. */
    void               *pvBuf;
    /** Size (in bytes) of own PCM buffer. */
    size_t              cbBuf;
    int                 old_optr;
} OSSAUDIOSTREAMOUT, *POSSAUDIOSTREAMOUT;

typedef struct OSSAUDIOCFG
{
#ifndef RT_OS_L4
    bool try_mmap;
#endif
    int nfrags;
    int fragsize;
    const char *devpath_out;
    const char *devpath_in;
    int debug;
} OSSAUDIOCFG, *POSSAUDIOCFG;

static OSSAUDIOCFG s_OSSConf =
{
#ifndef RT_OS_L4
    false,
#endif
    4,
    4096,
    "/dev/dsp",
    "/dev/dsp",
    0
};


/* http://www.df.lth.se/~john_e/gems/gem002d.html */
static uint32_t popcount(uint32_t u)
{
    u = ((u&0x55555555) + ((u>>1)&0x55555555));
    u = ((u&0x33333333) + ((u>>2)&0x33333333));
    u = ((u&0x0f0f0f0f) + ((u>>4)&0x0f0f0f0f));
    u = ((u&0x00ff00ff) + ((u>>8)&0x00ff00ff));
    u = ( u&0x0000ffff) + (u>>16);
    return u;
}


static uint32_t lsbindex(uint32_t u)
{
    return popcount ((u&-u)-1);
}


static int ossAudioFmtToOSS(PDMAUDIOFMT fmt)
{
    switch (fmt)
    {
        case PDMAUDIOFMT_S8:
            return AFMT_S8;

        case PDMAUDIOFMT_U8:
            return AFMT_U8;

        case PDMAUDIOFMT_S16:
            return AFMT_S16_LE;

        case PDMAUDIOFMT_U16:
            return AFMT_U16_LE;

        default:
            break;
    }

    AssertMsgFailed(("Format %ld not supported\n", fmt));
    return AFMT_U8;
}


static int ossOSSToAudioFmt(int fmt, PDMAUDIOFMT *pFmt, PDMAUDIOENDIANNESS *pENDIANNESS)
{
    switch (fmt)
    {
        case AFMT_S8:
            *pFmt = PDMAUDIOFMT_S8;
            if (pENDIANNESS)
                *pENDIANNESS = PDMAUDIOENDIANNESS_LITTLE;
            break;

        case AFMT_U8:
            *pFmt = PDMAUDIOFMT_U8;
            if (pENDIANNESS)
                *pENDIANNESS = PDMAUDIOENDIANNESS_LITTLE;
            break;

        case AFMT_S16_LE:
            *pFmt = PDMAUDIOFMT_S16;
            if (pENDIANNESS)
                *pENDIANNESS = PDMAUDIOENDIANNESS_LITTLE;
            break;

        case AFMT_U16_LE:
            *pFmt = PDMAUDIOFMT_U16;
            if (pENDIANNESS)
                *pENDIANNESS = PDMAUDIOENDIANNESS_LITTLE;
            break;

        case AFMT_S16_BE:
            *pFmt = PDMAUDIOFMT_S16;
            if (pENDIANNESS)
                *pENDIANNESS = PDMAUDIOENDIANNESS_BIG;
            break;

        case AFMT_U16_BE:
            *pFmt = PDMAUDIOFMT_U16;
            if (pENDIANNESS)
                *pENDIANNESS = PDMAUDIOENDIANNESS_BIG;
            break;

        default:
            AssertMsgFailed(("Format %ld not supported\n", fmt));
            return VERR_NOT_SUPPORTED;
    }

    return VINF_SUCCESS;
}


static int ossStreamClose(int *phFile)
{
    if (!phFile || !*phFile)
        return VINF_SUCCESS;

    int rc;
    if (close(*phFile))
    {
        LogRel(("OSS: Closing stream failed: %s\n", strerror(errno)));
        rc = VERR_GENERAL_FAILURE; /** @todo */
    }
    else
    {
        *phFile = -1;
        rc = VINF_SUCCESS;
    }

    return rc;
}


static int ossStreamOpen(const char *pszDev, int fOpen, POSSAUDIOSTREAMCFG pReq, POSSAUDIOSTREAMCFG pObt, int *phFile)
{
    int rc;

    int hFile = -1;
    do
    {
        hFile = open(pszDev, fOpen);
        if (hFile == -1)
        {
            LogRel(("OSS: Failed to open %s: %s (%d)\n", pszDev, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        int iFormat = ossAudioFmtToOSS(pReq->enmFormat);
        if (ioctl(hFile, SNDCTL_DSP_SAMPLESIZE, &iFormat))
        {
            LogRel(("OSS: Failed to set audio format to %ld: %s (%d)\n", iFormat, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        int cChannels = pReq->cChannels;
        if (ioctl(hFile, SNDCTL_DSP_CHANNELS, &cChannels))
        {
            LogRel(("OSS: Failed to set number of audio channels (%d): %s (%d)\n", pReq->cChannels, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        int freq = pReq->uFreq;
        if (ioctl(hFile, SNDCTL_DSP_SPEED, &freq))
        {
            LogRel(("OSS: Failed to set audio frequency (%dHZ): %s (%d)\n", pReq->uFreq, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        /* Obsolete on Solaris (using O_NONBLOCK is sufficient). */
#if !(defined(VBOX) && defined(RT_OS_SOLARIS))
        if (ioctl(hFile, SNDCTL_DSP_NONBLOCK))
        {
            LogRel(("OSS: Failed to set non-blocking mode: %s (%d)\n", strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }
#endif

        /* Check access mode (input or output). */
        bool fIn = ((fOpen & O_ACCMODE) == O_RDONLY);

        LogRel2(("OSS: Requested %RU16 %s fragments, %RU32 bytes each\n",
                 pReq->cFragments, fIn ? "input" : "output", pReq->cbFragmentSize));

        int mmmmssss = (pReq->cFragments << 16) | lsbindex(pReq->cbFragmentSize);
        if (ioctl(hFile, SNDCTL_DSP_SETFRAGMENT, &mmmmssss))
        {
            LogRel(("OSS: Failed to set %RU16 fragments to %RU32 bytes each: %s (%d)\n",
                    pReq->cFragments, pReq->cbFragmentSize, strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        audio_buf_info abinfo;
        if (ioctl(hFile, fIn ? SNDCTL_DSP_GETISPACE : SNDCTL_DSP_GETOSPACE, &abinfo))
        {
            LogRel(("OSS: Failed to retrieve %s buffer length: %s (%d)\n", fIn ? "input" : "output", strerror(errno), errno));
            rc = RTErrConvertFromErrno(errno);
            break;
        }

        rc = ossOSSToAudioFmt(iFormat, &pObt->enmFormat, &pObt->enmENDIANNESS);
        if (RT_SUCCESS(rc))
        {
            pObt->cChannels      = cChannels;
            pObt->uFreq          = freq;
            pObt->cFragments     = abinfo.fragstotal;
            pObt->cbFragmentSize = abinfo.fragsize;

            LogRel2(("OSS: Got %RU16 %s fragments, %RU32 bytes each\n",
                     pObt->cFragments, fIn ? "input" : "output", pObt->cbFragmentSize));

            *phFile = hFile;
        }
    }
    while (0);

    if (RT_FAILURE(rc))
        ossStreamClose(&hFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int ossControlStreamIn(/*PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd*/ void)
{
    /** @todo Nothing to do here right now!? */

    return VINF_SUCCESS;
}


static int ossControlStreamOut(PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    POSSAUDIOSTREAMOUT pStreamOut = (POSSAUDIOSTREAMOUT)pStream;

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
        {
            DrvAudioHlpClearBuf(&pStreamOut->Props, pStreamOut->pvBuf, pStreamOut->cbBuf,
                                AUDIOMIXBUF_B2S(&pStream->MixBuf, pStreamOut->cbBuf));

            int mask = PCM_ENABLE_OUTPUT;
            if (ioctl(pStreamOut->hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
            {
                LogRel(("OSS: Failed to enable output stream: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
            }

            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            int mask = 0;
            if (ioctl(pStreamOut->hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
            {
                LogRel(("OSS: Failed to disable output stream: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
            }

            break;
        }

        default:
            AssertMsgFailed(("Invalid command %ld\n", enmStreamCmd));
            rc = VERR_INVALID_PARAMETER;
            break;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostOSSAudioInit(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);

    LogFlowFuncEnter();

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamCapture(PPDMIHOSTAUDIO pInterface,
                                                      PPDMAUDIOSTREAM pStream, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead)
{
    RT_NOREF(pInterface, cbBuf, pvBuf);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    POSSAUDIOSTREAMIN pStrm = (POSSAUDIOSTREAMIN)pStream;

    int rc = VINF_SUCCESS;
    size_t cbToRead = RT_MIN(pStrm->cbBuf,
                             AudioMixBufFreeBytes(&pStream->MixBuf));

    LogFlowFunc(("cbToRead=%zu\n", cbToRead));

    uint32_t cWrittenTotal = 0;
    uint32_t cbTemp;
    ssize_t  cbRead;
    size_t   offWrite = 0;

    while (cbToRead)
    {
        cbTemp = RT_MIN(cbToRead, pStrm->cbBuf);
        AssertBreakStmt(cbTemp, rc = VERR_NO_DATA);
        cbRead = read(pStrm->hFile, (uint8_t *)pStrm->pvBuf + offWrite, cbTemp);

        LogFlowFunc(("cbRead=%zi, cbTemp=%RU32, cbToRead=%zu\n", cbRead, cbTemp, cbToRead));

        if (cbRead < 0)
        {
            switch (errno)
            {
                case 0:
                {
                    LogFunc(("Failed to read %z frames\n", cbRead));
                    rc = VERR_ACCESS_DENIED;
                    break;
                }

                case EINTR:
                case EAGAIN:
                    rc = VERR_NO_DATA;
                    break;

                default:
                    LogFlowFunc(("Failed to read %zu input frames, rc=%Rrc\n", cbTemp, rc));
                    rc = VERR_GENERAL_FAILURE; /** @todo Fix this. */
                    break;
            }

            if (RT_FAILURE(rc))
                break;
        }
        else if (cbRead)
        {
            uint32_t cWritten;
            rc = AudioMixBufWriteCirc(&pStream->MixBuf, pStrm->pvBuf, cbRead, &cWritten);
            if (RT_FAILURE(rc))
                break;

            uint32_t cbWritten = AUDIOMIXBUF_S2B(&pStream->MixBuf, cWritten);

            Assert(cbToRead >= cbWritten);
            cbToRead      -= cbWritten;
            offWrite      += cbWritten;
            cWrittenTotal += cWritten;
        }
        else /* No more data, try next round. */
            break;
    }

    if (rc == VERR_NO_DATA)
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        uint32_t cProcessed = 0;
        if (cWrittenTotal)
            rc = AudioMixBufMixToParent(&pStream->MixBuf, cWrittenTotal, &cProcessed);

        if (pcbRead)
            *pcbRead = cWrittenTotal;

        LogFlowFunc(("cWrittenTotal=%RU32 (%RU32 processed), rc=%Rrc\n",
                     cWrittenTotal, cProcessed, rc));
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int ossDestroyStreamIn(PPDMAUDIOSTREAM pStream)
{
    POSSAUDIOSTREAMIN pStrm = (POSSAUDIOSTREAMIN)pStream;

    LogFlowFuncEnter();

    if (pStrm->pvBuf)
    {
        Assert(pStrm->cbBuf);

        RTMemFree(pStrm->pvBuf);
        pStrm->pvBuf = NULL;
    }

    pStrm->cbBuf = 0;

    ossStreamClose(&pStrm->hFile);

    return VINF_SUCCESS;
}


static int ossDestroyStreamOut(PPDMAUDIOSTREAM pStream)
{
    POSSAUDIOSTREAMOUT pStrm = (POSSAUDIOSTREAMOUT)pStream;

    LogFlowFuncEnter();

#ifndef RT_OS_L4
    if (pStrm->fMMIO)
    {
        if (pStrm->pvBuf)
        {
            Assert(pStrm->cbBuf);

            int rc2 = munmap(pStrm->pvBuf, pStrm->cbBuf);
            if (rc2 == 0)
            {
                pStrm->pvBuf      = NULL;
                pStrm->cbBuf      = 0;

                pStrm->fMMIO      = false;
            }
            else
                LogRel(("OSS: Failed to memory unmap playback buffer on close: %s\n", strerror(errno)));
        }
    }
    else
    {
#endif
        if (pStrm->pvBuf)
        {
            Assert(pStrm->cbBuf);

            RTMemFree(pStrm->pvBuf);
            pStrm->pvBuf = NULL;
        }

        pStrm->cbBuf = 0;
#ifndef RT_OS_L4
    }
#endif

    ossStreamClose(&pStrm->hFile);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostOSSAudioGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);

    pBackendCfg->cbStreamIn  = sizeof(OSSAUDIOSTREAMIN);
    pBackendCfg->cbStreamOut = sizeof(OSSAUDIOSTREAMOUT);

    int hFile = open("/dev/dsp", O_WRONLY | O_NONBLOCK, 0);
    if (hFile == -1)
    {
        /* Try opening the mixing device instead. */
        hFile = open("/dev/mixer", O_RDONLY | O_NONBLOCK, 0);
    }

    int ossVer = -1;

#ifdef VBOX_WITH_AUDIO_OSS_SYSINFO
    oss_sysinfo ossInfo;
    RT_ZERO(ossInfo);
#endif

    if (hFile != -1)
    {
        int err = ioctl(hFile, OSS_GETVERSION, &ossVer);
        if (err == 0)
        {
            LogRel2(("OSS: Using version: %d\n", ossVer));
#ifdef VBOX_WITH_AUDIO_OSS_SYSINFO
            err = ioctl(hFile, OSS_SYSINFO, &ossInfo);
            if (err == 0)
            {
                LogRel2(("OSS: Number of DSPs: %d\n", ossInfo.numaudios));
                LogRel2(("OSS: Number of mixers: %d\n", ossInfo.nummixers));

                int cDev = ossInfo.nummixers;
                if (!cDev)
                    cDev = ossInfo.numaudios;

                pBackendCfg->cMaxStreamsIn   = UINT32_MAX;
                pBackendCfg->cMaxStreamsOut  = UINT32_MAX;
            }
            else
            {
#endif
                /* Since we cannot query anything, assume that we have at least
                 * one input and one output if we found "/dev/dsp" or "/dev/mixer". */

                pBackendCfg->cMaxStreamsIn   = UINT32_MAX;
                pBackendCfg->cMaxStreamsOut  = UINT32_MAX;
#ifdef VBOX_WITH_AUDIO_OSS_SYSINFO
            }
#endif
        }
        else
            LogRel(("OSS: Unable to determine installed version: %s (%d)\n", strerror(err), err));
    }
    else
        LogRel(("OSS: No devices found, audio is not available\n"));

    return VINF_SUCCESS;
}


static int ossCreateStreamIn(PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    POSSAUDIOSTREAMIN pStrm = (POSSAUDIOSTREAMIN)pStream;

    int rc;
    int hFile = -1;

    do
    {
        uint32_t cSamples;

        OSSAUDIOSTREAMCFG reqStream, obtStream;
        reqStream.enmFormat      = pCfgReq->enmFormat;
        reqStream.uFreq          = pCfgReq->uHz;
        reqStream.cChannels      = pCfgReq->cChannels;
        reqStream.cFragments     = s_OSSConf.nfrags;
        reqStream.cbFragmentSize = s_OSSConf.fragsize;

        rc = ossStreamOpen(s_OSSConf.devpath_in, O_RDONLY | O_NONBLOCK, &reqStream, &obtStream, &hFile);
        if (RT_SUCCESS(rc))
        {
            pCfgAcq->enmFormat     = obtStream.enmFormat;
            pCfgAcq->uHz           = obtStream.uFreq;
            pCfgAcq->cChannels     = obtStream.cChannels;
            pCfgAcq->enmEndianness = obtStream.enmENDIANNESS;

            rc = DrvAudioHlpStreamCfgToProps(pCfgAcq, &pStrm->Props);
            if (RT_SUCCESS(rc))
            {
                if (obtStream.cFragments * obtStream.cbFragmentSize & pStrm->Props.uAlign)
                {
                    LogRel(("OSS: Warning: Misaligned capturing buffer: Size = %zu, Alignment = %u\n",
                            obtStream.cFragments * obtStream.cbFragmentSize, pStrm->Props.uAlign + 1));
                }

                cSamples = (obtStream.cFragments * obtStream.cbFragmentSize) >> pStrm->Props.cShift;
                if (!cSamples)
                    rc = VERR_INVALID_PARAMETER;
            }

            if (RT_SUCCESS(rc))
            {
                size_t cbSample = (1 << pStrm->Props.cShift);

                size_t cbBuf = cSamples * cbSample;
                void  *pvBuf = RTMemAlloc(cbBuf);
                if (!pvBuf)
                {
                    LogRel(("OSS: Failed allocating capturing buffer with %RU32 samples (%zu bytes per sample)\n",
                            cSamples, cbSample));
                    rc = VERR_NO_MEMORY;
                    break;
                }

                pStrm->hFile = hFile;
                pStrm->pvBuf = pvBuf;
                pStrm->cbBuf = cbBuf;

                pCfgAcq->cSampleBufferSize = cSamples;
            }
        }

    } while (0);

    if (RT_FAILURE(rc))
        ossStreamClose(&hFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static int ossCreateStreamOut(PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    POSSAUDIOSTREAMOUT pStrm = (POSSAUDIOSTREAMOUT)pStream;

    int rc;
    int hFile = -1;

    do
    {
        uint32_t cSamples;

        OSSAUDIOSTREAMCFG reqStream, obtStream;
        reqStream.enmFormat      = pCfgReq->enmFormat;
        reqStream.uFreq          = pCfgReq->uHz;
        reqStream.cChannels      = pCfgReq->cChannels;
        reqStream.cFragments     = s_OSSConf.nfrags;
        reqStream.cbFragmentSize = s_OSSConf.fragsize;

        rc = ossStreamOpen(s_OSSConf.devpath_out, O_WRONLY, &reqStream, &obtStream, &hFile);
        if (RT_SUCCESS(rc))
        {
            pCfgAcq->enmFormat     = obtStream.enmFormat;
            pCfgAcq->uHz           = obtStream.uFreq;
            pCfgAcq->cChannels     = obtStream.cChannels;
            pCfgAcq->enmEndianness = obtStream.enmENDIANNESS;

            rc = DrvAudioHlpStreamCfgToProps(pCfgAcq, &pStrm->Props);
            if (RT_SUCCESS(rc))
            {
                cSamples = (obtStream.cFragments * obtStream.cbFragmentSize) >> pStrm->Props.cShift;

                if (obtStream.cFragments * obtStream.cbFragmentSize & pStrm->Props.uAlign)
                {
                    LogRel(("OSS: Warning: Misaligned playback buffer: Size = %zu, Alignment = %u\n",
                            obtStream.cFragments * obtStream.cbFragmentSize, pStrm->Props.uAlign + 1));
                }
            }
        }

        if (RT_SUCCESS(rc))
        {
            pStrm->fMMIO = false;

            size_t cbSample = (1 << pStrm->Props.cShift);

            size_t cbSamples = cSamples * cbSample;
            Assert(cbSamples);

#ifndef RT_OS_L4
            if (s_OSSConf.try_mmap)
            {
                pStrm->pvBuf = mmap(0, cbSamples, PROT_READ | PROT_WRITE, MAP_SHARED, hFile, 0);
                if (pStrm->pvBuf == MAP_FAILED)
                {
                    LogRel(("OSS: Failed to memory map %zu bytes of playback buffer: %s\n", cbSamples, strerror(errno)));
                    rc = RTErrConvertFromErrno(errno);
                    break;
                }
                else
                {
                    int mask = 0;
                    if (ioctl(hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
                    {
                        LogRel(("OSS: Failed to retrieve initial trigger mask for playback buffer: %s\n", strerror(errno)));
                        rc = RTErrConvertFromErrno(errno);
                        /* Note: No break here, need to unmap file first! */
                    }
                    else
                    {
                        mask = PCM_ENABLE_OUTPUT;
                        if (ioctl (hFile, SNDCTL_DSP_SETTRIGGER, &mask) < 0)
                        {
                            LogRel(("OSS: Failed to retrieve PCM_ENABLE_OUTPUT mask: %s\n", strerror(errno)));
                            rc = RTErrConvertFromErrno(errno);
                            /* Note: No break here, need to unmap file first! */
                        }
                        else
                        {
                            pStrm->fMMIO = true;
                            LogRel(("OSS: Using MMIO\n"));
                        }
                    }

                    if (RT_FAILURE(rc))
                    {
                        int rc2 = munmap(pStrm->pvBuf, cbSamples);
                        if (rc2)
                            LogRel(("OSS: Failed to memory unmap playback buffer: %s\n", strerror(errno)));
                        break;
                    }
                }
            }
#endif /* !RT_OS_L4 */

            /* Memory mapping failed above? Try allocating an own buffer. */
#ifndef RT_OS_L4
            if (!pStrm->fMMIO)
            {
#endif
                void *pvBuf = RTMemAlloc(cbSamples);
                if (!pvBuf)
                {
                    LogRel(("OSS: Failed allocating playback buffer with %RU32 samples (%zu bytes)\n", cSamples, cbSamples));
                    rc = VERR_NO_MEMORY;
                    break;
                }

                pStrm->hFile = hFile;
                pStrm->pvBuf = pvBuf;
                pStrm->cbBuf = cbSamples;
#ifndef RT_OS_L4
            }
#endif
            pCfgAcq->cSampleBufferSize = cSamples;
        }

    } while (0);

    if (RT_FAILURE(rc))
        ossStreamClose(&hFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamPlay(PPDMIHOSTAUDIO pInterface,
                                                   PPDMAUDIOSTREAM pStream, const void *pvBuf, uint32_t cbBuf,
                                                   uint32_t *pcbWritten)
{
    RT_NOREF(pInterface, cbBuf, pvBuf);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    POSSAUDIOSTREAMOUT pStrm = (POSSAUDIOSTREAMOUT)pStream;

    int rc = VINF_SUCCESS;
    uint32_t cbWrittenTotal = 0;

#ifndef RT_OS_L4
    count_info cntinfo;
#endif

    do
    {
        size_t cbBufSize = AudioMixBufSizeBytes(&pStream->MixBuf);

        uint32_t csLive = AudioMixBufLive(&pStream->MixBuf);
        uint32_t csToRead;

#ifndef RT_OS_L4
        if (pStrm->fMMIO)
        {
            /* Get current playback pointer. */
            int rc2 = ioctl(pStrm->hFile, SNDCTL_DSP_GETOPTR, &cntinfo);
            if (!rc2)
            {
                LogRel(("OSS: Failed to retrieve current playback pointer: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
                break;
            }

            /* Nothing to play? */
            if (cntinfo.ptr == pStrm->old_optr)
                break;

            int cbData;
            if (cntinfo.ptr > pStrm->old_optr)
                cbData = cntinfo.ptr - pStrm->old_optr;
            else
                cbData = cbBufSize + cntinfo.ptr - pStrm->old_optr;
            Assert(cbData);

            csToRead = RT_MIN((uint32_t)AUDIOMIXBUF_B2S(&pStream->MixBuf, cbData),
                             csLive);
        }
        else
        {
#endif
            audio_buf_info abinfo;
            int rc2 = ioctl(pStrm->hFile, SNDCTL_DSP_GETOSPACE, &abinfo);
            if (rc2 < 0)
            {
                LogRel(("OSS: Failed to retrieve current playback buffer: %s\n", strerror(errno)));
                rc = RTErrConvertFromErrno(errno);
                break;
            }

            if ((size_t)abinfo.bytes > cbBufSize)
            {
                LogRel2(("OSS: Warning: Too big output size (%d > %zu), limiting to %zu\n", abinfo.bytes, cbBufSize, cbBufSize));
                abinfo.bytes = cbBufSize;
                /* Keep going. */
            }

            if (abinfo.bytes < 0)
            {
                LogRel2(("OSS: Warning: Invalid available size (%d vs. %zu)\n", abinfo.bytes, cbBufSize));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            csToRead = RT_MIN((uint32_t)AUDIOMIXBUF_B2S(&pStream->MixBuf, abinfo.fragments * abinfo.fragsize), csLive);
            if (!csToRead)
                break;
#ifndef RT_OS_L4
        }
#endif
        size_t cbToRead = RT_MIN(AUDIOMIXBUF_S2B(&pStream->MixBuf, csToRead), pStrm->cbBuf);

        uint32_t csRead, cbRead;
        while (cbToRead)
        {
            rc = AudioMixBufReadCirc(&pStream->MixBuf, pStrm->pvBuf, cbToRead, &csRead);
            if (RT_FAILURE(rc))
                break;

            cbRead = AUDIOMIXBUF_S2B(&pStream->MixBuf, csRead);

            uint32_t cbChunk    = cbRead;
            uint32_t cbChunkOff = 0;
            while (cbChunk)
            {
                ssize_t cbChunkWritten = write(pStrm->hFile, (uint8_t *)pStrm->pvBuf + cbChunkOff,
                                               RT_MIN(cbChunk, (unsigned)s_OSSConf.fragsize));
                if (cbChunkWritten < 0)
                {
                    LogRel(("OSS: Failed writing output data: %s\n", strerror(errno)));
                    rc = RTErrConvertFromErrno(errno);
                    break;
                }

                if (cbChunkWritten & pStrm->Props.uAlign)
                {
                    LogRel(("OSS: Misaligned write (written %z, expected %RU32)\n", cbChunkWritten, cbChunk));
                    break;
                }

                cbChunkOff += (uint32_t)cbChunkWritten;
                Assert(cbChunkOff <= cbRead);
                Assert(cbChunk    >= (uint32_t)cbChunkWritten);
                cbChunk    -= (uint32_t)cbChunkWritten;
            }

            Assert(cbToRead >= cbRead);
            cbToRead       -= cbRead;
            cbWrittenTotal += cbRead;
        }

#ifndef RT_OS_L4
        /* Update read pointer. */
        if (pStrm->fMMIO)
            pStrm->old_optr = cntinfo.ptr;
#endif

    } while(0);

    if (RT_SUCCESS(rc))
    {
        uint32_t cWrittenTotal = AUDIOMIXBUF_B2S(&pStream->MixBuf, cbWrittenTotal);
        if (cWrittenTotal)
            AudioMixBufFinish(&pStream->MixBuf, cWrittenTotal);

        if (pcbWritten)
            *pcbWritten = cbWrittenTotal;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
static DECLCALLBACK(void) drvHostOSSAudioShutdown(PPDMIHOSTAUDIO pInterface)
{
    RT_NOREF(pInterface);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostOSSAudioGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);
    RT_NOREF(enmDir);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream,
                                                     PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = ossCreateStreamIn(pStream, pCfgReq, pCfgAcq);
    else
        rc = ossCreateStreamOut(pStream, pCfgReq, pCfgAcq);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    int rc;
    if (pStream->enmDir == PDMAUDIODIR_IN)
        rc = ossDestroyStreamIn(pStream);
    else
        rc = ossDestroyStreamOut(pStream);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamControl(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream,
                                                      PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    Assert(pStream->enmCtx == PDMAUDIOSTREAMCTX_HOST);

    int rc;
    if (pStream->enmDir == PDMAUDIODIR_IN)
        rc = ossControlStreamIn(/*pInterface,  pStream, enmStreamCmd*/);
    else
        rc = ossControlStreamOut(pStream, enmStreamCmd);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvHostOSSAudioStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    /* Nothing to do here for OSS. */
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTRMSTS) drvHostOSSAudioStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAM pStream)
{
    RT_NOREF(pInterface);
    RT_NOREF(pStream);

    PDMAUDIOSTRMSTS strmSts =   PDMAUDIOSTRMSTS_FLAG_INITIALIZED
                              | PDMAUDIOSTRMSTS_FLAG_ENABLED;

    strmSts |=   pStream->enmDir == PDMAUDIODIR_IN
               ? PDMAUDIOSTRMSTS_FLAG_DATA_READABLE
               : PDMAUDIOSTRMSTS_FLAG_DATA_WRITABLE;

    return strmSts;
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostOSSAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS       pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTOSSAUDIO pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTOSSAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);

    return NULL;
}

/**
 * Constructs an OSS audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostOSSAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg, fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTOSSAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTOSSAUDIO);
    LogRel(("Audio: Initializing OSS driver\n"));

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostOSSAudioQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostOSSAudio);

    return VINF_SUCCESS;
}

/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostOSSAudio =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "OSSAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "OSS audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTOSSAUDIO),
    /* pfnConstruct */
    drvHostOSSAudioConstruct,
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

