/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIGDetails class declaration
 */

/*
 * Copyright (C) 2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIGDetails_h__
#define __UIGDetails_h__

/* Qt includes: */
#include <QWidget>

/* Forward declartions: */
class QVBoxLayout;
class UIGDetailsModel;
class UIGDetailsView;
class UIVMItem;

/* Details widget: */
class UIGDetails : public QWidget
{
    Q_OBJECT;

signals:

    /* Link-click stuff: */
    void sigLinkClicked(const QString &strCategory, const QString &strControl, const QString &strId);

public:

    /* Constructor: */
    UIGDetails(QWidget *pParent);

    /* API: Group/machine stuff: */
    void setItems(const QList<UIVMItem*> &items);

private:

    /* Helpers: Prepare stuff: */
    void prepareConnections();

    /* Variables: */
    QVBoxLayout *m_pMainLayout;
    UIGDetailsModel *m_pDetailsModel;
    UIGDetailsView *m_pDetailsView;
};

#endif /* __UIGDetails_h__ */

