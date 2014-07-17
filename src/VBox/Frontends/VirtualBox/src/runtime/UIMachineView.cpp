/* $Id$ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIMachineView class implementation
 */

/*
 * Copyright (C) 2010-2012 Oracle Corporation
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
#include <QDesktopWidget>
#include <QMainWindow>
#include <QTimer>
#include <QPainter>
#include <QScrollBar>
#include <QMainWindow>
#include <VBox/VBoxVideo.h>
#include <iprt/asm.h>
#ifdef Q_WS_X11
# include <QX11Info>
#endif /* Q_WS_X11 */

/* GUI includes: */
#include "VBoxGlobal.h"
#include "UIMessageCenter.h"
#include "UIFrameBuffer.h"
#include "VBoxFBOverlay.h"
#include "UISession.h"
#include "UIKeyboardHandler.h"
#include "UIMouseHandler.h"
#include "UIMachineLogic.h"
#include "UIMachineWindow.h"
#include "UIMachineViewNormal.h"
#include "UIMachineViewFullscreen.h"
#include "UIMachineViewSeamless.h"
#include "UIMachineViewScale.h"
#include "UIExtraDataManager.h"

#ifdef VBOX_WITH_DRAG_AND_DROP
# include "UIDnDHandler.h"
#endif /* VBOX_WITH_DRAG_AND_DROP */

/* COM includes: */
#include "CSession.h"
#include "CConsole.h"
#include "CDisplay.h"
#include "CFramebuffer.h"
#ifdef VBOX_WITH_DRAG_AND_DROP
# include "CDnDSource.h"
# include "CDnDTarget.h"
# include "CGuest.h"
# include "CGuestDnDTarget.h"
# include "CGuestDnDSource.h"
#endif /* VBOX_WITH_DRAG_AND_DROP */

/* Other VBox includes: */
#ifdef Q_WS_X11
# include <X11/XKBlib.h>
# ifdef KeyPress
const int XFocusOut = FocusOut;
const int XFocusIn = FocusIn;
const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#  undef KeyRelease
#  undef KeyPress
#  undef FocusOut
#  undef FocusIn
# endif
#endif /* Q_WS_X11 */

#ifdef Q_WS_MAC
# include "DockIconPreview.h"
# include "DarwinKeyboard.h"
# include "UICocoaApplication.h"
# include <VBox/err.h>
# include <Carbon/Carbon.h>
#endif /* Q_WS_MAC */

UIMachineView* UIMachineView::create(  UIMachineWindow *pMachineWindow
                                     , ulong uScreenId
                                     , UIVisualStateType visualStateType
#ifdef VBOX_WITH_VIDEOHWACCEL
                                     , bool bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                     )
{
    UIMachineView *pMachineView = 0;
    switch (visualStateType)
    {
        case UIVisualStateType_Normal:
            pMachineView = new UIMachineViewNormal(  pMachineWindow
                                                   , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                   , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                                   );
            break;
        case UIVisualStateType_Fullscreen:
            pMachineView = new UIMachineViewFullscreen(  pMachineWindow
                                                       , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                       , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                                       );
            break;
        case UIVisualStateType_Seamless:
            pMachineView = new UIMachineViewSeamless(  pMachineWindow
                                                     , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                     , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                                     );
            break;
        case UIVisualStateType_Scale:
            pMachineView = new UIMachineViewScale(  pMachineWindow
                                                  , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                  , bAccelerate2DVideo
#endif
                                                  );
            break;
        default:
            break;
    }

    /* Prepare common things: */
    pMachineView->prepareCommon();

    /* Prepare event-filters: */
    pMachineView->prepareFilters();

    /* Prepare connections: */
    pMachineView->prepareConnections();

    /* Prepare console connections: */
    pMachineView->prepareConsoleConnections();

    /* Initialization: */
    pMachineView->sltMachineStateChanged();
    /** @todo Can we move the call to sltAdditionsStateChanged() from the
     * subclass constructors here too?  It is called for Normal and Seamless,
     * but not for Fullscreen and Scale.  However for Scale it is a no op.,
     * so it would not hurt.  Would it hurt for Fullscreen? */

    /* Set a preliminary maximum size: */
    pMachineView->setMaxGuestSize();

    return pMachineView;
}

void UIMachineView::destroy(UIMachineView *pMachineView)
{
    delete pMachineView;
}

double UIMachineView::aspectRatio() const
{
    return frameBuffer() ? (double)(frameBuffer()->width()) / frameBuffer()->height() : 0;
}

