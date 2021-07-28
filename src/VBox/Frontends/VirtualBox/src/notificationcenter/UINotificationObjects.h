/* $Id$ */
/** @file
 * VBox Qt GUI - Various UINotificationObjects declarations.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_notificationcenter_UINotificationObjects_h
#define FEQT_INCLUDED_SRC_notificationcenter_UINotificationObjects_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINotificationObject.h"

/* COM includes: */
#include "COMEnums.h"
#include "CAppliance.h"
#include "CCloudClient.h"
#include "CCloudMachine.h"
#include "CExtPackFile.h"
#include "CExtPackManager.h"
#include "CMachine.h"
#include "CMedium.h"
#include "CVirtualSystemDescription.h"

/** UINotificationProgress extension for medium move functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumMove : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs medium move notification-progress.
      * @param  comMedium  Brings the medium being moved.
      * @param  strFrom    Brings the initial location.
      * @param  strTo      Brings the final location. */
    UINotificationProgressMediumMove(const CMedium &comMedium,
                                     const QString &strFrom,
                                     const QString &strTo);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private:

    /** Holds the medium being moved. */
    CMedium  m_comMedium;
    /** Holds the initial location. */
    QString  m_strFrom;
    /** Holds the final location. */
    QString  m_strTo;
};

/** UINotificationProgress extension for medium create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a comMedium was created. */
    void sigMediumCreated(const CMedium &comMedium);

public:

    /** Constructs medium create notification-progress.
      * @param  comTarget  Brings the medium being the target.
      * @param  uSize      Brings the target medium size.
      * @param  variants   Brings the target medium options. */
    UINotificationProgressMediumCreate(const CMedium &comTarget,
                                       qulonglong uSize,
                                       const QVector<KMediumVariant> &variants);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the medium being the target. */
    CMedium                  m_comTarget;
    /** Holds the target medium size. */
    qulonglong               m_uSize;
    /** Holds the target medium options. */
    QVector<KMediumVariant>  m_variants;
};

/** UINotificationProgress extension for medium copy functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumCopy : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a comMedium was copied. */
    void sigMediumCopied(const CMedium &comMedium);

public:

    /** Constructs medium copy notification-progress.
      * @param  comSource  Brings the medium being copied.
      * @param  comTarget  Brings the medium being the target.
      * @param  variants   Brings the target medium options. */
    UINotificationProgressMediumCopy(const CMedium &comSource,
                                     const CMedium &comTarget,
                                     const QVector<KMediumVariant> &variants);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the medium being copied. */
    CMedium                  m_comSource;
    /** Holds the medium being the target. */
    CMedium                  m_comTarget;
    /** Holds the target medium options. */
    QVector<KMediumVariant>  m_variants;
};

/** UINotificationProgress extension for machine copy functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachineCopy : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a comMachine was copied. */
    void sigMachineCopied(const CMachine &comMachine);

public:

    /** Constructs medium move notification-progress.
      * @param  comSource     Brings the machine being copied.
      * @param  comTarget     Brings the machine being the target.
      * @param  enmCloneMode  Brings the cloning mode.
      * @param  options       Brings the cloning options. */
    UINotificationProgressMachineCopy(const CMachine &comSource,
                                      const CMachine &comTarget,
                                      const KCloneMode &enmCloneMode,
                                      const QVector<KCloneOptions> &options);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine being copied. */
    CMachine                m_comSource;
    /** Holds the machine being the target. */
    CMachine                m_comTarget;
    /** Holds the machine cloning mode. */
    KCloneMode              m_enmCloneMode;
    /** Holds the target machine options. */
    QVector<KCloneOptions>  m_options;
};

/** UINotificationProgress extension for machine media remove functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachineMediaRemove : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs machine media remove notification-progress.
      * @param  comMachine  Brings the machine being removed.
      * @param  media       Brings the machine media being removed. */
    UINotificationProgressMachineMediaRemove(const CMachine &comMachine,
                                             const CMediumVector &media);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private:

    /** Holds the machine being removed. */
    CMachine       m_comMachine;
    /** Holds the machine media being removed. */
    CMediumVector  m_media;
};

/** UINotificationProgress extension for cloud machine add functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineAdd : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about cloud @a comMachine was added.
      * @param  strShortProviderName  Brigns the short provider name.
      * @param  strProfileName        Brings the profile name. */
    void sigCloudMachineAdded(const QString &strShortProviderName,
                              const QString &strProfileName,
                              const CCloudMachine &comMachine);

public:

    /** Constructs cloud machine add notification-progress.
      * @param  comClient             Brings the cloud client being adding machine.
      * @param  comMachine            Brings the cloud machine being added.
      * @param  strInstanceName       Brings the instance name.
      * @param  strShortProviderName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    UINotificationProgressCloudMachineAdd(const CCloudClient &comClient,
                                          const CCloudMachine &comMachine,
                                          const QString &strInstanceName,
                                          const QString &strShortProviderName,
                                          const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being adding machine. */
    CCloudClient   m_comClient;
    /** Holds the cloud machine being added. */
    CCloudMachine  m_comMachine;
    /** Holds the instance name. */
    QString        m_strInstanceName;
    /** Holds the short provider name. */
    QString        m_strShortProviderName;
    /** Holds the profile name. */
    QString        m_strProfileName;
};

