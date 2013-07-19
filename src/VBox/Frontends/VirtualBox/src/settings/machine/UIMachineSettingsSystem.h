/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * UIMachineSettingsSystem class declaration
 */

/*
 * Copyright (C) 2008-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIMachineSettingsSystem_h__
#define __UIMachineSettingsSystem_h__

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsSystem.gen.h"

/* Machine settings / System page / Data / Boot item: */
struct UIBootItemData
{
    /* Constructor: */
    UIBootItemData() : m_type(KDeviceType_Null), m_fEnabled(false) {}

    /* Operator==: */
    bool operator==(const UIBootItemData &other) const
    {
        return (m_type == other.m_type) &&
               (m_fEnabled == other.m_fEnabled);
    }

    /* Variables: */
    KDeviceType m_type;
    bool m_fEnabled;
};

/* Machine settings / System page / Data: */
struct UIDataSettingsMachineSystem
{
    /* Constructor: */
    UIDataSettingsMachineSystem()
        /* Support flags: */
        : m_fSupportedPAE(false)
        , m_fSupportedHwVirtEx(false)
        /* Motherboard data: */
        , m_iRAMSize(-1)
        , m_bootItems(QList<UIBootItemData>())
        , m_chipsetType(KChipsetType_Null)
        , m_fEnabledIoApic(false)
        , m_fEnabledEFI(false)
        , m_fEnabledUTC(false)
        , m_fEnabledAbsoluteHID(false)
        /* CPU data: */
        , m_cCPUCount(-1)
        , m_iCPUExecCap(-1)
        , m_fEnabledPAE(false)
        /* Acceleration data: */
        , m_fEnabledHwVirtEx(false)
        , m_fEnabledNestedPaging(false)
    {}

    /* Functions: */
    bool equal(const UIDataSettingsMachineSystem &other) const
    {
        return /* Support flags: */
               (m_fSupportedPAE == other.m_fSupportedPAE) &&
               (m_fSupportedHwVirtEx == other.m_fSupportedHwVirtEx) &&
               /* Motherboard data: */
               (m_iRAMSize == other.m_iRAMSize) &&
               (m_bootItems == other.m_bootItems) &&
               (m_chipsetType == other.m_chipsetType) &&
               (m_fEnabledIoApic == other.m_fEnabledIoApic) &&
               (m_fEnabledEFI == other.m_fEnabledEFI) &&
               (m_fEnabledUTC == other.m_fEnabledUTC) &&
               (m_fEnabledAbsoluteHID == other.m_fEnabledAbsoluteHID) &&
               /* CPU data: */
               (m_cCPUCount == other.m_cCPUCount) &&
               (m_iCPUExecCap == other.m_iCPUExecCap) &&
               (m_fEnabledPAE == other.m_fEnabledPAE) &&
                /* Acceleration data: */
               (m_fEnabledHwVirtEx == other.m_fEnabledHwVirtEx) &&
               (m_fEnabledNestedPaging == other.m_fEnabledNestedPaging);
    }

    /* Operators: */
    bool operator==(const UIDataSettingsMachineSystem &other) const { return equal(other); }
    bool operator!=(const UIDataSettingsMachineSystem &other) const { return !equal(other); }

    /* Variables: Support flags: */
    bool m_fSupportedPAE;
    bool m_fSupportedHwVirtEx;

    /* Variables: Motherboard data: */
    int m_iRAMSize;
    QList<UIBootItemData> m_bootItems;
    KChipsetType m_chipsetType;
    bool m_fEnabledIoApic;
    bool m_fEnabledEFI;
    bool m_fEnabledUTC;
    bool m_fEnabledAbsoluteHID;
    /* Variables: CPU data: */
    int m_cCPUCount;
    int m_iCPUExecCap;
    bool m_fEnabledPAE;
    /* Variables: Acceleration data: */
    bool m_fEnabledHwVirtEx;
    bool m_fEnabledNestedPaging;
};
typedef UISettingsCache<UIDataSettingsMachineSystem> UICacheSettingsMachineSystem;

/* Machine settings / System page: */
class UIMachineSettingsSystem : public UISettingsPageMachine,
                                public Ui::UIMachineSettingsSystem
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIMachineSettingsSystem();

    /* API: Correlation stuff: */
    bool isHWVirtExEnabled() const;
    bool isHIDEnabled() const;
    KChipsetType chipsetType() const;
    void setOHCIEnabled(bool fEnabled);

protected:

    /* API: Cache stuff: */
    bool changed() const { return m_cache.wasChanged(); }

    /* API: Load data to cache from corresponding external object(s),
     * this task COULD be performed in other than GUI thread: */
    void loadToCacheFrom(QVariant &data);
    /* API: Load data to corresponding widgets from cache,
     * this task SHOULD be performed in GUI thread only: */
    void getFromCache();

    /* API: Save data from corresponding widgets to cache,
     * this task SHOULD be performed in GUI thread only: */
    void putToCache();
    /* API: Save data from cache to corresponding external object(s),
     * this task COULD be performed in other than GUI thread: */
    void saveFromCacheTo(QVariant &data);

    /* Helpers: Validation stuff: */
    void setValidator(QIWidgetValidator *pValidator);
    bool revalidate(QString &strWarning, QString &strTitle);

    /* Helper: Navigation stuff: */
    void setOrderAfter(QWidget *pWidget);

    /* Helper: Translation stuff: */
    void retranslateUi();

private slots:

    /* Handlers: RAM stuff: */
    void valueChangedRAM(int iValue);
    void textChangedRAM(const QString &strText);

    /* Handler: Boot-table stuff: */
    void onCurrentBootItemChanged(int iCurrentIndex);

    /* Handlers: CPU stuff: */
    void valueChangedCPU(int iValue);
    void textChangedCPU(const QString &strText);
    void sltValueChangedCPUExecCap(int iValue);
    void sltTextChangedCPUExecCap(const QString &strText);

private:

    /* Helper: Translation stuff: */
    void retranslateComboPointingChipsetType();

    /* Handler: Polishing stuff: */
    void polishPage();

    /* Handler: Event-filtration stuff: */
    bool eventFilter(QObject *aObject, QEvent *aEvent);

    /* Helper: Boot-table stuff: */
    void adjustBootOrderTWSize();

    /* Variable: Validation stuff: */
    QIWidgetValidator *m_pValidator;

    /* Variable: Boot-table stuff: */
    QList<KDeviceType> m_possibleBootItems;

    /* Variables: CPU stuff: */
    uint m_uMinGuestCPU;
    uint m_uMaxGuestCPU;
    uint m_uMinGuestCPUExecCap;
    uint m_uMedGuestCPUExecCap;
    uint m_uMaxGuestCPUExecCap;

    /* Variable: Correlation stuff: */
    bool m_fOHCIEnabled;

    /* Cache: */
    UICacheSettingsMachineSystem m_cache;
};

#endif // __UIMachineSettingsSystem_h__
