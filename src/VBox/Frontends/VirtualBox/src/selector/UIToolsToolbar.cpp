/* $Id$ */
/** @file
 * VBox Qt GUI - UIToolsToolbar class implementation.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QAction>
# include <QButtonGroup>
# include <QLabel>
# include <QStackedLayout>
# include <QToolButton>

/* GUI includes: */
# include "UIActionPoolSelector.h"
# include "UITabBar.h"
# include "UIToolBar.h"
# include "UIToolsToolbar.h"

/* Other VBox includes: */
# include "iprt/assert.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIToolsToolbar::UIToolsToolbar(UIActionPool *pActionPool, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pActionPool(pActionPool)
    , m_pLayoutMain(0)
    , m_pLayoutStacked(0)
    , m_pTabBarMachine(0)
    , m_pToolBar(0)
{
    /* Prepare: */
    prepare();
}

void UIToolsToolbar::sltHandleOpenToolMachine()
{
    /* Acquire sender action: */
    const QAction *pAction = sender() ? qobject_cast<const QAction*>(sender()) : 0;
    /* Acquire action type: */
    const ToolTypeMachine enmType = pAction->property("ToolTypeMachine").value<ToolTypeMachine>();

    /* Get corresponding tab id: */
    QUuid idTab;
    if (m_mapTabIdsMachine.contains(enmType))
        idTab = m_mapTabIdsMachine.value(enmType);
    else
    {
        idTab = m_pTabBarMachine->addTab(pAction->icon(), pAction->text().remove('&'));
        m_mapTabIdsMachine[enmType] = idTab;
    }
    /* And make it active: */
    m_pTabBarMachine->setCurrent(idTab);

    /* Notify listeners: */
    emit sigToolOpenedMachine(enmType);
}

void UIToolsToolbar::sltHandleCloseToolMachine(const QUuid &uuid)
{
    /* If the ID is registered: */
    if (m_mapTabIdsMachine.values().contains(uuid))
    {
        /* Notify listeners: */
        emit sigToolClosedMachine(m_mapTabIdsMachine.key(uuid));

        /* And remove the tab: */
        m_pTabBarMachine->removeTab(uuid);
        m_mapTabIdsMachine.remove(m_mapTabIdsMachine.key(uuid));
    }
}

void UIToolsToolbar::sltHandleToolChosenMachine(const QUuid &uuid)
{
    /* If the ID is registered => Notify listeners: */
    if (m_mapTabIdsMachine.values().contains(uuid))
        emit sigToolOpenedMachine(m_mapTabIdsMachine.key(uuid));
}

void UIToolsToolbar::sltHandleActionToggle()
{
    /* Acquire the sender: */
    UIAction *pAction = sender() ? qobject_cast<UIAction*>(sender()) : 0;
    if (!pAction)
        pAction = m_pActionPool->action(UIActionIndexST_M_Tools_T_Machine);

    /* Handle known actions: */
    if (m_pLayoutStacked)
    {
        if (pAction == m_pActionPool->action(UIActionIndexST_M_Tools_T_Machine))
            m_pLayoutStacked->setCurrentWidget(m_pTabBarMachine);
    }
}

void UIToolsToolbar::prepare()
{
    /* Prepare menu: */
    prepareMenu();
    /* Prepare widgets: */
    prepareWidgets();
}

void UIToolsToolbar::prepareMenu()
{
    /* Configure 'Machine' menu: */
    UIMenu *pMenuMachine = m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine)->menu();
    AssertPtrReturnVoid(pMenuMachine);
    {
        /* Add 'Details' action: */
        pMenuMachine->addAction(m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine_Details));
        connect(m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine_Details), &UIAction::triggered,
                this, &UIToolsToolbar::sltHandleOpenToolMachine);
        m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine_Details)->setProperty("ToolTypeMachine", QVariant::fromValue(ToolTypeMachine_Details));

        /* Add 'Snapshots' action: */
        pMenuMachine->addAction(m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine_Snapshots));
        connect(m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine_Snapshots), &UIAction::triggered,
                this, &UIToolsToolbar::sltHandleOpenToolMachine);
        m_pActionPool->action(UIActionIndexST_M_Tools_M_Machine_Snapshots)->setProperty("ToolTypeMachine", QVariant::fromValue(ToolTypeMachine_Snapshots));
    }

    /* Configure 'Machine' toggle action: */
    m_pActionPool->action(UIActionIndexST_M_Tools_T_Machine)->setMenu(pMenuMachine);
    connect(m_pActionPool->action(UIActionIndexST_M_Tools_T_Machine), &UIAction::toggled,
            this, &UIToolsToolbar::sltHandleActionToggle);
}

void UIToolsToolbar::prepareWidgets()
{
    /* Create main layout: */
    m_pLayoutMain = new QHBoxLayout(this);
    AssertPtrReturnVoid(m_pLayoutMain);
    {
        /* Configure layout: */
        m_pLayoutMain->setContentsMargins(0, 0, 0, 0);
        m_pLayoutMain->setSpacing(10);

        /* Create stacked layout: */
        m_pLayoutStacked = new QStackedLayout(m_pLayoutMain);
        AssertPtrReturnVoid(m_pLayoutStacked);
        {
            /* Create Machine tab-bar: */
            m_pTabBarMachine = new UITabBar;
            AssertPtrReturnVoid(m_pTabBarMachine);
            {
                /* Configure tab-bar: */
                connect(m_pTabBarMachine, &UITabBar::sigTabRequestForClosing,
                        this, &UIToolsToolbar::sltHandleCloseToolMachine);
                connect(m_pTabBarMachine, &UITabBar::sigCurrentTabChanged,
                        this, &UIToolsToolbar::sltHandleToolChosenMachine);

                /* Add into layout: */
                m_pLayoutStacked->addWidget(m_pTabBarMachine);
            }

            /* Add into layout: */
            m_pLayoutMain->addLayout(m_pLayoutStacked);
        }

        /* Create toolbar: */
        m_pToolBar = new UIToolBar;
        AssertPtrReturnVoid(m_pToolBar);
        {
            /* Configure toolbar: */
            m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            // TODO: Get rid of hard-coded stuff:
            const QSize toolBarIconSize = m_pToolBar->iconSize();
            if (toolBarIconSize.width() < 32 || toolBarIconSize.height() < 32)
                m_pToolBar->setIconSize(QSize(32, 32));

            /* Add actions: */
            UIAction *pActionMachine = m_pActionPool->action(UIActionIndexST_M_Tools_T_Machine);
            m_pToolBar->addAction(pActionMachine);
            QToolButton *pNamedMenuToolButton = qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(pActionMachine));
            pNamedMenuToolButton->setPopupMode(QToolButton::InstantPopup);

            /* Add into layout: */
            m_pLayoutMain->addWidget(m_pToolBar);
        }
    }
}

