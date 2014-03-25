/* $Id$ */
/** @file
 * VBox Qt GUI - UIGraphicsTextPane and UITask class implementation.
 */

/*
 * Copyright (C) 2012-2014 Oracle Corporation
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
#include <QApplication>
#include <QFontMetrics>
#include <QTextLayout>
#include <QPainter>
#include <QGraphicsSceneHoverEvent>

/* GUI includes: */
#include "UIGraphicsTextPane.h"

UIGraphicsTextPane::UIGraphicsTextPane(QIGraphicsWidget *pParent, QPaintDevice *pPaintDevice)
    : QIGraphicsWidget(pParent)
    , m_pPaintDevice(pPaintDevice)
    , m_iMargin(0)
    , m_iSpacing(10)
    , m_iMinimumTextColumnWidth(100)
    , m_fMinimumSizeHintInvalidated(true)
    , m_iMinimumTextWidth(0)
    , m_iMinimumTextHeight(0)
    , m_fAnchorCanBeHovered(true)
{
    /* We do support hover-events: */
    setAcceptHoverEvents(true);
}

UIGraphicsTextPane::~UIGraphicsTextPane()
{
    /* Clear text-layouts: */
    while (!m_leftList.isEmpty()) delete m_leftList.takeLast();
    while (!m_rightList.isEmpty()) delete m_rightList.takeLast();
}

void UIGraphicsTextPane::setText(const UITextTable &text)
{
    /* Clear text: */
    m_text.clear();

    /* For each the line of the passed table: */
    foreach (const UITextTableLine &line, text)
    {
        /* Lines: */
        QString strLeftLine = line.first;
        QString strRightLine = line.second;

        /* If 2nd line is NOT empty: */
        if (!strRightLine.isEmpty())
        {
            /* Take both lines 'as is': */
            m_text << UITextTableLine(strLeftLine, strRightLine);
        }
        /* If 2nd line is empty: */
        else
        {
            /* Parse the 1st one to sub-lines: */
            QStringList subLines = strLeftLine.split(QRegExp("\\n"));
            foreach (const QString &strSubLine, subLines)
                m_text << UITextTableLine(strSubLine, QString());
        }
    }

    /* Update text-layout: */
    updateTextLayout(true);

    /* Update minimum size-hint: */
    updateGeometry();
}

