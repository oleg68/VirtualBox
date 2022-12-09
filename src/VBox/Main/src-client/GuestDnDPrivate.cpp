/* $Id$ */
/** @file
 * Private guest drag and drop code, used by GuestDnDTarget + GuestDnDSource.
 */

/*
 * Copyright (C) 2011-2022 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#define LOG_GROUP LOG_GROUP_GUEST_DND
#include "LoggingNew.h"

#include "GuestImpl.h"
#include "AutoCaller.h"

#ifdef VBOX_WITH_DRAG_AND_DROP
# include "ConsoleImpl.h"
# include "ProgressImpl.h"
# include "GuestDnDPrivate.h"

# include <algorithm>

# include <iprt/dir.h>
# include <iprt/path.h>
# include <iprt/stream.h>
# include <iprt/semaphore.h>
# include <iprt/cpp/utils.h>

# include <VMMDev.h>

# include <VBox/GuestHost/DragAndDrop.h>
# include <VBox/HostServices/DragAndDropSvc.h>
# include <VBox/version.h>

/** @page pg_main_dnd  Dungeons & Dragons - Overview
 * Overview:
 *
 * Drag and Drop is handled over the internal HGCM service for the host <->
 * guest communication. Beside that we need to map the Drag and Drop protocols
 * of the various OS's we support to our internal channels, this is also highly
 * communicative in both directions. Unfortunately HGCM isn't really designed
 * for that. Next we have to foul some of the components. This includes to
 * trick X11 on the guest side, but also Qt needs to be tricked on the host
 * side a little bit.
 *
 * The following components are involved:
 *
 * 1. GUI: Uses the Qt classes for Drag and Drop and mainly forward the content
 *    of it to the Main IGuest / IGuestDnDSource / IGuestDnDTarget interfaces.
 * 2. Main: Public interface for doing Drag and Drop. Also manage the IProgress
 *    interfaces for blocking the caller by showing a progress dialog (see
 *    this file).
 * 3. HGCM service: Handle all messages from the host to the guest at once and
 *    encapsulate the internal communication details (see dndmanager.cpp and
 *    friends).
 * 4. Guest Additions: Split into the platform neutral part (see
 *    VBoxGuestR3LibDragAndDrop.cpp) and the guest OS specific parts.
 *    Receive/send message from/to the HGCM service and does all guest specific
 *    operations. For Windows guests VBoxTray is in charge, whereas on UNIX-y guests
 *    VBoxClient will be used.
 *
 * Terminology:
 *
 * All transfers contain a MIME format and according meta data. This meta data then can
 * be interpreted either as raw meta data or something else. When raw meta data is
 * being handled, this gets passed through to the destination (guest / host) without
 * modification. Other meta data (like URI lists) can and will be modified by the
 * receiving side before passing to OS. How and when modifications will be applied
 * depends on the MIME format.
 *
 * Host -> Guest:
 * 1. There are DnD Enter, Move, Leave events which are send exactly like this
 *    to the guest. The info includes the position, MIME types and allowed actions.
 *    The guest has to respond with an action it would accept, so the GUI could
 *    change the cursor accordingly.
 * 2. On drop, first a drop event is sent. If this is accepted a drop data
 *    event follows. This blocks the GUI and shows some progress indicator.
 *
 * Guest -> Host:
 * 1. The GUI is asking the guest if a DnD event is pending when the user moves
 *    the cursor out of the view window. If so, this returns the mimetypes and
 *    allowed actions.
 * (2. On every mouse move this is asked again, to make sure the DnD event is
 *     still valid.)
 * 3. On drop the host request the data from the guest. This blocks the GUI and
 *    shows some progress indicator.
 *
 * Implementation hints:
 * m_strSupportedFormats here in this file defines the allowed mime-types.
 * This is necessary because we need special handling for some of the
 * mime-types. E.g. for URI lists we need to transfer the actual dirs and
 * files. Text EOL may to be changed. Also unknown mime-types may need special
 * handling as well, which may lead to undefined behavior in the host/guest, if
 * not done.
 *
 * Dropping of a directory, means recursively transferring _all_ the content.
 *
 * Directories and files are placed into the user's temporary directory on the
 * guest (e.g. /tmp/VirtualBox Dropped Files). We can't delete them after the
 * DnD operation, because we didn't know what the DnD target does with it. E.g.
 * it could just be opened in place. This could lead ofc to filling up the disk
 * within the guest. To inform the user about this, a small app could be
 * developed which scans this directory regularly and inform the user with a
 * tray icon hint (and maybe the possibility to clean this up instantly). The
 * same has to be done in the G->H direction when it is implemented.
 *
 * Only regular files are supported; symlinks are not allowed.
 *
 * Transfers currently are an all-succeed or all-fail operation (see todos).
 *
 * On MacOS hosts we had to implement own DnD "promises" support for file transfers,
 * as Qt does not support this out-of-the-box.
 *
 * The code tries to preserve the file modes of the transfered directories / files.
 * This is useful (and maybe necessary) for two things:
 * 1. If a file is executable, it should be also after the transfer, so the
 *    user can just execute it, without manually tweaking the modes first.
 * 2. If a dir/file is not accessible by group/others in the host, it shouldn't
 *    be in the guest.
 * In any case, the user mode is always set to rwx (so that we can access it
 * ourself, in e.g. for a cleanup case after cancel).
 *
 * ACEs / ACLs currently are not supported.
 *
 * Cancelling ongoing transfers is supported in both directions by the guest
 * and/or host side and cleans up all previous steps. This also involves
 * removing partially transferred directories / files in the temporary directory.
 *
 ** @todo
 * - ESC doesn't really work (on Windows guests it's already implemented)
 *   ... in any case it seems a little bit difficult to handle from the Qt side.
 * - Transfers currently do not have any interactive (UI) callbacks / hooks which
 *   e.g. would allow to skip / replace / rename and entry, or abort the operation on failure.
 * - Add support for more MIME types (especially images, csv)
 * - Test unusual behavior:
 *   - DnD service crash in the guest during a DnD op (e.g. crash of VBoxClient or X11)
 *   - Not expected order of the events between HGCM and the guest
 * - Security considerations: We transfer a lot of memory between the guest and
 *   the host and even allow the creation of dirs/files. Maybe there should be
 *   limits introduced to preventing DoS attacks or filling up all the memory
 *   (both in the host and the guest).
 */


