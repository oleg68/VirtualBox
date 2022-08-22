/* $Id$ */
/** @file
 * VBoxTrayMsg - Globally registered messages (RPC) to/from VBoxTray.
 */

/*
 * Copyright (C) 2010-2022 Oracle and/or its affiliates.
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

#ifndef GA_INCLUDED_SRC_WINNT_VBoxTray_VBoxTrayMsg_h
#define GA_INCLUDED_SRC_WINNT_VBoxTray_VBoxTrayMsg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** The IPC pipe's prefix (native).
 * Will be followed by the username VBoxTray runs under. */
#define VBOXTRAY_IPC_PIPE_PREFIX      "\\\\.\\pipe\\VBoxTrayIPC-"
/** The IPC header's magic. */
#define VBOXTRAY_IPC_HDR_MAGIC        0x19840804

enum VBOXTRAYIPCMSGTYPE
{
    /** Restarts VBoxTray. */
    VBOXTRAYIPCMSGTYPE_RESTART        = 10,
    /** Shows a balloon message in the tray area. */
    VBOXTRAYIPCMSGTYPE_SHOWBALLOONMSG = 100,
    /** Retrieves the current user's last input
     *  time. This will be the user VBoxTray is running
     *  under. No actual message for this command
     *  required. */
    VBOXTRAYIPCMSGTYPE_USERLASTINPUT  = 120
};

/* VBoxTray's IPC header. */
typedef struct VBOXTRAYIPCHEADER
{
    /** The header's magic. */
    uint32_t uMagic;
    /** Header version, must be 0 by now. */
    uint32_t uHdrVersion;
    /** Message type. Specifies a message
     *  of VBOXTRAYIPCMSGTYPE. */
    uint32_t uMsgType;
    /** Message length (in bytes). This must
     *  include the overall message length, including
     *  (eventual) dynamically allocated areas which
     *  are passed into the message structure.
     */
    uint32_t uMsgLen;

} VBOXTRAYIPCHEADER, *PVBOXTRAYIPCHEADER;

/**
 * Tells VBoxTray to show a balloon message in Windows'
 * tray area. This may or may not work depending on the
 * system's configuration / set user preference.
 */
typedef struct VBOXTRAYIPCMSG_SHOWBALLOONMSG
{
    /** Length of message body (in bytes). */
    uint32_t cbMsgContent;
    /** Length of message title (in bytes). */
    uint32_t cbMsgTitle;
    /** Message type. */
    uint32_t uType;
    /** Time to show the message (in ms). */
    uint32_t uShowMS;
    /** Dynamically allocated stuff.
     *
     *  Note: These must come at the end of the
     *  structure to not overwrite any important
     *  stuff above.
     */
    /** Message body. Can be up to 256 chars
     *  long. */
    char     szMsgContent[1];
        /** Message title. Can be up to 73 chars
     *  long. */
    char     szMsgTitle[1];
} VBOXTRAYIPCMSG_SHOWBALLOONMSG, *PVBOXTRAYIPCMSG_SHOWBALLOONMSG;

/**
 * Response telling the last input of the current user.
 */
typedef struct VBOXTRAYIPCRES_USERLASTINPUT
{
    /** Last occurred user input event (in seconds). */
    uint32_t uLastInput;
} VBOXTRAYIPCRES_USERLASTINPUT, *PVBOXTRAYIPCRES_USERLASTINPUT;

#endif /* !GA_INCLUDED_SRC_WINNT_VBoxTray_VBoxTrayMsg_h */

