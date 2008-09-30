/* $Id$ */
/** @file
 * VBox Debugger GUI - Statistics.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___Debugger_VBoxDbgStats_h
#define ___Debugger_VBoxDbgStats_h

#include "VBoxDbgBase.h"

#include <QTreeView>
#include <QTimer>
#include <QComboBox>
#include <QMenu>

class VBoxDbgStats;
class VBoxDbgStatsModel;

/** Pointer to a statistics sample. */
typedef struct DBGGUISTATSNODE *PDBGGUISTATSNODE;
/** Pointer to a const statistics sample. */
typedef struct DBGGUISTATSNODE const *PCDBGGUISTATSNODE;


/**
 * The VM statistics tree view.
 *
 * A tree represenation of the STAM statistics.
 */
class VBoxDbgStatsView : public QTreeView, public VBoxDbgBase
{
    Q_OBJECT;

public:
    /**
     * Creates a VM statistics list view widget.
     *
     * @param   a_pVM       The VM which STAM data is being viewed.
     * @param   a_pModel    The model. Will take ownership of this and delete it together
     *                      with the view later
     * @param   a_pParent   Parent widget.
     */
    VBoxDbgStatsView(PVM a_pVM, VBoxDbgStatsModel *a_pModel, VBoxDbgStats *a_pParent = NULL);

    /** Destructor. */
    virtual ~VBoxDbgStatsView();

    /**
     * Updates the view with current information from STAM.
     * This will indirectly update the m_PatStr.
     *
     * @param   rPatStr     Selection pattern. NULL means everything, see STAM for further details.
     */
    void updateStats(const QString &rPatStr);

    /**
     * Resets the stats items matching the specified pattern.
     * This pattern doesn't have to be the one used for update, thus m_PatStr isn't updated.
     *
     * @param   rPatStr     Selection pattern. NULL means everything, see STAM for further details.
     */
    void resetStats(const QString &rPatStr);

private:
    /**
     * Callback function for the STAMR3Enum() made by update().
     *
     * @returns 0 (i.e. never halt enumeration).
     *
     * @param   pszName         The name of the sample.
     * @param   enmType         The type.
     * @param   pvSample        Pointer to the data. enmType indicates the format of this data.
     * @param   enmUnit         The unit.
     * @param   enmVisibility   The visibility.
     * @param   pszDesc         The description.
     * @param   pvUser          Pointer to the VBoxDbgStatsView object.
     */
//later:    static DECLCALLBACK(int) updateCallback(const char *pszName, STAMTYPE enmType, void *pvSample, STAMUNIT enmUnit,
//later:                                            STAMVISIBILITY enmVisibility, const char *pszDesc, void *pvUser);

protected:
    /**
     * Creates / finds the path to the specified stats item and makes is visible.
     *
     * @returns Parent node.
     * @param   pszName     Path to a stats item.
     */
//    VBoxDbgStatsItem *createPath(const char *pszName);

protected slots:
//later:    /** Context menu. */
//later:    void contextMenuReq(QListViewItem *pItem, const QPoint &rPoint, int iColumn);
//later:    /** Leaf context. */
//later:    void leafMenuActivated(int iId);
//later:    /** Branch context. */
//later:    void branchMenuActivated(int iId);
//later:    /** View context. */
//later:    void viewMenuActivated(int iId);

protected:
    typedef enum { eRefresh = 1, eReset, eExpand, eCollaps, eCopy, eLog, eLogRel } MenuId;

protected:
    /** Pointer to the data model. */
    VBoxDbgStatsModel *m_pModel;
    /** The current selection pattern. */
    QString m_PatStr;
    /** The parent widget. */
    VBoxDbgStats *m_pParent;
    /** Leaf item menu. */
    QMenu *m_pLeafMenu;
    /** Branch item menu. */
    QMenu *m_pBranchMenu;
    /** View menu. */
    QMenu *m_pViewMenu;
    /** The pointer to the node which is the current focus of a context menu. */
    PDBGGUISTATSNODE m_pContextNode;
};



/**
 * The VM statistics window.
 *
 * This class displays the statistics of a VM. The UI contains
 * a entry field for the selection pattern, a refresh interval
 * spinbutton, and the tree view with the statistics.
 */
class VBoxDbgStats :
#ifdef VBOXDBG_USE_QT4
    public QWidget,
#else
    public QVBox,
#endif
    public VBoxDbgBase
{
    Q_OBJECT;

public:
    /**
     * Creates a VM statistics list view widget.
     *
     * @param   pVM             The VM this is hooked up to.
     * @param   pszPat          Initial selection pattern. NULL means everything. (See STAM for details.)
     * @param   uRefreshRate    The refresh rate. 0 means not to refresh and is the default.
     * @param   pParent         Parent widget.
     */
    VBoxDbgStats(PVM pVM, const char *pszPat = NULL, unsigned uRefreshRate= 0, QWidget *pParent = NULL);

    /** Destructor. */
    virtual ~VBoxDbgStats();

protected slots:
    /** Apply the activated combobox pattern. */
    void apply(const QString &Str);
    /** The "All" button was pressed. */
    void applyAll();
    /** Refresh the data on timer tick and pattern changed. */
    void refresh();
    /**
     * Set the refresh rate.
     *
     * @param   iRefresh        The refresh interval in seconds.
     */
    void setRefresh(int iRefresh);

protected:

    /** The current selection pattern. */
    QString             m_PatStr;
    /** The pattern combo box. */
    QComboBox          *m_pPatCB;
    /** The refresh rate in seconds.
     * 0 means not to refresh. */
    unsigned            m_uRefreshRate;
    /** The refresh timer .*/
    QTimer             *m_pTimer;
    /** The tree view widget. */
    VBoxDbgStatsView   *m_pView;
};


#endif