void UIMachineView::sltPerformGuestResize(const QSize &toSize)
{
    /* If this slot is invoked directly then use the passed size otherwise get
     * the available size for the guest display. We assume here that centralWidget()
     * contains this view only and gives it all available space: */
    QSize newSize(toSize.isValid() ? toSize : machineWindow()->centralWidget()->size());
    AssertMsg(newSize.isValid(), ("Size should be valid!\n"));

    /* Expand current limitations: */
    setMaxGuestSize(newSize);

    /* Send new size-hint to the guest: */
    LogRelFlow(("UIMachineView: Sending guest size-hint to screen %d: %dx%d\n",
                (int)screenId(), newSize.width(), newSize.height()));
    session().GetConsole().GetDisplay().SetVideoModeHint(screenId(),
                                                         uisession()->isScreenVisible(screenId()),
                                                         false, 0, 0, newSize.width(), newSize.height(), 0);
    /* And track whether we have a "normal" or "fullscreen"/"seamless" size-hint sent: */
    gEDataManager->markLastGuestSizeHintAsFullScreen(m_uScreenId, isFullscreenOrSeamless(), vboxGlobal().managedVMUuid());
}

void UIMachineView::sltHandleNotifyChange(int iWidth, int iHeight)
{
    LogRelFlow(("UIMachineView::HandleNotifyChange: Screen=%d, Size=%dx%d.\n",
                (unsigned long)m_uScreenId, iWidth, iHeight));

    // TODO: Move to appropriate place!
    /* Some situations require frame-buffer resize-events to be ignored at all,
     * leaving machine-window, machine-view and frame-buffer sizes preserved: */
    if (uisession()->isGuestResizeIgnored())
        return;

    /* If machine-window is visible: */
    if (uisession()->isScreenVisible(m_uScreenId))
    {
        // TODO: Move to appropriate place!
        /* Adjust 'scale' mode for current machine-view size: */
        if (visualStateType() == UIVisualStateType_Scale)
            frameBuffer()->setScaledSize(size());

        /* Perform frame-buffer mode-change: */
        frameBuffer()->notifyChange(iWidth, iHeight);

        /* Scale-mode doesn't need this.. */
        if (visualStateType() != UIVisualStateType_Scale)
        {
            /* Adjust maximum-size restriction for machine-view: */
            setMaximumSize(sizeHint());

            /* Disable the resize hint override hack: */
            m_sizeHintOverride = QSize(-1, -1);

            /* Force machine-window update own layout: */
            QCoreApplication::sendPostedEvents(0, QEvent::LayoutRequest);

            /* Update machine-view sliders: */
            updateSliders();

            /* By some reason Win host forgets to update machine-window central-widget
             * after main-layout was updated, let's do it for all the hosts: */
            machineWindow()->centralWidget()->update();

            /* Normalize machine-window geometry: */
            if (visualStateType() == UIVisualStateType_Normal)
                machineWindow()->normalizeGeometry(true /* adjust position */);
        }

#ifdef Q_WS_MAC
        /* Update MacOS X dock icon size: */
        machineLogic()->updateDockIconSize(screenId(), iWidth, iHeight);
#endif /* Q_WS_MAC */
    }

    /* Emit a signal about guest was resized: */
    emit resizeHintDone();

    LogRelFlow(("UIMachineView::ResizeHandled: Screen=%d, Size=%dx%d.\n",
                (unsigned long)m_uScreenId, iWidth, iHeight));
}

void UIMachineView::sltHandleNotifyUpdate(int iX, int iY, int iWidth, int iHeight)
{
    /* Update corresponding viewport part: */
    viewport()->update(iX - contentsX(), iY - contentsY(), iWidth, iHeight);
}

void UIMachineView::sltHandleSetVisibleRegion(QRegion region)
{
    /* Used only in seamless-mode. */
    Q_UNUSED(region);
}

void UIMachineView::sltHandle3DOverlayVisibilityChange(bool fVisible)
{
    machineLogic()->notifyAbout3DOverlayVisibilityChange(fVisible);
}

void UIMachineView::sltDesktopResized()
{
    setMaxGuestSize();
}

