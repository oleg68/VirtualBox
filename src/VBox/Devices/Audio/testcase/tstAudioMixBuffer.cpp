/* $Id$ */
/** @file
 * Audio testcase - Mixing buffer.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>


#include "../AudioMixBuffer.h"
#include "../DrvAudio.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

static int tstSingle(RTTEST hTest)
{
    RTTestSubF(hTest, "Single buffer");

    PDMAUDIOSTREAMCFG config =
    {
        "44100Hz, 2 Channels, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
          2,                                                                  /* Channels */
          44100,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&config));

    uint32_t cBufSize = _1K;

    /*
     * General stuff.
     */
    PDMAUDIOMIXBUF mb;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&mb, "Single", &config.Props, cBufSize));
    RTTESTI_CHECK(AudioMixBufSize(&mb) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_B2S(&mb, AudioMixBufSizeBytes(&mb)) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_S2B(&mb, AudioMixBufSize(&mb)) == AudioMixBufSizeBytes(&mb));
    RTTESTI_CHECK(AudioMixBufFree(&mb) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_S2B(&mb, AudioMixBufFree(&mb)) == AudioMixBufFreeBytes(&mb));

    /*
     * Absolute writes.
     */
    uint32_t cSamplesRead  = 0, cSamplesWritten = 0, cSamplesWrittenAbs = 0;
    int8_t  samples8 [2] = { 0x12, 0x34 };
    int16_t samples16[2] = { 0xAA, 0xBB };
    int32_t samples32[2] = { 0xCC, 0xDD };
    /* int64_t samples64[2] = { 0xEE, 0xFF }; - unused */

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 0, &samples8, sizeof(samples8), &cSamplesWritten));
    RTTESTI_CHECK(cSamplesWritten == 0 /* Samples */);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 0, &samples16, sizeof(samples16), &cSamplesWritten));
    RTTESTI_CHECK(cSamplesWritten == 1 /* Samples */);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteAt(&mb, 2, &samples32, sizeof(samples32), &cSamplesWritten));
    RTTESTI_CHECK(cSamplesWritten == 2 /* Samples */);
    cSamplesWrittenAbs = 0;

    /* Beyond buffer. */
    RTTESTI_CHECK_RC(AudioMixBufWriteAt(&mb, AudioMixBufSize(&mb) + 1, &samples16, sizeof(samples16),
                                        &cSamplesWritten), VINF_BUFFER_OVERFLOW);
    /** @todo (bird): this was checking for VERR_BUFFER_OVERFLOW, which do you want
     *        the function to actually return? */

    /*
     * Circular writes.
     */
    uint32_t cToWrite = AudioMixBufSize(&mb) - cSamplesWrittenAbs - 1; /* -1 as padding plus -2 samples for above. */
    for (uint32_t i = 0; i < cToWrite; i++)
    {
        RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&mb, &samples16, sizeof(samples16), &cSamplesWritten));
        RTTESTI_CHECK(cSamplesWritten == 1);
    }
    RTTESTI_CHECK(!AudioMixBufIsEmpty(&mb));
    RTTESTI_CHECK(AudioMixBufFree(&mb) == 1);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, 1U));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cToWrite + cSamplesWrittenAbs /* + last absolute write */);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&mb, &samples16, sizeof(samples16), &cSamplesWritten));
    RTTESTI_CHECK(cSamplesWritten == 1);
    RTTESTI_CHECK(AudioMixBufFree(&mb) == 0);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, 0U));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cBufSize);

    /* Circular reads. */
    uint32_t cToRead = AudioMixBufSize(&mb) - cSamplesWrittenAbs - 1;
    for (uint32_t i = 0; i < cToWrite; i++)
    {
        RTTESTI_CHECK_RC_OK(AudioMixBufReadCirc(&mb, &samples16, sizeof(samples16), &cSamplesRead));
        RTTESTI_CHECK(cSamplesRead == 1);
        AudioMixBufFinish(&mb, cSamplesRead);
    }
    RTTESTI_CHECK(!AudioMixBufIsEmpty(&mb));
    RTTESTI_CHECK(AudioMixBufFree(&mb) == AudioMixBufSize(&mb) - cSamplesWrittenAbs - 1);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, cBufSize - cSamplesWrittenAbs - 1));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cBufSize - cToRead + cSamplesWrittenAbs);

    RTTESTI_CHECK_RC_OK(AudioMixBufReadCirc(&mb, &samples16, sizeof(samples16), &cSamplesRead));
    RTTESTI_CHECK(cSamplesRead == 1);
    AudioMixBufFinish(&mb, cSamplesRead);
    RTTESTI_CHECK(AudioMixBufFree(&mb) == cBufSize - cSamplesWrittenAbs);
    RTTESTI_CHECK(AudioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, cBufSize - cSamplesWrittenAbs));
    RTTESTI_CHECK(AudioMixBufUsed(&mb) == cSamplesWrittenAbs);

    AudioMixBufDestroy(&mb);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

