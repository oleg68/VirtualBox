/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <realmode.h>
#include <undipreload.h>

/** @file
 *
 * Preloaded UNDI stack
 *
 */

/**
 * Preloaded UNDI device
 *
 * This is the UNDI device that was present when Etherboot started
 * execution (i.e. when loading a .kpxe image).  The first driver to
 * claim this device must zero out this data structure.
 */
struct undi_device __data16 ( preloaded_undi );