/*********************************************************************************************************************************
 * Internal macros.                                                                                                              *
 ********************************************************************************************************************************/

/** Tries locking the GuestDnD object and returns on failure. */
#define GUESTDND_LOCK() \
    { \
        int rcLock = RTCritSectEnter(&m_CritSect); \
        if (RT_FAILURE(rcLock)) \
            return rcLock; \
    }

/** Tries locking the GuestDnD object and returns a_Ret failure. */
#define GUESTDND_LOCK_RET(a_Ret) \
    { \
        int rcLock = RTCritSectEnter(&m_CritSect); \
        if (RT_FAILURE(rcLock)) \
            return a_Ret; \
    }

/** Unlocks a formerly locked GuestDnD object. */
#define GUESTDND_UNLOCK() \
    { \
        int rcUnlock = RTCritSectLeave(&m_CritSect); RT_NOREF(rcUnlock); \
        AssertRC(rcUnlock); \
    }

/*********************************************************************************************************************************
 * GuestDnDSendCtx implementation.                                                                                               *
 ********************************************************************************************************************************/

GuestDnDSendCtx::GuestDnDSendCtx(void)
    : pTarget(NULL)
    , pState(NULL)
{
    reset();
}

/**
 * Resets a GuestDnDSendCtx object.
 */
void GuestDnDSendCtx::reset(void)
{
    if (pState)
        pState->reset();

    uScreenID  = 0;

    Transfer.reset();

    int rc2 = EventCallback.Reset();
    AssertRC(rc2);

    GuestDnDData::reset();
}

/*********************************************************************************************************************************
 * GuestDnDRecvCtx implementation.                                                                                               *
 ********************************************************************************************************************************/

GuestDnDRecvCtx::GuestDnDRecvCtx(void)
    : pSource(NULL)
    , pState(NULL)
{
    reset();
}

/**
 * Resets a GuestDnDRecvCtx object.
 */
void GuestDnDRecvCtx::reset(void)
{
    if (pState)
        pState->reset();

    lstFmtOffered.clear();
    strFmtReq  = "";
    strFmtRecv = "";
    enmAction  = 0;

    Transfer.reset();

    int rc2 = EventCallback.Reset();
    AssertRC(rc2);

    GuestDnDData::reset();
}

/*********************************************************************************************************************************
 * GuestDnDCallbackEvent implementation.                                                                                         *
 ********************************************************************************************************************************/

GuestDnDCallbackEvent::~GuestDnDCallbackEvent(void)
{
    if (NIL_RTSEMEVENT != m_SemEvent)
        RTSemEventDestroy(m_SemEvent);
}

/**
 * Resets a GuestDnDCallbackEvent object.
 */
int GuestDnDCallbackEvent::Reset(void)
{
    int rc = VINF_SUCCESS;

    if (NIL_RTSEMEVENT == m_SemEvent)
        rc = RTSemEventCreate(&m_SemEvent);

    m_Rc = VINF_SUCCESS;
    return rc;
}

/**
 * Completes a callback event by notifying the waiting side.
 *
 * @returns VBox status code.
 * @param   rc                  Result code to use for the event completion.
 */
int GuestDnDCallbackEvent::Notify(int rc /* = VINF_SUCCESS */)
{
    m_Rc = rc;
    return RTSemEventSignal(m_SemEvent);
}

/**
 * Waits on a callback event for being notified.
 *
 * @returns VBox status code.
 * @param   msTimeout           Timeout (in ms) to wait for callback event.
 */
int GuestDnDCallbackEvent::Wait(RTMSINTERVAL msTimeout)
{
    return RTSemEventWait(m_SemEvent, msTimeout);
}

/********************************************************************************************************************************
 *
 ********************************************************************************************************************************/

GuestDnDState::GuestDnDState(const ComObjPtr<Guest>& pGuest)
    : m_uProtocolVersion(0)
    , m_fGuestFeatures0(VBOX_DND_GF_NONE)
    , m_EventSem(NIL_RTSEMEVENT)
    , m_dndActionDefault(0)
    , m_dndLstActionsAllowed(0)
    , m_pParent(pGuest)
{
    int rc = RTSemEventCreate(&m_EventSem);
    if (RT_FAILURE(rc))
        throw rc;
}

GuestDnDState::~GuestDnDState(void)
{
    reset();

    int rc = RTSemEventDestroy(m_EventSem);
    AssertRC(rc);
}

/**
 * Notifies the waiting side about a guest notification response.
 */
int GuestDnDState::notifyAboutGuestResponse(void) const
{
    return RTSemEventSignal(m_EventSem);
}

/**
 * Resets a GuestDnDResponse object.
 */
void GuestDnDState::reset(void)
{
    LogFlowThisFuncEnter();

    m_dndActionDefault     = 0;
    m_dndLstActionsAllowed = 0;

    m_lstFormats.clear();
}

/**
 * Resets the progress object.
 *
 * @returns HRESULT
 * @param   pParent             Parent to set for the progress object.
 */
HRESULT GuestDnDState::resetProgress(const ComObjPtr<Guest>& pParent)
{
    m_pProgress.setNull();

    HRESULT hr = m_pProgress.createObject();
    if (SUCCEEDED(hr))
    {
        hr = m_pProgress->init(static_cast<IGuest *>(pParent),
                               Bstr(tr("Dropping data")).raw(),
                               TRUE /* aCancelable */);
    }

    return hr;
}

/**
 * Returns whether the progress object has been canceled or not.
 *
 * @returns \c true if canceled, \c false if not.
 */