void UIMachineView::sltMachineStateChanged()
{
    /* Get machine state: */
    KMachineState state = uisession()->machineState();
    switch (state)
    {
        case KMachineState_Paused:
        case KMachineState_TeleportingPausedVM:
        {
            if (   m_pFrameBuffer
                && (   state           != KMachineState_TeleportingPausedVM
                    || m_previousState != KMachineState_Teleporting))
            {
                takePauseShotLive();
                /* Fully repaint to pick up m_pauseShot: */
                viewport()->update();
            }
            break;
        }
        case KMachineState_Restoring:
        {
            /* Only works with the primary screen currently. */
            if (screenId() == 0)
            {
                takePauseShotSnapshot();
                /* Fully repaint to pick up m_pauseShot: */
                viewport()->update();
            }
            break;
        }
        case KMachineState_Running:
        {
            if (m_previousState == KMachineState_Paused ||
                m_previousState == KMachineState_TeleportingPausedVM ||
                m_previousState == KMachineState_Restoring)
            {
                if (m_pFrameBuffer)
                {
                    /* Reset the pixmap to free memory: */
                    resetPauseShot();
                    /* Ask for full guest display update (it will also update
                     * the viewport through IFramebuffer::NotifyUpdate): */
                    CDisplay dsp = session().GetConsole().GetDisplay();
                    dsp.InvalidateAndUpdate();
                }
            }
            break;
        }
        default:
            break;
    }

    m_previousState = state;
}

UIMachineView::UIMachineView(  UIMachineWindow *pMachineWindow
                             , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                             , bool bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                             )
    : QAbstractScrollArea(pMachineWindow->centralWidget())
    , m_pMachineWindow(pMachineWindow)
    , m_uScreenId(uScreenId)
    , m_pFrameBuffer(0)
    , m_previousState(KMachineState_Null)
    , m_maxGuestSizePolicy(MaxGuestSizePolicy_Invalid)
    , m_u64MaxGuestSize(0)
#ifdef VBOX_WITH_VIDEOHWACCEL
    , m_fAccelerate2DVideo(bAccelerate2DVideo)
#endif /* VBOX_WITH_VIDEOHWACCEL */
{
    /* Load machine view settings: */
    loadMachineViewSettings();

    /* Prepare viewport: */
    prepareViewport();

    /* Prepare frame buffer: */
    prepareFrameBuffer();
}

UIMachineView::~UIMachineView()
{
}

void UIMachineView::prepareViewport()
{
    /* Prepare viewport: */
    AssertPtrReturnVoid(viewport());
    {
        /* Enable manual painting: */
        viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
        /* Enable multi-touch support: */
        viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
    }
}

void UIMachineView::prepareFrameBuffer()
{
    /* Prepare frame-buffer: */
    UIFrameBuffer *pFrameBuffer = uisession()->frameBuffer(screenId());
    if (pFrameBuffer)
    {
        pFrameBuffer->setView(this);
        /* Mark framebuffer as used again: */
        LogRelFlow(("UIMachineView::prepareFrameBuffer: Start EMT callbacks accepting for screen: %d.\n", screenId()));
        pFrameBuffer->setMarkAsUnused(false);
        m_pFrameBuffer = pFrameBuffer;
    }
    else
    {
# ifdef VBOX_WITH_VIDEOHWACCEL
        if (m_fAccelerate2DVideo)
        {
            ComObjPtr<VBoxOverlayFrameBuffer> pOFB;
            pOFB.createObject();
            pOFB->init(this, &session(), (uint32_t)screenId());
            m_pFrameBuffer = pOFB;
        }
        else
        {
            m_pFrameBuffer.createObject();
            m_pFrameBuffer->init(this);
        }
# else /* VBOX_WITH_VIDEOHWACCEL */
        m_pFrameBuffer.createObject();
        m_pFrameBuffer->init(this);
# endif /* !VBOX_WITH_VIDEOHWACCEL */
        m_pFrameBuffer->setHiDPIOptimizationType(uisession()->hiDPIOptimizationType());

        uisession()->setFrameBuffer(screenId(), m_pFrameBuffer);
    }

    /* If frame-buffer was prepared: */
    if (m_pFrameBuffer)
    {
        /* Prepare display: */
        CDisplay display = session().GetConsole().GetDisplay();
        Assert(!display.isNull());
        CFramebuffer fb = display.QueryFramebuffer(m_uScreenId);
        /* Always perform AttachFramebuffer to ensure 3D gets notified: */
        if (!fb.isNull())
            display.DetachFramebuffer(m_uScreenId);
        display.AttachFramebuffer(m_uScreenId, CFramebuffer(m_pFrameBuffer));
    }

    QSize size;
#ifdef Q_WS_X11
    /* Processing pseudo resize-event to synchronize frame-buffer with stored
     * framebuffer size. On X11 this will be additional done when the machine
     * state was 'saved'. */
    if (session().GetMachine().GetState() == KMachineState_Saved)
        size = guestSizeHint();
#endif /* Q_WS_X11 */
    /* If there is a preview image saved, we will resize the framebuffer to the
     * size of that image. */
    ULONG buffer = 0, width = 0, height = 0;
    CMachine machine = session().GetMachine();
    machine.QuerySavedScreenshotPNGSize(0, buffer, width, height);
    if (buffer > 0)
    {
        /* Init with the screenshot size */
        size = QSize(width, height);
        /* Try to get the real guest dimensions from the save state */
        ULONG guestOriginX = 0, guestOriginY = 0, guestWidth = 0, guestHeight = 0;
        BOOL fEnabled = true;
        machine.QuerySavedGuestScreenInfo(m_uScreenId, guestOriginX, guestOriginY, guestWidth, guestHeight, fEnabled);
        if (   guestWidth  > 0
            && guestHeight > 0)
            size = QSize(guestWidth, guestHeight);
    }
    /* If we have a valid size, resize the framebuffer. */
    if (size.width() > 0 && size.height() > 0)
        frameBuffer()->resizeEvent(size.width(), size.height());
}

