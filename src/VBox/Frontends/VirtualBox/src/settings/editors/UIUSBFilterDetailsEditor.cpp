/* $Id$ */
/** @file
 * VBox Qt GUI - UIUSBFilterDetailsEditor class implementation.
 */

/*
 * Copyright (C) 2008-2022 Oracle Corporation
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
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIConverter.h"
#include "UIUSBFilterDetailsEditor.h"


UIUSBFilterDetailsEditor::UIUSBFilterDetailsEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI2<QIDialog>(pParent, Qt::Sheet)
    , m_pLabelName(0)
    , m_pEditorName(0)
    , m_pLabelVendorID(0)
    , m_pEditorVendorID(0)
    , m_pLabelProductID(0)
    , m_pEditorProductID(0)
    , m_pLabelRevision(0)
    , m_pEditorRevision(0)
    , m_pLabelManufacturer(0)
    , m_pEditorManufacturer(0)
    , m_pLabelProduct(0)
    , m_pEditorProduct(0)
    , m_pLabelSerialNo(0)
    , m_pEditorSerialNo(0)
    , m_pLabelPort(0)
    , m_pEditorPort(0)
    , m_pLabelRemote(0)
    , m_pComboRemote(0)
    , m_pButtonBox(0)
{
    prepare();
}

void UIUSBFilterDetailsEditor::setName(const QString &strName)
{
    if (m_pEditorName)
        m_pEditorName->setText(strName);
}

QString UIUSBFilterDetailsEditor::name() const
{
    return m_pEditorName ? wiped(m_pEditorName->text()) : QString();
}

void UIUSBFilterDetailsEditor::setVendorID(const QString &strVendorID)
{
    if (m_pEditorVendorID)
        m_pEditorVendorID->setText(strVendorID);
}

QString UIUSBFilterDetailsEditor::vendorID() const
{
    return m_pEditorVendorID ? wiped(m_pEditorVendorID->text()) : QString();
}

void UIUSBFilterDetailsEditor::setProductID(const QString &strProductID)
{
    if (m_pEditorProductID)
        m_pEditorProductID->setText(strProductID);
}

QString UIUSBFilterDetailsEditor::productID() const
{
    return m_pEditorProductID ? wiped(m_pEditorProductID->text()) : QString();
}

void UIUSBFilterDetailsEditor::setRevision(const QString &strRevision)
{
    if (m_pEditorRevision)
        m_pEditorRevision->setText(strRevision);
}

QString UIUSBFilterDetailsEditor::revision() const
{
    return m_pEditorRevision ? wiped(m_pEditorRevision->text()) : QString();
}

void UIUSBFilterDetailsEditor::setManufacturer(const QString &strManufacturer)
{
    if (m_pEditorManufacturer)
        m_pEditorManufacturer->setText(strManufacturer);
}

QString UIUSBFilterDetailsEditor::manufacturer() const
{
    return m_pEditorManufacturer ? wiped(m_pEditorManufacturer->text()) : QString();
}

void UIUSBFilterDetailsEditor::setProduct(const QString &strProduct)
{
    if (m_pEditorProduct)
        m_pEditorProduct->setText(strProduct);
}

QString UIUSBFilterDetailsEditor::product() const
{
    return m_pEditorProduct ? wiped(m_pEditorProduct->text()) : QString();
}

void UIUSBFilterDetailsEditor::setSerialNo(const QString &strSerialNo)
{
    if (m_pEditorSerialNo)
        m_pEditorSerialNo->setText(strSerialNo);
}

QString UIUSBFilterDetailsEditor::serialNo() const
{
    return m_pEditorSerialNo ? wiped(m_pEditorSerialNo->text()) : QString();
}

void UIUSBFilterDetailsEditor::setPort(const QString &strPort)
{
    if (m_pEditorPort)
        m_pEditorPort->setText(strPort);
}

QString UIUSBFilterDetailsEditor::port() const
{
    return m_pEditorPort ? wiped(m_pEditorPort->text()) : QString();
}

void UIUSBFilterDetailsEditor::setRemoteMode(const UIRemoteMode &enmRemoteMode)
{
    /* Look for proper index to choose: */
    if (m_pComboRemote)
    {
        const int iIndex = m_pComboRemote->findData(QVariant::fromValue(enmRemoteMode));
        if (iIndex != -1)
            m_pComboRemote->setCurrentIndex(iIndex);
    }
}

UIRemoteMode UIUSBFilterDetailsEditor::remoteMode() const
{
    return m_pComboRemote ? m_pComboRemote->currentData().value<UIRemoteMode>() : UIRemoteMode_Any;
}