static int tstParentChild(RTTEST hTest)
{
    uint32_t cSamples = 16;
    uint32_t cBufSize = RTRandU32Ex(cSamples /* Min */, 256 /* Max */);

    PDMAUDIOSTREAMCFG cfg_p =
    {
        "44100Hz, 2 Channels, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
          2,                                                                  /* Channels */
          44100,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_p));

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg_p.Props, cBufSize));

    PDMAUDIOSTREAMCFG cfg_c1 = /* Upmixing to parent */
    {
        "22050Hz, 2 Channels, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
          2,                                                                  /* Channels */
          22050,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_c1));

    PDMAUDIOMIXBUF child1;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child1, "Child1", &cfg_c1.Props, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child1, &parent));

    PDMAUDIOSTREAMCFG cfg_c2 = /* Downmixing to parent */
    {
        "48000Hz, 2 Channels, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
          2,                                                                  /* Channels */
          48000,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_c2));

    PDMAUDIOMIXBUF child2;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child2, "Child2", &cfg_c2.Props, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child2, &parent));

    /*
     * Writing + mixing from child/children -> parent, sequential.
     */
    uint32_t cbBuf = _1K;
    char pvBuf[_1K];
    int16_t samples[32] = { 0xAA, 0xBB };
    uint32_t cSamplesRead, cSamplesWritten, cSamplesMixed;

    uint32_t cSamplesChild1  = cSamples;
    uint32_t cSamplesChild2  = cSamples;

    uint32_t t = RTRandU32() % 1024;

    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "%RU32 iterations total\n", t);

    /*
     * Using AudioMixBufWriteAt for writing to children.
     */
    RTTestSubF(hTest, "2 Children -> Parent (AudioMixBufWriteAt)");

    for (uint32_t i = 0; i < t; i++)
    {
        RTTestPrintf(hTest, RTTESTLVL_DEBUG, "i=%RU32\n", i);
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufWriteAt(&child1, 0, &samples, sizeof(samples), &cSamplesWritten));
        RTTESTI_CHECK_MSG_BREAK(cSamplesWritten == cSamplesChild1, ("Child1: Expected %RU32 written samples, got %RU32\n", cSamplesChild1, cSamplesWritten));
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufMixToParent(&child1, cSamplesWritten, &cSamplesMixed));
        RTTESTI_CHECK_MSG_BREAK(AudioMixBufLive(&child1) == cSamplesMixed, ("Child1: Expected %RU32 mixed samples, got %RU32\n", AudioMixBufLive(&child1), cSamplesMixed));
        RTTESTI_CHECK_MSG_BREAK(AudioMixBufUsed(&child1) == AUDIOMIXBUF_S2S_RATIO(&parent, cSamplesMixed), ("Child1: Expected %RU32 used samples, got %RU32\n", AudioMixBufLive(&child1), AUDIOMIXBUF_S2S_RATIO(&parent, cSamplesMixed)));
        RTTESTI_CHECK_MSG_BREAK(AudioMixBufUsed(&parent) == 0, ("Parent: Expected 0 used samples, got %RU32\n", AudioMixBufUsed(&parent)));

        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufWriteAt(&child2, 0, &samples, sizeof(samples), &cSamplesWritten));
        RTTESTI_CHECK_MSG_BREAK(cSamplesWritten == cSamplesChild2, ("Child2: Expected %RU32 written samples, got %RU32\n", cSamplesChild2, cSamplesWritten));
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufMixToParent(&child2, cSamplesWritten, &cSamplesMixed));
        RTTESTI_CHECK_MSG_BREAK(AudioMixBufLive(&child2) == cSamplesMixed, ("Child2: Expected %RU32 mixed samples, got %RU32\n", AudioMixBufLive(&child2), AudioMixBufUsed(&parent)));
        RTTESTI_CHECK_MSG_BREAK(AudioMixBufUsed(&child2) == AUDIOMIXBUF_S2S_RATIO(&parent, cSamplesMixed), ("Child2: Expected %RU32 used samples, got %RU32\n", AudioMixBufLive(&child2), AUDIOMIXBUF_S2S_RATIO(&parent, cSamplesMixed)));
        RTTESTI_CHECK_MSG_BREAK(AudioMixBufUsed(&parent) == 0, ("Parent2: Expected 0 used samples, got %RU32\n", AudioMixBufUsed(&parent)));
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == AudioMixBufLive(&child1) + AudioMixBufLive(&child2));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufReadCirc(&parent, pvBuf, cbBuf, &cSamplesRead));
        if (!cSamplesRead)
            break;
        AudioMixBufFinish(&parent, cSamplesRead);
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child1) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child2) == 0);

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child1);
    AudioMixBufDestroy(&child2);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

