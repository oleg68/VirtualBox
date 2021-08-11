/* $Id$ */
/** @file
 * VBox Qt GUI - UIDownloader class implementation.
 */

/*
 * Copyright (C) 2006-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* GUI includes: */
#include "UICommon.h"
#include "VBoxUtils.h"
#include "UIDownloader.h"
#include "UIMessageCenter.h"
#include "UINetworkReply.h"


UIDownloader::UIDownloader()
    : m_state(UIDownloaderState_Null)
{
    /* Connect async listeners for our own commands: */
    connect(this, &UIDownloader::sigToStartAcknowledging, this, &UIDownloader::sltStartAcknowledging, Qt::QueuedConnection);
    connect(this, &UIDownloader::sigToStartDownloading,   this, &UIDownloader::sltStartDownloading,   Qt::QueuedConnection);
    connect(this, &UIDownloader::sigToStartVerifying,     this, &UIDownloader::sltStartVerifying,     Qt::QueuedConnection);
}

void UIDownloader::sltStartAcknowledging()
{
    /* Set state to acknowledging: */
    m_state = UIDownloaderState_Acknowledging;

    /* Send HEAD requests: */
    createNetworkRequest(UINetworkRequestType_HEAD, m_sources);
}

void UIDownloader::sltStartDownloading()
{
    /* Set state to downloading: */
    m_state = UIDownloaderState_Downloading;

    /* Send GET request: */
    createNetworkRequest(UINetworkRequestType_GET, QList<QUrl>() << m_source, m_strTarget);
}

void UIDownloader::sltStartVerifying()
{
    /* Set state to verifying: */
    m_state = UIDownloaderState_Verifying;

    /* Send GET request: */
    createNetworkRequest(UINetworkRequestType_GET, QList<QUrl>() << m_strPathSHA256SumsFile);
}

QString UIDownloader::description() const
{
    /* Look for known state: */
    switch (m_state)
    {
        case UIDownloaderState_Acknowledging: return tr("Looking for %1...");
        case UIDownloaderState_Downloading:   return tr("Downloading %1...");
        case UIDownloaderState_Verifying:     return tr("Verifying %1...");
        default:                              break;
    }
    /* Return null-string by default: */
    return QString();
}

void UIDownloader::processNetworkReplyProgress(qint64 iReceived, qint64 iTotal)
{
    /* Notify listeners: */
    emit sigProgressChange((double)iReceived / iTotal * 100);
}

void UIDownloader::processNetworkReplyFailed(const QString &strError)
{
    /* Notify listeners: */
    emit sigProgressFailed(strError);
}

void UIDownloader::processNetworkReplyCanceled(UINetworkReply *)
{
    /* Notify listeners: */
    emit sigProgressCanceled();
}

void UIDownloader::processNetworkReplyFinished(UINetworkReply *pNetworkReply)
{
    /* Process reply: */
    switch (m_state)
    {
        case UIDownloaderState_Acknowledging:
        {
            handleAcknowledgingResult(pNetworkReply);
            break;
        }
        case UIDownloaderState_Downloading:
        {
            handleDownloadingResult(pNetworkReply);
            break;
        }
        case UIDownloaderState_Verifying:
        {
            handleVerifyingResult(pNetworkReply);
            break;
        }
        default:
            break;
    }
}

void UIDownloader::handleAcknowledgingResult(UINetworkReply *pNetworkReply)
{
    /* Get the final source: */
    m_source = pNetworkReply->url();

    /* Ask for downloading: */
    if (askForDownloadingConfirmation(pNetworkReply))
    {
        /* Start downloading: */
        startDelayedDownloading();
    }
    else
    {
        /* Notify listeners: */
        emit sigProgressFinished();
    }
}

void UIDownloader::handleDownloadingResult(UINetworkReply *pNetworkReply)
{
    /* Handle downloaded object: */
    handleDownloadedObject(pNetworkReply);

    /* Check whether we should do verification: */
    if (!m_strPathSHA256SumsFile.isEmpty())
    {
        /* Start verifying: */
        startDelayedVerifying();
    }
    else
    {
        /* Notify listeners: */
        emit sigProgressFinished();
    }
}

void UIDownloader::handleVerifyingResult(UINetworkReply *pNetworkReply)
{
    /* Handle verified object: */
    handleVerifiedObject(pNetworkReply);

    /* Notify listeners: */
    emit sigProgressFinished();
}
