/*
 * Copyright (C) 2005 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef _APPMGMT_H
#define _APPMGMT_H

typedef struct _MANAGEDAPPLICATION
{
    LPWSTR pszPackageName;
    LPWSTR pszPublisher;
    DWORD dwVersionHi;
    DWORD dwVersionLo;
    DWORD dwRevision;
    GUID GpoId;
    LPWSTR pszPolicyName;
    GUID ProductId;
    LANGID Language;
    LPWSTR pszOwner;
    LPWSTR pszCompany;
    LPWSTR pszComments;
    LPWSTR pszContact;
    LPWSTR pszSupportUrl;
    DWORD dwPathType;
    BOOL bInstalled;
} MANAGEDAPPLICATION, *PMANAGEDAPPLICATION;


#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI CommandLineFromMsiDescriptor(WCHAR*,WCHAR*,DWORD*);
DWORD WINAPI GetManagedApplications(GUID*,DWORD,DWORD,LPDWORD,PMANAGEDAPPLICATION*);

#ifdef __cplusplus
}
#endif

#endif /* _APPMGMT_H */
