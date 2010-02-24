/** @file
 * IPRT - Polling I/O Handles.
 */

/*
 * Copyright (C) 2010 Sun Microsystems, Inc.
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
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___iprt_poll_h
#define ___iprt_poll_h

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_poll      RTPoll - Polling I/O Handles
 * @ingroup grp_rt
 * @{
 */

/** @name Poll events
 * @{ */
/** Readable without blocking. */
#define RTPOLL_EVT_READ     RT_BIT_32(0)
/** Writable without blocking. */
#define RTPOLL_EVT_WRITE    RT_BIT_32(1)
/** Error condition, hangup, exception or similar. */
#define RTPOLL_EVT_ERROR    RT_BIT_32(2)
/** @} */

/**
 * Polls on the specified poll set until an event occures on one of the handles
 * or the timeout expires.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if an event occured on a handle.  Note that these
 * @retval  VERR_INVALID_HANDLE if @a hPollSet is invalid.
 * @retval  VERR_TIMEOUT if @a cMillies ellapsed without any events.
 * @retval  VERR_DEADLOCK if @a cMillies is set to RT_INDEFINITE_WAIT and there
 *          are no valid handles in the set.
 *
 * @param   hPollSet            The set to poll on.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait for ever.
 * @param   pfEvents            Where to return details about the events that
 *                              occured.  Optional.
 * @param   pid                 Where to return the ID associated with the
 *                              handle when calling RTPollSetAdd.  Optional.
 *
 * @sa      RTPollNoResume
 */
RTDECL(int) RTPoll(RTPOLLSET hPollSet, RTMSINTERVAL cMillies, uint32_t *pfEvents, uint32_t *pid);

/**
 * Same as RTPoll except that it will return when interrupted.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if an event occured on a handle.  Note that these
 * @retval  VERR_INVALID_HANDLE if @a hPollSet is invalid.
 * @retval  VERR_TIMEOUT if @a cMillies ellapsed without any events.
 * @retval  VERR_DEADLOCK if @a cMillies is set to RT_INDEFINITE_WAIT and there
 *          are no valid handles in the set.
 * @retval  VERR_INTERRUPTED if a signal or other asynchronous event interrupted
 *          the polling.
 *
 * @param   hPollSet            The set to poll on.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait for ever.
 * @param   pfEvents            Where to return details about the events that
 *                              occured.  Optional.
 * @param   pid                 Where to return the ID associated with the
 *                              handle when calling RTPollSetAdd.  Optional.
 */
RTDECL(int) RTPollNoResume(RTPOLLSET hPollSet, RTMSINTERVAL cMillies, uint32_t *pfEvents, uint32_t *pid);

/**
 * Creates a poll set with no members.
 *
 * @returns IPRT status code.
 * @param   phPollSet           Where to return the poll set handle.
 */
RTDECL(int)  RTPollSetCreate(PRTPOLLSET hPollSet);

/**
 * Destroys a poll set.
 *
 * @returns IPRT status code.
 * @param   hPollSet            The poll set to destroy.  NIL_POLLSET is quietly
 *                              ignored (VINF_SUCCESS).
 */
RTDECL(int)  RTPollSetDestroy(RTPOLLSET hPollSet);

/**
 * Adds a generic handle to the poll set.
 *
 * @returns IPRT status code
 * @param   hPollSet            The poll set to modify.
 * @param   pHandle             The handle to add.
 * @param   fEvents             Which events to poll for.
 * @param   id                  The handle ID.
 */
RTDECL(int) RTPollSetAdd(RTPOLLSET hPollSet, PCRTHANDLE pHandle, uint32_t fEvents, uint32_t id);

/**
 * Removes a generic handle from the poll set.
 *
 * @returns IPRT status code
 * @param   hPollSet            The poll set to modify.
 * @param   id                  The handle ID of the handle that should be
 *                              removed.
 */
RTDECL(int) RTPollSetRemove(RTPOLLSET hPollSet, uint32_t id);


/**
 * Query a handle in the poll set by it's ID.
 *
 * @returns IPRT status code
 * @retval  VINF_SUCCESS if the handle was found.  @a *pHandle is set.
 * @retval  VERR_NOT_FOUND if there is no handle with that ID.
 * @retval  VERR_INVALID_HANDLE if @a hPollSet is invalid.
 *
 * @param   hPollSet            The poll set to query.
 * @param   id                  The ID of the handle.
 * @param   pHandle             Where to return the handle details.  Optional.
 */
RTDECL(int) RTPollSetQueryHandle(RTPOLLSET hPollSet, uint32_t id, PRTHANDLE pHandle);

/**
 * Gets the number of handles in the set.
 *
 * @retval  The handle count.
 * @retval  UINT32_MAX if @a hPollSet is invalid.
 *
 * @param   hPollSet            The poll set.
 */
RTDECL(uint32_t) RTPollSetCount(RTPOLLSET hPollSet);

/**
 * Adds a pipe handle to the set.
 *
 * @returns IPRT status code.
 * @param   hPollSet            The poll set.
 * @param   hPipe               The pipe handle.
 * @param   fEvents             Which events to poll for.
 * @param   id                  The handle ID.
 *
 * @todo    Maybe we could figure out what to poll for depending on the kind of
 *          pipe we're dealing with.
 */
DECLINLINE(int) RTPollSetAddPipe(RTPOLLSET hPollSet, RTPIPE hPipe, uint32_t fEvents, uint32_t id)
{
    RTHANDLE Handle;
    Handle.enmType = RTHANDLETYPE_PIPE;
    Handle.u.hPipe = hPipe;
    return RTPollSetAdd(hPollSet, &Handle, fEvents, id);
}

/**
 * Adds a socket handle to the set.
 *
 * @returns IPRT status code.
 * @param   hPollSet            The poll set.
 * @param   hSocket             The socket handle.
 * @param   fEvents             Which events to poll for.
 * @param   id                  The handle ID.
 */
DECLINLINE(int) RTPollSetAddSocket(RTPOLLSET hPollSet, RTSOCKET hSocket, uint32_t fEvents, uint32_t id)
{
    RTHANDLE Handle;
    Handle.enmType   = RTHANDLETYPE_SOCKET;
    Handle.u.hSocket = hSocket;
    return RTPollSetAdd(hPollSet, &Handle, fEvents, id);
}

/** @} */

RT_C_DECLS_END

#endif