/* Test 8-bit sample conversion (8-bit -> internal -> 8-bit). */
static int tstConversion8(RTTEST hTest)
{
    unsigned         i;
    uint32_t         cBufSize = 256;

    RTTestSubF(hTest, "Sample conversion (U8)");

    PDMAUDIOSTREAMCFG cfg_p =
    {
        "44100Hz, 1 Channel, U8",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 8,                                                                 /* Bits */
          false,                                                             /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(8 /* Bits */, 1 /* Channels */), /* Shift */
          1,                                                                 /* Channels */
          44100,                                                             /* Hz */
          false                                                              /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_p));

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg_p.Props, cBufSize));

    /* Child uses half the sample rate; that ensures the mixing engine can't
     * take shortcuts and performs conversion. Because conversion to double
     * the sample rate effectively inserts one additional sample between every
     * two source samples, N source samples will be converted to N * 2 - 1
     * samples. However, the last source sample will be saved for later
     * interpolation and not immediately output.
     */
    PDMAUDIOSTREAMCFG cfg_c =   /* Upmixing to parent */
    {
        "22050Hz, 1 Channel, U8",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 8,                                                                 /* Bits */
          false,                                                             /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(8 /* Bits */, 1 /* Channels */), /* Shift */
          1,                                                                 /* Channels */
          22050,                                                             /* Hz */
          false                                                              /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_c));

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child, "Child", &cfg_c.Props, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child, &parent));

    /* 8-bit unsigned samples. Often used with SB16 device. */
    uint8_t samples[16]  = { 0xAA, 0xBB, 0, 1, 43, 125, 126, 127,
                             128, 129, 130, 131, 132, UINT8_MAX - 1, UINT8_MAX, 0 };

    /*
     * Writing + mixing from child -> parent, sequential.
     */
    uint32_t    cbBuf = 256;
    char        achBuf[256];
    uint32_t    cSamplesRead, cSamplesWritten, cSamplesMixed;

    uint32_t cSamplesChild  = 16;
    uint32_t cSamplesParent = cSamplesChild * 2 - 2;
    uint32_t cSamplesTotalRead   = 0;

    /**** 8-bit unsigned samples ****/
    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Conversion test %uHz %uch 8-bit\n", cfg_c.Props.uHz, cfg_c.Props.cChannels);
    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &samples, sizeof(samples), &cSamplesWritten));
    RTTESTI_CHECK_MSG(cSamplesWritten == cSamplesChild, ("Child: Expected %RU32 written samples, got %RU32\n", cSamplesChild, cSamplesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cSamplesWritten, &cSamplesMixed));
    uint32_t cSamples = AudioMixBufUsed(&parent);
    RTTESTI_CHECK_MSG(AudioMixBufLive(&child) == cSamples, ("Child: Expected %RU32 mixed samples, got %RU32\n", AudioMixBufLive(&child), cSamples));

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == AudioMixBufLive(&child));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufReadCirc(&parent, achBuf, cbBuf, &cSamplesRead));
        if (!cSamplesRead)
            break;
        cSamplesTotalRead += cSamplesRead;
        AudioMixBufFinish(&parent, cSamplesRead);
    }

    RTTESTI_CHECK_MSG(cSamplesTotalRead == cSamplesParent, ("Parent: Expected %RU32 mixed samples, got %RU32\n", cSamplesParent, cSamplesTotalRead));

    /* Check that the samples came out unharmed. Every other sample is interpolated and we ignore it. */
    /* NB: This also checks that the default volume setting is 0dB attenuation. */
    uint8_t *pSrc8 = &samples[0];
    uint8_t *pDst8 = (uint8_t *)achBuf;

    for (i = 0; i < cSamplesChild - 1; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc8 == *pDst8, ("index %u: Dst=%d, Src=%d\n", i, *pDst8, *pSrc8));
        pSrc8 += 1;
        pDst8 += 2;
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child)  == 0);

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

