/*
 * Copyright (C) 2015 Michael Brown <mbrown@fensystems.co.uk>.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** @file
 *
 * SHA-512 family tests
 *
 * NIST test vectors are taken from
 *
 *  http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA512.pdf
 *  http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA384.pdf
 *  http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA512_256.pdf
 *  http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA512_224.pdf
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <ipxe/sha512.h>
#include <ipxe/test.h>
#include "digest_test.h"

/* Empty test vector (digest obtained from "sha512sum /dev/null") */
DIGEST_TEST ( sha512_empty, &sha512_algorithm, DIGEST_EMPTY,
	      DIGEST ( 0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd, 0xf1,
		       0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07, 0xd6, 0x20,
		       0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc, 0x83, 0xf4, 0xa9,
		       0x21, 0xd3, 0x6c, 0xe9, 0xce, 0x47, 0xd0, 0xd1, 0x3c,
		       0x5d, 0x85, 0xf2, 0xb0, 0xff, 0x83, 0x18, 0xd2, 0x87,
		       0x7e, 0xec, 0x2f, 0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41,
		       0x7a, 0x81, 0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda,
		       0x3e ) );

/* NIST test vector "abc" */
DIGEST_TEST ( sha512_nist_abc, &sha512_algorithm, DIGEST_NIST_ABC,
	      DIGEST ( 0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc,
		       0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31, 0x12, 0xe6,
		       0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 0x0a, 0x9e, 0xee,
		       0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21, 0x92, 0x99, 0x2a,
		       0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3,
		       0xfe, 0xeb, 0xbd, 0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c,
		       0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4,
		       0x9f ) );

/* NIST test vector "abc...stu" */
DIGEST_TEST ( sha512_nist_abc_stu, &sha512_algorithm, DIGEST_NIST_ABC_STU,
	      DIGEST ( 0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda, 0x8c,
		       0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f, 0x8f, 0x77,
		       0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1, 0x72, 0x99, 0xae,
		       0xad, 0xb6, 0x88, 0x90, 0x18, 0x50, 0x1d, 0x28, 0x9e,
		       0x49, 0x00, 0xf7, 0xe4, 0x33, 0x1b, 0x99, 0xde, 0xc4,
		       0xb5, 0x43, 0x3a, 0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd,
		       0x26, 0x54, 0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9,
		       0x09 ) );

/* Empty test vector (digest obtained from "sha384sum /dev/null") */
DIGEST_TEST ( sha384_empty, &sha384_algorithm, DIGEST_EMPTY,
	      DIGEST ( 0x38, 0xb0, 0x60, 0xa7, 0x51, 0xac, 0x96, 0x38, 0x4c,
		       0xd9, 0x32, 0x7e, 0xb1, 0xb1, 0xe3, 0x6a, 0x21, 0xfd,
		       0xb7, 0x11, 0x14, 0xbe, 0x07, 0x43, 0x4c, 0x0c, 0xc7,
		       0xbf, 0x63, 0xf6, 0xe1, 0xda, 0x27, 0x4e, 0xde, 0xbf,
		       0xe7, 0x6f, 0x65, 0xfb, 0xd5, 0x1a, 0xd2, 0xf1, 0x48,
		       0x98, 0xb9, 0x5b ) );

/* NIST test vector "abc" */
DIGEST_TEST ( sha384_nist_abc, &sha384_algorithm, DIGEST_NIST_ABC,
	      DIGEST ( 0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5,
		       0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07, 0x27, 0x2c,
		       0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63, 0x1a, 0x8b, 0x60,
		       0x5a, 0x43, 0xff, 0x5b, 0xed, 0x80, 0x86, 0x07, 0x2b,
		       0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34,
		       0xc8, 0x25, 0xa7 ) );

/* NIST test vector "abc...stu" */
DIGEST_TEST ( sha384_nist_abc_stu, &sha384_algorithm, DIGEST_NIST_ABC_STU,
	      DIGEST ( 0x09, 0x33, 0x0c, 0x33, 0xf7, 0x11, 0x47, 0xe8, 0x3d,
		       0x19, 0x2f, 0xc7, 0x82, 0xcd, 0x1b, 0x47, 0x53, 0x11,
		       0x1b, 0x17, 0x3b, 0x3b, 0x05, 0xd2, 0x2f, 0xa0, 0x80,
		       0x86, 0xe3, 0xb0, 0xf7, 0x12, 0xfc, 0xc7, 0xc7, 0x1a,
		       0x55, 0x7e, 0x2d, 0xb9, 0x66, 0xc3, 0xe9, 0xfa, 0x91,
		       0x74, 0x60, 0x39 ) );

