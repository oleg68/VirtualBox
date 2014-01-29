/* $Id$ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIDnDMIMEData class declaration
 */

/*
 * Copyright (C) 2011-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIDnDMIMEData_h___
#define ___UIDnDMIMEData_h___

/* Qt includes: */
#include <QMimeData>

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CConsole.h"
#include "CGuest.h"

#include "UIDnDHandler.h"

/** @todo Subclass QWindowsMime / QMacPasteboardMime
 *  to register own/more MIME types. */

/**
 * Own implementation of QMimeData for starting and
 * handling all guest-to-host transfers.
 */
class UIDnDMimeData: public QMimeData
{
    Q_OBJECT;

    enum State
    {
        Dragging = 0,
        Dropped,
        Finished,
        Canceled
    };

public:

    UIDnDMimeData(CSession &session, QStringList formats,
                  Qt::DropAction defAction,
                  Qt::DropActions actions, QWidget *pParent);

public slots:

    void sltDropActionChanged(Qt::DropAction dropAction);

protected:
    /** @name Overridden functions of QMimeData.
     * @{ */
    virtual QStringList formats(void) const;

    virtual bool hasFormat(const QString &mimeType) const;

    virtual QVariant retrieveData(const QString &mimeType, QVariant::Type type) const;

#ifndef RT_OS_WINDOWS
    bool eventFilter(QObject * /* pObject */, QEvent *pEvent);
#endif
    /** @}  */

private slots:

#ifndef RT_OS_WINDOWS
    void sltInstallEventFilter(void);
#endif

private:

    /* Private members. */
    QWidget          *m_pParent;
    CSession          m_session;
    QStringList       m_lstFormats;
    Qt::DropAction    m_defAction;
    Qt::DropActions   m_actions;
    mutable State     m_enmState;
    mutable QVariant  m_data;
};

#endif /* ___UIDnDMIMEData_h___ */