void UIGraphicsTextPane::updateTextLayout(bool fFull /* = false */)
{
    /* Prepare variables: */
    QFontMetrics fm(font(), m_pPaintDevice);
    int iMaximumTextWidth = (int)size().width() - 2 * m_iMargin - m_iSpacing;

    /* Search for the maximum column widths: */
    int iMaximumLeftColumnWidth = 0;
    int iMaximumRightColumnWidth = 0;
    bool fSingleColumnText = true;
    foreach (const UITextTableLine &line, m_text)
    {
        bool fRightColumnPresent = !line.second.isEmpty();
        if (fRightColumnPresent)
            fSingleColumnText = false;
        QString strLeftLine = fRightColumnPresent ? line.first + ":" : line.first;
        QString strRightLine = line.second;
        iMaximumLeftColumnWidth = qMax(iMaximumLeftColumnWidth, fm.width(strLeftLine));
        iMaximumRightColumnWidth = qMax(iMaximumRightColumnWidth, fm.width(strRightLine));
    }
    iMaximumLeftColumnWidth += 1;
    iMaximumRightColumnWidth += 1;

    /* Calculate text attributes: */
    int iLeftColumnWidth = 0;
    int iRightColumnWidth = 0;
    /* Left column only: */
    if (fSingleColumnText)
    {
        /* Full update? */
        if (fFull)
        {
            /* Minimum width for left column: */
            int iMinimumLeftColumnWidth = qMin(m_iMinimumTextColumnWidth, iMaximumLeftColumnWidth);
            /* Minimum width for whole text: */
            m_iMinimumTextWidth = iMinimumLeftColumnWidth;
        }

        /* Current width for left column: */
        iLeftColumnWidth = qMax(m_iMinimumTextColumnWidth, iMaximumTextWidth);
    }
    /* Two columns: */
    else
    {
        /* Full update? */
        if (fFull)
        {
            /* Minimum width for left column: */
            int iMinimumLeftColumnWidth = iMaximumLeftColumnWidth;
            /* Minimum width for right column: */
            int iMinimumRightColumnWidth = qMin(m_iMinimumTextColumnWidth, iMaximumRightColumnWidth);
            /* Minimum width for whole text: */
            m_iMinimumTextWidth = iMinimumLeftColumnWidth + m_iSpacing + iMinimumRightColumnWidth;
        }

        /* Current width for left column: */
        iLeftColumnWidth = iMaximumLeftColumnWidth;
        /* Current width for right column: */
        iRightColumnWidth = iMaximumTextWidth - iLeftColumnWidth;
    }

    /* Clear old text-layouts: */
    while (!m_leftList.isEmpty()) delete m_leftList.takeLast();
    while (!m_rightList.isEmpty()) delete m_rightList.takeLast();

    /* Prepare new text-layouts: */
    int iTextX = m_iMargin;
    int iTextY = m_iMargin;
    /* Populate text-layouts: */
    m_iMinimumTextHeight = 0;
    foreach (const UITextTableLine &line, m_text)
    {
        /* Left layout: */
        int iLeftColumnHeight = 0;
        if (!line.first.isEmpty())
        {
            bool fRightColumnPresent = !line.second.isEmpty();
            m_leftList << buildTextLayout(font(), m_pPaintDevice,
                                          fRightColumnPresent ? line.first + ":" : line.first,
                                          iLeftColumnWidth, iLeftColumnHeight,
                                          m_strHoveredAnchor);
            m_leftList.last()->setPosition(QPointF(iTextX, iTextY));
        }

        /* Right layout: */
        int iRightColumnHeight = 0;
        if (!line.second.isEmpty())
        {
            m_rightList << buildTextLayout(font(), m_pPaintDevice,
                                           line.second,
                                           iRightColumnWidth, iRightColumnHeight,
                                           m_strHoveredAnchor);
            m_rightList.last()->setPosition(QPointF(iTextX + iLeftColumnWidth + m_iSpacing, iTextY));
        }

        /* Maximum colum height? */
        int iMaximumColumnHeight = qMax(iLeftColumnHeight, iRightColumnHeight);

        /* Indent Y: */
        iTextY += iMaximumColumnHeight;
        /* Append summary text height: */
        m_iMinimumTextHeight += iMaximumColumnHeight;
    }
}

void UIGraphicsTextPane::updateGeometry()
{
    /* Discard cached minimum size-hint: */
    m_fMinimumSizeHintInvalidated = true;

    /* Call to base-class to notify layout if any: */
    QIGraphicsWidget::updateGeometry();

    /* And notify listeners which are not layouts: */
    emit sigGeometryChanged();
}

QSizeF UIGraphicsTextPane::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    /* For minimum size-hint: */
    if (which == Qt::MinimumSize)
    {
        /* If minimum size-hint invalidated: */
        if (m_fMinimumSizeHintInvalidated)
        {
            /* Recache minimum size-hint: */
            m_minimumSizeHint = QSizeF(2 * m_iMargin + m_iMinimumTextWidth,
                                       2 * m_iMargin + m_iMinimumTextHeight);
            m_fMinimumSizeHintInvalidated = false;
        }
        /* Return cached minimum size-hint: */
        return m_minimumSizeHint;
    }
    /* Call to base-class for other size-hints: */
    return QIGraphicsWidget::sizeHint(which, constraint);
}

void UIGraphicsTextPane::resizeEvent(QGraphicsSceneResizeEvent*)
{
    /* Update text-layout: */
    updateTextLayout();

    /* Update minimum size-hint: */
    updateGeometry();
}

void UIGraphicsTextPane::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent)
{
    /* Redirect to common handler: */
    handleHoverEvent(pEvent);
}