/* Empty test vector (digest obtained from "shasum -a 512256 /dev/null") */
DIGEST_TEST ( sha512_256_empty, &sha512_256_algorithm, DIGEST_EMPTY,
	      DIGEST ( 0xc6, 0x72, 0xb8, 0xd1, 0xef, 0x56, 0xed, 0x28, 0xab,
		       0x87, 0xc3, 0x62, 0x2c, 0x51, 0x14, 0x06, 0x9b, 0xdd,
		       0x3a, 0xd7, 0xb8, 0xf9, 0x73, 0x74, 0x98, 0xd0, 0xc0,
		       0x1e, 0xce, 0xf0, 0x96, 0x7a ) );

/* NIST test vector "abc" */
DIGEST_TEST ( sha512_256_nist_abc, &sha512_256_algorithm, DIGEST_NIST_ABC,
	      DIGEST ( 0x53, 0x04, 0x8e, 0x26, 0x81, 0x94, 0x1e, 0xf9, 0x9b,
		       0x2e, 0x29, 0xb7, 0x6b, 0x4c, 0x7d, 0xab, 0xe4, 0xc2,
		       0xd0, 0xc6, 0x34, 0xfc, 0x6d, 0x46, 0xe0, 0xe2, 0xf1,
		       0x31, 0x07, 0xe7, 0xaf, 0x23 ) );

/* NIST test vector "abc...stu" */
DIGEST_TEST ( sha512_256_nist_abc_stu, &sha512_256_algorithm,
	      DIGEST_NIST_ABC_STU,
	      DIGEST ( 0x39, 0x28, 0xe1, 0x84, 0xfb, 0x86, 0x90, 0xf8, 0x40,
		       0xda, 0x39, 0x88, 0x12, 0x1d, 0x31, 0xbe, 0x65, 0xcb,
		       0x9d, 0x3e, 0xf8, 0x3e, 0xe6, 0x14, 0x6f, 0xea, 0xc8,
		       0x61, 0xe1, 0x9b, 0x56, 0x3a ) );

/* Empty test vector (digest obtained from "shasum -a 512224 /dev/null") */
DIGEST_TEST ( sha512_224_empty, &sha512_224_algorithm, DIGEST_EMPTY,
	      DIGEST ( 0x6e, 0xd0, 0xdd, 0x02, 0x80, 0x6f, 0xa8, 0x9e, 0x25,
		       0xde, 0x06, 0x0c, 0x19, 0xd3, 0xac, 0x86, 0xca, 0xbb,
		       0x87, 0xd6, 0xa0, 0xdd, 0xd0, 0x5c, 0x33, 0x3b, 0x84,
		       0xf4 ) );

/* NIST test vector "abc" */
DIGEST_TEST ( sha512_224_nist_abc, &sha512_224_algorithm, DIGEST_NIST_ABC,
	      DIGEST ( 0x46, 0x34, 0x27, 0x0f, 0x70, 0x7b, 0x6a, 0x54, 0xda,
		       0xae, 0x75, 0x30, 0x46, 0x08, 0x42, 0xe2, 0x0e, 0x37,
		       0xed, 0x26, 0x5c, 0xee, 0xe9, 0xa4, 0x3e, 0x89, 0x24,
		       0xaa ) );

/* NIST test vector "abc...stu" */
DIGEST_TEST ( sha512_224_nist_abc_stu, &sha512_224_algorithm,
	      DIGEST_NIST_ABC_STU,
	      DIGEST ( 0x23, 0xfe, 0xc5, 0xbb, 0x94, 0xd6, 0x0b, 0x23, 0x30,
		       0x81, 0x92, 0x64, 0x0b, 0x0c, 0x45, 0x33, 0x35, 0xd6,
		       0x64, 0x73, 0x4f, 0xe4, 0x0e, 0x72, 0x68, 0x67, 0x4a,
		       0xf9 ) );

/**
 * Perform SHA-512 family self-test
 *
 */
static void sha512_test_exec ( void ) {

	/* Correctness tests */
	digest_ok ( &sha512_empty );
	digest_ok ( &sha512_nist_abc );
	digest_ok ( &sha512_nist_abc_stu );
	digest_ok ( &sha384_empty );
	digest_ok ( &sha384_nist_abc );
	digest_ok ( &sha384_nist_abc_stu );
	digest_ok ( &sha512_256_empty );
	digest_ok ( &sha512_256_nist_abc );
	digest_ok ( &sha512_256_nist_abc_stu );
	digest_ok ( &sha512_224_empty );
	digest_ok ( &sha512_224_nist_abc );
	digest_ok ( &sha512_224_nist_abc_stu );

	/* Speed tests */
	DBG ( "SHA512 required %ld cycles per byte\n",
	      digest_cost ( &sha512_algorithm ) );
	DBG ( "SHA384 required %ld cycles per byte\n",
	      digest_cost ( &sha384_algorithm ) );
	DBG ( "SHA512/256 required %ld cycles per byte\n",
	      digest_cost ( &sha512_256_algorithm ) );
	DBG ( "SHA512/224 required %ld cycles per byte\n",
	      digest_cost ( &sha512_224_algorithm ) );
}

/** SHA-512 family self-test */
struct self_test sha512_test __self_test = {
	.name = "sha512",
	.exec = sha512_test_exec,
};
