/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef ITEMEFFECTSMANAGER_H
#define ITEMEFFECTSMANAGER_H

#include <qobject.h>
#include <qpixmap.h>
#include <kurl.h>
#include <q3valuelist.h>
class KFileItem;

/**
 * @brief Abstract class to implement item effects for a Dolphin view.
 *
 * Derived classes must implement the following pure virtual methods:
 * - ItemEffectsManager::setContextPixmap()
 * - ItemEffectsManager::contextPixmap()
 * - ItemEffectsManager::firstContext()
 * - ItemEffectsManager::nextContext()
 * - ItemEffectsManager::contextFileInfo()
 *
 * The item effects manager highlights currently active items and also
 * respects cutted items. A 'context' is defined as abstract data type,
 * which usually is represented by a KFileListViewItem or
 * a KFileIconViewItem.
 *
 * In Qt4 the item effects manager should get integrated as part of Interview
 * and hence no abstract context handling should be necessary anymore. The main
 * purpose of the current interface is to prevent code duplication as there is
 * no common model shared by QListView and QIconView of Qt3.
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 * @author Peter Penz <peter.penz@gmx.at>
 */
class ItemEffectsManager
{
public:
    ItemEffectsManager();
    virtual ~ItemEffectsManager();

    /** Is invoked before the items get updated. */
    virtual void beginItemUpdates() = 0;

    /** Is invoked after the items have been updated. */
    virtual void endItemUpdates() = 0;

    /**
     * Increases the size of the current set view mode and refreshes
     * all views. Derived implementations must invoke the base implementation
     * if zooming in had been done.
     */
    virtual void zoomIn();

    /**
     * Decreases the size of the current set view mode and refreshes
     * all views. Derived implementations must invoke the base implementation
     * if zooming out had been done.
     */
    virtual void zoomOut();

    /**
     * Returns true, if zooming in is possible. If false is returned,
     * the minimal zoom size is possible.
     */
    virtual bool isZoomInPossible() const = 0;

    /**
     * Returns true, if zooming in is possible. If false is returned,
     * the minimal zoom size is possible.
     */
    virtual bool isZoomOutPossible() const = 0;

protected:
    virtual void setContextPixmap(void* context,
                                  const QPixmap& pixmap) = 0;
    virtual const QPixmap* contextPixmap(void* context) = 0;
    virtual void* firstContext() = 0;
    virtual void* nextContext(void* context) = 0;
    virtual KFileItem* contextFileInfo(void* context) = 0;

    void activateItem(void* context);
    void resetActivatedItem();
    void updateDisabledItems();

private:
    struct DisabledItem {
        KUrl url;
        QPixmap pixmap;
    };

    QPixmap* m_pixmapCopy;
    KUrl m_highlightedURL;

    // contains all items which have been disabled by a 'cut' operation
    Q3ValueList<DisabledItem> m_disabledItems;

    /** Returns the text for the statusbar for an activated item. */
    QString statusBarText(KFileItem* fileInfo) const;
};

#endif
