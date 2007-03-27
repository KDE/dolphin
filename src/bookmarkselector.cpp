/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "bookmarkselector.h"

#include "urlnavigator.h"

#include <assert.h>

#include <kiconloader.h>
#include <kglobalsettings.h>
#include <kbookmarkmanager.h>
#include <kmenu.h>
#include <kdebug.h>

#include <QPainter>
#include <QPixmap>
#include <kicon.h>

BookmarkSelector::BookmarkSelector(UrlNavigator* parent, KBookmarkManager* bookmarkManager) :
    UrlButton(parent),
    m_selectedAddress(),
    m_urlNavigator(parent),
    m_bookmarkManager(bookmarkManager)
{
    setFocusPolicy(Qt::NoFocus);

    m_bookmarksMenu = new KMenu(this);

    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark bookmark = root.first();
    int i = 0;
    while (!bookmark.isNull()) {
        QAction* action = new QAction(MainBarIcon(bookmark.icon()),
                                      bookmark.text(),
                                      this);
        m_bookmarksMenu->addAction(action);
        QString address = QChar('/');
        address += QString::number(i);
        action->setData(address);
        if (address == m_selectedAddress) {
            QPixmap pixmap = SmallIcon(bookmark.icon());
            setIcon(QIcon(pixmap));
            setIconSize(pixmap.size());
            setMinimumWidth(pixmap.width() + 2);
        }
        bookmark = root.next(bookmark);
        ++i;
    }

    connect(m_bookmarksMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(activateBookmark(QAction*)));

    setMenu(m_bookmarksMenu);
}

BookmarkSelector::~BookmarkSelector()
{
}

void BookmarkSelector::updateSelection(const KUrl& url)
{
    KBookmark bookmark = m_bookmarkManager->root().closestBookmark(url);
    if (!bookmark.isNull()) {
        m_selectedAddress = bookmark.address();
        setIcon(KIcon(bookmark.icon()));
    }
    else {
        m_selectedAddress = QString();
        // No bookmark has been found which matches to the given Url. Show
        // a generic folder icon as pixmap for indication:
        setIcon(KIcon("folder"));
    }
}

KBookmark BookmarkSelector::selectedBookmark() const
{
    return m_bookmarkManager->findByAddress(m_selectedAddress);
}

QSize BookmarkSelector::sizeHint() const
{
    const int height = UrlButton::sizeHint().height();
    return QSize(height, height);
}

void BookmarkSelector::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    const int buttonWidth  = width();
    const int buttonHeight = height();

    QColor backgroundColor;
    QColor foregroundColor;
    const bool isHighlighted = isDisplayHintEnabled(EnteredHint) ||
                               isDisplayHintEnabled(DraggedHint);
    if (isHighlighted) {
        backgroundColor = KGlobalSettings::highlightColor();
        foregroundColor = KGlobalSettings::highlightedTextColor();
    }
    else {
        backgroundColor = palette().brush(QPalette::Background).color();
        foregroundColor = KGlobalSettings::buttonTextColor();
    }

    // dimm the colors if the parent view does not have the focus
    const bool isActive = m_urlNavigator->isActive();
    if (!isActive) {
        QColor dimmColor(palette().brush(QPalette::Background).color());
        foregroundColor = mixColors(foregroundColor, dimmColor);
        if (isHighlighted) {
            backgroundColor = mixColors(backgroundColor, dimmColor);
        }
    }

    if (!(isDisplayHintEnabled(ActivatedHint) && isActive) && !isHighlighted) {
        // dimm the foreground color by mixing it with the background
        foregroundColor = mixColors(foregroundColor, backgroundColor);
        painter.setPen(foregroundColor);
    }

    // draw button backround
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);
    painter.drawRect(0, 0, buttonWidth, buttonHeight);

    // draw icon
    const QPixmap pixmap = icon().pixmap();
    const int x = (buttonWidth -  pixmap.width()) / 2;
    const int y = (buttonHeight - pixmap.height()) / 2;
    painter.drawPixmap(x, y, pixmap);
}

void BookmarkSelector::activateBookmark(QAction* action)
{
    assert(action != 0);
    m_selectedAddress = action->data().toString();

    const KBookmark bookmark = selectedBookmark();
    setPixmap(SmallIcon(bookmark.icon()));
    emit bookmarkActivated(bookmark.url());
}

#include "bookmarkselector.moc"

