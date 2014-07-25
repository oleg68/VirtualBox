/* $Id$ */
/** @file
 * VBox Qt GUI - UIMachineLogicSeamless class implementation.
 */

/*
 * Copyright (C) 2010-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QDesktopWidget>
#ifndef Q_WS_MAC
# include <QTimer>
#endif /* !Q_WS_MAC */

/* GUI includes: */
#include "VBoxGlobal.h"
#include "UIMessageCenter.h"
#include "UIPopupCenter.h"
#include "UISession.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogicSeamless.h"
#include "UIMachineWindowSeamless.h"
#include "UIMultiScreenLayout.h"
#include "UIShortcutPool.h"
#ifndef Q_WS_MAC
# include "QIMenu.h"
#else /* Q_WS_MAC */
# include "VBoxUtils.h"
#endif /* Q_WS_MAC */

UIMachineLogicSeamless::UIMachineLogicSeamless(QObject *pParent, UISession *pSession)
    : UIMachineLogic(pParent, pSession, UIVisualStateType_Seamless)
#ifndef Q_WS_MAC
    , m_pPopupMenu(0)
#endif /* !Q_WS_MAC */
{
    /* Create multiscreen layout: */
    m_pScreenLayout = new UIMultiScreenLayout(this);
}

UIMachineLogicSeamless::~UIMachineLogicSeamless()
{
    /* Delete multiscreen layout: */
    delete m_pScreenLayout;
}

bool UIMachineLogicSeamless::checkAvailability()
{
    /* Temporary get a machine object: */
    const CMachine &machine = uisession()->session().GetMachine();

    /* Check if there is enough physical memory to enter seamless: */
    if (uisession()->isGuestAdditionsActive())
    {
        quint64 availBits = machine.GetVRAMSize() /* VRAM */ * _1M /* MiB to bytes */ * 8 /* to bits */;
        quint64 usedBits = m_pScreenLayout->memoryRequirements();
        if (availBits < usedBits)
        {
            msgCenter().cannotEnterSeamlessMode(0, 0, 0,
                                                (((usedBits + 7) / 8 + _1M - 1) / _1M) * _1M);
            return false;
        }
    }

    /* Show the info message. */
    const UIShortcut &shortcut =
            gShortcutPool->shortcut(actionPool()->shortcutsExtraDataID(),
                                    actionPool()->action(UIActionIndexRT_M_View_T_Seamless)->shortcutExtraDataID());
    const QString strHotKey = QString("Host+%1").arg(shortcut.toString());
    if (!msgCenter().confirmGoingSeamless(strHotKey))
        return false;

    return true;
}

/** Adjusts guest screen count/size for the machine-logic we have. */
void UIMachineLogicSeamless::maybeAdjustGuestScreenSize()
{
    /* We should rebuild screen-layout: */
    m_pScreenLayout->rebuild();
    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
}

int UIMachineLogicSeamless::hostScreenForGuestScreen(int iScreenId) const
{
    return m_pScreenLayout->hostScreenForGuestScreen(iScreenId);
}

bool UIMachineLogicSeamless::hasHostScreenForGuestScreen(int iScreenId) const
{
    return m_pScreenLayout->hasHostScreenForGuestScreen(iScreenId);
}

void UIMachineLogicSeamless::notifyAbout3DOverlayVisibilityChange(bool)
{
    /* If active machine-window is defined now: */
    if (activeMachineWindow())
    {
        /* Reinstall corresponding popup-stack and make sure it has proper type: */
        popupCenter().hidePopupStack(activeMachineWindow());
        popupCenter().setPopupStackType(activeMachineWindow(), UIPopupStackType_Separate);
        popupCenter().showPopupStack(activeMachineWindow());
    }
}

void UIMachineLogicSeamless::sltCheckForRequestedVisualStateType()
{
    /* Do not try to change visual-state type if machine was not started yet: */
    if (!uisession()->isRunning() && !uisession()->isPaused())
        return;

    /* If 'seamless' visual-state type is no more supported: */
    if (!uisession()->isGuestSupportsSeamless())
    {
        LogRel(("UIMachineLogicSeamless::sltCheckForRequestedVisualStateType: "
                "Leaving 'seamless' as it is no more supported...\n"));
        uisession()->setRequestedVisualState(UIVisualStateType_Seamless);
        uisession()->changeVisualState(UIVisualStateType_Normal);
    }
}

void UIMachineLogicSeamless::sltMachineStateChanged()
{
    /* Call to base-class: */
    UIMachineLogic::sltMachineStateChanged();

    /* If machine-state changed from 'paused' to 'running': */
    if (uisession()->isRunning() && uisession()->wasPaused())
    {
        LogRelFlow(("UIMachineLogicSeamless: "
                    "Machine-state changed from 'paused' to 'running': "
                    "Updating screen-layout...\n"));

        /* Make sure further code will be called just once: */
        uisession()->forgetPreviousMachineState();
        /* We should rebuild screen-layout: */
        m_pScreenLayout->rebuild();
        /* Make sure all machine-window(s) have proper geometry: */
        foreach (UIMachineWindow *pMachineWindow, machineWindows())
            pMachineWindow->showInNecessaryMode();
    }
}

#ifndef Q_WS_MAC
void UIMachineLogicSeamless::sltInvokePopupMenu()
{
    /* Popup main-menu if present: */
    if (m_pPopupMenu && !m_pPopupMenu->isEmpty())
    {
        m_pPopupMenu->popup(activeMachineWindow()->geometry().center());
        QTimer::singleShot(0, m_pPopupMenu, SLOT(sltHighlightFirstAction()));
    }
}
#endif /* !Q_WS_MAC */

void UIMachineLogicSeamless::sltScreenLayoutChanged()
{
    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
}