/** UINotificationProgress extension for cloud machine create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about cloud @a comMachine was created.
      * @param  strShortProviderName  Brigns the short provider name.
      * @param  strProfileName        Brings the profile name. */
    void sigCloudMachineCreated(const QString &strShortProviderName,
                                const QString &strProfileName,
                                const CCloudMachine &comMachine);

public:

    /** Constructs cloud machine create notification-progress.
      * @param  comClient             Brings the cloud client being adding machine.
      * @param  comMachine            Brings the cloud machine being added.
      * @param  comVSD                Brings the virtual system description.
      * @param  strShortProviderName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    UINotificationProgressCloudMachineCreate(const CCloudClient &comClient,
                                             const CCloudMachine &comMachine,
                                             const CVirtualSystemDescription &comVSD,
                                             const QString &strShortProviderName,
                                             const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being adding machine. */
    CCloudClient               m_comClient;
    /** Holds the cloud machine being added. */
    CCloudMachine              m_comMachine;
    /** Holds the the virtual system description. */
    CVirtualSystemDescription  m_comVSD;
    /** Holds the name acquired from VSD. */
    QString                    m_strName;
    /** Holds the short provider name. */
    QString                    m_strShortProviderName;
    /** Holds the profile name. */
    QString                    m_strProfileName;
};

/** UINotificationProgress extension for export appliance functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressApplianceExport : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs export appliance notification-progress.
      * @param  comAppliance  Brings the appliance being exported.
      * @param  strFormat     Brings the appliance format.
      * @param  options       Brings the export options to be taken into account.
      * @param  strPath       Brings the appliance path. */
    UINotificationProgressApplianceExport(const CAppliance &comAppliance,
                                          const QString &strFormat,
                                          const QVector<KExportOptions> &options,
                                          const QString &strPath);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private:

    /** Holds the appliance being exported. */
    CAppliance               m_comAppliance;
    /** Holds the appliance format. */
    QString                  m_strFormat;
    /** Holds the export options to be taken into account. */
    QVector<KExportOptions>  m_options;
    /** Holds the appliance path. */
    QString                  m_strPath;
};

/** UINotificationProgress extension for import appliance functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressApplianceImport : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs import appliance notification-progress.
      * @param  comAppliance  Brings the appliance being imported.
      * @param  options       Brings the import options to be taken into account. */
    UINotificationProgressApplianceImport(const CAppliance &comAppliance,
                                          const QVector<KImportOptions> &options);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private:

    /** Holds the appliance being imported. */
    CAppliance               m_comAppliance;
    /** Holds the import options to be taken into account. */
    QVector<KImportOptions>  m_options;
};

/** UINotificationProgress extension for extension pack install functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressExtensionPackInstall : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about extension pack installed.
      * @param  strExtensionPackName  Brigns extension pack name. */
    void sigExtensionPackInstalled(const QString &strExtensionPackName);

public:

    /** Constructs extension pack install notification-progress.
      * @param  comExtPackFile        Brings the extension pack file to install.
      * @param  fReplace              Brings whether extension pack should be replaced.
      * @param  strExtensionPackName  Brings the extension pack name.
      * @param  strDisplayInfo        Brings the display info. */
    UINotificationProgressExtensionPackInstall(const CExtPackFile &comExtPackFile,
                                               bool fReplace,
                                               const QString &strExtensionPackName,
                                               const QString &strDisplayInfo);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the extension pack file to install. */
    CExtPackFile     m_comExtPackFile;
    /** Holds whether extension pack should be replaced. */
    bool             m_fReplace;
    /** Holds the extension pack name. */
    QString          m_strExtensionPackName;
    /** Holds the display info. */
    QString          m_strDisplayInfo;
};

/** UINotificationProgress extension for extension pack uninstall functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressExtensionPackUninstall : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about extension pack uninstalled.
      * @param  strExtensionPackName  Brigns extension pack name. */
    void sigExtensionPackUninstalled(const QString &strExtensionPackName);

public:

    /** Constructs extension pack uninstall notification-progress.
      * @param  comExtPackManager     Brings the extension pack manager.
      * @param  strExtensionPackName  Brings the extension pack name.
      * @param  strDisplayInfo        Brings the display info. */
    UINotificationProgressExtensionPackUninstall(const CExtPackManager &comExtPackManager,
                                                 const QString &strExtensionPackName,
                                                 const QString &strDisplayInfo);

protected:

    /** Returns object name. */
    virtual QString name() const /* override final */;
    /** Returns object details. */
    virtual QString details() const /* override final */;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) /* override final */;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the extension pack manager. */
    CExtPackManager  m_comExtPackManager;
    /** Holds the extension pack name. */
    QString          m_strExtensionPackName;
    /** Holds the display info. */
    QString          m_strDisplayInfo;
};

#endif /* !FEQT_INCLUDED_SRC_notificationcenter_UINotificationObjects_h */