void UIMachineView::prepareCommon()
{
    /* Prepare view frame: */
    setFrameStyle(QFrame::NoFrame);

    /* Setup palette: */
    QPalette palette(viewport()->palette());
    palette.setColor(viewport()->backgroundRole(), Qt::black);
    viewport()->setPalette(palette);

    /* Setup focus policy: */
    setFocusPolicy(Qt::WheelFocus);

#ifdef VBOX_WITH_DRAG_AND_DROP
    /* Enable Drag & Drop. */
    setAcceptDrops(true);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

void UIMachineView::prepareFilters()
{
    /* Enable MouseMove events: */
    viewport()->setMouseTracking(true);

    /* QScrollView does the below on its own, but let's
     * do it anyway for the case it will not do it in the future: */
    viewport()->installEventFilter(this);

    /* We want to be notified on some parent's events: */
    machineWindow()->installEventFilter(this);
}

void UIMachineView::prepareConnections()
{
    /* Desktop resolution change (e.g. monitor hotplug): */
    connect(QApplication::desktop(), SIGNAL(resized(int)), this,
            SLOT(sltDesktopResized()));
}

void UIMachineView::prepareConsoleConnections()
{
    /* Machine state-change updater: */
    connect(uisession(), SIGNAL(sigMachineStateChange()), this, SLOT(sltMachineStateChanged()));
}

void UIMachineView::loadMachineViewSettings()
{
    /* Global settings: */
    {
        /* Remember the maximum guest size policy for telling the guest about
         * video modes we like: */
        QString maxGuestSize = vboxGlobal().settings().publicProperty("GUI/MaxGuestResolution");
        if ((maxGuestSize == QString::null) || (maxGuestSize == "auto"))
            m_maxGuestSizePolicy = MaxGuestSizePolicy_Automatic;
        else if (maxGuestSize == "any")
            m_maxGuestSizePolicy = MaxGuestSizePolicy_Any;
        else  /** @todo Mea culpa, but what about error checking? */
        {
            int width  = maxGuestSize.section(',', 0, 0).toInt();
            int height = maxGuestSize.section(',', 1, 1).toInt();
            m_maxGuestSizePolicy = MaxGuestSizePolicy_Fixed;
            m_fixedMaxGuestSize = QSize(width, height);
        }
    }
}

void UIMachineView::cleanupFrameBuffer()
{
    /* Make sure proper framebuffer assigned: */
    AssertReturnVoid(m_pFrameBuffer);
    AssertReturnVoid(m_pFrameBuffer == uisession()->frameBuffer(screenId()));

    /* Mark framebuffer as unused: */
    LogRelFlow(("UIMachineView::cleanupFrameBuffer: Stop EMT callbacks accepting for screen: %d.\n", screenId()));
    m_pFrameBuffer->setMarkAsUnused(true);

    /* Process pending framebuffer events: */
    QApplication::sendPostedEvents(this, QEvent::MetaCall);

#ifdef VBOX_WITH_VIDEOHWACCEL
    if (m_fAccelerate2DVideo)
        QApplication::sendPostedEvents(this, VHWACommandProcessType);
#endif /* VBOX_WITH_VIDEOHWACCEL */

    /* Temporarily detach the framebuffer from IDisplay before detaching
     * from view in order to respect the thread synchonisation logic (see UIFrameBuffer.h).
     * Note: VBOX_WITH_CROGL additionally requires us to call DetachFramebuffer
     * to ensure 3D gets notified of view being destroyed... */
    CDisplay display = session().GetConsole().GetDisplay();
    display.DetachFramebuffer(m_uScreenId);

    /* Detach framebuffer from view: */
    m_pFrameBuffer->setView(NULL);
}

UIMachineLogic* UIMachineView::machineLogic() const
{
    return machineWindow()->machineLogic();
}

UISession* UIMachineView::uisession() const
{
    return machineLogic()->uisession();
}

CSession& UIMachineView::session()
{
    return uisession()->session();
}

QSize UIMachineView::sizeHint() const
{
    if (m_sizeHintOverride.isValid())
        return m_sizeHintOverride;
#ifdef VBOX_WITH_DEBUGGER_GUI
    // TODO: Fix all DEBUGGER stuff!
    /* HACK ALERT! Really ugly workaround for the resizing to 9x1 done by DevVGA if provoked before power on. */
    QSize fb(m_pFrameBuffer->width(), m_pFrameBuffer->height());
    if (fb.width() < 16 || fb.height() < 16)
        if (vboxGlobal().isStartPausedEnabled() || vboxGlobal().isDebuggerAutoShowEnabled())
            fb = QSize(640, 480);
    return QSize(fb.width() + frameWidth() * 2, fb.height() + frameWidth() * 2);
#else /* !VBOX_WITH_DEBUGGER_GUI */
    return QSize(m_pFrameBuffer->width() + frameWidth() * 2, m_pFrameBuffer->height() + frameWidth() * 2);
#endif /* !VBOX_WITH_DEBUGGER_GUI */
}

int UIMachineView::contentsX() const
{
    return horizontalScrollBar()->value();
}

int UIMachineView::contentsY() const
{
    return verticalScrollBar()->value();
}

int UIMachineView::contentsWidth() const
{
    return m_pFrameBuffer->width();
}

int UIMachineView::contentsHeight() const
{
    return m_pFrameBuffer->height();
}

int UIMachineView::visibleWidth() const
{
    return horizontalScrollBar()->pageStep();
}

int UIMachineView::visibleHeight() const
{
    return verticalScrollBar()->pageStep();
}

void UIMachineView::setMaxGuestSize(const QSize &minimumSizeHint /* = QSize() */)
{
    QSize maxSize;
    switch (m_maxGuestSizePolicy)
    {
        case MaxGuestSizePolicy_Fixed:
            maxSize = m_fixedMaxGuestSize;
            break;
        case MaxGuestSizePolicy_Automatic:
            maxSize = calculateMaxGuestSize().expandedTo(minimumSizeHint);
            break;
        case MaxGuestSizePolicy_Any:
        default:
            AssertMsg(m_maxGuestSizePolicy == MaxGuestSizePolicy_Any,
                      ("Invalid maximum guest size policy %d!\n",
                       m_maxGuestSizePolicy));
            /* (0, 0) means any of course. */
            maxSize = QSize(0, 0);
    }
    ASMAtomicWriteU64(&m_u64MaxGuestSize,
                      RT_MAKE_U64(maxSize.height(), maxSize.width()));
}

QSize UIMachineView::maxGuestSize()
{
    uint64_t u64Size = ASMAtomicReadU64(&m_u64MaxGuestSize);
    return QSize(int(RT_HI_U32(u64Size)), int(RT_LO_U32(u64Size)));
}

QSize UIMachineView::guestSizeHint()
{
    /* Load guest-screen size-hint: */
    QSize size = gEDataManager->lastGuestSizeHint(m_uScreenId, vboxGlobal().managedVMUuid());

    /* Return loaded or default: */
    return size.isValid() ? size : QSize(800, 600);
}

void UIMachineView::storeGuestSizeHint(const QSize &size)
{
    /* Save guest-screen size-hint: */
    gEDataManager->setLastGuestSizeHint(m_uScreenId, size, vboxGlobal().managedVMUuid());
}

void UIMachineView::takePauseShotLive()
{
    /* Take a screen snapshot. Note that TakeScreenShot() always needs a 32bpp image: */
    QImage shot = QImage(m_pFrameBuffer->width(), m_pFrameBuffer->height(), QImage::Format_RGB32);
    /* If TakeScreenShot fails or returns no image, just show a black image. */
    shot.fill(0);
    CDisplay dsp = session().GetConsole().GetDisplay();
    dsp.TakeScreenShot(screenId(), shot.bits(), shot.width(), shot.height());
    /* TakeScreenShot() may fail if, e.g. the Paused notification was delivered
     * after the machine execution was resumed. It's not fatal: */
    if (dsp.isOk())
        dimImage(shot);
    m_pauseShot = QPixmap::fromImage(shot);
}

void UIMachineView::takePauseShotSnapshot()
{
    CMachine machine = session().GetMachine();
    ULONG width = 0, height = 0;
    QVector<BYTE> screenData = machine.ReadSavedScreenshotPNGToArray(0, width, height);
    if (screenData.size() != 0)
    {
        ULONG guestOriginX = 0, guestOriginY = 0, guestWidth = 0, guestHeight = 0;
        BOOL fEnabled = true;
        machine.QuerySavedGuestScreenInfo(m_uScreenId, guestOriginX, guestOriginY, guestWidth, guestHeight, fEnabled);
        QImage shot = QImage::fromData(screenData.data(), screenData.size(), "PNG").scaled(guestWidth > 0 ? QSize(guestWidth, guestHeight) : guestSizeHint());
        dimImage(shot);
        m_pauseShot = QPixmap::fromImage(shot);
    }
}

void UIMachineView::updateSliders()
{
    QSize p = viewport()->size();
    QSize m = maximumViewportSize();

    QSize v = QSize(frameBuffer()->width(), frameBuffer()->height());
    /* No scroll bars needed: */
    if (m.expandedTo(v) == m)
        p = m;

    horizontalScrollBar()->setRange(0, v.width() - p.width());
    verticalScrollBar()->setRange(0, v.height() - p.height());
    horizontalScrollBar()->setPageStep(p.width());
    verticalScrollBar()->setPageStep(p.height());
}

QPoint UIMachineView::viewportToContents(const QPoint &vp) const
{
    return QPoint(vp.x() + contentsX(), vp.y() + contentsY());
}

void UIMachineView::scrollBy(int dx, int dy)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
    verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
}

