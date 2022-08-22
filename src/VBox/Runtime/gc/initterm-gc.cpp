/* $Id$ */
/** @file
 * IPRT - Init Raw-mode Context.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */



/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_DEFAULT

#include <iprt/initterm.h>
#include <iprt/time.h>
#include <iprt/log.h>
#include <iprt/errcore.h>
#include <iprt/string.h>
#include "internal/time.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Program start nanosecond TS.
 */
DECL_HIDDEN_DATA(uint64_t) g_u64ProgramStartNanoTS;


/**
 * Initializes the raw-mode context runtime library.
 *
 * @returns iprt status code.
 *
 * @param   u64ProgramStartNanoTS  The startup timestamp.
 */
RTRCDECL(int) RTRCInit(uint64_t u64ProgramStartNanoTS)
{
    /*
     * Init the program start TSes.
     */
    g_u64ProgramStartNanoTS = u64ProgramStartNanoTS;

    LogFlow(("RTGCInit: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Terminates the raw-mode context runtime library.
 */
RTRCDECL(void) RTRCTerm(void)
{
    /* do nothing */
}
