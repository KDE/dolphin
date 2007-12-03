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

#include <QListView>
#include <QStyleOption>

#include <kurl.h>

class DolphinColumnView;
class DolphinModel;
class DolphinSortFilterProxyModel;
class KDirLister;
class KFileItem;
class KFileItemList;
class QPixmap;

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

    /** Sets the size of the icons. */
    void setDecorationSize(const QSize& size);

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
    void reload();

    void setShowHiddenFiles(bool show);
    void setShowPreview(bool show);

    /**
     * Updates the background color dependent from the activation state
     * \a isViewActive of the column view.
     */
    void updateBackground();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);
    virtual void setModel ( QAbstractItemModel * model );

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
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private slots:
    /**
     * If the item specified by \a index is a directory, then this
     * directory will be loaded in a new column. If the  item is a
     * file, the corresponding application will get started.
     */
    void triggerItem(const QModelIndex& index);

    /**
     * Generates a preview image for each file item in \a items.
     * The current preview settings (maximum size, 'Show Preview' menu)
     * are respected.
     */
    void generatePreviews(const KFileItemList& items);

    /**
     * Replaces the icon of the item \a item by the preview pixmap
     * \a pixmap.
     */
    void showPreview(const KFileItem& item, const QPixmap& pixmap);

    void slotEntered(const QModelIndex& index);

private:
    /** Used by DolphinColumnWidget::setActive(). */
    void activate();

    /** Used by DolphinColumnWidget::setActive(). */
    void deactivate();

    /**
     * Returns true, if the item \a item has been cut into
     * the clipboard.
     */
    bool isCutItem(const KFileItem& item) const;

    KFileItem itemForIndex(const QModelIndex& index) const;

private:
    bool m_active;
    bool m_showPreview;
    DolphinColumnView* m_view;
    KUrl m_url;      // URL of the directory that is shown
    KUrl m_childUrl; // URL of the next column that is shown
    QStyleOptionViewItem m_viewOptions;
    KDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    DolphinSortFilterProxyModel* m_proxyModel;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4
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
        reload();
    }
}

inline const KUrl& DolphinColumnWidget::url() const
{
    return m_url;
}

inline QStyleOptionViewItem DolphinColumnWidget::viewOptions() const
{
    return m_viewOptions;
}

#endif
