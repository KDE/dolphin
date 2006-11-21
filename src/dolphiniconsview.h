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

#ifndef DOLPHINICONSVIEW_H
#define DOLPHINICONSVIEW_H

#include <kfileiconview.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <kurl.h>
#include <itemeffectsmanager.h>

class DolphinView;

/**
 * @brief Represents the view, where each item is shown as an icon.
 *
 * It is also possible that instead of the icon a preview of the item
 * content is shown.
 *
 * @author Peter Penz
 */
class DolphinIconsView : public KFileIconView, public ItemEffectsManager
{
    Q_OBJECT

public:
    enum LayoutMode {
        Icons,
        Previews
    };

    DolphinIconsView(DolphinView *parent, LayoutMode layoutMode);

    virtual ~DolphinIconsView();

    void setLayoutMode(LayoutMode mode);
    LayoutMode layoutMode() const { return m_layoutMode; }

    /** @see ItemEffectsManager::updateItems */
    virtual void beginItemUpdates();

    /** @see ItemEffectsManager::updateItems */
    virtual void endItemUpdates();

    /**
     * Reads out the dolphin settings for the icons view and refreshs
     * the details view.
     */
    // TODO: Other view implementations use a similar interface. When using
    // Interview in Qt4 this method should be moved to a base class (currently
    // not possible due to having different base classes for the views).
    void refreshSettings();

    /** @see ItemEffectsManager::zoomIn() */
    virtual void zoomIn();

    /** @see ItemEffectsManager::zoomOut() */
    virtual void zoomOut();

    /** @see ItemEffectsManager::isZoomInPossible() */
    virtual bool isZoomInPossible() const;

    /** @see ItemEffectsManager::isZoomOutPossible() */
    virtual bool isZoomOutPossible() const;

public slots:
    /**
     * Bypass a layout issue in KFileIconView in combination with previews.
     * @see KFileIconView::arrangeItemsInGrid
     */
    virtual void arrangeItemsInGrid(bool updated = true);

signals:
    /**
     * Is send, if the details view should be activated. Usually an activation
     * is triggered by a mouse click.
     */
    void signalRequestActivation();

protected:
    /** @see ItemEffectsManager::setContextPixmap */
    virtual void setContextPixmap(void* context,
                                  const QPixmap& pixmap);

    /** @see ItemEffectsManager::contextPixmap */
    virtual const QPixmap* contextPixmap(void* context);

    /** @see ItemEffectsManager::firstContext */
    virtual void* firstContext();

    /** @see ItemEffectsManager::nextContext */
    virtual void* nextContext(void* context);

    /** @see ItemEffectsManager::contextFileInfo */
    virtual KFileItem* contextFileInfo(void* context);

    /** @see KFileIconView::contentsMousePressEvent */
    virtual void contentsMousePressEvent(QMouseEvent* event);

    /** @see KFileIconView::contentsMouseReleaseEvent */
    virtual void contentsMouseReleaseEvent(QMouseEvent* event);

    /** @see KFileIconView::drawBackground */
    virtual void drawBackground(QPainter* painter, const QRect& rect);

    /** @see KFileIconView::dragObject */
    virtual Q3DragObject* dragObject();

    /** @see KFileIconView::contentsDragEnterEvent */
    virtual void contentsDragEnterEvent(QDragEnterEvent* event);

    /** @see KFileIconView::contentsDragMoveEvent */
    virtual void contentsDragMoveEvent(QDragMoveEvent* event);

    /** @see KFileIconView::contentsDropEvent */
    virtual void contentsDropEvent(QDropEvent* event);

private slots:
    /** Is connected to the onItem-signal from KFileIconView. */
    void slotOnItem(Q3IconViewItem* item);

    /** Is connected to the onViewport-signal from KFileIconView. */
    void slotOnViewport();

    /**
     * Opens the context menu for the item \a item on the given
     * position \a pos.
     */
    void slotContextMenuRequested(Q3IconViewItem* item,
                                  const QPoint& pos);

    /** Renames the item \a item to the name \a name. */
    void slotItemRenamed(Q3IconViewItem* item,
                         const QString& name);

    void slotActivationUpdate();
    void slotUpdateDisabledItems();

private:
    int m_previewIconSize;
    LayoutMode m_layoutMode;
    DolphinView* m_dolphinView;

    /** Returns the increased icon size for the size \a size. */
    int increasedIconSize(int size) const;

    /** Returns the decreased icon size for the size \a size. */
    int decreasedIconSize(int size) const;
};

#endif
