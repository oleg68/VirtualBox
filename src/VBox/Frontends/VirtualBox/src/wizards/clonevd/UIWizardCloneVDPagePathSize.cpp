/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPagePathSize class implementation.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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
#include <QDir>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIWizardCloneVDPagePathSize.h"
#include "UIWizardDiskEditors.h"
#include "UIWizardCloneVD.h"
#include "UIMessageCenter.h"

UIWizardCloneVDPagePathSize::UIWizardCloneVDPagePathSize(qulonglong uSourceDiskLogicaSize)
    : m_pMediumSizePathGroupBox(0)
{
    prepare(uSourceDiskLogicaSize);
}

void UIWizardCloneVDPagePathSize::prepare(qulonglong uSourceDiskLogicaSize)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    m_pMediumSizePathGroupBox = new UIMediumSizeAndPathGroupBox(false /* expert mode */, 0 /* parent */, uSourceDiskLogicaSize);
    if (m_pMediumSizePathGroupBox)
    {
        pMainLayout->addWidget(m_pMediumSizePathGroupBox);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumLocationButtonClicked,
                this, &UIWizardCloneVDPagePathSize::sltSelectLocationButtonClicked);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumPathChanged,
                this, &UIWizardCloneVDPagePathSize::sltMediumPathChanged);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumSizeChanged,
                this, &UIWizardCloneVDPagePathSize::sltMediumSizeChanged);
    }
    pMainLayout->addStretch();
    retranslateUi();
}

void UIWizardCloneVDPagePathSize::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Location and size of the disk image"));
}

void UIWizardCloneVDPagePathSize::initializePage()
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVD>() && m_pMediumSizePathGroupBox);
    /* Translate page: */
    retranslateUi();
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    m_pMediumSizePathGroupBox->blockSignals(true);

    /* Initialize medium size widget and wizard's medium size parameter: */
    if (!m_userModifiedParameters.contains("MediumSize"))
    {
        m_pMediumSizePathGroupBox->setMediumSize(pWizard->sourceDiskLogicalSize());
        pWizard->setMediumSize(m_pMediumSizePathGroupBox->mediumSize());
    }

    if (!m_userModifiedParameters.contains("MediumPath"))
    {
        QString strExtension = UIDiskFormatsGroupBox::defaultExtension(pWizard->mediumFormat(), KDeviceType_HardDisk);
        QString strSourceDiskPath = QDir::toNativeSeparators(QFileInfo(pWizard->sourceDiskFilePath()).absolutePath());
        /* Disk name without the format extension: */
        QString strDiskName = QString("%1_%2").arg(QFileInfo(pWizard->sourceDiskName()).completeBaseName()).arg(tr("copy"));

        QString strMediumFilePath =
            UIDiskEditorGroupBox::constructMediumFilePath(UIDiskVariantGroupBox::appendExtension(strDiskName,
                                                                                                 strExtension), strSourceDiskPath);
        m_pMediumSizePathGroupBox->setMediumPath(strMediumFilePath);
        pWizard->setMediumPath(strMediumFilePath);
    }
    m_pMediumSizePathGroupBox->blockSignals(false);
}

bool UIWizardCloneVDPagePathSize::isComplete() const
{
    AssertReturn(m_pMediumSizePathGroupBox, false);
    return m_pMediumSizePathGroupBox->isComplete();
}

bool UIWizardCloneVDPagePathSize::validatePage()
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturn(pWizard, false);
    /* Make sure such file doesn't exists already: */
    QString strMediumPath(pWizard->mediumPath());
    if (QFileInfo(strMediumPath).exists())
    {
        msgCenter().cannotOverwriteHardDiskStorage(strMediumPath, this);
        return false;
    }
    return pWizard->copyVirtualDisk();
}

void UIWizardCloneVDPagePathSize::sltSelectLocationButtonClicked()
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    CMediumFormat comMediumFormat(pWizard->mediumFormat());
    QString strSelectedPath =
        UIDiskEditorGroupBox::openFileDialogForDiskFile(pWizard->mediumPath(), comMediumFormat, pWizard->deviceType(), pWizard);

    if (strSelectedPath.isEmpty())
        return;
    QString strMediumPath =
        UIDiskEditorGroupBox::appendExtension(strSelectedPath,
                                              UIDiskFormatsGroupBox::defaultExtension(pWizard->mediumFormat(), KDeviceType_HardDisk));
    QFileInfo mediumPath(strMediumPath);
    m_pMediumSizePathGroupBox->setMediumPath(QDir::toNativeSeparators(mediumPath.absoluteFilePath()));
}

void UIWizardCloneVDPagePathSize::sltMediumPathChanged(const QString &strPath)
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    m_userModifiedParameters << "MediumPath";
    QString strMediumPath =
        UIDiskEditorGroupBox::appendExtension(strPath,
                                              UIDiskFormatsGroupBox::defaultExtension(pWizard->mediumFormat(), pWizard->deviceType()));
    pWizard->setMediumPath(strMediumPath);
    emit completeChanged();
}

void UIWizardCloneVDPagePathSize::sltMediumSizeChanged(qulonglong uSize)
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    m_userModifiedParameters << "MediumSize";
    pWizard->setMediumSize(uSize);
    emit completeChanged();
}