void UIGraphicsTextPane::hoverMoveEvent(QGraphicsSceneHoverEvent *pEvent)
{
    /* Redirect to common handler: */
    handleHoverEvent(pEvent);
}

void UIGraphicsTextPane::handleHoverEvent(QGraphicsSceneHoverEvent *pEvent)
{
    /* Ignore if anchor can't be hovered: */
    if (!m_fAnchorCanBeHovered)
        return;

    /* Prepare variables: */
    QPoint mousePosition = pEvent->pos().toPoint();

    /* If we currently have no anchor hovered: */
    if (m_strHoveredAnchor.isNull())
    {
        /* Search it in the left list: */
        m_strHoveredAnchor = searchForHoveredAnchor(m_pPaintDevice, m_leftList, mousePosition);
        if (!m_strHoveredAnchor.isNull())
            return updateHoverStuff();
        /* Then search it in the right one: */
        m_strHoveredAnchor = searchForHoveredAnchor(m_pPaintDevice, m_rightList, mousePosition);
        if (!m_strHoveredAnchor.isNull())
            return updateHoverStuff();
    }
    /* If we currently have some anchor hovered: */
    else
    {
        QString strHoveredAnchorName;
        /* Validate it through the left list: */
        strHoveredAnchorName = searchForHoveredAnchor(m_pPaintDevice, m_leftList, mousePosition);
        if (!strHoveredAnchorName.isNull())
        {
            m_strHoveredAnchor = strHoveredAnchorName;
            return updateHoverStuff();
        }
        /* Then validate it through the right one: */
        strHoveredAnchorName = searchForHoveredAnchor(m_pPaintDevice, m_rightList, mousePosition);
        if (!strHoveredAnchorName.isNull())
        {
            m_strHoveredAnchor = strHoveredAnchorName;
            return updateHoverStuff();
        }
        /* Finally clear it for good: */
        m_strHoveredAnchor.clear();
        return updateHoverStuff();
    }
}

void UIGraphicsTextPane::updateHoverStuff()
{
    /* Update mouse-cursor: */
    if (m_strHoveredAnchor.isNull())
        unsetCursor();
    else
        setCursor(Qt::PointingHandCursor);

    /* Update text-layout: */
    updateTextLayout();

    /* Update text-pane: */
    update();
}

void UIGraphicsTextPane::mousePressEvent(QGraphicsSceneMouseEvent*)
{
    /* Make sure some anchor hovered: */
    if (m_strHoveredAnchor.isNull())
        return;

    /* Restrict anchor hovering: */
    m_fAnchorCanBeHovered = false;

    /* Cache clicked anchor: */
    QString strClickedAnchor = m_strHoveredAnchor;

    /* Clear hovered anchor: */
    m_strHoveredAnchor.clear();
    updateHoverStuff();

    /* Notify listeners about anchor clicked: */
    emit sigAnchorClicked(strClickedAnchor);

    /* Allow anchor hovering again: */
    m_fAnchorCanBeHovered = true;
}

void UIGraphicsTextPane::paint(QPainter *pPainter, const QStyleOptionGraphicsItem*, QWidget*)
{
    /* Draw all the text-layouts: */
    foreach (QTextLayout *pTextLayout, m_leftList)
        pTextLayout->draw(pPainter, QPoint(0, 0));
    foreach (QTextLayout *pTextLayout, m_rightList)
        pTextLayout->draw(pPainter, QPoint(0, 0));
}

