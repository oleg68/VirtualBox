/* $Id$ */
/** @file
 * VBoxSDL - Darwin Cocoa helper functions.
 */

/*
 * Copyright (C) 2006-2022 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define NO_SDL_H
#import "VBoxSDL.h"
#import <Cocoa/Cocoa.h>

void *VBoxSDLGetDarwinWindowId(void)
{
    NSView            *pView = nil;
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    {
        NSApplication *pApp = NSApp;
        NSWindow *pMainWnd;
        pMainWnd = [pApp mainWindow];
        if (!pMainWnd)
            pMainWnd = pApp->_mainWindow; /* UGLY!! but mApp->_AppFlags._active = 0, so mainWindow() fails. */
        pView = [pMainWnd contentView];
    }
    [pPool release];
    return pView;
}

