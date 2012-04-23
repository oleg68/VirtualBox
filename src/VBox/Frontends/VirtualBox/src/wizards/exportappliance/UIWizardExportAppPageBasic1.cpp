/* $Id$ */
/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * UIWizardExportAppPageBasic1 class implementation
 */

/*
 * Copyright (C) 2009-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Global includes: */
#include <QVBoxLayout>

/* Local includes: */
#include "UIWizardExportAppPageBasic1.h"
#include "UIWizardExportApp.h"
#include "UIWizardExportAppDefs.h"
#include "VBoxGlobal.h"
#include "UIMessageCenter.h"
#include "QILabelSeparator.h"
#include "QIRichTextLabel.h"

UIWizardExportAppPage1::UIWizardExportAppPage1()
{
}

void UIWizardExportAppPage1::populateVMSelectorItems(const QStringList &selectedVMNames)
{
    /* Add all VM items into 'VM Selector': */
    foreach (const CMachine &machine, vboxGlobal().virtualBox().GetMachines())
    {
        QPixmap pixIcon;
        QString strName;
        QString strUuid;
        bool fInSaveState = false;
        bool fEnabled = false;
        if (machine.GetAccessible())
        {
            pixIcon = vboxGlobal().vmGuestOSTypeIcon(machine.GetOSTypeId()).scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            strName = machine.GetName();
            strUuid = machine.GetId();
            fEnabled = machine.GetSessionState() == KSessionState_Unlocked;
            fInSaveState = machine.GetState() == KMachineState_Saved;
        }
        else
        {
            QString settingsFile = machine.GetSettingsFilePath();
            QFileInfo fi(settingsFile);
            strName = VBoxGlobal::hasAllowedExtension(fi.completeSuffix(), VBoxDefs::VBoxFileExts) ? fi.completeBaseName() : fi.fileName();
            pixIcon = QPixmap(":/os_other.png").scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        QListWidgetItem *pItem = new VMListWidgetItem(pixIcon, strName, strUuid, fInSaveState, m_pVMSelector);
        if (!fEnabled)
            pItem->setFlags(0);
        m_pVMSelector->addItem(pItem);
    }
    m_pVMSelector->sortItems();

    /* Choose initially selected items (if passed): */
    for (int i = 0; i < selectedVMNames.size(); ++i)
    {
        QList<QListWidgetItem*> list = m_pVMSelector->findItems(selectedVMNames[i], Qt::MatchExactly);
        if (list.size() > 0)
        {
            if (m_pVMSelector->selectedItems().isEmpty())
                m_pVMSelector->setCurrentItem(list.first());
            else
                list.first()->setSelected(true);
        }
    }
}

QStringList UIWizardExportAppPage1::machineNames() const
{
    /* Prepare list: */
    QStringList machineNames;
    /* Iterate over all the selected items: */
    foreach (QListWidgetItem *pItem, m_pVMSelector->selectedItems())
        machineNames << pItem->text();
    /* Return result list: */
    return machineNames;
}

QStringList UIWizardExportAppPage1::machineIDs() const
{
    /* Prepare list: */
    QStringList machineIDs;
    /* Iterate over all the selected items: */
    foreach (QListWidgetItem *pItem, m_pVMSelector->selectedItems())
        machineIDs << static_cast<VMListWidgetItem*>(pItem)->uuid();
    /* Return result list: */
    return machineIDs;
}

UIWizardExportAppPageBasic1::UIWizardExportAppPageBasic1(const QStringList &selectedVMNames)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pVMSelectorLabel = new QILabelSeparator(this);
        {
            m_pVMSelectorLabel->hide();
        }
        m_pVMSelector = new QListWidget(this);
        {
            m_pVMSelector->setAlternatingRowColors(true);
            m_pVMSelector->setSelectionMode(QAbstractItemView::ExtendedSelection);
            m_pVMSelectorLabel->setBuddy(m_pVMSelector);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pVMSelectorLabel);
        pMainLayout->addWidget(m_pVMSelector);
        populateVMSelectorItems(selectedVMNames);
    }

    /* Setup connections: */
    connect(m_pVMSelector, SIGNAL(itemSelectionChanged()), this, SIGNAL(completeChanged()));

    /* Register fields: */
    registerField("machineNames", this, "machineNames");
    registerField("machineIDs", this, "machineIDs");
}

void UIWizardExportAppPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardExportApp::tr("Welcome to the Appliance Export wizard!"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardExportApp::tr("<p>This wizard will guide you through the process of "
                                            "exporting an appliance.</p><p>%1</p><p>Please select "
                                            "the virtual machines that should be added to the "
                                            "appliance. You can select more than one. Please note "
                                            "that these machines have to be turned off before they "
                                            "can be exported.</p>")
                                            .arg(standardHelpText()));
    m_pVMSelectorLabel->setText(UIWizardExportApp::tr("VM &List"));
}

void UIWizardExportAppPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardExportAppPageBasic1::isComplete() const
{
    /* There should be at least one vm selected: */
    return m_pVMSelector->selectedItems().size() > 0;
}

bool UIWizardExportAppPageBasic1::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Ask user about machines which are in save state currently: */
    QStringList savedMachines;
    QList<QListWidgetItem*> pItems = m_pVMSelector->selectedItems();
    for (int i=0; i < pItems.size(); ++i)
    {
        if (static_cast<VMListWidgetItem*>(pItems.at(i))->isInSaveState())
            savedMachines << pItems.at(i)->text();
    }
    if (!savedMachines.isEmpty())
        fResult = msgCenter().confirmExportMachinesInSaveState(savedMachines, this);

    /* Return result: */
    return fResult;
}

int UIWizardExportAppPageBasic1::nextId() const
{
    /* Skip next (2nd, storage-type) page for now! */
    return UIWizardExportApp::Page3;
}