void UIMachineView::dimImage(QImage &img)
{
    for (int y = 0; y < img.height(); ++ y)
    {
        if (y % 2)
        {
            if (img.depth() == 32)
            {
                for (int x = 0; x < img.width(); ++ x)
                {
                    int gray = qGray(img.pixel (x, y)) / 2;
                    img.setPixel(x, y, qRgb (gray, gray, gray));
                }
            }
            else
            {
                ::memset(img.scanLine (y), 0, img.bytesPerLine());
            }
        }
        else
        {
            if (img.depth() == 32)
            {
                for (int x = 0; x < img.width(); ++ x)
                {
                    int gray = (2 * qGray (img.pixel (x, y))) / 3;
                    img.setPixel(x, y, qRgb (gray, gray, gray));
                }
            }
        }
    }
}

void UIMachineView::scrollContentsBy(int dx, int dy)
{
#ifdef VBOX_WITH_VIDEOHWACCEL
    if (m_pFrameBuffer)
    {
        m_pFrameBuffer->viewportScrolled(dx, dy);
    }
#endif /* VBOX_WITH_VIDEOHWACCEL */
    QAbstractScrollArea::scrollContentsBy(dx, dy);

    session().GetConsole().GetDisplay().ViewportChanged(screenId(),
                            contentsX(),
                            contentsY(),
                            visibleWidth(),
                            visibleHeight());
}