bool GuestDnDState::isProgressCanceled(void) const
{
    BOOL fCanceled;
    if (!m_pProgress.isNull())
    {
        HRESULT hr = m_pProgress->COMGETTER(Canceled)(&fCanceled);
        AssertComRC(hr);
    }
    else
        fCanceled = TRUE;

    return RT_BOOL(fCanceled);
}

/**
 * Sets a callback for a specific HGCM message.
 *
 * @returns VBox status code.
 * @param   uMsg                HGCM message ID to set callback for.
 * @param   pfnCallback         Callback function pointer to use.
 * @param   pvUser              User-provided arguments for the callback function. Optional and can be NULL.
 */
int GuestDnDState::setCallback(uint32_t uMsg, PFNGUESTDNDCALLBACK pfnCallback, void *pvUser /* = NULL */)
{
    GuestDnDCallbackMap::iterator it = m_mapCallbacks.find(uMsg);

    /* Add. */
    if (pfnCallback)
    {
        if (it == m_mapCallbacks.end())
        {
            m_mapCallbacks[uMsg] = GuestDnDCallback(pfnCallback, uMsg, pvUser);
            return VINF_SUCCESS;
        }

        AssertMsgFailed(("Callback for message %RU32 already registered\n", uMsg));
        return VERR_ALREADY_EXISTS;
    }

    /* Remove. */
    if (it != m_mapCallbacks.end())
        m_mapCallbacks.erase(it);

    return VINF_SUCCESS;
}

/**
 * Sets the progress object to a new state.
 *
 * @returns VBox status code.
 * @param   uPercentage         Percentage (0-100) to set.
 * @param   uStatus             Status (of type DND_PROGRESS_XXX) to set.
 * @param   rcOp                IPRT-style result code to set. Optional.
 * @param   strMsg              Message to set. Optional.
 */