void UIUSBFilterDetailsEditor::retranslateUi()
{
    setWindowTitle(tr("USB Filter Details"));

    if (m_pLabelName)
        m_pLabelName->setText(tr("&Name:"));
    if (m_pEditorName)
        m_pEditorName->setToolTip(tr("Holds the filter name."));

    if (m_pLabelVendorID)
        m_pLabelVendorID->setText(tr("&Vendor ID:"));
    if (m_pEditorVendorID)
        m_pEditorVendorID->setToolTip(tr("Holds the vendor ID filter. The <i>exact match</i> string format is <tt>XXXX</tt> "
                                         "where <tt>X</tt> is a hexadecimal digit. An empty string will match any value."));

    if (m_pLabelProductID)
        m_pLabelProductID->setText(tr("&Product ID:"));
    if (m_pEditorProductID)
        m_pEditorProductID->setToolTip(tr("Holds the product ID filter. The <i>exact match</i> string format is <tt>XXXX</tt> "
                                          "where <tt>X</tt> is a hexadecimal digit. An empty string will match any value."));

    if (m_pLabelRevision)
        m_pLabelRevision->setText(tr("&Revision:"));
    if (m_pEditorRevision)
        m_pEditorRevision->setToolTip(tr("Holds the revision number filter. The <i>exact match</i> string format is "
                                         "<tt>IIFF</tt> where <tt>I</tt> is a decimal digit of the integer part and <tt>F</tt> "
                                         "is a decimal digit of the fractional part. An empty string will match any value."));

    if (m_pLabelManufacturer)
        m_pLabelManufacturer->setText(tr("&Manufacturer:"));
    if (m_pEditorManufacturer)
        m_pEditorManufacturer->setToolTip(tr("Holds the manufacturer filter as an <i>exact match</i> string. An empty string "
                                             "will match any value."));

    if (m_pLabelProduct)
        m_pLabelProduct->setText(tr("Pro&duct:"));
    if (m_pEditorProduct)
        m_pEditorProduct->setToolTip(tr("Holds the product name filter as an <i>exact match</i> string. An empty string will "
                                        "match any value."));

    if (m_pLabelSerialNo)
        m_pLabelSerialNo->setText(tr("&Serial No.:"));
    if (m_pEditorSerialNo)
        m_pEditorSerialNo->setToolTip(tr("Holds the serial number filter as an <i>exact match</i> string. An empty string will "
                                         "match any value."));

    if (m_pLabelPort)
        m_pLabelPort->setText(tr("Por&t:"));
    if (m_pEditorPort)
        m_pEditorPort->setToolTip(tr("Holds the host USB port filter as an <i>exact match</i> string. An empty string will match "
                                     "any value."));

    if (m_pLabelRemote)
        m_pLabelRemote->setText(tr("R&emote:"));
    if (m_pComboRemote)
    {
        for (int i = 0; i < m_pComboRemote->count(); ++i)
        {
            const UIRemoteMode enmType = m_pComboRemote->itemData(i).value<UIRemoteMode>();
            m_pComboRemote->setItemText(i, gpConverter->toString(enmType));
        }
        m_pComboRemote->setToolTip(tr("Holds whether this filter applies to USB devices attached locally to the host computer "
                                      "(No), to a VRDP client's computer (Yes), or both (Any)."));
    }
}

void UIUSBFilterDetailsEditor::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();

    /* Adjust dialog size: */
    adjustSize();

#ifdef VBOX_WS_MAC
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(minimumSize());
#endif /* VBOX_WS_MAC */
}

