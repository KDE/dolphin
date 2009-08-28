Don't compile

/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef DOLPHINCOLUMNWIDGET_H
#define DOLPHINCOLUMNWIDGET_H

#include "dolphinview.h"

#include <QFont>
#include <QListView>
#include <QSize>
#include <QStyleOption>

#include <kurl.h>

class DolphinColumnView;
class DolphinModel;
class DolphinSortFilterProxyModel;
class DolphinDirLister;
class DolphinViewAutoScroller;
class KFilePreviewGenerator;
class KFileItem;
class KFileItemList;
class SelectionManager;
class ToolTipManager;

/**
 * Represents one column inside the DolphinColumnView and has been
 * extended to respect view options and hovering information.
 */
class DolphinColumnWidget : public QListView
{
    Q_OBJECT

public:
    DolphinColumnWidget(QWidget* parent,
                        DolphinColumnView* columnView,
                        const KUrl& url);
    virtual ~DolphinColumnWidget();

    /**
     * An active column is defined as column, which shows the same URL
     * as indicated by the URL navigator. The active column is usually
     * drawn in a lighter color. All operations are applied to this column.
     */
    void setActive(bool active);
    bool isActive() const;

    /**
     * Sets the directory URL of the child column that is shown next to
     * this column. This property is only used for a visual indication
     * of the shown directory, it does not trigger a loading of the model.
     */
    void setChildUrl(const KUrl& url);
    const KUrl& childUrl() const;

    /** Sets the directory URL that is shown inside the column widget. */
    void setUrl(const KUrl& url);

    /** Returns the directory URL that is shown inside the column widget. */
    const KUrl& url() const;

    /** Reloads the directory DolphinColumnWidget::url(). */
    //void reload();

    /*void setSorting(DolphinView::Sorting sorting);
    void setSortOrder(Qt::SortOrder order);
    void setSortFoldersFirst(bool foldersFirst);
    void setShowHiddenFiles(bool show);
    void setShowPreview(bool show);*/

    /**
     * Updates the background color dependent from the activation state
     * \a isViewActive of the column view.
     */
    void updateBackground();

    /**
     * Does an inline editing for the item \a item.
     */
    void editItem(const KFileItem& item);

    /**
     * Returns the item on the position \a pos. The KFileItem instance
     * is null if no item is below the position.
     */
    KFileItem itemAt(const QPoint& pos) const;

    KFileItemList selectedItems() const;

    /**
     * Returns the MIME data for the selected items.
     */
    QMimeData* selectionMimeData() const;

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous);

private slots:
    void slotEntered(const QModelIndex& index);
    void requestActivation();
    void updateFont();

    void slotShowPreviewChanged();

private:
    /** Used by DolphinColumnWidget::setActive(). */
    void activate();

    /** Used by DolphinColumnWidget::setActive(). */
    void deactivate();

    void updateDecorationSize(bool showPreview);

private:
    bool m_active;
    DolphinColumnView* m_view;
    SelectionManager* m_selectionManager;
    DolphinViewAutoScroller* m_autoScroller;
    KUrl m_url;      // URL of the directory that is shown
    KUrl m_childUrl; // URL of the next column that is shown

    QFont m_font;
    QSize m_decorationSize;

    DolphinDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    DolphinSortFilterProxyModel* m_proxyModel;

    KFilePreviewGenerator* m_previewGenerator;

    ToolTipManager* m_toolTipManager;

    QRect m_dropRect;

    friend class DolphinColumnView;
};

inline bool DolphinColumnWidget::isActive() const
{
    return m_active;
}

inline void DolphinColumnWidget::setChildUrl(const KUrl& url)
{
    m_childUrl = url;
}

inline const KUrl& DolphinColumnWidget::childUrl() const
{
    return m_childUrl;
}

inline void DolphinColumnWidget::setUrl(const KUrl& url)
{
    if (url != m_url) {
        m_url = url;
        //reload();
    }
}

inline const KUrl& DolphinColumnWidget::url() const
{
    return m_url;
}

#endif
