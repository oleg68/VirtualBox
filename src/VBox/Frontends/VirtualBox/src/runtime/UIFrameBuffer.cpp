/* $Id$ */
/** @file
 * VBox Qt GUI - UIFrameBuffer class implementation.
 */

/*
 * Copyright (C) 2010-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include "precomp.h"
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */
/* Qt includes: */
# include <QPainter>
/* GUI includes: */
# include "UIFrameBuffer.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindow.h"
# include "UIMachineView.h"
# include "UIPopupCenter.h"
# include "VBoxGlobal.h"
# ifndef VBOX_WITH_TRANSLUCENT_SEAMLESS
#  include "UIMachineWindow.h"
# endif /* !VBOX_WITH_TRANSLUCENT_SEAMLESS */
/* COM includes: */
#include "CConsole.h"
#include "CDisplay.h"
/* Other VBox includes: */
# include <VBox/VBoxVideo3D.h>
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* COM stuff: */
#ifdef Q_WS_WIN
static CComModule _Module;
#else /* !Q_WS_WIN */
NS_DECL_CLASSINFO(UIFrameBuffer)
NS_IMPL_THREADSAFE_ISUPPORTS1_CI(UIFrameBuffer, IFramebuffer)
#endif /* !Q_WS_WIN */

UIFrameBuffer::UIFrameBuffer(UIMachineView *pMachineView)
    : m_iWidth(0), m_iHeight(0)
    , m_pMachineView(pMachineView)
    , m_iWinId(0)
    , m_fUpdatesAllowed(true)
    , m_fUnused(false)
    , m_fAutoEnabled(false)
#ifdef Q_OS_WIN
    , m_iRefCnt(0)
#endif /* Q_OS_WIN */
    , m_hiDPIOptimizationType(HiDPIOptimizationType_None)
    , m_dBackingScaleFactor(1.0)
{
    /* Assign mahine-view: */
    AssertMsg(m_pMachineView, ("UIMachineView must not be NULL\n"));
    /* Cache window ID: */
    m_iWinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

    /* Initialize critical-section: */
    int rc = RTCritSectInit(&m_critSect);
    AssertRC(rc);

    /* Connect handlers: */
    if (m_pMachineView)
        prepareConnections();

    /* Resize frame-buffer to default size: */
    resizeEvent(640, 480);
}

UIFrameBuffer::~UIFrameBuffer()
{
    /* Disconnect handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Deinitialize critical-section: */
    RTCritSectDelete(&m_critSect);
}

void UIFrameBuffer::setView(UIMachineView *pMachineView)
{
    /* Disconnect old handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Reassign machine-view: */
    m_pMachineView = pMachineView;
    /* Recache window ID: */
    m_iWinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

    /* Connect new handlers: */
    if (m_pMachineView)
        prepareConnections();
}