/* static  */
QTextLayout* UIGraphicsTextPane::buildTextLayout(const QFont &font, QPaintDevice *pPaintDevice,
                                                 const QString &strText, int iWidth, int &iHeight,
                                                 const QString &strHoveredAnchor)
{
    /* Prepare variables: */
    QFontMetrics fm(font, pPaintDevice);
    int iLeading = fm.leading();
    QString strModifiedText(strText);
    QList<QTextLayout::FormatRange> formatRangeList;

    /* Handle bold sub-strings: */
    QRegExp boldRegExp("<b>([\\s\\S]+)</b>");
    boldRegExp.setMinimal(true);
    while (boldRegExp.indexIn(strModifiedText) != -1)
    {
        /* Prepare format: */
        QTextLayout::FormatRange formatRange;
        QFont font = formatRange.format.font();
        font.setBold(true);
        formatRange.format.setFont(font);
        formatRange.start = boldRegExp.pos(0);
        formatRange.length = boldRegExp.cap(1).size();
        /* Add format range to list: */
        formatRangeList << formatRange;
        /* Replace sub-string: */
        strModifiedText.replace(boldRegExp.cap(0), boldRegExp.cap(1));
    }

    /* Handle anchored sub-strings: */
    QRegExp anchoredRegExp("<a href=([^>]+)>([^<>]+)</a>");
    anchoredRegExp.setMinimal(true);
    while (anchoredRegExp.indexIn(strModifiedText) != -1)
    {
        /* Prepare format: */
        QTextLayout::FormatRange formatRange;
        formatRange.format.setAnchor(true);
        formatRange.format.setAnchorHref(anchoredRegExp.cap(1));
        if (formatRange.format.anchorHref() == strHoveredAnchor)
            formatRange.format.setForeground(qApp->palette().color(QPalette::Link));
        formatRange.start = anchoredRegExp.pos(0);
        formatRange.length = anchoredRegExp.cap(2).size();
        /* Add format range to list: */
        formatRangeList << formatRange;
        /* Replace sub-string: */
        strModifiedText.replace(anchoredRegExp.cap(0), anchoredRegExp.cap(2));
    }

    /* Create layout; */
    QTextLayout *pTextLayout = new QTextLayout(strModifiedText, font, pPaintDevice);
    pTextLayout->setAdditionalFormats(formatRangeList);

    /* Configure layout: */
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    pTextLayout->setTextOption(textOption);

    /* Build layout: */
    pTextLayout->beginLayout();
    while (1)
    {
        QTextLine line = pTextLayout->createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(iWidth);
        iHeight += iLeading;
        line.setPosition(QPointF(0, iHeight));
        iHeight += line.height();
    }
    pTextLayout->endLayout();

    /* Return layout: */
    return pTextLayout;
}

/* static */
QString UIGraphicsTextPane::searchForHoveredAnchor(QPaintDevice *pPaintDevice, const QList<QTextLayout*> &list, const QPoint &mousePosition)
{
    /* Analyze passed text-layouts: */
    foreach (QTextLayout *pTextLayout, list)
    {
        /* Prepare variables: */
        QFontMetrics fm(pTextLayout->font(), pPaintDevice);

        /* Text-layout attributes: */
        const QPoint layoutPosition = pTextLayout->position().toPoint();
        const QString strLayoutText = pTextLayout->text();

        /* Enumerate format ranges: */
        foreach (const QTextLayout::FormatRange &range, pTextLayout->additionalFormats())
        {
            /* Skip unrelated formats: */
            if (!range.format.isAnchor())
                continue;

            /* Parse 'anchor' format: */
            const int iStart = range.start;
            const int iLength = range.length;
            QRegion formatRegion;
            for (int iTextPosition = iStart; iTextPosition < iStart + iLength; ++iTextPosition)
            {
                QTextLine layoutLine = pTextLayout->lineForTextPosition(iTextPosition);
                QPoint linePosition = layoutLine.position().toPoint();
                int iSymbolX = (int)layoutLine.cursorToX(iTextPosition);
                QRect symbolRect = QRect(layoutPosition.x() + linePosition.x() + iSymbolX,
                                         layoutPosition.y() + linePosition.y(),
                                         fm.width(strLayoutText[iTextPosition]) + 1, fm.height());
                formatRegion += symbolRect;
            }

            /* Is that something we looking for? */
            if (formatRegion.contains(mousePosition))
                return range.format.anchorHref();
        }
    }

    /* Null string by default: */
    return QString();
}

