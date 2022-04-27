/* $Id$ */
/** @file
 * VBox Qt GUI - Update routine related implementations.
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
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
#include <QCoreApplication>
#include <QLocale>
#include <QStringList>

/* GUI includes: */
#include "UICommon.h"
#include "UIUpdateDefs.h"


/* static: */
VBoxUpdateDayList VBoxUpdateData::m_dayList = VBoxUpdateDayList();

/* static */
void VBoxUpdateData::populate()
{
    /* Clear list initially: */
    m_dayList.clear();

    // WORKAROUND:
    // To avoid re-translation complexity
    // all values will be retranslated separately.

    /* Separately retranslate each day: */
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 day"),  "1 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "2 days"), "2 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "3 days"), "3 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "4 days"), "4 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "5 days"), "5 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "6 days"), "6 d");

    /* Separately retranslate each week: */
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 week"),  "1 w");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "2 weeks"), "2 w");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "3 weeks"), "3 w");

    /* Separately retranslate each month: */
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 month"), "1 m");
}

/* static */
QStringList VBoxUpdateData::list()
{
    QStringList result;
    for (int i = 0; i < m_dayList.size(); ++i)
        result << m_dayList.at(i).val;
    return result;
}

VBoxUpdateData::VBoxUpdateData(const QString &strData)
    : m_strData(strData)
    , m_enmPeriodIndex(Period1Day)
    , m_enmUpdateChannel(KUpdateChannel_Stable)
{
    decode();
}

VBoxUpdateData::VBoxUpdateData(PeriodType enmPeriodIndex, KUpdateChannel enmUpdateChannel)
    : m_strData(QString())
    , m_enmPeriodIndex(enmPeriodIndex)
    , m_enmUpdateChannel(enmUpdateChannel)
{
    encode();
}

bool VBoxUpdateData::isNoNeedToCheck() const
{
    /* No need to check if Period == Never: */
    return m_enmPeriodIndex == PeriodNever;
}

bool VBoxUpdateData::isNeedToCheck() const
{
    /* Return 'false' if there is no need to check: */
    if (isNoNeedToCheck())
        return false;

    /* Return 'true' if date of next check is today or missed: */
    if (QDate::currentDate() >= m_date)
        return true;

    /* Return 'true' if saved version value is NOT valid or NOT equal to current: */
    if (!version().isValid() || version() != UIVersion(uiCommon().vboxVersionStringNormalized()))
        return true;

    /* Return 'false' in all other cases: */
    return false;
}

QString VBoxUpdateData::data() const
{
    return m_strData;
}

VBoxUpdateData::PeriodType VBoxUpdateData::periodIndex() const
{
    return m_enmPeriodIndex;
}

QString VBoxUpdateData::date() const
{
    return isNoNeedToCheck() ? QCoreApplication::translate("UIUpdateManager", "Never")
                             : QLocale::system().toString(m_date, QLocale::ShortFormat);
}

QDate VBoxUpdateData::internalDate() const
{
    return m_date;
}

KUpdateChannel VBoxUpdateData::updateChannel() const
{
    return m_enmUpdateChannel;
}

QString VBoxUpdateData::updateChannelName() const
{
    return updateChannelToInternalString(m_enmUpdateChannel);
}

UIVersion VBoxUpdateData::version() const
{
    return m_version;
}

bool VBoxUpdateData::isEqual(const VBoxUpdateData &another) const
{
    return    true
           && (m_strData == another.data())
           && (m_enmPeriodIndex == another.periodIndex())
           && (m_date == another.internalDate())
           && (m_enmUpdateChannel == another.updateChannel())
           && (m_version == another.version())
              ;
}

bool VBoxUpdateData::operator==(const VBoxUpdateData &another) const
{
    return isEqual(another);
}

bool VBoxUpdateData::operator!=(const VBoxUpdateData &another) const
{
    return !isEqual(another);
}

/* static */
QString VBoxUpdateData::updateChannelToInternalString(KUpdateChannel enmUpdateChannel)
{
    switch (enmUpdateChannel)
    {
        case KUpdateChannel_WithTesting: return "withtesting";
        case KUpdateChannel_WithBetas: return "withbetas";
        case KUpdateChannel_All: return "allrelease";
        default: return "stable";
    }
}

/* static */
KUpdateChannel VBoxUpdateData::updateChannelFromInternalString(const QString &strUpdateChannel)
{
    QMap<QString, KUpdateChannel> pairs;
    pairs["withtesting"] = KUpdateChannel_WithTesting;
    pairs["withbetas"] = KUpdateChannel_WithBetas;
    pairs["allrelease"] = KUpdateChannel_All;
    return pairs.value(strUpdateChannel, KUpdateChannel_Stable);
}

void VBoxUpdateData::decode()
{
    /* Parse standard values: */
    if (m_strData == "never")
        m_enmPeriodIndex = PeriodNever;
    /* Parse other values: */
    else
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QStringList parser = m_strData.split(", ", Qt::SkipEmptyParts);
#else
        QStringList parser = m_strData.split(", ", QString::SkipEmptyParts);
#endif

        /* Parse 'period' value: */
        if (parser.size() > 0)
        {
            if (m_dayList.isEmpty())
                populate();
            PeriodType index = (PeriodType)m_dayList.indexOf(VBoxUpdateDay(QString(), parser.at(0)));
            m_enmPeriodIndex = index == PeriodUndefined ? Period1Day : index;
        }

        /* Parse 'date' value: */
        if (parser.size() > 1)
        {
            QDate date = QDate::fromString(parser.at(1), Qt::ISODate);
            m_date = date.isValid() ? date : QDate::currentDate();
        }

        /* Parse 'update channel' value: */
        if (parser.size() > 2)
        {
            m_enmUpdateChannel = updateChannelFromInternalString(parser.at(2));
        }

        /* Parse 'version' value: */
        if (parser.size() > 3)
        {
            m_version = UIVersion(parser.at(3));
        }
    }
}

void VBoxUpdateData::encode()
{
    /* Encode standard values: */
    if (m_enmPeriodIndex == PeriodNever)
        m_strData = "never";
    /* Encode other values: */
    else
    {
        /* Encode 'period' value: */
        if (m_dayList.isEmpty())
            populate();
        QString remindPeriod = m_dayList[m_enmPeriodIndex].key;

        /* Encode 'date' value: */
        m_date = QDate::currentDate();
        QStringList parser(remindPeriod.split(' '));
        if (parser[1] == "d")
            m_date = m_date.addDays(parser[0].toInt());
        else if (parser[1] == "w")
            m_date = m_date.addDays(parser[0].toInt() * 7);
        else if (parser[1] == "m")
            m_date = m_date.addMonths(parser[0].toInt());
        QString remindDate = m_date.toString(Qt::ISODate);

        /* Encode 'update channel' value: */
        QString strUpdateChannel = updateChannelName();

        /* Encode 'version' value: */
        QString versionValue = UIVersion(uiCommon().vboxVersionStringNormalized()).toString();

        /* Composite m_strData: */
        m_strData = QString("%1, %2, %3, %4").arg(remindPeriod, remindDate, strUpdateChannel, versionValue);
    }
}

