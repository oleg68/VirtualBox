/** @file
  LockBox protocol header file.
  This is used to resolve dependency problem. The LockBox implementation
  install this to broadcast that LockBox API is ready. The driver who will
  use LockBox at its ENTRYPOINT should add this dependency.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LOCK_BOX_PROTOCOL_H_
#define _LOCK_BOX_PROTOCOL_H_

///
/// Global ID for the EFI LOCK BOX Protocol.
///
#define EFI_LOCK_BOX_PROTOCOL_GUID \
  { 0xbd445d79, 0xb7ad, 0x4f04, { 0x9a, 0xd8, 0x29, 0xbd, 0x20, 0x40, 0xeb, 0x3c }}

extern EFI_GUID gEfiLockBoxProtocolGuid;

#endif
