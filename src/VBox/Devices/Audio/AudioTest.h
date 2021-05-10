/* $Id$ */
/** @file
 * Audio testing routines.
 * Common code which is being used by the ValidationKit audio test (VKAT)
 * and the debug / ValdikationKit audio driver(s).
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

#ifndef VBOX_INCLUDED_SRC_Audio_AudioTest_h
#define VBOX_INCLUDED_SRC_Audio_AudioTest_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#define AUDIOTEST_PATH_PREFIX_STR "audio-test-"

/**
 * Structure for handling an audio (sine wave) test tone.
 */
typedef struct AUDIOTESTTONE
{
    /** The PCM properties. */
    PDMAUDIOPCMPROPS Props;
    /** Current sample index for generate the sine wave. */
    uint64_t         uSample;
    /** The fixed portion of the sin() input. */
    double           rdFixed;
    /** Frequency (in Hz) of the sine wave to generate. */
    double           rdFreqHz;
} AUDIOTESTTONE;
/** Pointer to an audio test tone. */
typedef AUDIOTESTTONE *PAUDIOTESTTONE;

/**
 * Structure for handling audio test tone parameters.
 */
typedef struct AUDIOTESTTONEPARMS
{
    /** The PCM properties. */
    PDMAUDIOPCMPROPS Props;
    /** Prequel (in ms) to play silence. Optional and can be set to 0. */
    RTMSINTERVAL     msPrequel;
    /** Duration (in ms) to play the test tone. */
    RTMSINTERVAL     msDuration;
    /** Sequel (in ms) to play silence. Optional and can be set to 0. */
    RTMSINTERVAL     msSequel;
    /** Volume (in percent, 0-100) to use.
     *  If set to 0, the tone is muted (i.e. silent). */
    uint8_t          uVolumePercent;
} AUDIOTESTTONEPARMS;
/** Pointer to audio test tone parameters. */
typedef AUDIOTESTTONEPARMS *PAUDIOTESTTONEPARMS;

typedef struct AUDIOTESTSET
{
    /** Absolute path where to store the test audio data.
     *  If NULL, no test audio data will be written. */
    char             szPathOutAbs[RTPATH_MAX];

} AUDIOTESTSET;
/** Pointer to audio test set parameters. */
typedef AUDIOTESTSET *PAUDIOTESTSET;


double AudioTestToneInitRandom(PAUDIOTESTTONE pTone, PPDMAUDIOPCMPROPS pProps);
int    AudioTestToneWrite(PAUDIOTESTTONE pTone, void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten);

int    AudioTestToneParamsInitRandom(PAUDIOTESTTONEPARMS pToneParams, PPDMAUDIOPCMPROPS pProps);

int    AudioTestPathCreateTemp(char *pszPath, size_t cbPath, const char *pszUUID);
int    AudioTestPathCreate(char *pszPath, size_t cbPath, const char *pszUUID);

int    AudioTestSetCreate(PAUDIOTESTSET pSet, const char *pszPath, const char *pszTag);
void   AudioTestSetDestroy(PAUDIOTESTSET pSet);
int    AudioTestSetOpen(PAUDIOTESTSET pSet, const char *pszPath, const char *pszTag);
void   AudioTestSetClose(PAUDIOTESTSET pSet);
int    AudioTestSetPack(PAUDIOTESTSET pSet, const char *pszOutDir);
int    AudioTestSetUnpack(const char *pszFile, const char *pszOutDir);
int    AudioTestSetVerify(PAUDIOTESTSET pSet, const char *pszTag);

#endif /* !VBOX_INCLUDED_SRC_Audio_AudioTest_h */

