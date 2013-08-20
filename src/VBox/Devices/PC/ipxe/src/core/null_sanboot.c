/*
 * Copyright (C) 2010 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <errno.h>
#include <ipxe/sanboot.h>

static int null_san_hook ( struct uri *uri __unused,
			   unsigned int drive __unused ) {
	return -EOPNOTSUPP;
}

static void null_san_unhook ( unsigned int drive __unused ) {
	/* Do nothing */
}

static int null_san_boot ( unsigned int drive __unused ) {
	return -EOPNOTSUPP;
}

static int null_san_describe ( unsigned int drive __unused ) {
	return -EOPNOTSUPP;
}

PROVIDE_SANBOOT_INLINE ( null, san_default_drive );
PROVIDE_SANBOOT ( null, san_hook, null_san_hook );
PROVIDE_SANBOOT ( null, san_unhook, null_san_unhook );
PROVIDE_SANBOOT ( null, san_boot, null_san_boot );
PROVIDE_SANBOOT ( null, san_describe, null_san_describe );