#ifdef Q_WS_MAC
void UIMachineView::updateDockIcon()
{
    machineLogic()->updateDockIcon();
}

CGImageRef UIMachineView::vmContentImage()
{
    /* Use pause-image if exists: */
    if (!m_pauseShot.isNull())
        return darwinToCGImageRef(&m_pauseShot);

    /* Create the image ref out of the frame-buffer: */
    return frameBuffertoCGImageRef(m_pFrameBuffer);
}

CGImageRef UIMachineView::frameBuffertoCGImageRef(UIFrameBuffer *pFrameBuffer)
{
    CGImageRef ir = 0;
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    if (cs)
    {
        /* Create the image copy of the framebuffer */
        CGDataProviderRef dp = CGDataProviderCreateWithData(pFrameBuffer, pFrameBuffer->address(), pFrameBuffer->bitsPerPixel() / 8 * pFrameBuffer->width() * pFrameBuffer->height(), NULL);
        if (dp)
        {
            ir = CGImageCreate(pFrameBuffer->width(), pFrameBuffer->height(), 8, 32, pFrameBuffer->bytesPerLine(), cs,
                               kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host, dp, 0, false,
                               kCGRenderingIntentDefault);
            CGDataProviderRelease(dp);
        }
        CGColorSpaceRelease(cs);
    }
    return ir;
}
#endif /* Q_WS_MAC */

UIVisualStateType UIMachineView::visualStateType() const
{
    return machineLogic()->visualStateType();
}

bool UIMachineView::isFullscreenOrSeamless() const
{
    return    visualStateType() == UIVisualStateType_Fullscreen
           || visualStateType() == UIVisualStateType_Seamless;
}

