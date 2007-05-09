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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef DOLPHINCONTROLLER_H
#define DOLPHINCONTROLLER_H

#include <dolphinview.h>
#include <kurl.h>
#include <QtCore/QObject>
#include <libdolphin_export.h>

class KUrl;
class QModelIndex;
class QPoint;

/**
 * @brief Allows to control Dolphin views and to react on state changes.
 *
 * One instance of a DolphinController can be assigned to a variable number of
 * Dolphin views (DolphinIconsView, DolphinDetailsView) by passing it in
 * the constructor:
 *
 * \code
 * DolphinController* controller = new DolphinController(parent);
 * DolphinDetailsView* detailsView = new DolphinDetailsView(parent, controller);
 * DolphinIconsView* iconsView = new DolphinIconsView(parent, controller);
 * \endcode
 *
 * The Dolphin view assures that the controller gets informed about selection changes,
 * when an item should be triggered and a lot more. The controller emits the corresponding signals
 * so that the receiver may react on those changes.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinController : public QObject
{
    Q_OBJECT

public:
    explicit DolphinController(QObject* parent);
    virtual ~DolphinController();

    void setUrl(const KUrl& url)
    {
        m_url = url;
    }
    const KUrl& url() const
    {
        return m_url;
    }

    void triggerContextMenuRequest(const QPoint& pos);

    void triggerActivation();

    void indicateDroppedUrls(const KUrl::List& urls,
                             const QModelIndex& index,
                             QWidget* source);

    void indicateSortingChange(DolphinView::Sorting sorting);

    void indicateSortOrderChange(Qt::SortOrder order);

    void setShowPreview(bool showPreview);
    bool showPreview() const
    {
        return m_showPreview;
    }

    void triggerZoomIn();
    void setZoomInPossible(bool possible)
    {
        m_zoomInPossible = possible;
    }
    bool isZoomInPossible() const
    {
        return m_zoomInPossible;
    }

    void triggerZoomOut();
    void setZoomOutPossible(bool possible)
    {
        m_zoomOutPossible = possible;
    }
    bool isZoomOutPossible() const
    {
        return m_zoomOutPossible;
    }

public slots:
    void triggerItem(const QModelIndex& index);
    void indicateSelectionChange();

signals:
    /**
     * Is emitted if a context menu should be opened.
     * @param pos       Position relative to the view widget where the
     *                  context menu should be opened. It is recommended
     *                  to get the corresponding model index from
     *                  this position.
     */
    void requestContextMenu(const QPoint& pos);

    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     */
    void activated();

    /**
     * Is emitted if the URLs \a urls have been dropped to the index
     * \a index. \a source indicates the widget where the dragging has
     * been started from.
     */
    void urlsDropped(const KUrl::List& urls,
                     const QModelIndex& index,
                     QWidget* source);

    /** Is emitted if the sorting has been changed to \a sorting. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order has been changed to \a sort order. */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if the state for showing previews has been
     * changed to \a showPreview.
     */
    void showPreviewChanged(bool showPreview);

    /**
     * Is emitted if the item with the index \a index should be triggered.
     * Usually triggering on a directory opens the directory, triggering
     * on a file opens the corresponding application.
     */
    void itemTriggered(const QModelIndex& index);

    /** Is emitted if the selection has been changed by the user. */
    void selectionChanged();

    /** Is emitted if the view should zoom in. */
    void zoomIn();

    /** Is emitted if the view should zoom out. */
    void zoomOut();

private:
    bool m_showPreview;
    bool m_zoomInPossible;
    bool m_zoomOutPossible;
    KUrl m_url;
};

#endif