int GuestDnDState::setProgress(unsigned uPercentage, uint32_t uStatus,
                               int rcOp /* = VINF_SUCCESS */, const Utf8Str &strMsg /* = "" */)
{
    LogFlowFunc(("uPercentage=%u, uStatus=%RU32, , rcOp=%Rrc, strMsg=%s\n",
                 uPercentage, uStatus, rcOp, strMsg.c_str()));

    HRESULT hr = S_OK;

    if (m_pProgress.isNull())
        return VINF_SUCCESS;

    BOOL fCompleted = FALSE;
    hr = m_pProgress->COMGETTER(Completed)(&fCompleted);
    AssertComRCReturn(hr, VERR_COM_UNEXPECTED);

    BOOL fCanceled  = FALSE;
    hr = m_pProgress->COMGETTER(Canceled)(&fCanceled);
    AssertComRCReturn(hr, VERR_COM_UNEXPECTED);

    LogFlowFunc(("Progress fCompleted=%RTbool, fCanceled=%RTbool\n", fCompleted, fCanceled));

    int rc = VINF_SUCCESS;

    switch (uStatus)
    {
        case DragAndDropSvc::DND_PROGRESS_ERROR:
        {
            LogRel(("DnD: Guest reported error %Rrc\n", rcOp));

            if (!fCompleted)
                hr = m_pProgress->i_notifyComplete(VBOX_E_DND_ERROR,
                                                   COM_IIDOF(IGuest),
                                                   m_pParent->getComponentName(), strMsg.c_str());
            reset();
            break;
        }

        case DragAndDropSvc::DND_PROGRESS_CANCELLED:
        {
            LogRel2(("DnD: Guest cancelled operation\n"));

            if (!fCanceled)
            {
                hr = m_pProgress->Cancel();
                AssertComRC(hr);
            }

            if (!fCompleted)
            {
                hr = m_pProgress->i_notifyComplete(S_OK);
                AssertComRC(hr);
            }

            reset();
            break;
        }

        case DragAndDropSvc::DND_PROGRESS_RUNNING:
            RT_FALL_THROUGH();
        case DragAndDropSvc::DND_PROGRESS_COMPLETE:
        {
            LogRel2(("DnD: Guest reporting running/completion status with %u%%\n", uPercentage));

            if (   !fCompleted
                && !fCanceled)
            {
                hr = m_pProgress->SetCurrentOperationProgress(uPercentage);
                AssertComRCReturn(hr, VERR_COM_UNEXPECTED);
                if (   uStatus     == DragAndDropSvc::DND_PROGRESS_COMPLETE
                    || uPercentage >= 100)
                {
                    hr = m_pProgress->i_notifyComplete(S_OK);
                    AssertComRCReturn(hr, VERR_COM_UNEXPECTED);
                }
            }
            break;
        }

        default:
            break;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Dispatching function for handling the host service service callback.
 *
 * @returns VBox status code.
 * @param   u32Function         HGCM message ID to handle.
 * @param   pvParms             Pointer to optional data provided for a particular message. Optional.
 * @param   cbParms             Size (in bytes) of \a pvParms.
 */
int GuestDnDState::onDispatch(uint32_t u32Function, void *pvParms, uint32_t cbParms)
{
    LogFlowFunc(("u32Function=%RU32, pvParms=%p, cbParms=%RU32\n", u32Function, pvParms, cbParms));

    int rc = VERR_WRONG_ORDER; /* Play safe. */

    /* Whether or not to try calling host-installed callbacks after successfully processing the message. */
    bool fTryCallbacks = false;

    switch (u32Function)
    {
        case DragAndDropSvc::GUEST_DND_FN_CONNECT:
        {
            DragAndDropSvc::PVBOXDNDCBCONNECTDATA pCBData = reinterpret_cast<DragAndDropSvc::PVBOXDNDCBCONNECTDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBCONNECTDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_CONNECT == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            m_uProtocolVersion = pCBData->uProtocolVersion;
            /** @todo Handle flags. */

            LogThisFunc(("Client connected, using protocol v%RU32\n", m_uProtocolVersion));

            rc = VINF_SUCCESS;
            break;
        }

        case DragAndDropSvc::GUEST_DND_FN_REPORT_FEATURES:
        {
            DragAndDropSvc::PVBOXDNDCBREPORTFEATURESDATA pCBData = reinterpret_cast<DragAndDropSvc::PVBOXDNDCBREPORTFEATURESDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBREPORTFEATURESDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_REPORT_FEATURES == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            m_fGuestFeatures0 = pCBData->fGuestFeatures0;

            LogThisFunc(("Client reported features: %#RX64\n", m_fGuestFeatures0));

            rc = VINF_SUCCESS;
            break;
        }

        case DragAndDropSvc::GUEST_DND_FN_DISCONNECT:
        {
            LogThisFunc(("Client disconnected\n"));
            rc = setProgress(100, DND_PROGRESS_CANCELLED, VINF_SUCCESS);
            break;
        }

        case DragAndDropSvc::GUEST_DND_FN_HG_ACK_OP:
        {
            DragAndDropSvc::PVBOXDNDCBHGACKOPDATA pCBData = reinterpret_cast<DragAndDropSvc::PVBOXDNDCBHGACKOPDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBHGACKOPDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_HG_ACK_OP == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            LogRel2(("DnD: Guest responded with action '%s' for host->guest drag event\n", DnDActionToStr(pCBData->uAction)));

            setActionDefault(pCBData->uAction);
            rc = notifyAboutGuestResponse();
            break;
        }

        case DragAndDropSvc::GUEST_DND_FN_HG_REQ_DATA:
        {
            DragAndDropSvc::PVBOXDNDCBHGREQDATADATA pCBData = reinterpret_cast<DragAndDropSvc::PVBOXDNDCBHGREQDATADATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBHGREQDATADATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_HG_REQ_DATA == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            if (   pCBData->cbFormat  == 0
                || pCBData->cbFormat  > _64K /** @todo Make this configurable? */
                || pCBData->pszFormat == NULL)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (!RTStrIsValidEncoding(pCBData->pszFormat))
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                setFormats(GuestDnD::toFormatList(pCBData->pszFormat));
                rc = VINF_SUCCESS;
            }

            int rc2 = notifyAboutGuestResponse();
            if (RT_SUCCESS(rc))
                rc = rc2;
            break;
        }

        case DragAndDropSvc::GUEST_DND_FN_HG_EVT_PROGRESS:
        {
            DragAndDropSvc::PVBOXDNDCBHGEVTPROGRESSDATA pCBData =
               reinterpret_cast<DragAndDropSvc::PVBOXDNDCBHGEVTPROGRESSDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBHGEVTPROGRESSDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_HG_EVT_PROGRESS == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = setProgress(pCBData->uPercentage, pCBData->uStatus, pCBData->rc);
            if (RT_SUCCESS(rc))
                rc = notifyAboutGuestResponse();
            break;
        }
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
        case DragAndDropSvc::GUEST_DND_FN_GH_ACK_PENDING:
        {
            DragAndDropSvc::PVBOXDNDCBGHACKPENDINGDATA pCBData =
               reinterpret_cast<DragAndDropSvc::PVBOXDNDCBGHACKPENDINGDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBGHACKPENDINGDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_GH_ACK_PENDING == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            LogRel2(("DnD: Guest responded with pending action '%s' (%RU32 bytes format data) to guest->host drag event\n",
                     DnDActionToStr(pCBData->uDefAction), pCBData->cbFormat));

            if (   pCBData->cbFormat  == 0
                || pCBData->cbFormat  > _64K /** @todo Make the maximum size configurable? */
                || pCBData->pszFormat == NULL)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (!RTStrIsValidEncoding(pCBData->pszFormat))
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                setFormats   (GuestDnD::toFormatList(pCBData->pszFormat));
                setActionDefault (pCBData->uDefAction);
                setActionsAllowed(pCBData->uAllActions);

                rc = VINF_SUCCESS;
            }

            int rc2 = notifyAboutGuestResponse();
            if (RT_SUCCESS(rc))
                rc = rc2;
            break;
        }
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */
        default:
            /* * Try if the event is covered by a registered callback. */
            fTryCallbacks = true;
            break;
    }

    /*
     * Try the host's installed callbacks (if any).
     */
    if (fTryCallbacks)
    {
        GuestDnDCallbackMap::const_iterator it = m_mapCallbacks.find(u32Function);
        if (it != m_mapCallbacks.end())
        {
            AssertPtr(it->second.pfnCallback);
            rc = it->second.pfnCallback(u32Function, pvParms, cbParms, it->second.pvUser);
        }
        else
        {
            LogFlowFunc(("No callback for function %RU32 defined\n", u32Function));
            rc = VERR_NOT_SUPPORTED; /* Tell the guest. */
        }
    }

    LogFlowFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

/**
 * Helper function to query the internal progress object to an IProgress interface.
 *
 * @returns HRESULT
 * @param   ppProgress          Where to query the progress object to.
 */
HRESULT GuestDnDState::queryProgressTo(IProgress **ppProgress)
{
    return m_pProgress.queryInterfaceTo(ppProgress);
}

/**
 * Waits for a guest response to happen.
 *
 * @returns VBox status code.
 * @param   msTimeout           Timeout (in ms) for waiting. Optional, waits 3000 ms if not specified.
 */
int GuestDnDState::waitForGuestResponse(RTMSINTERVAL msTimeout /*= 3000 */) const
{
    int rc = RTSemEventWait(m_EventSem, msTimeout);
#ifdef DEBUG_andy
    LogFlowFunc(("msTimeout=%RU32, rc=%Rrc\n", msTimeout, rc));
#endif
    return rc;
}

/*********************************************************************************************************************************
 * GuestDnD implementation.                                                                                                      *
 ********************************************************************************************************************************/

/** Static (Singleton) instance of the GuestDnD object. */
GuestDnD* GuestDnD::s_pInstance = NULL;

GuestDnD::GuestDnD(const ComObjPtr<Guest> &pGuest)
    : m_pGuest(pGuest)
    , m_cTransfersPending(0)
{
    LogFlowFuncEnter();

    try
    {
        m_pState = new GuestDnDState(pGuest);
    }
    catch (std::bad_alloc &)
    {
        throw VERR_NO_MEMORY;
    }

    int rc = RTCritSectInit(&m_CritSect);
    if (RT_FAILURE(rc))
        throw rc;

    /* List of supported default MIME types. */
    LogRel2(("DnD: Supported default host formats:\n"));
    const com::Utf8Str arrEntries[] = { VBOX_DND_FORMATS_DEFAULT };
    for (size_t i = 0; i < RT_ELEMENTS(arrEntries); i++)
    {
        m_strDefaultFormats.push_back(arrEntries[i]);
        LogRel2(("DnD: \t%s\n", arrEntries[i].c_str()));
    }
}

GuestDnD::~GuestDnD(void)
{
    LogFlowFuncEnter();

    Assert(m_cTransfersPending == 0); /* Sanity. */

    RTCritSectDelete(&m_CritSect);

    if (m_pState)
        delete m_pState;
}

/**
 * Adjusts coordinations to a given screen.
 *
 * @returns HRESULT
 * @param   uScreenId           ID of screen to adjust coordinates to.
 * @param   puX                 Pointer to X coordinate to adjust. Will return the adjusted value on success.
 * @param   puY                 Pointer to Y coordinate to adjust. Will return the adjusted value on success.
 */
HRESULT GuestDnD::adjustScreenCoordinates(ULONG uScreenId, ULONG *puX, ULONG *puY) const
{
    /** @todo r=andy Save the current screen's shifting coordinates to speed things up.
     *               Only query for new offsets when the screen ID or the screen's resolution has changed. */

    /* For multi-monitor support we need to add shift values to the coordinates
     * (depending on the screen number). */
    ComObjPtr<Console> pConsole = m_pGuest->i_getConsole();
    ComPtr<IDisplay> pDisplay;
    HRESULT hr = pConsole->COMGETTER(Display)(pDisplay.asOutParam());
    if (FAILED(hr))
        return hr;

    ULONG dummy;
    LONG xShift, yShift;
    GuestMonitorStatus_T monitorStatus;
    hr = pDisplay->GetScreenResolution(uScreenId, &dummy, &dummy, &dummy,
                                       &xShift, &yShift, &monitorStatus);
    if (FAILED(hr))
        return hr;

    if (puX)
        *puX += xShift;
    if (puY)
        *puY += yShift;

    LogFlowFunc(("uScreenId=%RU32, x=%RU32, y=%RU32\n", uScreenId, puX ? *puX : 0, puY ? *puY : 0));
    return S_OK;
}

/**
 * Returns a DnD guest state.
 *
 * @returns Pointer to DnD guest state, or NULL if not found / invalid.
 * @param   uID                 ID of DnD guest state to return.
 */
GuestDnDState *GuestDnD::getState(uint32_t uID /* = 0 */) const
{
    AssertMsgReturn(uID == 0, ("Only one state (0) is supported at the moment\n"), NULL);

    return m_pState;
}

/**
 * Sends a (blocking) message to the host side of the host service.
 *
 * @returns VBox status code.
 * @param   u32Function         HGCM message ID to send.
 * @param   cParms              Number of parameters to send.
 * @param   paParms             Array of parameters to send. Must match \c cParms.
 */
int GuestDnD::hostCall(uint32_t u32Function, uint32_t cParms, PVBOXHGCMSVCPARM paParms) const
{
    Assert(!m_pGuest.isNull());
    ComObjPtr<Console> pConsole = m_pGuest->i_getConsole();

    /* Forward the information to the VMM device. */
    Assert(!pConsole.isNull());
    VMMDev *pVMMDev = pConsole->i_getVMMDev();
    if (!pVMMDev)
        return VERR_COM_OBJECT_NOT_FOUND;

    return pVMMDev->hgcmHostCall("VBoxDragAndDropSvc", u32Function, cParms, paParms);
}

/**
 * Registers a GuestDnDSource object with the GuestDnD manager.
 *
 * Currently only one source is supported at a time.
 *
 * @returns VBox status code.
 * @param   Source              Source to register.
 */
int GuestDnD::registerSource(const ComObjPtr<GuestDnDSource> &Source)
{
    GUESTDND_LOCK();

    Assert(m_lstSrc.size() == 0); /* We only support one source at a time at the moment. */
    m_lstSrc.push_back(Source);

    GUESTDND_UNLOCK();
    return VINF_SUCCESS;
}

/**
 * Unregisters a GuestDnDSource object from the GuestDnD manager.
 *
 * @returns VBox status code.
 * @param   Source              Source to unregister.
 */
int GuestDnD::unregisterSource(const ComObjPtr<GuestDnDSource> &Source)
{
    GUESTDND_LOCK();

    GuestDnDSrcList::iterator itSrc = std::find(m_lstSrc.begin(), m_lstSrc.end(), Source);
    if (itSrc != m_lstSrc.end())
        m_lstSrc.erase(itSrc);

    GUESTDND_UNLOCK();
    return VINF_SUCCESS;
}

/**
 * Returns the current number of registered sources.
 *
 * @returns Current number of registered sources.
 */
size_t GuestDnD::getSourceCount(void)
{
    GUESTDND_LOCK_RET(0);

    size_t cSources = m_lstSrc.size();

    GUESTDND_UNLOCK();
    return cSources;
}

/**
 * Registers a GuestDnDTarget object with the GuestDnD manager.
 *
 * Currently only one target is supported at a time.
 *
 * @returns VBox status code.
 * @param   Target              Target to register.
 */
int GuestDnD::registerTarget(const ComObjPtr<GuestDnDTarget> &Target)
{
    GUESTDND_LOCK();

    Assert(m_lstTgt.size() == 0); /* We only support one target at a time at the moment. */
    m_lstTgt.push_back(Target);

    GUESTDND_UNLOCK();
    return VINF_SUCCESS;
}

/**
 * Unregisters a GuestDnDTarget object from the GuestDnD manager.
 *
 * @returns VBox status code.
 * @param   Target              Target to unregister.
 */
int GuestDnD::unregisterTarget(const ComObjPtr<GuestDnDTarget> &Target)
{
    GUESTDND_LOCK();

    GuestDnDTgtList::iterator itTgt = std::find(m_lstTgt.begin(), m_lstTgt.end(), Target);
    if (itTgt != m_lstTgt.end())
        m_lstTgt.erase(itTgt);

    GUESTDND_UNLOCK();
    return VINF_SUCCESS;
}

/**
 * Returns the current number of registered targets.
 *
 * @returns Current number of registered targets.
 */
size_t GuestDnD::getTargetCount(void)
{
    GUESTDND_LOCK_RET(0);

    size_t cTargets = m_lstTgt.size();

    GUESTDND_UNLOCK();
    return cTargets;
}

/**
 * Static main dispatcher function to handle callbacks from the DnD host service.
 *
 * @returns VBox status code.
 * @param   pvExtension         Pointer to service extension.
 * @param   u32Function         Callback HGCM message ID.
 * @param   pvParms             Pointer to optional data provided for a particular message. Optional.
 * @param   cbParms             Size (in bytes) of \a pvParms.
 */
/* static */
DECLCALLBACK(int) GuestDnD::notifyDnDDispatcher(void *pvExtension, uint32_t u32Function,
                                                void *pvParms, uint32_t cbParms)
{
    LogFlowFunc(("pvExtension=%p, u32Function=%RU32, pvParms=%p, cbParms=%RU32\n",
                 pvExtension, u32Function, pvParms, cbParms));

    GuestDnD *pGuestDnD = reinterpret_cast<GuestDnD*>(pvExtension);
    AssertPtrReturn(pGuestDnD, VERR_INVALID_POINTER);

    /** @todo In case we need to handle multiple guest DnD responses at a time this
     *        would be the place to lookup and dispatch to those. For the moment we
     *        only have one response -- simple. */
    if (pGuestDnD->m_pState)
        return pGuestDnD->m_pState->onDispatch(u32Function, pvParms, cbParms);

    return VERR_NOT_SUPPORTED;
}

/**
 * Static helper function to determine whether a format is part of a given MIME list.
 *
 * @returns \c true if found, \c false if not.
 * @param   strFormat           Format to search for.
 * @param   lstFormats          MIME list to search in.
 */
/* static */
bool GuestDnD::isFormatInFormatList(const com::Utf8Str &strFormat, const GuestDnDMIMEList &lstFormats)
{
    return std::find(lstFormats.begin(), lstFormats.end(), strFormat) != lstFormats.end();
}

/**
 * Static helper function to create a GuestDnDMIMEList out of a format list string.
 *
 * @returns MIME list object.
 * @param   strFormats          List of formats to convert.
 * @param   strSep              Separator to use. If not specified, DND_FORMATS_SEPARATOR_STR will be used.
 */
/* static */
GuestDnDMIMEList GuestDnD::toFormatList(const com::Utf8Str &strFormats, const com::Utf8Str &strSep /* = DND_FORMATS_SEPARATOR_STR */)
{
    GuestDnDMIMEList lstFormats;
    RTCList<RTCString> lstFormatsTmp = strFormats.split(strSep);

    for (size_t i = 0; i < lstFormatsTmp.size(); i++)
        lstFormats.push_back(com::Utf8Str(lstFormatsTmp.at(i)));

    return lstFormats;
}

/**
 * Static helper function to create a format list string from a given GuestDnDMIMEList object.
 *
 * @returns Format list string.
 * @param   lstFormats          GuestDnDMIMEList to convert.
 * @param   strSep              Separator to use between formats.
 *                              Uses DND_FORMATS_SEPARATOR_STR as default.
 */
/* static */
com::Utf8Str GuestDnD::toFormatString(const GuestDnDMIMEList &lstFormats, const com::Utf8Str &strSep /* = DND_FORMATS_SEPARATOR_STR */)
{
    com::Utf8Str strFormat;
    for (size_t i = 0; i < lstFormats.size(); i++)
    {
        const com::Utf8Str &f = lstFormats.at(i);
        strFormat += f + strSep;
    }

    return strFormat;
}

/**
 * Static helper function to create a filtered GuestDnDMIMEList object from supported and wanted formats.
 *
 * @returns Filtered MIME list object.
 * @param   lstFormatsSupported     MIME list of supported formats.
 * @param   lstFormatsWanted        MIME list of wanted formats in returned object.
 */
/* static */
GuestDnDMIMEList GuestDnD::toFilteredFormatList(const GuestDnDMIMEList &lstFormatsSupported, const GuestDnDMIMEList &lstFormatsWanted)
{
    GuestDnDMIMEList lstFormats;

    for (size_t i = 0; i < lstFormatsWanted.size(); i++)
    {
        /* Only keep supported format types. */
        if (std::find(lstFormatsSupported.begin(),
                      lstFormatsSupported.end(), lstFormatsWanted.at(i)) != lstFormatsSupported.end())
        {
            lstFormats.push_back(lstFormatsWanted[i]);
        }
    }

    return lstFormats;
}

/**
 * Static helper function to create a filtered GuestDnDMIMEList object from supported and wanted formats.
 *
 * @returns Filtered MIME list object.
 * @param   lstFormatsSupported     MIME list of supported formats.
 * @param   strFormatsWanted        Format list string of wanted formats in returned object.
 */
/* static */
GuestDnDMIMEList GuestDnD::toFilteredFormatList(const GuestDnDMIMEList &lstFormatsSupported, const com::Utf8Str &strFormatsWanted)
{
    GuestDnDMIMEList lstFmt;

    RTCList<RTCString> lstFormats = strFormatsWanted.split(DND_FORMATS_SEPARATOR_STR);
    size_t i = 0;
    while (i < lstFormats.size())
    {
        /* Only keep allowed format types. */
        if (std::find(lstFormatsSupported.begin(),
                      lstFormatsSupported.end(), lstFormats.at(i)) != lstFormatsSupported.end())
        {
            lstFmt.push_back(lstFormats[i]);
        }
        i++;
    }

    return lstFmt;
}

/**
 * Static helper function to convert a Main DnD action an internal DnD action.
 *
 * @returns Internal DnD action, or VBOX_DND_ACTION_IGNORE if not found / supported.
 * @param   enmAction               Main DnD action to convert.
 */
/* static */
VBOXDNDACTION GuestDnD::toHGCMAction(DnDAction_T enmAction)
{
    VBOXDNDACTION dndAction = VBOX_DND_ACTION_IGNORE;
    switch (enmAction)
    {
        case DnDAction_Copy:
            dndAction = VBOX_DND_ACTION_COPY;
            break;
        case DnDAction_Move:
            dndAction = VBOX_DND_ACTION_MOVE;
            break;
        case DnDAction_Link:
            /* For now it doesn't seems useful to allow a link
               action between host & guest. Later? */
        case DnDAction_Ignore:
            /* Ignored. */
            break;
        default:
            AssertMsgFailed(("Action %RU32 not recognized!\n", enmAction));
            break;
    }

    return dndAction;
}

/**
 * Static helper function to convert a Main DnD default action and allowed Main actions to their
 * corresponding internal representations.
 *
 * @param   enmDnDActionDefault     Default Main action to convert.
 * @param   pDnDActionDefault       Where to store the converted default action.
 * @param   vecDnDActionsAllowed    Allowed Main actions to convert.
 * @param   pDnDLstActionsAllowed   Where to store the converted allowed actions.
 */
/* static */
void GuestDnD::toHGCMActions(DnDAction_T                    enmDnDActionDefault,
                             VBOXDNDACTION                 *pDnDActionDefault,
                             const std::vector<DnDAction_T> vecDnDActionsAllowed,
                             VBOXDNDACTIONLIST             *pDnDLstActionsAllowed)
{
    VBOXDNDACTIONLIST dndLstActionsAllowed = VBOX_DND_ACTION_IGNORE;
    VBOXDNDACTION     dndActionDefault     = toHGCMAction(enmDnDActionDefault);

    if (!vecDnDActionsAllowed.empty())
    {
        /* First convert the allowed actions to a bit array. */
        for (size_t i = 0; i < vecDnDActionsAllowed.size(); i++)
            dndLstActionsAllowed |= toHGCMAction(vecDnDActionsAllowed[i]);

        /*
         * If no default action is set (ignoring), try one of the
         * set allowed actions, preferring copy, move (in that order).
         */
        if (isDnDIgnoreAction(dndActionDefault))
        {
            if (hasDnDCopyAction(dndLstActionsAllowed))
                dndActionDefault = VBOX_DND_ACTION_COPY;
            else if (hasDnDMoveAction(dndLstActionsAllowed))
                dndActionDefault = VBOX_DND_ACTION_MOVE;
        }
    }

    if (pDnDActionDefault)
        *pDnDActionDefault     = dndActionDefault;
    if (pDnDLstActionsAllowed)
        *pDnDLstActionsAllowed = dndLstActionsAllowed;
}

/**
 * Static helper function to convert an internal DnD action to its Main representation.
 *
 * @returns Converted Main DnD action.
 * @param   dndAction           DnD action to convert.
 */
/* static */
DnDAction_T GuestDnD::toMainAction(VBOXDNDACTION dndAction)
{
    /* For now it doesn't seems useful to allow a
     * link action between host & guest. Maybe later! */
    return isDnDCopyAction(dndAction) ? DnDAction_Copy
         : isDnDMoveAction(dndAction) ? DnDAction_Move
         :                              DnDAction_Ignore;
}

/**
 * Static helper function to convert an internal DnD action list to its Main representation.
 *
 * @returns Converted Main DnD action list.
 * @param   dndActionList       DnD action list to convert.
 */
/* static */
std::vector<DnDAction_T> GuestDnD::toMainActions(VBOXDNDACTIONLIST dndActionList)
{
    std::vector<DnDAction_T> vecActions;

    /* For now it doesn't seems useful to allow a
     * link action between host & guest. Maybe later! */
    RTCList<DnDAction_T> lstActions;
    if (hasDnDCopyAction(dndActionList))
        lstActions.append(DnDAction_Copy);
    if (hasDnDMoveAction(dndActionList))
        lstActions.append(DnDAction_Move);

    for (size_t i = 0; i < lstActions.size(); ++i)
        vecActions.push_back(lstActions.at(i));

    return vecActions;
}

/*********************************************************************************************************************************
 * GuestDnDBase implementation.                                                                                                  *
 ********************************************************************************************************************************/

GuestDnDBase::GuestDnDBase(void)
    : m_fIsPending(false)
{
    /* Initialize public attributes. */
    m_lstFmtSupported = GuestDnDInst()->defaultFormats();
}

/**
 * Checks whether a given DnD format is supported or not.
 *
 * @returns \c true if supported, \c false if not.
 * @param   aFormat             DnD format to check.
 */
bool GuestDnDBase::i_isFormatSupported(const com::Utf8Str &aFormat) const
{
    return std::find(m_lstFmtSupported.begin(), m_lstFmtSupported.end(), aFormat) != m_lstFmtSupported.end();
}

/**
 * Returns the currently supported DnD formats.
 *
 * @returns List of the supported DnD formats.
 */
const GuestDnDMIMEList &GuestDnDBase::i_getFormats(void) const
{
    return m_lstFmtSupported;
}

/**
 * Adds DnD formats to the supported formats list.
 *
 * @returns HRESULT
 * @param   aFormats            List of DnD formats to add.
 */
HRESULT GuestDnDBase::i_addFormats(const GuestDnDMIMEList &aFormats)
{
    for (size_t i = 0; i < aFormats.size(); ++i)
    {
        Utf8Str strFormat = aFormats.at(i);
        if (std::find(m_lstFmtSupported.begin(),
                      m_lstFmtSupported.end(), strFormat) == m_lstFmtSupported.end())
        {
            m_lstFmtSupported.push_back(strFormat);
        }
    }

    return S_OK;
}

/**
 * Removes DnD formats from tehh supported formats list.
 *
 * @returns HRESULT
 * @param   aFormats            List of DnD formats to remove.
 */
HRESULT GuestDnDBase::i_removeFormats(const GuestDnDMIMEList &aFormats)
{
    for (size_t i = 0; i < aFormats.size(); ++i)
    {
        Utf8Str strFormat = aFormats.at(i);
        GuestDnDMIMEList::iterator itFormat = std::find(m_lstFmtSupported.begin(),
                                                        m_lstFmtSupported.end(), strFormat);
        if (itFormat != m_lstFmtSupported.end())
            m_lstFmtSupported.erase(itFormat);
    }

    return S_OK;
}

/**
 * Adds a new guest DnD message to the internal message queue.
 *
 * @returns VBox status code.
 * @param   pMsg                Pointer to message to add.
 */
int GuestDnDBase::msgQueueAdd(GuestDnDMsg *pMsg)
{
    m_DataBase.lstMsgOut.push_back(pMsg);
    return VINF_SUCCESS;
}

/**
 * Returns the next guest DnD message in the internal message queue (FIFO).
 *
 * @returns Pointer to guest DnD message, or NULL if none found.
 */
GuestDnDMsg *GuestDnDBase::msgQueueGetNext(void)
{
    if (m_DataBase.lstMsgOut.empty())
        return NULL;
    return m_DataBase.lstMsgOut.front();
}

/**
 * Removes the next guest DnD message from the internal message queue.
 */
void GuestDnDBase::msgQueueRemoveNext(void)
{
    if (!m_DataBase.lstMsgOut.empty())
    {
        GuestDnDMsg *pMsg = m_DataBase.lstMsgOut.front();
        if (pMsg)
            delete pMsg;
        m_DataBase.lstMsgOut.pop_front();
    }
}

/**
 * Clears the internal message queue.
 */
void GuestDnDBase::msgQueueClear(void)
{
    LogFlowFunc(("cMsg=%zu\n", m_DataBase.lstMsgOut.size()));

    GuestDnDMsgList::iterator itMsg = m_DataBase.lstMsgOut.begin();
    while (itMsg != m_DataBase.lstMsgOut.end())
    {
        GuestDnDMsg *pMsg = *itMsg;
        if (pMsg)
            delete pMsg;

        itMsg++;
    }

    m_DataBase.lstMsgOut.clear();
}

/**
 * Sends a request to the guest side to cancel the current DnD operation.
 *
 * @returns VBox status code.
 */
int GuestDnDBase::sendCancel(void)
{
    GuestDnDMsg Msg;
    Msg.setType(HOST_DND_FN_CANCEL);
    if (m_pState->m_uProtocolVersion >= 3)
        Msg.appendUInt32(0); /** @todo ContextID not used yet. */

    LogRel2(("DnD: Cancelling operation on guest ...\n"));

    int rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
    if (RT_FAILURE(rc))
        LogRel(("DnD: Cancelling operation on guest failed with %Rrc\n", rc));

    return rc;
}

/**
 * Helper function to update the progress based on given a GuestDnDData object.
 *
 * @returns VBox status code.
 * @param   pData               GuestDnDData object to use for accounting.
 * @param   pState              Guest state to update its progress object for.
 * @param   cbDataAdd           By how much data (in bytes) to update the progress.
 */
int GuestDnDBase::updateProgress(GuestDnDData *pData, GuestDnDState *pState,
                                 size_t cbDataAdd /* = 0 */)
{
    AssertPtrReturn(pData, VERR_INVALID_POINTER);
    AssertPtrReturn(pState, VERR_INVALID_POINTER);
    /* cbDataAdd is optional. */

    LogFlowFunc(("cbExtra=%zu, cbProcessed=%zu, cbRemaining=%zu, cbDataAdd=%zu\n",
                 pData->cbExtra, pData->cbProcessed, pData->getRemaining(), cbDataAdd));

    if (   !pState
        || !cbDataAdd) /* Only update if something really changes. */
        return VINF_SUCCESS;

    if (cbDataAdd)
        pData->addProcessed(cbDataAdd);

    const uint8_t uPercent = pData->getPercentComplete();

    LogRel2(("DnD: Transfer %RU8%% complete\n", uPercent));

    int rc = pState->setProgress(uPercent,
                                   pData->isComplete()
                                 ? DND_PROGRESS_COMPLETE
                                 : DND_PROGRESS_RUNNING);
    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Waits for a specific guest callback event to get signalled.
 *
 * @returns VBox status code. Will return VERR_CANCELLED if the user has cancelled the progress object.
 * @param   pEvent                  Callback event to wait for.
 * @param   pState                  Guest state to update.
 * @param   msTimeout               Timeout (in ms) to wait.
 */
int GuestDnDBase::waitForEvent(GuestDnDCallbackEvent *pEvent, GuestDnDState *pState, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    AssertPtrReturn(pState, VERR_INVALID_POINTER);

    int rc;

    LogFlowFunc(("msTimeout=%RU32\n", msTimeout));

    uint64_t tsStart = RTTimeMilliTS();
    do
    {
        /*
         * Wait until our desired callback triggered the
         * wait event. As we don't want to block if the guest does not
         * respond, do busy waiting here.
         */
        rc = pEvent->Wait(500 /* ms */);
        if (RT_SUCCESS(rc))
        {
            rc = pEvent->Result();
            LogFlowFunc(("Callback done, result is %Rrc\n", rc));
            break;
        }
        else if (rc == VERR_TIMEOUT) /* Continue waiting. */
            rc = VINF_SUCCESS;

        if (   msTimeout != RT_INDEFINITE_WAIT
            && RTTimeMilliTS() - tsStart > msTimeout)
        {
            rc = VERR_TIMEOUT;
            LogRel2(("DnD: Error: Guest did not respond within time\n"));
        }
        else if (pState->isProgressCanceled())
        {
            LogRel2(("DnD: Operation was canceled by user\n"));
            rc = VERR_CANCELLED;
        }

    } while (RT_SUCCESS(rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_DRAG_AND_DROP */