bool UIMachineView::event(QEvent *pEvent)
{
    switch (pEvent->type())
    {
#ifdef Q_WS_MAC
        /* Event posted OnShowWindow: */
        case ShowWindowEventType:
        {
            /* Dunno what Qt3 thinks a window that has minimized to the dock should be - it is not hidden,
             * neither is it minimized. OTOH it is marked shown and visible, but not activated.
             * This latter isn't of much help though, since at this point nothing is marked activated.
             * I might have overlooked something, but I'm buggered what if I know what. So, I'll just always
             * show & activate the stupid window to make it get out of the dock when the user wishes to show a VM: */
            window()->show();
            window()->activateWindow();
            return true;
        }
#endif /* Q_WS_MAC */

#ifdef VBOX_WITH_VIDEOHWACCEL
        case VHWACommandProcessType:
        {
            m_pFrameBuffer->doProcessVHWACommand(pEvent);
            return true;
        }
#endif /* VBOX_WITH_VIDEOHWACCEL */

        default:
            break;
    }

    return QAbstractScrollArea::event(pEvent);
}

bool UIMachineView::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    if (pWatched == viewport())
    {
        switch (pEvent->type())
        {
            case QEvent::Resize:
            {
#ifdef VBOX_WITH_VIDEOHWACCEL
                QResizeEvent* pResizeEvent = static_cast<QResizeEvent*>(pEvent);
                if (m_pFrameBuffer)
                    m_pFrameBuffer->viewportResized(pResizeEvent);
#endif /* VBOX_WITH_VIDEOHWACCEL */
                session().GetConsole().GetDisplay().ViewportChanged(screenId(),
                        contentsX(),
                        contentsY(),
                        visibleWidth(),
                        visibleHeight());
                break;
            }
            default:
                break;
        }
    }
    if (pWatched == machineWindow())
    {
        switch (pEvent->type())
        {
            case QEvent::WindowStateChange:
            {
                /* During minimizing and state restoring machineWindow() gets
                 * the focus which belongs to console view window, so returning it properly. */
                QWindowStateChangeEvent *pWindowEvent = static_cast<QWindowStateChangeEvent*>(pEvent);
                if (pWindowEvent->oldState() & Qt::WindowMinimized)
                {
                    if (QApplication::focusWidget())
                    {
                        QApplication::focusWidget()->clearFocus();
                        qApp->processEvents();
                    }
                    QTimer::singleShot(0, this, SLOT(setFocus()));
                }
                break;
            }
#ifdef Q_WS_MAC
            case QEvent::Move:
            {
                /* Update backing scale factor for underlying frame-buffer: */
                if (m_pFrameBuffer)
                    m_pFrameBuffer->setBackingScaleFactor(darwinBackingScaleFactor(machineWindow()));
                break;
            }
#endif /* Q_WS_MAC */
            default:
                break;
        }
    }

    return QAbstractScrollArea::eventFilter(pWatched, pEvent);
}

void UIMachineView::resizeEvent(QResizeEvent *pEvent)
{
    updateSliders();
    return QAbstractScrollArea::resizeEvent(pEvent);
}

void UIMachineView::moveEvent(QMoveEvent *pEvent)
{
    return QAbstractScrollArea::moveEvent(pEvent);
}

void UIMachineView::paintEvent(QPaintEvent *pPaintEvent)
{
    /* Use pause-image if exists: */
    if (!m_pauseShot.isNull())
    {
        /* We have a snapshot for the paused state: */
        QRect rect = pPaintEvent->rect().intersect(viewport()->rect());
        QPainter painter(viewport());
        painter.drawPixmap(rect, m_pauseShot, QRect(rect.x() + contentsX(), rect.y() + contentsY(),
                                                    rect.width(), rect.height()));
#ifdef Q_WS_MAC
        /* Update the dock icon: */
        updateDockIcon();
#endif /* Q_WS_MAC */
        return;
    }

    /* Delegate the paint function to the UIFrameBuffer interface: */
    if (m_pFrameBuffer)
        m_pFrameBuffer->paintEvent(pPaintEvent);
#ifdef Q_WS_MAC
    /* Update the dock icon if we are in the running state: */
    if (uisession()->isRunning())
        updateDockIcon();
#endif /* Q_WS_MAC */
}

#ifdef VBOX_WITH_DRAG_AND_DROP
void UIMachineView::dragEnterEvent(QDragEnterEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    /* Get mouse-pointer location. */
    const QPoint &cpnt = viewportToContents(pEvent->pos());

    CGuest guest = session().GetConsole().GetGuest();
    CDnDTarget dndTarget = static_cast<CDnDTarget>(guest.GetDnDTarget());

    /* Ask the target for starting a DnD event. */
    Qt::DropAction result = DnDHandler()->dragEnter(dndTarget,
                                                    screenId(),
                                                    frameBuffer()->convertHostXTo(cpnt.x()),
                                                    frameBuffer()->convertHostYTo(cpnt.y()),
                                                    pEvent->proposedAction(),
                                                    pEvent->possibleActions(),
                                                    pEvent->mimeData(), this /* pParent */);

    /* Set the DnD action returned by the guest. */
    pEvent->setDropAction(result);
    pEvent->accept();
}

