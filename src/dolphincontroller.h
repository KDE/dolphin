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
class QBrush;
class QModelIndex;
class QPoint;
class QRect;
class QWidget;

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

    /** Sets the URL to \a url and emits the signal urlChanged(). */
    void setUrl(const KUrl& url);
    inline const KUrl& url() const;

    void triggerContextMenuRequest(const QPoint& pos);

    void triggerActivation();

    void indicateDroppedUrls(const KUrl::List& urls,
                             const QModelIndex& index,
                             QWidget* source);

    void indicateSortingChange(DolphinView::Sorting sorting);

    void indicateSortOrderChange(Qt::SortOrder order);

    void setShowPreview(bool show);
    inline bool showPreview() const;

    void setShowAdditionalInfo(bool show);
    inline bool showAdditionalInfo() const;

    void triggerZoomIn();
    inline void setZoomInPossible(bool possible);
    inline bool isZoomInPossible() const;

    void triggerZoomOut();
    inline void setZoomOutPossible(bool possible);
    inline bool isZoomOutPossible() const;

    // TODO: remove this method when the issue #160611 is solved in Qt 4.4
    static void drawHoverIndication(QWidget* widget,
                                    const QRect& bounds,
                                    const QBrush& brush);

public slots:
    /**
     * Emits the signal itemTriggered(). The method should be invoked by the
     * controller parent whenever the user has triggered an item. */
    void triggerItem(const QModelIndex& index);

    /**
     * Emits the signal itemEntered(). The method should be invoked by
     * the controller parent whenever the mouse cursor is above an item.
     */
    void emitItemEntered(const QModelIndex& index);

    /**
     * Emits the signal viewportEntered(). The method should be invoked by
     * the controller parent whenever the mouse cursor is above the viewport.
     */
    void emitViewportEntered();

signals:
    /**
     * Is emitted if the URL for the Dolphin controller has been changed
     * to \a url.
     */
    void urlChanged(const KUrl& url);

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
     * changed to \a show.
     */
    void showPreviewChanged(bool show);

    /**
     * Is emitted if the state for showing additional info has been
     * changed to \a show.
     */
    void showAdditionalInfoChanged(bool show);

    /**
     * Is emitted if the item with the index \a index should be triggered.
     * Usually triggering on a directory opens the directory, triggering
     * on a file opens the corresponding application.
     */
    void itemTriggered(const QModelIndex& index);

    /**
     * Is emitted if the mouse cursor has entered the item
     * given by \a index.
     */
    void itemEntered(const QModelIndex& index);

    /**
     * Is emitted if the mouse cursor has entered
     * the viewport. */
    void viewportEntered();

    /** Is emitted if the view should zoom in. */
    void zoomIn();

    /** Is emitted if the view should zoom out. */
    void zoomOut();

private:
    bool m_showPreview;
    bool m_showAdditionalInfo;
    bool m_zoomInPossible;
    bool m_zoomOutPossible;
    KUrl m_url;
};

const KUrl& DolphinController::url() const
{
    return m_url;
}

bool DolphinController::showPreview() const
{
    return m_showPreview;
}

bool DolphinController::showAdditionalInfo() const
{
    return m_showAdditionalInfo;
}

void DolphinController::setZoomInPossible(bool possible)
{
    m_zoomInPossible = possible;
}

bool DolphinController::isZoomInPossible() const
{
    return m_zoomInPossible;
}

void DolphinController::setZoomOutPossible(bool possible)
{
    m_zoomOutPossible = possible;
}

bool DolphinController::isZoomOutPossible() const
{
    return m_zoomOutPossible;
}

#endif
