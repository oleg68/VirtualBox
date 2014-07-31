/** @file
 * VBox Client Session COM Class definition
 */

/*
 * Copyright (C) 2006-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_SESSIONIMPL
#define ____H_SESSIONIMPL

#include "SessionWrap.h"
#include "ConsoleImpl.h"

#ifdef RT_OS_WINDOWS
# include "win/resource.h"
#endif

#ifdef RT_OS_WINDOWS
[threading(free)]
#endif
class ATL_NO_VTABLE Session :
    public SessionWrap
#ifdef RT_OS_WINDOWS
    , public CComCoClass<Session, &CLSID_Session>
#endif
{
public:

    DECLARE_CLASSFACTORY()

    DECLARE_REGISTRY_RESOURCEID(IDR_VIRTUALBOX)

    DECLARE_NOT_AGGREGATABLE(Session)

    DECLARE_EMPTY_CTOR_DTOR(Session)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializers/uninitializers only for internal purposes
    HRESULT init();
    void uninit();

private:

    // Wrapped Isession properties
    HRESULT getState(SessionState_T *aState);
    HRESULT getType(SessionType_T *aType);
    HRESULT getMachine(ComPtr<IMachine> &aMachine);
    HRESULT getConsole(ComPtr<IConsole> &aConsole);

    // Wrapped Isession methods
    HRESULT unlockMachine();
    HRESULT getPID(ULONG *aPid);
    HRESULT getRemoteConsole(ComPtr<IConsole> &aConsole);
#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
    HRESULT assignMachine(const ComPtr<IMachine> &aMachine,
                          LockType_T aLockType,
                          const com::Utf8Str &aTokenId);
#else
    HRESULT assignMachine(const ComPtr<IMachine> &aMachine,
                          LockType_T aLockType,
                          const ComPtr<IToken> &aToken);
#endif /* !VBOX_WITH_GENERIC_SESSION_WATCHER */
    HRESULT assignRemoteMachine(const ComPtr<IMachine> &aMachine,
                                const ComPtr<IConsole> &aConsole);
    HRESULT updateMachineState(MachineState_T aMachineState);
    HRESULT uninitialize();
    HRESULT onNetworkAdapterChange(const ComPtr<INetworkAdapter> &aNetworkAdapter,
                                   BOOL aChangeAdapter);
    HRESULT onSerialPortChange(const ComPtr<ISerialPort> &aSerialPort);
    HRESULT onParallelPortChange(const ComPtr<IParallelPort> &aParallelPort);
    HRESULT onStorageControllerChange();
    HRESULT onMediumChange(const ComPtr<IMediumAttachment> &aMediumAttachment,
                           BOOL aForce);
    HRESULT onStorageDeviceChange(const ComPtr<IMediumAttachment> &aMediumAttachment,
                                  BOOL aRemove,
                                  BOOL aSilent);
    HRESULT onClipboardModeChange(ClipboardMode_T aClipboardMode);
    HRESULT onDnDModeChange(DnDMode_T aDndMode);
    HRESULT onCPUChange(ULONG aCpu,
                        BOOL aAdd);
    HRESULT onCPUExecutionCapChange(ULONG aExecutionCap);
    HRESULT onVRDEServerChange(BOOL aRestart);
    HRESULT onVideoCaptureChange();
    HRESULT onUSBControllerChange();
    HRESULT onSharedFolderChange(BOOL aGlobal);
    HRESULT onUSBDeviceAttach(const ComPtr<IUSBDevice> &aDevice,
                              const ComPtr<IVirtualBoxErrorInfo> &aError,
                              ULONG aMaskedInterfaces);
    HRESULT onUSBDeviceDetach(const com::Guid &aId,
                              const ComPtr<IVirtualBoxErrorInfo> &aError);
    HRESULT onShowWindow(BOOL aCheck,
                         BOOL *aCanShow,
                         LONG64 *aWinId);
    HRESULT onBandwidthGroupChange(const ComPtr<IBandwidthGroup> &aBandwidthGroup);
    HRESULT accessGuestProperty(const com::Utf8Str &aName,
                                const com::Utf8Str &aValue,
                                const com::Utf8Str &aFlags,
                                BOOL aIsSetter,
                                com::Utf8Str &aRetValue,
                                LONG64 *aRetTimestamp,
                                com::Utf8Str &aRetFlags);
    HRESULT enumerateGuestProperties(const com::Utf8Str &aPatterns,
                                     std::vector<com::Utf8Str> &aKeys,
                                     std::vector<com::Utf8Str> &aValues,
                                     std::vector<LONG64> &aTimestamps,
                                     std::vector<com::Utf8Str> &aFlags);
    HRESULT onlineMergeMedium(const ComPtr<IMediumAttachment> &aMediumAttachment,
                              ULONG aSourceIdx,
                              ULONG aTargetIdx,
                              const ComPtr<IProgress> &aProgress);
    HRESULT enableVMMStatistics(BOOL aEnable);
    HRESULT pauseWithReason(Reason_T aReason);
    HRESULT resumeWithReason(Reason_T aReason);
    HRESULT saveStateWithReason(Reason_T aReason,
                                ComPtr<IProgress> &aProgress);
    HRESULT unlockMachine(bool aFinalRelease, bool aFromServer);

    SessionState_T mState;
    SessionType_T mType;

    ComPtr<IInternalMachineControl> mControl;

#ifndef VBOX_COM_INPROC_API_CLIENT
    ComObjPtr<Console> mConsole;
#endif

    ComPtr<IMachine> mRemoteMachine;
    ComPtr<IConsole> mRemoteConsole;

    ComPtr<IVirtualBox> mVirtualBox;

    class ClientTokenHolder;

    ClientTokenHolder *mClientTokenHolder;
};

#endif // !____H_SESSIONIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
