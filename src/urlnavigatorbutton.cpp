/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "urlnavigatorbutton.h"
#include <qcursor.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <Q3PopupMenu>
#include <QEvent>
#include <QDragEnterEvent>

#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kurl.h>
#include <assert.h>

#include "urlnavigator.h"
#include "dolphinview.h"
#include "dolphin.h"

URLNavigatorButton::URLNavigatorButton(int index, URLNavigator* parent) :
    URLButton(parent),
    m_index(-1),
    m_listJob(0)
{
    setAcceptDrops(true);
    setMinimumWidth(arrowWidth());
    setIndex(index);
    connect(this, SIGNAL(clicked()), this, SLOT(updateNavigatorURL()));

    m_popupDelay = new QTimer(this);
    connect(m_popupDelay, SIGNAL(timeout()), this, SLOT(startListJob()));
    connect(this, SIGNAL(pressed()), this, SLOT(startPopupDelay()));
}

URLNavigatorButton::~URLNavigatorButton()
{
}

void URLNavigatorButton::setIndex(int index)
{
    m_index = index;

    if (m_index < 0) {
        return;
    }

    QString path(urlNavigator()->url().pathOrUrl());
    setText(path.section('/', index, index));

    // Check whether the button indicates the full path of the URL. If
    // this is the case, the button is marked as 'active'.
    ++index;
    QFont adjustedFont(font());
    if (path.section('/', index, index).isEmpty()) {
        setDisplayHintEnabled(ActivatedHint, true);
        adjustedFont.setBold(true);
    }
    else {
        setDisplayHintEnabled(ActivatedHint, false);
        adjustedFont.setBold(false);
    }

    setFont(adjustedFont);
    update();
}

int URLNavigatorButton::index() const
{
    return m_index;
}

void URLNavigatorButton::drawButton(QPainter* painter)
{
    const int buttonWidth  = width();
    const int buttonHeight = height();

    QColor backgroundColor;
    QColor foregroundColor;
    const bool isHighlighted = isDisplayHintEnabled(EnteredHint) ||
                               isDisplayHintEnabled(DraggedHint) ||
                               isDisplayHintEnabled(PopupActiveHint);
    if (isHighlighted) {
        backgroundColor = KGlobalSettings::highlightColor();
        foregroundColor = KGlobalSettings::highlightedTextColor();
    }
    else {
        backgroundColor = colorGroup().background();
        foregroundColor = KGlobalSettings::buttonTextColor();
    }

    // dimm the colors if the parent view does not have the focus
    const DolphinView* parentView = urlNavigator()->dolphinView();
    const Dolphin& dolphin = Dolphin::mainWin();

    const bool isActive = (dolphin.activeView() == parentView);
    if (!isActive) {
        QColor dimmColor(colorGroup().background());
        foregroundColor = mixColors(foregroundColor, dimmColor);
        if (isHighlighted) {
            backgroundColor = mixColors(backgroundColor, dimmColor);
        }
    }

    // draw button background
    painter->setPen(Qt::NoPen);
    painter->setBrush(backgroundColor);
    painter->drawRect(0, 0, buttonWidth, buttonHeight);

    int textWidth = buttonWidth;
    if (isDisplayHintEnabled(ActivatedHint) && isActive || isHighlighted) {
        painter->setPen(foregroundColor);
    }
    else {
        // dimm the foreground color by mixing it with the background
        foregroundColor = mixColors(foregroundColor, backgroundColor);
        painter->setPen(foregroundColor);
    }

    if (!isDisplayHintEnabled(ActivatedHint)) {
        // draw arrow
        const int border = 2;  // horizontal border
        const int middleY = height() / 2;
        const int width = arrowWidth();
        const int startX = (buttonWidth - width) - (2 * border);
        const int startTopY = middleY - (width - 1);
        const int startBottomY = middleY + (width - 1);
        for (int i = 0; i < width; ++i) {
            painter->drawLine(startX, startTopY + i, startX + i, startTopY + i);
            painter->drawLine(startX, startBottomY - i, startX + i, startBottomY - i);
        }

        textWidth = startX - border;
    }

    const bool clipped = isTextClipped();
    const int align = clipped ? Qt::AlignVCenter : Qt::AlignCenter;
    painter->drawText(QRect(0, 0, textWidth, buttonHeight), align, text());

    if (clipped) {
        // Blend the right area of the text with the background, as the
        // text is clipped.
        // TODO: use alpha blending in Qt4 instead of drawing the text that often
        const int blendSteps = 16;

        QColor blendColor(backgroundColor);
        const int redInc   = (foregroundColor.red()   - backgroundColor.red())   / blendSteps;
        const int greenInc = (foregroundColor.green() - backgroundColor.green()) / blendSteps;
        const int blueInc  = (foregroundColor.blue()  - backgroundColor.blue())  / blendSteps;
        for (int i = 0; i < blendSteps; ++i) {
            painter->setClipRect(QRect(textWidth - i, 0, 1, buttonHeight));
            painter->setPen(blendColor);
            painter->drawText(QRect(0, 0, textWidth, buttonHeight), align, text());

            blendColor.setRgb(blendColor.red()   + redInc,
                              blendColor.green() + greenInc,
                              blendColor.blue()  + blueInc);
        }
    }
}