/* Test 16-bit sample conversion (16-bit -> internal -> 16-bit). */
static int tstConversion16(RTTEST hTest)
{
    unsigned         i;
    uint32_t         cBufSize = 256;

    RTTestSubF(hTest, "Sample conversion (S16)");

    PDMAUDIOSTREAMCFG cfg_p =
    {
        "44100Hz, 1 Channel, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 1 /* Channels */), /* Shift */
          1,                                                                  /* Channels */
          44100,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_p));

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg_p.Props, cBufSize));

    PDMAUDIOSTREAMCFG cfg_c =   /* Upmixing to parent */
    {
        "22050Hz, 1 Channel, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 1 /* Channels */), /* Shift */
          1,                                                                  /* Channels */
          22050,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg_c));

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child, "Child", &cfg_c.Props, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child, &parent));

    /* 16-bit signed. More or less exclusively used as output, and usually as input, too. */
    int16_t     samples[16] = { 0xAA, 0xBB, INT16_MIN, INT16_MIN + 1, INT16_MIN / 2, -3, -2, -1,
                                0, 1, 2, 3, INT16_MAX / 2, INT16_MAX - 1, INT16_MAX, 0 };

    /*
     * Writing + mixing from child -> parent, sequential.
     */
    uint32_t    cbBuf = 256;
    char        achBuf[256];
    uint32_t    cSamplesRead, cSamplesWritten, cSamplesMixed;

    uint32_t cSamplesChild  = 16;
    uint32_t cSamplesParent = cSamplesChild * 2 - 2;
    uint32_t cSamplesTotalRead   = 0;

    /**** 16-bit signed samples ****/
    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Conversion test %uHz %uch 16-bit\n", cfg_c.Props.uHz, cfg_c.Props.cChannels);
    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &samples, sizeof(samples), &cSamplesWritten));
    RTTESTI_CHECK_MSG(cSamplesWritten == cSamplesChild, ("Child: Expected %RU32 written samples, got %RU32\n", cSamplesChild, cSamplesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cSamplesWritten, &cSamplesMixed));
    uint32_t cSamples = AudioMixBufUsed(&parent);
    RTTESTI_CHECK_MSG(AudioMixBufLive(&child) == cSamples, ("Child: Expected %RU32 mixed samples, got %RU32\n", AudioMixBufLive(&child), cSamples));

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == AudioMixBufLive(&child));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufReadCirc(&parent, achBuf, cbBuf, &cSamplesRead));
        if (!cSamplesRead)
            break;
        cSamplesTotalRead += cSamplesRead;
        AudioMixBufFinish(&parent, cSamplesRead);
    }
    RTTESTI_CHECK_MSG(cSamplesTotalRead == cSamplesParent, ("Parent: Expected %RU32 mixed samples, got %RU32\n", cSamplesParent, cSamplesTotalRead));

    /* Check that the samples came out unharmed. Every other sample is interpolated and we ignore it. */
    /* NB: This also checks that the default volume setting is 0dB attenuation. */
    int16_t *pSrc16 = &samples[0];
    int16_t *pDst16 = (int16_t *)achBuf;

    for (i = 0; i < cSamplesChild - 1; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc16 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        pSrc16 += 1;
        pDst16 += 2;
    }

    RTTESTI_CHECK(AudioMixBufUsed(&parent) == 0);
    RTTESTI_CHECK(AudioMixBufLive(&child)  == 0);

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

