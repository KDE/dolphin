/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2000 Rik Hemsley <rik@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konqmimetyperesolver_h
#define __konqmimetyperesolver_h

#include <qlist.h>
#include <qtimer.h>
#include <kdebug.h>

/**
 * This class implements the "delayed-mimetype-determination" feature,
 * for konqueror's directory views. I hope kfile uses it one day too :)
 *
 * It determines the mimetypes of the icons in the background, but giving
 * preferrence to the visible icons.
 *
 * It is implemented as a template, so that it can work with both QListViewItem
 * and QIconViewItem, without requiring hacks such as void * or QPtrDict lookups.
 *
 * Here's what the parent must implement :
 - void mimeTypeDeterminationFinished();
 - QScrollView * scrollWidget();
 - void determineIcon( IconItem * item ), which should call
   KFileItem::determineMimeType on the fileItem, and update the icon, etc.
*/
template<class IconItem, class Parent>
class KonqMimeTypeResolver // if only this could be a QObject....
{
public:
    KonqMimeTypeResolver( Parent * parent, QTimer * timer )
        : m_parent(parent), m_timer(timer), m_delayNonVisibleIcons(10) {}

    ~KonqMimeTypeResolver() { m_timer->stop(); }

    /**
     * Start the mimetype-determination. Call this when the listing is completed.
     * @param delayNonVisibleIcons the delay to use between icons not on screen.
     * Usually 10, but should be set to 0 when the image preview feature is
     * activated, because image preview can only start once we know the mimetypes
     */
    void start( uint delayNonVisibleIcons = 10 )
    {
        m_timer->start( 0, true /* single shot */ );
        m_delayNonVisibleIcons = delayNonVisibleIcons;
    }

    /**
     * The list of items to process. The view is free to
     * clear it, insert new items into it, remove items, etc.
     */
    QList<IconItem> m_lstPendingMimeIconItems;

    /**
     * "Connected" to the viewportAdjusted signal of the scrollview
     */
    void slotViewportAdjusted();

    /**
     * "Connected" to the timer
     */
    void slotProcessMimeIcons();

private:
    /**
     * Find a visible icon and determine its mimetype.
     * KonqDirPart will call this method repeatedly until it returns 0L
     * (no more visible icon to process).
     * @return the file item that was just processed.
     */
    IconItem * findVisibleIcon();

    Parent * m_parent;
    QTimer * m_timer;
    uint m_delayNonVisibleIcons;
};

// The main slot
template<class IconItem, class Parent>
inline void KonqMimeTypeResolver<IconItem, Parent>::slotProcessMimeIcons()
{
    //kdDebug(1203) << "KonqMimeTypeResolver::slotProcessMimeIcons() "
    //              << m_lstPendingMimeIconItems.count() << endl;
    IconItem * item = 0L;
    int nextDelay = 0;

    if ( m_lstPendingMimeIconItems.count() > 0 )
    {
        // We only find mimetypes for icons that are visible. When more
        // of our viewport is exposed, we'll get a signal and then get
        // the mimetypes for the newly visible icons. (Rikkus)
        item = findVisibleIcon();
    }

    // No more visible items.
    if (0 == item)
    {
        // Do the unvisible ones, then, but with a bigger delay, if so configured
        if ( m_lstPendingMimeIconItems.count() > 0 )
        {
            item = m_lstPendingMimeIconItems.first();
            nextDelay = m_delayNonVisibleIcons;
        }
        else
        {
            m_parent->mimeTypeDeterminationFinished();
            return;
        }
    }

    m_parent->determineIcon(item);
    m_lstPendingMimeIconItems.remove(item);
    m_timer->start( nextDelay, true /* single shot */ );
}

template<class IconItem, class Parent>
inline void KonqMimeTypeResolver<IconItem, Parent>::slotViewportAdjusted()
{
    if (m_lstPendingMimeIconItems.isEmpty()) return;
    IconItem * item = findVisibleIcon();
    if (item)
    {
        m_parent->determineIcon( item );
        m_lstPendingMimeIconItems.remove(item);
        m_timer->start( 0, true /* single shot */ );
    }
}

template<class IconItem, class Parent>
inline IconItem * KonqMimeTypeResolver<IconItem, Parent>::findVisibleIcon()
{
    // Find an icon that's visible and whose mimetype we don't know.

    QListIterator<IconItem> it(m_lstPendingMimeIconItems);
    if ( m_lstPendingMimeIconItems.count()<20) // for few items, it's faster to not bother
        return m_lstPendingMimeIconItems.first();

    QScrollView * view = m_parent->scrollWidget();
    QRect visibleContentsRect
        (
            view->viewportToContents(QPoint(0, 0)),
            view->viewportToContents
            (
                QPoint(view->visibleWidth(), view->visibleHeight())
                )
            );

    for (; it.current(); ++it)
        if (visibleContentsRect.intersects(it.current()->rect()))
            return it.current();

    return 0L;
}

#endif