void UIMachineLogicSeamless::sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo)
{
    LogRelFlow(("UIMachineLogicSeamless: Guest-screen count changed.\n"));

    /* Update multi-screen layout before any window update: */
    if (changeType == KGuestMonitorChangedEventType_Enabled ||
        changeType == KGuestMonitorChangedEventType_Disabled)
        m_pScreenLayout->rebuild();

    /* Call to base-class: */
    UIMachineLogic::sltGuestMonitorChange(changeType, uScreenId, screenGeo);
}

void UIMachineLogicSeamless::sltHostScreenCountChanged()
{
    LogRelFlow(("UIMachineLogicSeamless: Host-screen count changed.\n"));

    /* Update multi-screen layout before any window update: */
    m_pScreenLayout->rebuild();

    /* Call to base-class: */
    UIMachineLogic::sltHostScreenCountChanged();
}

void UIMachineLogicSeamless::prepareActionGroups()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionGroups();

    /* Restrict 'Disable Mouse Integration' action for 'Machine' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuMachine(UIActionRestrictionLevel_Logic,
                                                            RuntimeMenuMachineActionType_MouseIntegration);
    /* Restrict 'Adjust Window', 'Guest Autoresize', 'Status Bar' and 'Resize' actions for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Logic,
                                                         (RuntimeMenuViewActionType)
                                                         (RuntimeMenuViewActionType_AdjustWindow |
                                                          RuntimeMenuViewActionType_GuestAutoresize |
                                                          RuntimeMenuViewActionType_StatusBar |
                                                          RuntimeMenuViewActionType_Resize));

    /* Take care of view-action toggle state: */
    UIAction *pActionSeamless = actionPool()->action(UIActionIndexRT_M_View_T_Seamless);
    if (!pActionSeamless->isChecked())
    {
        pActionSeamless->blockSignals(true);
        pActionSeamless->setChecked(true);
        pActionSeamless->blockSignals(false);
    }
}

void UIMachineLogicSeamless::prepareActionConnections()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionConnections();

    /* Prepare 'View' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToNormal()));
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToFullscreen()));
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToScale()));
}

void UIMachineLogicSeamless::prepareMachineWindows()
{
    /* Do not create machine-window(s) if they created already: */
    if (isMachineWindowsCreated())
        return;

#ifdef Q_WS_MAC
    /* We have to make sure that we are getting the front most process.
     * This is necessary for Qt versions > 4.3.3: */
    darwinSetFrontMostProcess();
#endif /* Q_WS_MAC */

    /* Update the multi-screen layout: */
    m_pScreenLayout->update();

    // TODO: Make this through action-pool.
    m_pScreenLayout->setViewMenu(actionPool()->action(UIActionIndexRT_M_View)->menu());

    /* Create machine-window(s): */
    for (uint cScreenId = 0; cScreenId < session().GetMachine().GetMonitorCount(); ++cScreenId)
        addMachineWindow(UIMachineWindow::create(this, cScreenId));

    /* Listen for frame-buffer resize: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        connect(pMachineWindow, SIGNAL(sigFrameBufferResize()),
                this, SIGNAL(sigFrameBufferResize()));

    /* Connect multi-screen layout change handler: */
    connect(m_pScreenLayout, SIGNAL(sigScreenLayoutChanged()),
            this, SLOT(sltScreenLayoutChanged()));

    /* Mark machine-window(s) created: */
    setMachineWindowsCreated(true);
}

#ifndef Q_WS_MAC
void UIMachineLogicSeamless::prepareMenu()
{
    /* Prepare popup-menu: */
    m_pPopupMenu = new QIMenu;
    AssertPtrReturnVoid(m_pPopupMenu);
    {
        /* Prepare popup-menu: */
        foreach (QMenu *pMenu, actionPool()->menus())
            m_pPopupMenu->addMenu(pMenu);
    }
}
#endif /* !Q_WS_MAC */

#ifndef Q_WS_MAC
void UIMachineLogicSeamless::cleanupMenu()
{
    /* Cleanup popup-menu: */
    delete m_pPopupMenu;
    m_pPopupMenu = 0;
}
#endif /* !Q_WS_MAC */

void UIMachineLogicSeamless::cleanupMachineWindows()
{
    /* Do not destroy machine-window(s) if they destroyed already: */
    if (!isMachineWindowsCreated())
        return;

    /* Mark machine-window(s) destroyed: */
    setMachineWindowsCreated(false);

    /* Destroy machine-window(s): */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        UIMachineWindow::destroy(pMachineWindow);
}

void UIMachineLogicSeamless::cleanupActionConnections()
{
    /* "View" actions disconnections: */
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToNormal()));
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToFullscreen()));
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToScale()));

    /* Call to base-class: */
    UIMachineLogic::cleanupActionConnections();
}

void UIMachineLogicSeamless::cleanupActionGroups()
{
    /* Take care of view-action toggle state: */
    UIAction *pActionSeamless = actionPool()->action(UIActionIndexRT_M_View_T_Seamless);
    if (pActionSeamless->isChecked())
    {
        pActionSeamless->blockSignals(true);
        pActionSeamless->setChecked(false);
        pActionSeamless->blockSignals(false);
    }

    /* Allow 'Disable Mouse Integration' action for 'Machine' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuMachine(UIActionRestrictionLevel_Logic,
                                                            RuntimeMenuMachineActionType_Invalid);
    /* Allow 'Adjust Window', 'Guest Autoresize', 'Status Bar' and 'Resize' actions for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Logic,
                                                         RuntimeMenuViewActionType_Invalid);

    /* Call to base-class: */
    UIMachineLogic::cleanupActionGroups();
}