/* Test volume control. */
static int tstVolume(RTTEST hTest)
{
    unsigned         i;
    uint32_t         cBufSize = 256;

    RTTestSubF(hTest, "Volume control");

    /* Same for parent/child. */
    PDMAUDIOSTREAMCFG cfg =
    {
        "44100Hz, 2 Channels, S16",
        PDMAUDIODIR_OUT,
        { PDMAUDIOPLAYBACKDEST_UNKNOWN },
        { 16,                                                                 /* Bits */
          true,                                                               /* Signed */
          PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(16 /* Bits */, 2 /* Channels */), /* Shift */
          2,                                                                  /* Channels */
          44100,                                                              /* Hz */
          false                                                               /* Swap Endian */ },
        0 /* cSampleBufferSize */
    };

    RTTESTI_CHECK(DrvAudioHlpStreamCfgIsValid(&cfg));

    PDMAUDIOVOLUME vol = { false, 0, 0 };   /* Not muted. */
    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&parent, "Parent", &cfg.Props, cBufSize));

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(AudioMixBufInit(&child, "Child", &cfg.Props, cBufSize));
    RTTESTI_CHECK_RC_OK(AudioMixBufLinkTo(&child, &parent));

    /* A few 16-bit signed samples. */
    int16_t     samples[16] = { INT16_MIN, INT16_MIN + 1, -128, -64, -4, -1, 0, 1,
                                2, 255, 256, INT16_MAX / 2, INT16_MAX - 2, INT16_MAX - 1, INT16_MAX, 0 };

    /*
     * Writing + mixing from child -> parent.
     */
    uint32_t    cbBuf = 256;
    char        achBuf[256];
    uint32_t    cSamplesRead, cSamplesWritten, cSamplesMixed;

    uint32_t cSamplesChild  = 8;
    uint32_t cSamplesParent = cSamplesChild;
    uint32_t cSamplesTotalRead;
    int16_t *pSrc16;
    int16_t *pDst16;

    /**** Volume control test ****/
    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Volume control test %uHz %uch \n", cfg.Props.uHz, cfg.Props.cChannels);

    /* 1) Full volume/0dB attenuation (255). */
    vol.uLeft = vol.uRight = 255;
    AudioMixBufSetVolume(&child, &vol);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &samples, sizeof(samples), &cSamplesWritten));
    RTTESTI_CHECK_MSG(cSamplesWritten == cSamplesChild, ("Child: Expected %RU32 written samples, got %RU32\n", cSamplesChild, cSamplesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cSamplesWritten, &cSamplesMixed));

    cSamplesTotalRead = 0;
    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufReadCirc(&parent, achBuf, cbBuf, &cSamplesRead));
        if (!cSamplesRead)
            break;
        cSamplesTotalRead += cSamplesRead;
        AudioMixBufFinish(&parent, cSamplesRead);
    }
    RTTESTI_CHECK_MSG(cSamplesTotalRead == cSamplesParent, ("Parent: Expected %RU32 mixed samples, got %RU32\n", cSamplesParent, cSamplesTotalRead));

    /* Check that at 0dB the samples came out unharmed. */
    pSrc16 = &samples[0];
    pDst16 = (int16_t *)achBuf;

    for (i = 0; i < cSamplesParent * 2 /* stereo */; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc16 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        ++pSrc16;
        ++pDst16;
    }
    AudioMixBufReset(&child);

    /* 2) Half volume/-6dB attenuation (16 steps down). */
    vol.uLeft = vol.uRight = 255 - 16;
    AudioMixBufSetVolume(&child, &vol);

    RTTESTI_CHECK_RC_OK(AudioMixBufWriteCirc(&child, &samples, sizeof(samples), &cSamplesWritten));
    RTTESTI_CHECK_MSG(cSamplesWritten == cSamplesChild, ("Child: Expected %RU32 written samples, got %RU32\n", cSamplesChild, cSamplesWritten));
    RTTESTI_CHECK_RC_OK(AudioMixBufMixToParent(&child, cSamplesWritten, &cSamplesMixed));

    cSamplesTotalRead = 0;
    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(AudioMixBufReadCirc(&parent, achBuf, cbBuf, &cSamplesRead));
        if (!cSamplesRead)
            break;
        cSamplesTotalRead += cSamplesRead;
        AudioMixBufFinish(&parent, cSamplesRead);
    }
    RTTESTI_CHECK_MSG(cSamplesTotalRead == cSamplesParent, ("Parent: Expected %RU32 mixed samples, got %RU32\n", cSamplesParent, cSamplesTotalRead));

    /* Check that at -6dB the sample values are halved. */
    pSrc16 = &samples[0];
    pDst16 = (int16_t *)achBuf;

    for (i = 0; i < cSamplesParent * 2 /* stereo */; ++i)
    {
        /* Watch out! For negative values, x >> 1 is not the same as x / 2. */
        RTTESTI_CHECK_MSG(*pSrc16 >> 1 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        ++pSrc16;
        ++pDst16;
    }

    AudioMixBufDestroy(&parent);
    AudioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    /*
     * Initialize IPRT and create the test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstAudioMixBuffer", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    rc = tstSingle(hTest);
    if (RT_SUCCESS(rc))
        rc = tstParentChild(hTest);
    if (RT_SUCCESS(rc))
        rc = tstConversion8(hTest);
    if (RT_SUCCESS(rc))
        rc = tstConversion16(hTest);
    if (RT_SUCCESS(rc))
        rc = tstVolume(hTest);

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}