void URLNavigatorButton::enterEvent(QEvent* event)
{
    URLButton::enterEvent(event);

    // if the text is clipped due to a small window width, the text should
    // be shown as tooltip
    if (isTextClipped()) {
        QToolTip::add(this, text());
    }
}

void URLNavigatorButton::leaveEvent(QEvent* event)
{
    URLButton::leaveEvent(event);
    QToolTip::remove(this);
}

void URLNavigatorButton::dropEvent(QDropEvent* event)
{
    if (m_index < 0) {
        return;
    }

    KUrl::List urls;
    /* KDE4-TODO:
    if (KUrlDrag::decode(event, urls) && !urls.isEmpty()) {
        setDisplayHintEnabled(DraggedHint, true);

        QString path(urlNavigator()->url().prettyURL());
        path = path.section('/', 0, m_index);

        Dolphin::mainWin().dropURLs(urls, KUrl(path));

        setDisplayHintEnabled(DraggedHint, false);
        update();
    }*/
}

void URLNavigatorButton::dragEnterEvent(QDragEnterEvent* event)
{
    /* KDE4-TODO:
    event->accept(KUrlDrag::canDecode(event));

    setDisplayHintEnabled(DraggedHint, true);*/
    update();
}

void URLNavigatorButton::dragLeaveEvent(QDragLeaveEvent* event)
{
    URLButton::dragLeaveEvent(event);

    setDisplayHintEnabled(DraggedHint, false);
    update();
}


void URLNavigatorButton::updateNavigatorURL()
{
    if (m_index < 0) {
        return;
    }

    URLNavigator* navigator = urlNavigator();
    assert(navigator != 0);
    navigator->setURL(navigator->url(m_index));
}

void URLNavigatorButton::startPopupDelay()
{
    if (m_popupDelay->isActive() || m_listJob || m_index < 0) {
        return;
    }

    m_popupDelay->start(300, true);
}

void URLNavigatorButton::stopPopupDelay()
{
    m_popupDelay->stop();
    if (m_listJob) {
        m_listJob->kill();
        m_listJob = 0;
    }
}

void URLNavigatorButton::startListJob()
{
    if (m_listJob) {
        return;
    }

    KUrl url = urlNavigator()->url(m_index);
    m_listJob = KIO::listDir(url, false, false);
    m_subdirs.clear(); // just to be ++safe

    connect(m_listJob, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList &)),
            this, SLOT(entriesList(KIO::Job*, const KIO::UDSEntryList&)));
    connect(m_listJob, SIGNAL(result(KIO::Job*)), this, SLOT(listJobFinished(KIO::Job*)));
}

void URLNavigatorButton::entriesList(KIO::Job* job, const KIO::UDSEntryList& entries)
{
    if (job != m_listJob) {
        return;
    }

    KIO::UDSEntryList::const_iterator it = entries.constBegin();
    KIO::UDSEntryList::const_iterator itEnd = entries.constEnd();
    while (it != itEnd) {
        QString name;
        bool isDir = false;
        KIO::UDSEntry entry = *it;

        /* KDE3 reference:
            KIO::UDSEntry::const_iterator atomIt = entry.constBegin();
            KIO::UDSEntry::const_iterator atomEndIt = entry.constEnd();

            while (atomIt != atomEndIt) {
            switch ((*atomIt).m_uds) {
                case KIO::UDS_NAME:
                    name = (*atomIt).m_str;
                    break;
                case KIO::UDS_FILE_TYPE:
                    isDir = S_ISDIR((*atomIt).m_long);
                    break;
                default:
                    break;
            }
            ++atomIt;
         }
         if (isDir) {
             m_subdirs.append(name);
         }
        */

        if (entry.isDir()) {
            m_subdirs.append(entry.stringValue(KIO::UDS_NAME));
        }

        ++it;
    }

    m_subdirs.sort();
}

void URLNavigatorButton::listJobFinished(KIO::Job* job)
{
    if (job != m_listJob) {
        return;
    }

    if (job->error() || m_subdirs.isEmpty()) {
        // clear listing
        return;
    }

    setDisplayHintEnabled(PopupActiveHint, true);
    update(); // ensure the button is drawn highlighted
    Q3PopupMenu* dirsMenu = new Q3PopupMenu(this);
    //setPopup(dirsMenu);
    QStringList::const_iterator it = m_subdirs.constBegin();
    QStringList::const_iterator itEnd = m_subdirs.constEnd();
    int i = 0;
    while (it != itEnd) {
        dirsMenu->insertItem(*it, i);
        ++i;
        ++it;
    }

    int result = dirsMenu->exec(urlNavigator()->mapToGlobal(geometry().bottomLeft()));

    if (result != -1) {
        KUrl url = urlNavigator()->url(m_index);
        url.addPath(m_subdirs[result]);
        urlNavigator()->setURL(url);
    }

    m_listJob = 0;
    m_subdirs.clear();
    delete dirsMenu;
    setDisplayHintEnabled(PopupActiveHint, false);
}

int URLNavigatorButton::arrowWidth() const
{
    int width = (height() / 2) - 7;
    if (width < 4) {
        width = 4;
    }
    return width;
}

bool URLNavigatorButton::isTextClipped() const
{
    int availableWidth = width();
    if (!isDisplayHintEnabled(ActivatedHint)) {
        availableWidth -= arrowWidth() + 1;
    }

    QFontMetrics fontMetrics(font());
    return fontMetrics.width(text()) >= availableWidth;
}