void UIMachineView::dragMoveEvent(QDragMoveEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    /* Get mouse-pointer location. */
    const QPoint &cpnt = viewportToContents(pEvent->pos());

    CGuest guest = session().GetConsole().GetGuest();
    CDnDTarget dndTarget = static_cast<CDnDTarget>(guest.GetDnDTarget());

    /* Ask the guest for moving the drop cursor. */
    Qt::DropAction result = DnDHandler()->dragMove(dndTarget,
                                                   screenId(),
                                                   frameBuffer()->convertHostXTo(cpnt.x()),
                                                   frameBuffer()->convertHostYTo(cpnt.y()),
                                                   pEvent->proposedAction(),
                                                   pEvent->possibleActions(),
                                                   pEvent->mimeData(), this /* pParent */);

    /* Set the DnD action returned by the guest. */
    pEvent->setDropAction(result);
    pEvent->accept();
}

void UIMachineView::dragLeaveEvent(QDragLeaveEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    CGuest guest = session().GetConsole().GetGuest();
    CDnDTarget dndTarget = static_cast<CDnDTarget>(guest.GetDnDTarget());

    /* Ask the guest for stopping this DnD event. */
    DnDHandler()->dragLeave(dndTarget,
                            screenId(), this /* pParent */);
    pEvent->accept();
}

void UIMachineView::dragIsPending(void)
{
    /* At the moment we only support guest->host DnD. */
    /** @todo Add guest->guest DnD functionality here by getting
     *        the source of guest B (when copying from B to A). */
    CGuest guest = session().GetConsole().GetGuest();
    CDnDSource dndSource = static_cast<CDnDSource>(guest.GetDnDSource());

    /* Check for a pending DnD event within the guest and if so, handle all the
     * magic. */
    DnDHandler()->dragIsPending(session(), dndSource, screenId(), this /* pParent */);
}

void UIMachineView::dropEvent(QDropEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    /* Get mouse-pointer location. */
    const QPoint &cpnt = viewportToContents(pEvent->pos());

    CGuest guest = session().GetConsole().GetGuest();
    CDnDTarget dndTarget = static_cast<CDnDTarget>(guest.GetDnDTarget());

    /* Ask the guest for dropping data. */
    Qt::DropAction result = DnDHandler()->dragDrop(session(),
                                                   dndTarget,
                                                   screenId(),
                                                   frameBuffer()->convertHostXTo(cpnt.x()),
                                                   frameBuffer()->convertHostYTo(cpnt.y()),
                                                   pEvent->proposedAction(),
                                                   pEvent->possibleActions(),
                                                   pEvent->mimeData(), this /* pParent */);

    /* Set the DnD action returned by the guest. */
    pEvent->setDropAction(result);
    pEvent->accept();
}
#endif /* VBOX_WITH_DRAG_AND_DROP */

#if defined(Q_WS_WIN)

bool UIMachineView::winEvent(MSG *pMsg, long* /* piResult */)
{
    AssertPtrReturn(pMsg, false);

    /* Check if some system event should be filtered out.
     * Returning @c true means filtering-out,
     * Returning @c false means passing event to Qt. */
    bool fResult = false; /* Pass to Qt by default. */
    switch (pMsg->message)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            /* Filter using keyboard filter? */
            bool fKeyboardFilteringResult =
                machineLogic()->keyboardHandler()->winEventFilter(pMsg, screenId());
            /* Keyboard filter rules the result? */
            fResult = fKeyboardFilteringResult;
            break;
        }
        default:
            break;
    }

    return fResult;
}

#elif defined(Q_WS_X11)

bool UIMachineView::x11Event(XEvent *pEvent)
{
    AssertPtrReturn(pEvent, false);

    /* Check if some system event should be filtered out.
     * Returning @c true means filtering-out,
     * Returning @c false means passing event to Qt. */
    bool fResult = false; /* Pass to Qt by default. */
    switch (pEvent->type)
    {
        case XFocusOut:
        case XFocusIn:
        case XKeyPress:
        case XKeyRelease:
        {
            /* Filter using keyboard-filter? */
            bool fKeyboardFilteringResult =
                machineLogic()->keyboardHandler()->x11EventFilter(pEvent, screenId());
            /* Filter using mouse-filter? */
            bool fMouseFilteringResult =
                machineLogic()->mouseHandler()->x11EventFilter(pEvent, screenId());
            /* If at least one of filters wants to filter event out then the result is true. */
            fResult = fKeyboardFilteringResult || fMouseFilteringResult;
            break;
        }
        default:
            break;
    }

    return fResult;
}

#endif /* Q_WS_X11 */

