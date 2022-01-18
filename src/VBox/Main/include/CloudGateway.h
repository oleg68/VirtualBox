/* $Id$ */
/** @file
 * Implementation of local and cloud gateway management.
 */

/*
 * Copyright (C) 2019-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_CloudGateway_h
#define MAIN_INCLUDED_CloudGateway_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

struct GatewayInfo
{
    Bstr    mTargetVM;
    Utf8Str mGatewayInstanceId;
    Utf8Str mPublicSshKey;
    Utf8Str mPrivateSshKey;
    Bstr    mCloudProvider;
    Bstr    mCloudProfile;
    Utf8Str mCloudPublicIp;
    Utf8Str mCloudSecondaryPublicIp;
    RTMAC   mCloudMacAddress;
    RTMAC   mLocalMacAddress;
    int     mAdapterSlot;

    HRESULT setCloudMacAddress(const Utf8Str& mac);
    HRESULT setLocalMacAddress(const Utf8Str& mac);

    GatewayInfo() {}

    GatewayInfo(const GatewayInfo& other)
        : mGatewayInstanceId(other.mGatewayInstanceId),
          mPublicSshKey(other.mPublicSshKey),
          mPrivateSshKey(other.mPrivateSshKey),
          mCloudProvider(other.mCloudProvider),
          mCloudProfile(other.mCloudProfile),
          mCloudPublicIp(other.mCloudPublicIp),
          mCloudSecondaryPublicIp(other.mCloudSecondaryPublicIp),
          mCloudMacAddress(other.mCloudMacAddress),
          mLocalMacAddress(other.mLocalMacAddress),
          mAdapterSlot(other.mAdapterSlot)
    {}

    GatewayInfo& operator=(const GatewayInfo& other)
    {
        mGatewayInstanceId = other.mGatewayInstanceId;
        mPublicSshKey = other.mPublicSshKey;
        mPrivateSshKey = other.mPrivateSshKey;
        mCloudProvider = other.mCloudProvider;
        mCloudProfile = other.mCloudProfile;
        mCloudPublicIp = other.mCloudPublicIp;
        mCloudSecondaryPublicIp = other.mCloudSecondaryPublicIp;
        mCloudMacAddress = other.mCloudMacAddress;
        mLocalMacAddress = other.mLocalMacAddress;
        mAdapterSlot = other.mAdapterSlot;
        return *this;
    }

    void setNull()
    {
        mGatewayInstanceId.setNull();
        mPublicSshKey.setNull();
        mPrivateSshKey.setNull();
        mCloudProvider.setNull();
        mCloudProfile.setNull();
        mCloudPublicIp.setNull();
        mCloudSecondaryPublicIp.setNull();
        memset(&mCloudMacAddress, 0, sizeof(mCloudMacAddress));
        memset(&mLocalMacAddress, 0, sizeof(mLocalMacAddress));
        mAdapterSlot = -1;
    }
};

class CloudNetwork;

HRESULT startCloudGateway(ComPtr<IVirtualBox> virtualBox, ComPtr<ICloudNetwork> network, GatewayInfo& pGateways);
HRESULT stopCloudGateway(ComPtr<IVirtualBox> virtualBox, GatewayInfo& gateways);
HRESULT generateKeys(GatewayInfo& gateways);

#endif /* !MAIN_INCLUDED_CloudGateway_h */