void UIFrameBuffer::setMarkAsUnused(bool fUnused)
{
    lock();
    m_fUnused = fUnused;
    unlock();
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Width)(ULONG *puWidth)
{
    if (!puWidth)
        return E_POINTER;
    *puWidth = (ULONG)width();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Height)(ULONG *puHeight)
{
    if (!puHeight)
        return E_POINTER;
    *puHeight = (ULONG)height();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(BitsPerPixel)(ULONG *puBitsPerPixel)
{
    if (!puBitsPerPixel)
        return E_POINTER;
    *puBitsPerPixel = bitsPerPixel();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(BytesPerLine)(ULONG *puBytesPerLine)
{
    if (!puBytesPerLine)
        return E_POINTER;
    *puBytesPerLine = bytesPerLine();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(PixelFormat)(ULONG *puPixelFormat)
{
    if (!puPixelFormat)
        return E_POINTER;
    *puPixelFormat = pixelFormat();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(HeightReduction)(ULONG *puHeightReduction)
{
    if (!puHeightReduction)
        return E_POINTER;
    *puHeightReduction = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Overlay)(IFramebufferOverlay **ppOverlay)
{
    if (!ppOverlay)
        return E_POINTER;
    *ppOverlay = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(WinId)(LONG64 *pWinId)
{
    if (!pWinId)
        return E_POINTER;
    *pWinId = m_iWinId;
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::NotifyChange(ULONG uScreenId, ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    CDisplaySourceBitmap sourceBitmap;
    m_pMachineView->session().GetConsole().GetDisplay().QuerySourceBitmap(uScreenId, sourceBitmap);

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel(("UIFrameBuffer::NotifyChange: Screen=%lu, Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                (unsigned long)uScreenId,
                (unsigned long)uX, (unsigned long)uY,
                (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyChange: */
        return E_FAIL;
    }

    /* Disable screen updates: */
    m_fUpdatesAllowed = false;

    /* While updates are disabled, visible region will be saved:  */
    m_pendingSyncVisibleRegion = QRegion();

    /* Acquire new pending bitmap: */
    m_pendingSourceBitmap = sourceBitmap;

    /* Widget resize is NOT thread-safe and *probably* never will be,
     * We have to notify machine-view with the async-signal to perform resize operation. */
    LogRel(("UIFrameBuffer::NotifyChange: Screen=%lu, Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
            (unsigned long)uScreenId,
            (unsigned long)uX, (unsigned long)uY,
            (unsigned long)uWidth, (unsigned long)uHeight));
    emit sigNotifyChange(uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Give up control token to other thread: */
    RTThreadYield();

    /* Confirm NotifyChange: */
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::NotifyUpdate(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    /* Retina screens with physical-to-logical scaling requires
     * odd/even pixel updates to be taken into account,
     * otherwise we have artifacts on the borders of incoming rectangle. */
    uX = qMax(0, (int)uX - 1);
    uY = qMax(0, (int)uY - 1);
    uWidth = qMin(m_iWidth, (int)uWidth + 2);
    uHeight = qMin(m_iHeight, (int)uHeight + 2);

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("UIFrameBuffer::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyUpdate: */
        return E_FAIL;
    }

    /* Widget update is NOT thread-safe and *seems* never will be,
     * We have to notify machine-view with the async-signal to perform update operation. */
    LogRel2(("UIFrameBuffer::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
             (unsigned long)uX, (unsigned long)uY,
             (unsigned long)uWidth, (unsigned long)uHeight));
    emit sigNotifyUpdate(uX, uY, uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm NotifyUpdate: */
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::NotifyUpdateImage(ULONG aX,
                                              ULONG aY,
                                              ULONG aWidth,
                                              ULONG aHeight,
                                              ComSafeArrayIn(BYTE, aImage))
{
    return E_NOTIMPL;
}

STDMETHODIMP UIFrameBuffer::VideoModeSupported(ULONG uWidth, ULONG uHeight, ULONG uBPP, BOOL *pfSupported)
{
    /* Make sure result pointer is valid: */
    if (!pfSupported)
    {
        LogRel2(("UIFrameBuffer::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Invalid pfSupported pointer!\n",
                 (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("UIFrameBuffer::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore VideoModeSupported: */
        return E_FAIL;
    }

    /* Determine if supported: */
    *pfSupported = TRUE;
    QSize screenSize = m_pMachineView->maxGuestSize();
    if (   (screenSize.width() != 0)
        && (uWidth > (ULONG)screenSize.width())
        && (uWidth > (ULONG)width()))
        *pfSupported = FALSE;
    if (   (screenSize.height() != 0)
        && (uHeight > (ULONG)screenSize.height())
        && (uHeight > (ULONG)height()))
        *pfSupported = FALSE;
    LogRel2(("UIFrameBuffer::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Supported=%s\n",
             (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight, *pfSupported ? "TRUE" : "FALSE"));

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm VideoModeSupported: */
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::GetVisibleRegion(BYTE *pRectangles, ULONG uCount, ULONG *puCountCopied)
{
    PRTRECT rects = (PRTRECT)pRectangles;

    if (!rects)
        return E_POINTER;

    Q_UNUSED(uCount);
    Q_UNUSED(puCountCopied);

    return S_OK;
}

STDMETHODIMP UIFrameBuffer::SetVisibleRegion(BYTE *pRectangles, ULONG uCount)
{
    /* Make sure rectangles were passed: */
    if (!pRectangles)
    {
        LogRel2(("UIFrameBuffer::SetVisibleRegion: Rectangle count=%lu, Invalid pRectangles pointer!\n",
                 (unsigned long)uCount));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("UIFrameBuffer::SetVisibleRegion: Rectangle count=%lu, Ignored!\n",
                 (unsigned long)uCount));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore SetVisibleRegion: */
        return E_FAIL;
    }

    /* Compose region: */
    QRegion region;
    PRTRECT rects = (PRTRECT)pRectangles;
    for (ULONG ind = 0; ind < uCount; ++ind)
    {
        /* Get current rectangle: */
        QRect rect;
        rect.setLeft(rects->xLeft);
        rect.setTop(rects->yTop);
        /* Which is inclusive: */
        rect.setRight(rects->xRight - 1);
        rect.setBottom(rects->yBottom - 1);
        /* Append region: */
        region += rect;
        ++rects;
    }

    if (m_fUpdatesAllowed)
    {
        /* We are directly updating synchronous visible-region: */
        m_syncVisibleRegion = region;
        /* And send async-signal to update asynchronous one: */
        LogRel2(("UIFrameBuffer::SetVisibleRegion: Rectangle count=%lu, Sending to async-handler\n",
                 (unsigned long)uCount));
        emit sigSetVisibleRegion(region);
    }
    else
    {
        /* Save the region. */
        m_pendingSyncVisibleRegion = region;
        LogRel2(("UIFrameBuffer::SetVisibleRegion: Rectangle count=%lu, Saved\n",
                 (unsigned long)uCount));
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm SetVisibleRegion: */
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::ProcessVHWACommand(BYTE *pCommand)
{
    Q_UNUSED(pCommand);
    return E_NOTIMPL;
}

STDMETHODIMP UIFrameBuffer::Notify3DEvent(ULONG uType, ComSafeArrayIn(BYTE, aData))
{
    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("UIFrameBuffer::Notify3DEvent: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore Notify3DEvent: */
        return E_FAIL;
    }

    com::SafeArray<BYTE> data(ComSafeArrayInArg(aData));
    switch (uType)
    {
        case VBOX3D_NOTIFY_EVENT_TYPE_VISIBLE_3DDATA:
        {
            /* Notify machine-view with the async-signal
             * about 3D overlay visibility change: */
            BOOL fVisible = data[0];
            LogRel2(("UIFrameBuffer::Notify3DEvent: Sending to async-handler: "
                     "(VBOX3D_NOTIFY_EVENT_TYPE_VISIBLE_3DDATA = %s)\n",
                     fVisible ? "TRUE" : "FALSE"));
            emit sigNotifyAbout3DOverlayVisibilityChange(fVisible);

            /* Unlock access to frame-buffer: */
            unlock();

            /* Confirm Notify3DEvent: */
            return S_OK;
        }

        case VBOX3D_NOTIFY_EVENT_TYPE_TEST_FUNCTIONAL:
        {
            HRESULT hr = m_fUnused ? E_FAIL : S_OK;
            unlock();
            return hr;
        }

        default:
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Ignore Notify3DEvent: */
    return E_INVALIDARG;
}

void UIFrameBuffer::notifyChange(int iWidth, int iHeight)
{
    LogRel(("UIFrameBuffer::notifyChange: Size=%dx%d\n", iWidth, iHeight));

    /* Make sure machine-view is assigned: */
    AssertPtrReturnVoid(m_pMachineView);

    /* Lock access to frame-buffer: */
    lock();

    /* If there is NO pending source-bitmap: */
    if (m_pendingSourceBitmap.isNull())
    {
        /* Do nothing, change-event already processed: */
        LogRel2(("UIFrameBuffer::notifyChange: Already processed.\n"));
        /* Unlock access to frame-buffer: */
        unlock();
        /* Return immediately: */
        return;
    }

    /* Release the current bitmap and keep the pending one: */
    m_sourceBitmap = m_pendingSourceBitmap;
    m_pendingSourceBitmap = 0;

    /* Unlock access to frame-buffer: */
    unlock();

    /* Perform frame-buffer resize: */
    resizeEvent(iWidth, iHeight);
}

void UIFrameBuffer::resizeEvent(int iWidth, int iHeight)
{
    LogRel(("UIFrameBuffer::resizeEvent: Size=%dx%d\n", iWidth, iHeight));

    /* Make sure machine-view is assigned: */
    AssertPtrReturnVoid(m_pMachineView);

    /* Invalidate visible-region (if necessary): */
    if (m_pMachineView->machineLogic()->visualStateType() == UIVisualStateType_Seamless &&
        (m_iWidth != iWidth || m_iHeight != iHeight))
    {
        lock();
        m_syncVisibleRegion = QRegion();
        m_asyncVisibleRegion = QRegion();
        unlock();
    }

    /* If source-bitmap invalid: */
    if (m_sourceBitmap.isNull())
    {
        LogRel(("UIFrameBuffer::resizeEvent: "
                "Using FALLBACK buffer due to source-bitmap is not provided..\n"));

        /* Remember new size came from hint: */
        m_iWidth = iWidth;
        m_iHeight = iHeight;

        /* And recreate fallback buffer: */
        m_image = QImage(m_iWidth, m_iHeight, QImage::Format_RGB32);
        m_image.fill(0);
    }
    /* If source-bitmap valid: */
    else
    {
        LogRel(("UIFrameBuffer::resizeEvent: "
                "Directly using source-bitmap content\n"));

        /* Acquire source-bitmap attributes: */
        BYTE *pAddress = NULL;
        ULONG ulWidth = 0;
        ULONG ulHeight = 0;
        ULONG ulBitsPerPixel = 0;
        ULONG ulBytesPerLine = 0;
        ULONG ulPixelFormat = 0;
        m_sourceBitmap.QueryBitmapInfo(pAddress,
                                       ulWidth,
                                       ulHeight,
                                       ulBitsPerPixel,
                                       ulBytesPerLine,
                                       ulPixelFormat);
        Assert(ulBitsPerPixel == 32);

        /* Remember new actual size: */
        m_iWidth = (int)ulWidth;
        m_iHeight = (int)ulHeight;

        /* Recreate QImage on the basis of source-bitmap content: */
        m_image = QImage(pAddress, m_iWidth, m_iHeight, ulBytesPerLine, QImage::Format_RGB32);

        /* Check whether guest color depth differs from the bitmap color depth: */
        ULONG ulGuestBitsPerPixel = 0;
        LONG xOrigin = 0;
        LONG yOrigin = 0;
        CDisplay display = m_pMachineView->uisession()->session().GetConsole().GetDisplay();
        display.GetScreenResolution(m_pMachineView->screenId(),
                                    ulWidth, ulHeight, ulGuestBitsPerPixel, xOrigin, yOrigin);

        /* Remind user if necessary: */
        if (   ulGuestBitsPerPixel != ulBitsPerPixel
            && m_pMachineView->uisession()->isGuestAdditionsActive())
            popupCenter().remindAboutWrongColorDepth(m_pMachineView->machineWindow(),
                                                     ulGuestBitsPerPixel, ulBitsPerPixel);
        else
            popupCenter().forgetAboutWrongColorDepth(m_pMachineView->machineWindow());
    }

    lock();

    /* Enable screen updates: */
    m_fUpdatesAllowed = true;

    if (!m_pendingSyncVisibleRegion.isEmpty())
    {
        /* Directly update synchronous visible-region: */
        m_syncVisibleRegion = m_pendingSyncVisibleRegion;
        m_pendingSyncVisibleRegion = QRegion();

        /* And send async-signal to update asynchronous one: */
        LogRel2(("UIFrameBuffer::resizeEvent: Rectangle count=%lu, Sending to async-handler\n",
                 (unsigned long)m_syncVisibleRegion.rectCount()));
        emit sigSetVisibleRegion(m_syncVisibleRegion);
    }

    unlock();
}

void UIFrameBuffer::paintEvent(QPaintEvent *pEvent)
{
    LogRel2(("UIFrameBuffer::paintEvent: Origin=%lux%lu, Size=%dx%d\n",
             pEvent->rect().x(), pEvent->rect().y(),
             pEvent->rect().width(), pEvent->rect().height()));

    /* On mode switch the enqueued paint-event may still come
     * while the machine-view is already null (before the new machine-view set),
     * ignore paint-event in that case. */
    if (!m_pMachineView)
        return;

    /* Lock access to frame-buffer: */
    lock();

    /* But if updates disabled: */
    if (!m_fUpdatesAllowed)
    {
        /* Unlock access to frame-buffer: */
        unlock();
        /* And return immediately: */
        return;
    }

    /* Depending on visual-state type: */
    switch (m_pMachineView->machineLogic()->visualStateType())
    {
        case UIVisualStateType_Seamless:
            paintSeamless(pEvent);
            break;
        case UIVisualStateType_Scale:
            paintScaled(pEvent);
            break;
        default:
            paintDefault(pEvent);
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();
}

void UIFrameBuffer::applyVisibleRegion(const QRegion &region)
{
    /* Make sure async visible-region has changed: */
    if (m_asyncVisibleRegion == region)
        return;

    /* We are accounting async visible-regions one-by-one
     * to keep corresponding viewport area always updated! */
    if (!m_asyncVisibleRegion.isEmpty())
        m_pMachineView->viewport()->update(m_asyncVisibleRegion - region);

    /* Remember last visible region: */
    m_asyncVisibleRegion = region;

#ifndef VBOX_WITH_TRANSLUCENT_SEAMLESS
    /* We have to use async visible-region to apply to [Q]Widget [set]Mask: */
    m_pMachineView->machineWindow()->setMask(m_asyncVisibleRegion);
#endif /* !VBOX_WITH_TRANSLUCENT_SEAMLESS */
}

#ifdef VBOX_WITH_VIDEOHWACCEL
void UIFrameBuffer::doProcessVHWACommand(QEvent *pEvent)
{
    Q_UNUSED(pEvent);
    /* should never be here */
    AssertBreakpoint();
    // TODO: Is this required? ^
}
#endif /* VBOX_WITH_VIDEOHWACCEL */

void UIFrameBuffer::prepareConnections()
{
    connect(this, SIGNAL(sigNotifyChange(int, int)),
            m_pMachineView, SLOT(sltHandleNotifyChange(int, int)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigNotifyUpdate(int, int, int, int)),
            m_pMachineView, SLOT(sltHandleNotifyUpdate(int, int, int, int)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigSetVisibleRegion(QRegion)),
            m_pMachineView, SLOT(sltHandleSetVisibleRegion(QRegion)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigNotifyAbout3DOverlayVisibilityChange(bool)),
            m_pMachineView, SLOT(sltHandle3DOverlayVisibilityChange(bool)),
            Qt::QueuedConnection);
}

void UIFrameBuffer::cleanupConnections()
{
    disconnect(this, SIGNAL(sigNotifyChange(int, int)),
               m_pMachineView, SLOT(sltHandleNotifyChange(int, int)));
    disconnect(this, SIGNAL(sigNotifyUpdate(int, int, int, int)),
               m_pMachineView, SLOT(sltHandleNotifyUpdate(int, int, int, int)));
    disconnect(this, SIGNAL(sigSetVisibleRegion(QRegion)),
               m_pMachineView, SLOT(sltHandleSetVisibleRegion(QRegion)));
    disconnect(this, SIGNAL(sigNotifyAbout3DOverlayVisibilityChange(bool)),
               m_pMachineView, SLOT(sltHandle3DOverlayVisibilityChange(bool)));
}

void UIFrameBuffer::paintDefault(QPaintEvent *pEvent)
{
    /* Get rectangle to paint: */
    QRect paintRect = pEvent->rect().intersected(m_image.rect()).intersected(m_pMachineView->viewport()->geometry());
    if (paintRect.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

    /* Draw image rectangle: */
    drawImageRect(painter, m_image, paintRect,
                  m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                  hiDPIOptimizationType(), backingScaleFactor());
}

void UIFrameBuffer::paintSeamless(QPaintEvent *pEvent)
{
    /* Get rectangle to paint: */
    QRect paintRect = pEvent->rect().intersected(m_image.rect()).intersected(m_pMachineView->viewport()->geometry());
    if (paintRect.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

    /* Determine the region to erase: */
    lock();
    QRegion regionToErase = (QRegion)paintRect - m_syncVisibleRegion;
    unlock();
    if (!regionToErase.isEmpty())
    {
        /* Optimize composition-mode: */
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        /* Erase required region, slowly, rectangle-by-rectangle: */
        foreach (const QRect &rect, regionToErase.rects())
            painter.eraseRect(rect);
        /* Restore composition-mode: */
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    /* Determine the region to paint: */
    lock();
    QRegion regionToPaint = (QRegion)paintRect & m_syncVisibleRegion;
    unlock();
    if (!regionToPaint.isEmpty())
    {
        /* Paint required region, slowly, rectangle-by-rectangle: */
        foreach (const QRect &rect, regionToPaint.rects())
        {
#if defined(VBOX_WITH_TRANSLUCENT_SEAMLESS) && defined(Q_WS_WIN)
            /* Replace translucent background with black one,
             * that is necessary for window with Qt::WA_TranslucentBackground: */
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(rect, QColor(Qt::black));
            painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
#endif /* VBOX_WITH_TRANSLUCENT_SEAMLESS && Q_WS_WIN */

            /* Draw image rectangle: */
            drawImageRect(painter, m_image, rect,
                          m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                          hiDPIOptimizationType(), backingScaleFactor());
        }
    }
}

void UIFrameBuffer::paintScaled(QPaintEvent *pEvent)
{
    /* Scaled image is NULL by default: */
    QImage scaledImage;
    /* But if scaled-factor is set and current image is NOT null: */
    if (m_scaledSize.isValid() && !m_image.isNull())
    {
        /* We are doing a deep copy of the image to make sure it will not be
         * detached during scale process, otherwise we can get a frozen frame-buffer. */
        scaledImage = m_image.copy();
        /* And scaling the image to predefined scaled-factor: */
        scaledImage = scaledImage.scaled(m_scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    /* Finally we are choosing image to paint from: */
    QImage &sourceImage = scaledImage.isNull() ? m_image : scaledImage;

    /* Get rectangle to paint: */
    QRect paintRect = pEvent->rect().intersected(sourceImage.rect()).intersected(m_pMachineView->viewport()->geometry());
    if (paintRect.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

    /* Draw image rectangle: */
    drawImageRect(painter, sourceImage, paintRect,
                  m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                  hiDPIOptimizationType(), backingScaleFactor());
}

/* static */
void UIFrameBuffer::drawImageRect(QPainter &painter, const QImage &image, const QRect &rect,
                                  int iContentsShiftX, int iContentsShiftY,
                                  HiDPIOptimizationType hiDPIOptimizationType,
                                  double dBackingScaleFactor)
{
    /* Calculate offset: */
    size_t offset = (rect.x() + iContentsShiftX) * image.depth() / 8 +
                    (rect.y() + iContentsShiftY) * image.bytesPerLine();

    /* Restrain boundaries: */
    int iSubImageWidth = qMin(rect.width(), image.width() - rect.x() - iContentsShiftX);
    int iSubImageHeight = qMin(rect.height(), image.height() - rect.y() - iContentsShiftY);

    /* Create sub-image (no copy involved): */
    QImage subImage = QImage(image.bits() + offset,
                             iSubImageWidth, iSubImageHeight,
                             image.bytesPerLine(), image.format());

#ifndef QIMAGE_FRAMEBUFFER_WITH_DIRECT_OUTPUT
    /* Create sub-pixmap on the basis of sub-image above (1st copy involved): */
    QPixmap subPixmap = QPixmap::fromImage(subImage);

    /* If HiDPI 'backing scale factor' defined: */
    if (dBackingScaleFactor > 1.0)
    {
        /* Should we optimize HiDPI output for performance? */
        if (hiDPIOptimizationType == HiDPIOptimizationType_Performance)
        {
            /* Fast scale sub-pixmap (2nd copy involved): */
            subPixmap = subPixmap.scaled(subPixmap.size() * dBackingScaleFactor,
                                         Qt::IgnoreAspectRatio, Qt::FastTransformation);
# ifdef Q_WS_MAC
#  ifdef VBOX_GUI_WITH_HIDPI
            /* Mark sub-pixmap as HiDPI: */
            subPixmap.setDevicePixelRatio(dBackingScaleFactor);
#  endif /* VBOX_GUI_WITH_HIDPI */
# endif /* Q_WS_MAC */
        }
    }

    /* Draw sub-pixmap: */
    painter.drawPixmap(rect.x(), rect.y(), subPixmap);
#else /* QIMAGE_FRAMEBUFFER_WITH_DIRECT_OUTPUT */
    /* If HiDPI 'backing scale factor' defined: */
    if (dBackingScaleFactor > 1.0)
    {
        /* Should we optimize HiDPI output for performance? */
        if (hiDPIOptimizationType == HiDPIOptimizationType_Performance)
        {
            /* Create fast-scaled-sub-image (1st copy involved): */
            QImage scaledSubImage = subImage.scaled(subImage.size() * dBackingScaleFactor,
                                                    Qt::IgnoreAspectRatio, Qt::FastTransformation);
# ifdef Q_WS_MAC
#  ifdef VBOX_GUI_WITH_HIDPI
            /* Mark sub-pixmap as HiDPI: */
            scaledSubImage.setDevicePixelRatio(dBackingScaleFactor);
#  endif /* VBOX_GUI_WITH_HIDPI */
# endif /* Q_WS_MAC */
            /* Directly draw scaled-sub-image: */
            painter.drawImage(rect.x(), rect.y(), scaledSubImage);
            return;
        }
    }
    /* Directly draw sub-image: */
    painter.drawImage(rect.x(), rect.y(), subImage);
#endif /* QIMAGE_FRAMEBUFFER_WITH_DIRECT_OUTPUT */
}

