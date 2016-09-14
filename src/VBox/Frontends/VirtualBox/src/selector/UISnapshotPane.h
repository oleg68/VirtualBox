/* $Id$ */
/** @file
 * VBox Qt GUI - UISnapshotPane class declaration.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UISnapshotPane_h___
#define ___UISnapshotPane_h___

/* Qt includes: */
#include <QTimer>
#include <QIcon>

/* GUI includes: */
#include "UISnapshotPane.gen.h"
#include "VBoxGlobal.h"
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "CMachine.h"

/* Forward declarations: */
class SnapshotWgtItem;


/** Snapshot age format. */
enum SnapshotAgeFormat
{
    SnapshotAgeFormat_InSeconds,
    SnapshotAgeFormat_InMinutes,
    SnapshotAgeFormat_InHours,
    SnapshotAgeFormat_InDays,
    SnapshotAgeFormat_Max
};


/** QWidget extension providing GUI with the pane to control snapshot related functionality. */
class UISnapshotPane : public QIWithRetranslateUI<QWidget>, public Ui::UISnapshotPane
{
    Q_OBJECT;

public:

    /** Constructs snapshot pane passing @a pParent to the base-class. */
    UISnapshotPane(QWidget *pParent);

    /** Defines the @a comMachine to be parsed. */
    void setMachine(const CMachine &comMachine);

    /** Returns cached snapshot-item icon depending on @a fOnline flag. */
    const QIcon &snapshotItemIcon(bool fOnline) const { return !fOnline ? m_snapshotIconOffline : m_snapshotIconOnline; }

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** @name Tree-view handlers
      * @{ */
        /** Handles cursor change to @a pItem. */
        void sltCurrentItemChanged(QTreeWidgetItem *pItem = 0);
        /** Handles context menu request for @a point. */
        void sltContextMenuRequested(const QPoint &point);
        /** Handles modification for @a pItem. */
        void sltItemChanged(QTreeWidgetItem *pItem);
        /** Handles double-click for @a pItem. */
        void sltItemDoubleClicked(QTreeWidgetItem *pItem);
    /** @} */

    /** @name Snapshot operations
      * @{ */
        /** Proposes to take a snapshot. */
        void sltTakeSnapshot() { takeSnapshot(); }
        /** Proposes to restore the snapshot. */
        void sltRestoreSnapshot(bool fSuppressNonCriticalWarnings = false);
        /** Proposes to delete the snapshot. */
        void sltDeleteSnapshot();
        /** Displays the snapshot details dialog. */
        void sltShowSnapshotDetails();
        /** Proposes to clone the snapshot. */
        void sltCloneSnapshot();
    /** @} */

    /** @name Main event handlers
      * @{ */
        /** Handles machine data change for machine with @a strMachineID. */
        void sltMachineDataChange(QString strMachineID);
        /** Handles machine @a enmState change for machine with @a strMachineID. */
        void sltMachineStateChange(QString strMachineID, KMachineState enmState);
        /** Handles session @a enmState change for machine with @a strMachineID. */
        void sltSessionStateChange(QString strMachineID, KSessionState enmState);
    /** @} */

    /** @name Timer event handlers
      * @{ */
        /** Updates snapshots age. */
        void sltUpdateSnapshotsAge();
    /** @} */

private:

    /** @name Snapshot operations
      * @{ */
        /** Proposes to take a snapshot. */
        bool takeSnapshot();
        /** Proposes to restore the snapshot. */
        //bool restoreSnapshot();
        /** Proposes to delete the snapshot. */
        //bool deleteSnapshot();
        /** Displays the snapshot details dialog. */
        //bool showSnapshotDetails();
        /** Proposes to clone the snapshot. */
        //bool cloneSnapshot();
    /** @} */

    /** Refreshes everything. */
    void refreshAll();

    /** Searches for an item with corresponding @a strSnapshotID. */
    SnapshotWgtItem *findItem(const QString &strSnapshotID) const;
    /** Returns the "current state" item. */
    SnapshotWgtItem *currentStateItem() const;

    /** Populates snapshot items for corresponding @a comSnapshot using @a pItem as parent. */
    void populateSnapshots(const CSnapshot &comSnapshot, QTreeWidgetItem *pItem);

    /** Searches for smallest snapshot age starting with @a pItem as parent. */
    SnapshotAgeFormat traverseSnapshotAge(QTreeWidgetItem *pItem) const;

    /** Holds the machine COM wrapper. */
    CMachine         m_comMachine;
    /** Holds the machine ID. */
    QString          m_strMachineID;
    /** Holds the cached session state. */
    KSessionState    m_enmSessionState;
    /** Holds the current snapshot item reference. */
    SnapshotWgtItem *m_pCurrentSnapshotItem;
    /** Holds the snapshot item editing protector. */
    bool             m_fEditProtector;

    /** Holds the snapshot item action group instance. */
    QActionGroup    *m_pSnapshotItemActionGroup;
    /** Holds the current item action group instance. */
    QActionGroup    *m_pCurrentStateItemActionGroup;

    /** Holds the snapshot restore action instance. */
    QAction         *m_pActionRestoreSnapshot;
    /** Holds the snapshot delete action instance. */
    QAction         *m_pActionDeleteSnapshot;
    /** Holds the show snapshot details action instance. */
    QAction         *m_pActionShowSnapshotDetails;
    /** Holds the snapshot take action instance. */
    QAction         *m_pActionTakeSnapshot;
    /** Holds the snapshot clone action instance. */
    QAction         *m_pActionCloneSnapshot;

    /** Holds the snapshot age update timer. */
    QTimer           m_ageUpdateTimer;

    /** Holds whether the snapshot operations are allowed. */
    bool             m_fShapshotOperationsAllowed;

    /** Holds the cached snapshot-item pixmap for 'offline' state. */
    QIcon            m_snapshotIconOffline;
    /** Holds the cached snapshot-item pixmap for 'online' state. */
    QIcon            m_snapshotIconOnline;
};

#endif /* !___UISnapshotPane_h___ */

