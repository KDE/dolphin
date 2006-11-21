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

#ifndef DOLPHINDETAILSVIEW_H
#define DOLPHINDETAILSVIEW_H

#include <kfiledetailview.h>
#include <itemeffectsmanager.h>
//Added by qt3to4:
#include <QPixmap>
#include <QResizeEvent>
#include <QEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QPaintEvent>

class QRect;
class QTimer;
class DolphinView;

/**
 * @brief Represents the details view which shows the name, size,
 * date, permissions, owner and group of an item.
 *
 * The width of the columns are automatically adjusted in a way
 * that full available width of the view is used by stretching the width
 * of the name column.
 *
 * @author Peter Penz
 */
class DolphinDetailsView : public KFileDetailView, public ItemEffectsManager
{
    Q_OBJECT

public:
    /**
     * Maps the column indices of KFileDetailView to a
     * descriptive column name.
     */
    enum ColumnName {
        NameColumn        = 0,
        SizeColumn        = 1,
        DateColumn        = 2,
        PermissionsColumn = 3,
        OwnerColumn       = 4,
        GroupColumn       = 5
    };

    DolphinDetailsView(DolphinView* parent);

    virtual ~DolphinDetailsView();

    /** @see ItemEffectsManager::updateItems */
    virtual void beginItemUpdates();

    /** @see ItemEffectsManager::updateItems */
    virtual void endItemUpdates();

    /** @see KFileView::insertItem */
    virtual void insertItem(KFileItem* fileItem);

    /**
     * @return  True, if the position \a pos is above the name of
     *          item \a item.
     */
    bool isOnFilename(const Q3ListViewItem* item, const QPoint& pos) const;

    /**
     * Reads out the dolphin settings for the details view and refreshs
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

signals:
    /**
     * Is send, if the details view should be activated. Usually an activation
     * is triggered by a mouse click.
     */
    void signalRequestActivation();

public slots:
    /** @see KFileDetailView::resizeContents */
    virtual void resizeContents(int width, int height);

    /** Is connected to the onItem-signal from KFileDetailView. */
    void slotOnItem(Q3ListViewItem* item);

    /** Is connected to the onViewport-signal from KFileDetailView. */
    void slotOnViewport();

protected:
    /** @see ItemEffectsManager::setContextPixmap() */
    virtual void setContextPixmap(void* context,
                                  const QPixmap& pixmap);

    /** @see ItemEffectsManager::setContextPixmap() */
    virtual const QPixmap* contextPixmap(void* context);

    /** @see ItemEffectsManager::setContextPixmap() */
    virtual void* firstContext();

    /** @see ItemEffectsManager::setContextPixmap() */
    virtual void* nextContext(void* context);

    /** @see ItemEffectsManager::setContextPixmap() */
    virtual KFileItem* contextFileInfo(void* context);

    /** @see KFileDetailView::contentsDragMoveEvent() */
    virtual void contentsDragMoveEvent(QDragMoveEvent* event);

    /** @see KFileDetailView::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

    /** @see KFileDetailView::acceptDrag() */
    virtual bool acceptDrag (QDropEvent* event) const;

    /** @see KFileDetailView::contentsDropEvent() */
    virtual void contentsDropEvent(QDropEvent* event);

    /** @see KFileDetailView::contentsMousePressEvent() */
    virtual void contentsMousePressEvent(QMouseEvent* event);

    /** @see KFileDetailView::contentsMouseMoveEvent() */
    virtual void contentsMouseMoveEvent(QMouseEvent* event);

    /** @see KFileDetailView::contentsMouseReleaseEvent() */
    virtual void contentsMouseReleaseEvent(QMouseEvent* event);

    /** @see QListView::paintEmptyArea() */
    virtual void paintEmptyArea(QPainter* painter, const QRect& rect);

    /** Draws the selection rubber. */
    void drawRubber();

    /** @see QListView::viewportPaintEvent() */
    virtual void viewportPaintEvent(QPaintEvent* paintEvent);

    /** @see QWidget::leaveEvent() */
    virtual void leaveEvent(QEvent* event);

private slots:
    void slotActivationUpdate();
    void slotContextMenuRequested(Q3ListViewItem* item,
                                  const QPoint& pos,
                                  int col);
    void slotUpdateDisabledItems();
    void slotAutoScroll();
    void updateColumnsWidth();
    void slotItemRenamed(Q3ListViewItem* item,
                         const QString& name,
                         int column);

    /**
     * Is invoked when a section from the header has
     * been clicked and stores the sort column and sort
     * order.
     */
    void slotHeaderClicked(int section);

private:
    class DolphinListViewItem : public KFileListViewItem {
    public:
        DolphinListViewItem(Q3ListView* parent,
                            KFileItem* fileItem);
        virtual ~DolphinListViewItem();
        virtual void paintCell(QPainter* painter,
                               const QColorGroup& colorGroup,
                               int column,
                               int cellWidth,
                               int alignment);

        virtual void paintFocus(QPainter* painter,
                                const QColorGroup& colorGroup,
                                const QRect& rect);
    };

    DolphinView* m_dolphinView;
    QTimer* m_resizeTimer;
    QTimer* m_scrollTimer;
    QRect* m_rubber;

    /**
     * Returns the width of the filename in pixels including
     * the icon. It is assured that the returned width is
     * <= the width of the filename column.
     */
    int filenameWidth(const Q3ListViewItem* item) const;

};

#endif