void UIUSBFilterDetailsEditor::prepareWidgets()
{
    /* Prepare main layout: */
    QGridLayout *pLayout = new QGridLayout(this);
    if (pLayout)
    {
        pLayout->setRowStretch(9, 1);

        /* Prepare name label: */
        m_pLabelName = new QLabel(this);
        if (m_pLabelName)
        {
            m_pLabelName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelName, 0, 0);
        }
        /* Prepare name editor: */
        m_pEditorName = new QLineEdit(this);
        if (m_pEditorName)
        {
            if (m_pLabelName)
                m_pLabelName->setBuddy(m_pEditorName);
            m_pEditorName->setValidator(new QRegularExpressionValidator(QRegularExpression(".+"), this));

            pLayout->addWidget(m_pEditorName, 0, 1);
        }

        /* Prepare vendor ID label: */
        m_pLabelVendorID = new QLabel(this);
        if (m_pLabelVendorID)
        {
            m_pLabelVendorID->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelVendorID, 1, 0);
        }
        /* Prepare vendor ID editor: */
        m_pEditorVendorID = new QLineEdit(this);
        if (m_pEditorVendorID)
        {
            if (m_pLabelVendorID)
                m_pLabelVendorID->setBuddy(m_pEditorVendorID);
            m_pEditorVendorID->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-F]{0,4}"), this));

            pLayout->addWidget(m_pEditorVendorID, 1, 1);
        }

        /* Prepare product ID label: */
        m_pLabelProductID = new QLabel(this);
        if (m_pLabelProductID)
        {
            m_pLabelProductID->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelProductID, 2, 0);
        }
        /* Prepare product ID editor: */
        m_pEditorProductID = new QLineEdit(this);
        if (m_pEditorProductID)
        {
            if (m_pLabelProductID)
                m_pLabelProductID->setBuddy(m_pEditorProductID);
            m_pEditorProductID->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-F]{0,4}"), this));

            pLayout->addWidget(m_pEditorProductID, 2, 1);
        }

        /* Prepare revision label: */
        m_pLabelRevision = new QLabel(this);
        if (m_pLabelRevision)
        {
            m_pLabelRevision->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelRevision, 3, 0);
        }
        /* Prepare revision editor: */
        m_pEditorRevision = new QLineEdit(this);
        if (m_pEditorRevision)
        {
            if (m_pLabelRevision)
                m_pLabelRevision->setBuddy(m_pEditorRevision);
            m_pEditorRevision->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-F]{0,4}"), this));

            pLayout->addWidget(m_pEditorRevision, 3, 1);
        }

        /* Prepare manufacturer label: */
        m_pLabelManufacturer = new QLabel(this);
        if (m_pLabelManufacturer)
        {
            m_pLabelManufacturer->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelManufacturer, 4, 0);
        }
        /* Prepare manufacturer editor: */
        m_pEditorManufacturer = new QLineEdit(this);
        if (m_pEditorManufacturer)
        {
            if (m_pLabelManufacturer)
                m_pLabelManufacturer->setBuddy(m_pEditorManufacturer);
            pLayout->addWidget(m_pEditorManufacturer, 4, 1);
        }

        /* Prepare product label: */
        m_pLabelProduct = new QLabel(this);
        if (m_pLabelProduct)
        {
            m_pLabelProduct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelProduct, 5, 0);
        }
        /* Prepare product editor: */
        m_pEditorProduct = new QLineEdit(this);
        if (m_pEditorProduct)
        {
            if (m_pLabelProduct)
                m_pLabelProduct->setBuddy(m_pEditorProduct);
            pLayout->addWidget(m_pEditorProduct, 5, 1);
        }

        /* Prepare serial NO label: */
        m_pLabelSerialNo = new QLabel(this);
        if (m_pLabelSerialNo)
        {
            m_pLabelSerialNo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelSerialNo, 6, 0);
        }
        /* Prepare serial NO editor: */
        m_pEditorSerialNo = new QLineEdit(this);
        if (m_pEditorSerialNo)
        {
            if (m_pLabelSerialNo)
                m_pLabelSerialNo->setBuddy(m_pEditorSerialNo);
            pLayout->addWidget(m_pEditorSerialNo, 6, 1);
        }

        /* Prepare port label: */
        m_pLabelPort = new QLabel(this);
        if (m_pLabelPort)
        {
            m_pLabelPort->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelPort, 7, 0);
        }
        /* Prepare port editor: */
        m_pEditorPort = new QLineEdit(this);
        if (m_pEditorPort)
        {
            if (m_pLabelPort)
                m_pLabelPort->setBuddy(m_pEditorPort);
            m_pEditorPort->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this));

            pLayout->addWidget(m_pEditorPort, 7, 1);
        }

        /* Prepare remote label: */
        m_pLabelRemote = new QLabel(this);
        if (m_pLabelRemote)
        {
            m_pLabelRemote->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayout->addWidget(m_pLabelRemote, 8, 0);
        }
        /* Prepare remote combo: */
        m_pComboRemote = new QComboBox(this);
        if (m_pComboRemote)
        {
            if (m_pLabelRemote)
                m_pLabelRemote->setBuddy(m_pComboRemote);
            m_pComboRemote->addItem(QString(), QVariant::fromValue(UIRemoteMode_Any)); /* Any */
            m_pComboRemote->addItem(QString(), QVariant::fromValue(UIRemoteMode_On));  /* Yes */
            m_pComboRemote->addItem(QString(), QVariant::fromValue(UIRemoteMode_Off)); /* No */

            pLayout->addWidget(m_pComboRemote, 8, 1);
        }

        /* Prepare button-box: */
        m_pButtonBox = new QIDialogButtonBox(this);
        if (m_pButtonBox)
        {
            m_pButtonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
            pLayout->addWidget(m_pButtonBox, 10, 0, 1, 2);
        }
    }
}

void UIUSBFilterDetailsEditor::prepareConnections()
{
    if (m_pButtonBox)
    {
        connect(m_pButtonBox, &QIDialogButtonBox::accepted,
                this, &UIUSBFilterDetailsEditor::accept);
        connect(m_pButtonBox, &QIDialogButtonBox::rejected,
                this, &UIUSBFilterDetailsEditor::reject);
    }
}

/* static */
QString UIUSBFilterDetailsEditor::wiped(const QString &strString)
{
    return strString.isEmpty() ? QString() : strString;
}
