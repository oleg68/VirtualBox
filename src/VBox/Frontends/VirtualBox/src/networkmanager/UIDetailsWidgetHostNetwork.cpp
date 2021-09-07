/* $Id$ */
/** @file
 * VBox Qt GUI - UIDetailsWidgetHostNetwork class implementation.
 */

/*
 * Copyright (C) 2009-2021 Oracle Corporation
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
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExpValidator>
#include <QStyleOption>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "QILineEdit.h"
#include "QITabWidget.h"
#include "UIIconPool.h"
#include "UIDetailsWidgetHostNetwork.h"
#include "UIMessageCenter.h"
#include "UINetworkManagerUtils.h"
#include "UINotificationCenter.h"

/* Other VBox includes: */
#include "iprt/assert.h"
#include "iprt/cidr.h"


UIDetailsWidgetHostNetwork::UIDetailsWidgetHostNetwork(EmbedTo enmEmbedding, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_pTabWidget(0)
    , m_pButtonAutomatic(0)
    , m_pButtonManual(0)
    , m_pLabelIPv4(0), m_pEditorIPv4(0)
    , m_pLabelNMv4(0), m_pEditorNMv4(0)
    , m_pLabelIPv6(0), m_pEditorIPv6(0)
    , m_pLabelNMv6(0), m_pEditorNMv6(0)
    , m_pButtonBoxInterface(0)
    , m_pCheckBoxDHCP(0)
    , m_pLabelDHCPAddress(0), m_pEditorDHCPAddress(0)
    , m_pLabelDHCPMask(0), m_pEditorDHCPMask(0)
    , m_pLabelDHCPLowerAddress(0), m_pEditorDHCPLowerAddress(0)
    , m_pLabelDHCPUpperAddress(0), m_pEditorDHCPUpperAddress(0)
    , m_pButtonBoxServer(0)
{
    prepare();
}

void UIDetailsWidgetHostNetwork::setData(const UIDataHostNetwork &data)
{
    /* Cache old/new data: */
    m_oldData = data;
    m_newData = m_oldData;

    /* Load 'Interface' data: */
    loadDataForInterface();
    /* Load 'DHCP server' data: */
    loadDataForDHCPServer();
}

bool UIDetailsWidgetHostNetwork::revalidate() const
{
    /* Validate 'Interface' tab content: */
    if (   m_newData.m_interface.m_fDHCPEnabled
        && !m_newData.m_dhcpserver.m_fEnabled)
    {
        UINotificationMessage::warnAboutDHCPServerIsNotEnabled(m_newData.m_interface.m_strName);
        return false;
    }
    if (   !m_newData.m_interface.m_fDHCPEnabled
        && !m_newData.m_interface.m_strAddress.trimmed().isEmpty()
        && (   !RTNetIsIPv4AddrStr(m_newData.m_interface.m_strAddress.toUtf8().constData())
            || RTNetStrIsIPv4AddrAny(m_newData.m_interface.m_strAddress.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidIPv4Address(m_newData.m_interface.m_strName);
        return false;
    }
    if (   !m_newData.m_interface.m_fDHCPEnabled
        && !m_newData.m_interface.m_strMask.trimmed().isEmpty()
        && (   !RTNetIsIPv4AddrStr(m_newData.m_interface.m_strMask.toUtf8().constData())
            || RTNetStrIsIPv4AddrAny(m_newData.m_interface.m_strMask.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidIPv4Mask(m_newData.m_interface.m_strName);
        return false;
    }
    if (    !m_newData.m_interface.m_fDHCPEnabled
        && m_newData.m_interface.m_fSupportedIPv6
        && !m_newData.m_interface.m_strAddress6.trimmed().isEmpty()
        && (   !RTNetIsIPv6AddrStr(m_newData.m_interface.m_strAddress6.toUtf8().constData())
            || RTNetStrIsIPv6AddrAny(m_newData.m_interface.m_strAddress6.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidIPv6Address(m_newData.m_interface.m_strName);
        return false;
    }
    bool fIsMaskPrefixLengthNumber = false;
    const int iMaskPrefixLength = m_newData.m_interface.m_strPrefixLength6.trimmed().toInt(&fIsMaskPrefixLengthNumber);
    if (   !m_newData.m_interface.m_fDHCPEnabled
        && m_newData.m_interface.m_fSupportedIPv6
        && (   !fIsMaskPrefixLengthNumber
            || iMaskPrefixLength < 0
            || iMaskPrefixLength > 128))
    {
        UINotificationMessage::warnAboutInvalidIPv6PrefixLength(m_newData.m_interface.m_strName);
        return false;
    }

    /* Validate 'DHCP server' tab content: */
    if (   m_newData.m_dhcpserver.m_fEnabled
        && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strAddress.toUtf8().constData())
            || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strAddress.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidDHCPServerAddress(m_newData.m_interface.m_strName);
        return false;
    }
    if (   m_newData.m_dhcpserver.m_fEnabled
        && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strMask.toUtf8().constData())
            || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strMask.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidDHCPServerMask(m_newData.m_interface.m_strName);
        return false;
    }
    if (   m_newData.m_dhcpserver.m_fEnabled
        && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strLowerAddress.toUtf8().constData())
            || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strLowerAddress.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidDHCPServerLowerAddress(m_newData.m_interface.m_strName);
        return false;
    }
    if (   m_newData.m_dhcpserver.m_fEnabled
        && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strUpperAddress.toUtf8().constData())
            || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strUpperAddress.toUtf8().constData())))
    {
        UINotificationMessage::warnAboutInvalidDHCPServerUpperAddress(m_newData.m_interface.m_strName);
        return false;
    }

    /* True by default: */
    return true;
}

void UIDetailsWidgetHostNetwork::updateButtonStates()
{
//    if (m_oldData != m_newData)
//        printf("Interface: %s, %s, %s, %s;  DHCP server: %d, %s, %s, %s, %s\n",
//               m_newData.m_interface.m_strAddress.toUtf8().constData(),
//               m_newData.m_interface.m_strMask.toUtf8().constData(),
//               m_newData.m_interface.m_strAddress6.toUtf8().constData(),
//               m_newData.m_interface.m_strPrefixLength6.toUtf8().constData(),
//               (int)m_newData.m_dhcpserver.m_fEnabled,
//               m_newData.m_dhcpserver.m_strAddress.toUtf8().constData(),
//               m_newData.m_dhcpserver.m_strMask.toUtf8().constData(),
//               m_newData.m_dhcpserver.m_strLowerAddress.toUtf8().constData(),
//               m_newData.m_dhcpserver.m_strUpperAddress.toUtf8().constData());

    /* Update 'Apply' / 'Reset' button states: */
    if (m_pButtonBoxInterface)
    {
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setEnabled(m_oldData != m_newData);
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setEnabled(m_oldData != m_newData);
    }
    if (m_pButtonBoxServer)
    {
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setEnabled(m_oldData != m_newData);
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setEnabled(m_oldData != m_newData);
    }

    /* Notify listeners as well: */
    emit sigDataChanged(m_oldData != m_newData);
}

void UIDetailsWidgetHostNetwork::retranslateUi()
{
    /* Translate tab-widget: */
    if (m_pTabWidget)
    {
        m_pTabWidget->setTabText(0, tr("&Adapter"));
        m_pTabWidget->setTabText(1, tr("&DHCP Server"));
    }

    /* Translate 'Interface' tab content: */
    if (m_pButtonAutomatic)
        m_pButtonAutomatic->setText(tr("Configure Adapter &Automatically"));
    if (m_pButtonManual)
        m_pButtonManual->setText(tr("Configure Adapter &Manually"));
    if (m_pLabelIPv4)
        m_pLabelIPv4->setText(tr("&IPv4 Address:"));
    if (m_pEditorIPv4)
        m_pEditorIPv4->setToolTip(tr("Holds the host IPv4 address for this adapter."));
    if (m_pLabelNMv4)
        m_pLabelNMv4->setText(tr("IPv4 Network &Mask:"));
    if (m_pEditorNMv4)
        m_pEditorNMv4->setToolTip(tr("Holds the host IPv4 network mask for this adapter."));
    if (m_pLabelIPv6)
        m_pLabelIPv6->setText(tr("I&Pv6 Address:"));
    if (m_pEditorIPv6)
        m_pEditorIPv6->setToolTip(tr("Holds the host IPv6 address for this adapter if IPv6 is supported."));
    if (m_pLabelNMv6)
        m_pLabelNMv6->setText(tr("IPv6 Prefix &Length:"));
    if (m_pEditorNMv6)
        m_pEditorNMv6->setToolTip(tr("Holds the host IPv6 prefix length for this adapter if IPv6 is supported."));
    if (m_pButtonBoxInterface)
    {
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setText(tr("Reset"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setText(tr("Apply"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setShortcut(Qt::Key_Escape);
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setShortcut(QString("Ctrl+Return"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setStatusTip(tr("Reset changes in current interface details"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setStatusTip(tr("Apply changes in current interface details"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->
            setToolTip(tr("Reset Changes (%1)").arg(m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->shortcut().toString()));
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->
            setToolTip(tr("Apply Changes (%1)").arg(m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->shortcut().toString()));
    }

    /* Translate 'DHCP server' tab content: */
    if (m_pCheckBoxDHCP)
    {
        m_pCheckBoxDHCP->setText(tr("&Enable Server"));
        m_pCheckBoxDHCP->setToolTip(tr("When checked, the DHCP Server will be enabled for this network on machine start-up."));
    }
    if (m_pLabelDHCPAddress)
        m_pLabelDHCPAddress->setText(tr("Server Add&ress:"));
    if (m_pEditorDHCPAddress)
        m_pEditorDHCPAddress->setToolTip(tr("Holds the address of the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pLabelDHCPMask)
        m_pLabelDHCPMask->setText(tr("Server &Mask:"));
    if (m_pEditorDHCPMask)
        m_pEditorDHCPMask->setToolTip(tr("Holds the network mask of the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pLabelDHCPLowerAddress)
        m_pLabelDHCPLowerAddress->setText(tr("&Lower Address Bound:"));
    if (m_pEditorDHCPLowerAddress)
        m_pEditorDHCPLowerAddress->setToolTip(tr("Holds the lower address bound offered by the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pLabelDHCPUpperAddress)
        m_pLabelDHCPUpperAddress->setText(tr("&Upper Address Bound:"));
    if (m_pEditorDHCPUpperAddress)
        m_pEditorDHCPUpperAddress->setToolTip(tr("Holds the upper address bound offered by the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pButtonBoxServer)
    {
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setText(tr("Reset"));
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setText(tr("Apply"));
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setShortcut(Qt::Key_Escape);
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setShortcut(QString("Ctrl+Return"));
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setStatusTip(tr("Reset changes in current DHCP server details"));
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setStatusTip(tr("Apply changes in current DHCP server details"));
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->
            setToolTip(tr("Reset Changes (%1)").arg(m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->shortcut().toString()));
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->
            setToolTip(tr("Apply Changes (%1)").arg(m_pButtonBoxServer->button(QDialogButtonBox::Ok)->shortcut().toString()));
    }
}

void UIDetailsWidgetHostNetwork::sltToggledButtonAutomatic(bool fChecked)
{
    m_newData.m_interface.m_fDHCPEnabled = fChecked;
    loadDataForInterface();
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltToggledButtonManual(bool fChecked)
{
    m_newData.m_interface.m_fDHCPEnabled = !fChecked;
    loadDataForInterface();
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedIPv4(const QString &strText)
{
    m_newData.m_interface.m_strAddress = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedNMv4(const QString &strText)
{
    m_newData.m_interface.m_strMask = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedIPv6(const QString &strText)
{
    m_newData.m_interface.m_strAddress6 = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedNMv6(const QString &strText)
{
    m_newData.m_interface.m_strPrefixLength6 = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltStatusChangedServer(int iChecked)
{
    m_newData.m_dhcpserver.m_fEnabled = (bool)iChecked;
    loadDataForDHCPServer();
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedAddress(const QString &strText)
{
    m_newData.m_dhcpserver.m_strAddress = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedMask(const QString &strText)
{
    m_newData.m_dhcpserver.m_strMask = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedLowerAddress(const QString &strText)
{
    m_newData.m_dhcpserver.m_strLowerAddress = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedUpperAddress(const QString &strText)
{
    m_newData.m_dhcpserver.m_strUpperAddress = strText;
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltHandleButtonBoxClick(QAbstractButton *pButton)
{
    /* Make sure button-boxes exist: */
    if (!m_pButtonBoxInterface || !m_pButtonBoxServer)
        return;

    /* Disable buttons first of all: */
    m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setEnabled(false);
    m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setEnabled(false);
    m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setEnabled(false);

    /* Compare with known buttons: */
    if (   pButton == m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)
        || pButton == m_pButtonBoxServer->button(QDialogButtonBox::Cancel))
        emit sigDataChangeRejected();
    else
    if (   pButton == m_pButtonBoxInterface->button(QDialogButtonBox::Ok)
        || pButton == m_pButtonBoxServer->button(QDialogButtonBox::Ok))
        emit sigDataChangeAccepted();
}

void UIDetailsWidgetHostNetwork::prepare()
{
    /* Prepare this: */
    prepareThis();

    /* Apply language settings: */
    retranslateUi();

    /* Update button states finally: */
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::prepareThis()
{
    /* Create layout: */
    new QVBoxLayout(this);
    if (layout())
    {
        /* Configure layout: */
        layout()->setContentsMargins(0, 0, 0, 0);

        /* Prepare tab-widget: */
        prepareTabWidget();
    }
}

void UIDetailsWidgetHostNetwork::prepareTabWidget()
{
    /* Create tab-widget: */
    m_pTabWidget = new QITabWidget(this);
    if (m_pTabWidget)
    {
        /* Prepare 'Interface' tab: */
        prepareTabInterface();
        /* Prepare 'DHCP server' tab: */
        prepareTabDHCPServer();

        /* Add into layout: */
        layout()->addWidget(m_pTabWidget);
    }
}

void UIDetailsWidgetHostNetwork::prepareTabInterface()
{
    /* Prepare 'Interface' tab: */
    QWidget *pTabInterface = new QWidget(m_pTabWidget);
    if (pTabInterface)
    {
        /* Prepare 'Interface' layout: */
        QGridLayout *pLayoutInterface = new QGridLayout(pTabInterface);
        if (pLayoutInterface)
        {
#ifdef VBOX_WS_MAC
            pLayoutInterface->setSpacing(10);
            pLayoutInterface->setContentsMargins(10, 10, 10, 10);
#endif

            /* Prepare automatic interface configuration layout: */
            QHBoxLayout *pLayoutAutomatic = new QHBoxLayout;
            if (pLayoutAutomatic)
            {
                pLayoutAutomatic->setContentsMargins(0, 0, 0, 0);

                /* Prepare automatic interface configuration radio-button: */
                m_pButtonAutomatic = new QRadioButton(pTabInterface);
                if (m_pButtonAutomatic)
                {
                    connect(m_pButtonAutomatic, &QRadioButton::toggled,
                            this, &UIDetailsWidgetHostNetwork::sltToggledButtonAutomatic);
                    pLayoutAutomatic->addWidget(m_pButtonAutomatic);
                }

                pLayoutInterface->addLayout(pLayoutAutomatic, 0, 0, 1, 3);
#ifdef VBOX_WS_MAC
                pLayoutInterface->setRowMinimumHeight(0, 22);
#endif
            }

            /* Prepare manual interface configuration layout: */
            QHBoxLayout *pLayoutManual = new QHBoxLayout;
            if (pLayoutManual)
            {
                pLayoutManual->setContentsMargins(0, 0, 0, 0);

                /* Prepare manual interface configuration radio-button: */
                m_pButtonManual = new QRadioButton(pTabInterface);
                if (m_pButtonManual)
                {
                    connect(m_pButtonManual, &QRadioButton::toggled,
                            this, &UIDetailsWidgetHostNetwork::sltToggledButtonManual);
                    pLayoutManual->addWidget(m_pButtonManual);
                }

                pLayoutInterface->addLayout(pLayoutManual, 1, 0, 1, 3);
#ifdef VBOX_WS_MAC
                pLayoutInterface->setRowMinimumHeight(1, 22);
#endif
            }

            /* Prepare IPv4 address label: */
            m_pLabelIPv4 = new QLabel(pTabInterface);
            if (m_pLabelIPv4)
            {
                m_pLabelIPv4->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutInterface->addWidget(m_pLabelIPv4, 2, 1);
            }
            /* Prepare IPv4 layout: */
            QHBoxLayout *pLayoutIPv4 = new QHBoxLayout;
            if (pLayoutIPv4)
            {
                pLayoutIPv4->setContentsMargins(0, 0, 0, 0);

                /* Prepare IPv4 address editor: */
                m_pEditorIPv4 = new QILineEdit(pTabInterface);
                if (m_pEditorIPv4)
                {
                    m_pLabelIPv4->setBuddy(m_pEditorIPv4);
                    connect(m_pEditorIPv4, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedIPv4);

                    pLayoutIPv4->addWidget(m_pEditorIPv4);
                }

                pLayoutInterface->addLayout(pLayoutIPv4, 2, 2);
            }

            /* Prepare NMv4 network mask label: */
            m_pLabelNMv4 = new QLabel(pTabInterface);
            if (m_pLabelNMv4)
            {
                m_pLabelNMv4->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutInterface->addWidget(m_pLabelNMv4, 3, 1);
            }
            /* Prepare NMv4 layout: */
            QHBoxLayout *pLayoutNMv4 = new QHBoxLayout;
            if (pLayoutNMv4)
            {
                pLayoutNMv4->setContentsMargins(0, 0, 0, 0);

                /* Prepare NMv4 network mask editor: */
                m_pEditorNMv4 = new QILineEdit(pTabInterface);
                if (m_pEditorNMv4)
                {
                    m_pLabelNMv4->setBuddy(m_pEditorNMv4);
                    connect(m_pEditorNMv4, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedNMv4);

                    pLayoutNMv4->addWidget(m_pEditorNMv4);
                }

                pLayoutInterface->addLayout(pLayoutNMv4, 3, 2);
            }

            /* Prepare IPv6 address label: */
            m_pLabelIPv6 = new QLabel(pTabInterface);
            if (m_pLabelIPv6)
            {
                m_pLabelIPv6->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutInterface->addWidget(m_pLabelIPv6, 4, 1);
            }
            /* Prepare IPv6 layout: */
            QHBoxLayout *pLayoutIPv6 = new QHBoxLayout;
            if (pLayoutIPv6)
            {
                pLayoutIPv6->setContentsMargins(0, 0, 0, 0);

                /* Prepare IPv6 address editor: */
                m_pEditorIPv6 = new QILineEdit(pTabInterface);
                if (m_pEditorIPv6)
                {
                    m_pLabelIPv6->setBuddy(m_pEditorIPv6);
                    connect(m_pEditorIPv6, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedIPv6);

                    pLayoutIPv6->addWidget(m_pEditorIPv6);
                }

                pLayoutInterface->addLayout(pLayoutIPv6, 4, 2);
            }

            /* Prepare NMv6 network mask label: */
            m_pLabelNMv6 = new QLabel(pTabInterface);
            if (m_pLabelNMv6)
            {
                m_pLabelNMv6->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutInterface->addWidget(m_pLabelNMv6, 5, 1);
            }
            /* Prepare NMv6 layout: */
            QHBoxLayout *pLayoutNMv6 = new QHBoxLayout;
            if (pLayoutNMv6)
            {
                pLayoutNMv6->setContentsMargins(0, 0, 0, 0);

                /* Prepare NMv6 network mask editor: */
                m_pEditorNMv6 = new QILineEdit(pTabInterface);
                if (m_pEditorNMv6)
                {
                    m_pLabelNMv6->setBuddy(m_pEditorNMv6);
                    connect(m_pEditorNMv6, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedNMv6);

                    pLayoutNMv6->addWidget(m_pEditorNMv6);
                }

                pLayoutInterface->addLayout(pLayoutNMv6, 5, 2);
            }

            /* Prepare indent: */
            QStyleOption options;
            options.initFrom(m_pButtonManual);
            const int iWidth = m_pButtonManual->style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &options, m_pButtonManual) +
                               m_pButtonManual->style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, &options, m_pButtonManual) -
                               pLayoutInterface->spacing() - 1;
            QSpacerItem *pSpacer1 = new QSpacerItem(iWidth, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
            if (pSpacer1)
                pLayoutInterface->addItem(pSpacer1, 2, 0, 4);
            /* Prepare stretch: */
            QSpacerItem *pSpacer2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            if (pSpacer2)
                pLayoutInterface->addItem(pSpacer2, 6, 0, 1, 3);

            /* If parent embedded into stack: */
            if (m_enmEmbedding == EmbedTo_Stack)
            {
                /* Prepare button-box: */
                m_pButtonBoxInterface = new QIDialogButtonBox(pTabInterface);
                if (m_pButtonBoxInterface)
                {
                    m_pButtonBoxInterface->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                    connect(m_pButtonBoxInterface, &QIDialogButtonBox::clicked, this, &UIDetailsWidgetHostNetwork::sltHandleButtonBoxClick);

                    pLayoutInterface->addWidget(m_pButtonBoxInterface, 7, 0, 1, 3);
                }
            }
        }

        m_pTabWidget->addTab(pTabInterface, QString());
    }
}

void UIDetailsWidgetHostNetwork::prepareTabDHCPServer()
{
    /* Prepare 'DHCP server' tab: */
    QWidget *pTabDHCPServer = new QWidget(m_pTabWidget);
    if (pTabDHCPServer)
    {
        /* Prepare 'DHCP server' layout: */
        QGridLayout *pLayoutDHCPServer = new QGridLayout(pTabDHCPServer);
        if (pLayoutDHCPServer)
        {
#ifdef VBOX_WS_MAC
            pLayoutDHCPServer->setSpacing(10);
            pLayoutDHCPServer->setContentsMargins(10, 10, 10, 10);
#endif

            /* Prepare DHCP server status check-box: */
            m_pCheckBoxDHCP = new QCheckBox(pTabDHCPServer);
            if (m_pCheckBoxDHCP)
            {
                connect(m_pCheckBoxDHCP, &QCheckBox::stateChanged,
                        this, &UIDetailsWidgetHostNetwork::sltStatusChangedServer);
                pLayoutDHCPServer->addWidget(m_pCheckBoxDHCP, 0, 0, 1, 2);
#ifdef VBOX_WS_MAC
                pLayoutDHCPServer->setRowMinimumHeight(0, 22);
#endif
            }

            /* Prepare DHCP address label: */
            m_pLabelDHCPAddress = new QLabel(pTabDHCPServer);
            if (m_pLabelDHCPAddress)
            {
                m_pLabelDHCPAddress->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutDHCPServer->addWidget(m_pLabelDHCPAddress, 1, 1);
            }
            /* Prepare DHCP address layout: */
            QHBoxLayout *pLayoutDHCPAddress = new QHBoxLayout;
            if (pLayoutDHCPAddress)
            {
                pLayoutDHCPAddress->setContentsMargins(0, 0, 0, 0);

                /* Prepare DHCP address editor: */
                m_pEditorDHCPAddress = new QILineEdit(pTabDHCPServer);
                if (m_pEditorDHCPAddress)
                {
                    m_pLabelDHCPAddress->setBuddy(m_pEditorDHCPAddress);
                    connect(m_pEditorDHCPAddress, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedAddress);

                    pLayoutDHCPAddress->addWidget(m_pEditorDHCPAddress);
                }

                pLayoutDHCPServer->addLayout(pLayoutDHCPAddress, 1, 2);
            }

            /* Prepare DHCP network mask label: */
            m_pLabelDHCPMask = new QLabel(pTabDHCPServer);
            if (m_pLabelDHCPMask)
            {
                m_pLabelDHCPMask->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutDHCPServer->addWidget(m_pLabelDHCPMask, 2, 1);
            }
            /* Prepare DHCP mask layout: */
            QHBoxLayout *pLayoutDHCPMask = new QHBoxLayout;
            if (pLayoutDHCPMask)
            {
                pLayoutDHCPMask->setContentsMargins(0, 0, 0, 0);

                /* Prepare DHCP network mask editor: */
                m_pEditorDHCPMask = new QILineEdit(pTabDHCPServer);
                if (m_pEditorDHCPMask)
                {
                    m_pLabelDHCPMask->setBuddy(m_pEditorDHCPMask);
                    connect(m_pEditorDHCPMask, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedMask);

                    pLayoutDHCPMask->addWidget(m_pEditorDHCPMask);
                }

                pLayoutDHCPServer->addLayout(pLayoutDHCPMask, 2, 2);
            }

            /* Prepare DHCP lower address label: */
            m_pLabelDHCPLowerAddress = new QLabel(pTabDHCPServer);
            if (m_pLabelDHCPLowerAddress)
            {
                m_pLabelDHCPLowerAddress->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutDHCPServer->addWidget(m_pLabelDHCPLowerAddress, 3, 1);
            }
            /* Prepare DHCP lower address layout: */
            QHBoxLayout *pLayoutDHCPLowerAddress = new QHBoxLayout;
            if (pLayoutDHCPLowerAddress)
            {
                pLayoutDHCPLowerAddress->setContentsMargins(0, 0, 0, 0);

                /* Prepare DHCP lower address editor: */
                m_pEditorDHCPLowerAddress = new QILineEdit(pTabDHCPServer);
                if (m_pEditorDHCPLowerAddress)
                {
                    m_pLabelDHCPLowerAddress->setBuddy(m_pEditorDHCPLowerAddress);
                    connect(m_pEditorDHCPLowerAddress, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedLowerAddress);

                    pLayoutDHCPLowerAddress->addWidget(m_pEditorDHCPLowerAddress);
                }

                pLayoutDHCPServer->addLayout(pLayoutDHCPLowerAddress, 3, 2);
            }

            /* Prepare DHCP upper address label: */
            m_pLabelDHCPUpperAddress = new QLabel(pTabDHCPServer);
            if (m_pLabelDHCPUpperAddress)
            {
                m_pLabelDHCPUpperAddress->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                pLayoutDHCPServer->addWidget(m_pLabelDHCPUpperAddress, 4, 1);
            }
            /* Prepare DHCP upper address layout: */
            QHBoxLayout *pLayoutDHCPUpperAddress = new QHBoxLayout;
            if (pLayoutDHCPUpperAddress)
            {
                pLayoutDHCPUpperAddress->setContentsMargins(0, 0, 0, 0);

                /* Prepare DHCP upper address editor: */
                m_pEditorDHCPUpperAddress = new QILineEdit(pTabDHCPServer);
                if (m_pEditorDHCPUpperAddress)
                {
                    m_pLabelDHCPUpperAddress->setBuddy(m_pEditorDHCPUpperAddress);
                    connect(m_pEditorDHCPUpperAddress, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedUpperAddress);

                    pLayoutDHCPUpperAddress->addWidget(m_pEditorDHCPUpperAddress);
                }

                pLayoutDHCPServer->addLayout(pLayoutDHCPUpperAddress, 4, 2);
            }

            /* Prepare indent: */
            QStyleOption options;
            options.initFrom(m_pCheckBoxDHCP);
            const int iWidth = m_pCheckBoxDHCP->style()->pixelMetric(QStyle::PM_IndicatorWidth, &options, m_pCheckBoxDHCP) +
                               m_pCheckBoxDHCP->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &options, m_pCheckBoxDHCP) -
                               pLayoutDHCPServer->spacing() - 1;
            QSpacerItem *pSpacer1 = new QSpacerItem(iWidth, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
            if (pSpacer1)
                pLayoutDHCPServer->addItem(pSpacer1, 1, 0, 4);
            /* Prepare stretch: */
            QSpacerItem *pSpacer2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            if (pSpacer2)
                pLayoutDHCPServer->addItem(pSpacer2, 5, 0, 1, 3);

            /* If parent embedded into stack: */
            if (m_enmEmbedding == EmbedTo_Stack)
            {
                /* Prepare button-box: */
                m_pButtonBoxServer = new QIDialogButtonBox(pTabDHCPServer);
                if (m_pButtonBoxServer)
                {
                    m_pButtonBoxServer->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                    connect(m_pButtonBoxServer, &QIDialogButtonBox::clicked, this, &UIDetailsWidgetHostNetwork::sltHandleButtonBoxClick);

                    pLayoutDHCPServer->addWidget(m_pButtonBoxServer, 6, 0, 1, 3);
                }
            }
        }

        m_pTabWidget->addTab(pTabDHCPServer, QString());
    }
}

void UIDetailsWidgetHostNetwork::loadDataForInterface()
{
    /* Check whether interface exists and configurable: */
    const bool fIsInterfaceExists = m_newData.m_interface.m_fExists;
    const bool fIsInterfaceConfigurable = !m_newData.m_interface.m_fDHCPEnabled;

    /* Toggle radio-buttons availability: */
    if (m_pButtonAutomatic)
        m_pButtonAutomatic->setEnabled(fIsInterfaceExists);
    if (m_pButtonManual)
        m_pButtonManual->setEnabled(fIsInterfaceExists);

    /* Toggle IPv4 & IPv6 interface fields availability: */
    if (m_pLabelIPv4)
        m_pLabelIPv4->setEnabled(fIsInterfaceExists && fIsInterfaceConfigurable);
    if (m_pLabelNMv4)
        m_pLabelNMv4->setEnabled(fIsInterfaceExists && fIsInterfaceConfigurable);
    if (m_pEditorIPv4)
        m_pEditorIPv4->setEnabled(fIsInterfaceExists && fIsInterfaceConfigurable);
    if (m_pEditorNMv4)
        m_pEditorNMv4->setEnabled(fIsInterfaceExists && fIsInterfaceConfigurable);

    /* Load IPv4 interface fields: */
    if (m_pButtonAutomatic)
        m_pButtonAutomatic->setChecked(!fIsInterfaceConfigurable);
    if (m_pButtonManual)
        m_pButtonManual->setChecked(fIsInterfaceConfigurable);
    if (m_pEditorIPv4)
        m_pEditorIPv4->setText(m_newData.m_interface.m_strAddress);
    if (m_pEditorNMv4)
        m_pEditorNMv4->setText(m_newData.m_interface.m_strMask);

    /* Toggle IPv6 interface fields availability: */
    const bool fIsIpv6Configurable = fIsInterfaceConfigurable && m_newData.m_interface.m_fSupportedIPv6;
    if (m_pLabelIPv6)
        m_pLabelIPv6->setEnabled(fIsInterfaceExists && fIsIpv6Configurable);
    if (m_pLabelNMv6)
        m_pLabelNMv6->setEnabled(fIsInterfaceExists && fIsIpv6Configurable);
    if (m_pEditorIPv6)
        m_pEditorIPv6->setEnabled(fIsInterfaceExists && fIsIpv6Configurable);
    if (m_pEditorNMv6)
        m_pEditorNMv6->setEnabled(fIsInterfaceExists && fIsIpv6Configurable);

    /* Load IPv6 interface fields: */
    if (m_pEditorIPv6)
        m_pEditorIPv6->setText(m_newData.m_interface.m_strAddress6);
    if (m_pEditorNMv6)
        m_pEditorNMv6->setText(m_newData.m_interface.m_strPrefixLength6);
}

void UIDetailsWidgetHostNetwork::loadDataForDHCPServer()
{
    /* Check whether interface exists and DHCP server available: */
    const bool fIsInterfaceExists = m_newData.m_interface.m_fExists;
    const bool fIsDHCPServerEnabled = m_newData.m_dhcpserver.m_fEnabled;

    /* Toggle check-box availability: */
    m_pCheckBoxDHCP->setEnabled(fIsInterfaceExists);

    /* Toggle DHCP server fields availability: */
    if (m_pLabelDHCPAddress)
        m_pLabelDHCPAddress->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pLabelDHCPMask)
        m_pLabelDHCPMask->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pLabelDHCPLowerAddress)
        m_pLabelDHCPLowerAddress->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pLabelDHCPUpperAddress)
        m_pLabelDHCPUpperAddress->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pEditorDHCPAddress)
        m_pEditorDHCPAddress->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pEditorDHCPMask)
        m_pEditorDHCPMask->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pEditorDHCPLowerAddress)
        m_pEditorDHCPLowerAddress->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);
    if (m_pEditorDHCPUpperAddress)
        m_pEditorDHCPUpperAddress->setEnabled(fIsInterfaceExists && fIsDHCPServerEnabled);

    /* Load DHCP server fields: */
    if (m_pCheckBoxDHCP)
        m_pCheckBoxDHCP->setChecked(fIsDHCPServerEnabled);
    if (m_pEditorDHCPAddress)
        m_pEditorDHCPAddress->setText(m_newData.m_dhcpserver.m_strAddress);
    if (m_pEditorDHCPMask)
        m_pEditorDHCPMask->setText(m_newData.m_dhcpserver.m_strMask);
    if (m_pEditorDHCPLowerAddress)
        m_pEditorDHCPLowerAddress->setText(m_newData.m_dhcpserver.m_strLowerAddress);
    if (m_pEditorDHCPUpperAddress)
        m_pEditorDHCPUpperAddress->setText(m_newData.m_dhcpserver.m_strUpperAddress);

    /* Invent default values if server was enabled
     * but at least one current value is invalid: */
    if (   fIsDHCPServerEnabled
        && m_pEditorDHCPAddress
        && m_pEditorDHCPMask
        && m_pEditorDHCPLowerAddress
        && m_pEditorDHCPUpperAddress
        && (   m_pEditorDHCPAddress->text().isEmpty()
            || m_pEditorDHCPAddress->text() == "0.0.0.0"
            || m_pEditorDHCPMask->text().isEmpty()
            || m_pEditorDHCPMask->text() == "0.0.0.0"
            || m_pEditorDHCPLowerAddress->text().isEmpty()
            || m_pEditorDHCPLowerAddress->text() == "0.0.0.0"
            || m_pEditorDHCPUpperAddress->text().isEmpty()
            || m_pEditorDHCPUpperAddress->text() == "0.0.0.0"))
    {
        const QStringList &proposal = makeDhcpServerProposal(m_oldData.m_interface.m_strAddress,
                                                             m_oldData.m_interface.m_strMask);
        m_pEditorDHCPAddress->setText(proposal.at(0));
        m_pEditorDHCPMask->setText(proposal.at(1));
        m_pEditorDHCPLowerAddress->setText(proposal.at(2));
        m_pEditorDHCPUpperAddress->setText(proposal.at(3));
    }
}
