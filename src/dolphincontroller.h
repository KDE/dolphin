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
    const KUrl& url() const;

    void triggerContextMenuRequest(const QPoint& pos);

    void triggerActivation();

    void indicateDroppedUrls(const KUrl::List& urls,
                             const KUrl& destPath,
                             const QModelIndex& destIndex,
                             QWidget* source);

    void indicateSortingChange(DolphinView::Sorting sorting);

    void indicateSortOrderChange(Qt::SortOrder order);

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    void setShowPreview(bool show);
    bool showPreview() const;

    void setAdditionalInfoCount(int count);
    bool additionalInfoCount() const;

    void triggerZoomIn();
    void setZoomInPossible(bool possible);
    bool isZoomInPossible() const;

    void triggerZoomOut();
    void setZoomOutPossible(bool possible);
    bool isZoomOutPossible() const;

    // TODO: remove this method when the issue #160611 is solved in Qt 4.4
    static void drawHoverIndication(QWidget* widget,
                                    const QRect& bounds,
                                    const QBrush& brush);

public slots:
    /**
     * Emits the signal itemTriggered(). The method should be invoked by the
     * controller parent whenever the user has triggered an item. */
    void triggerItem(const KFileItem& item);

    /**
     * Emits the signal itemEntered(). The method should be invoked by
     * the controller parent whenever the mouse cursor is above an item.
     */
    void emitItemEntered(const KFileItem& item);

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
     * Is emitted if the URLs \a urls have been dropped to the destination
     * path \a destPath. If the URLs have been dropped above an item of
     * the destination path, the item is indicated by \a destIndex.
     * \a source indicates the widget where the dragging has been started from.
     */
    void urlsDropped(const KUrl::List& urls,
                     const KUrl& destPath,
                     const QModelIndex& destIndex,
                     QWidget* source);

    /** Is emitted if the sorting has been changed to \a sorting. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order has been changed to \a sort order. */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if the state for showing hidden files has been
     * changed to \a show.
     */
    void showHiddenFilesChanged(bool show);

    /**
     * Is emitted if the state for showing previews has been
     * changed to \a show.
     */
    void showPreviewChanged(bool show);

    /**
     * Is emitted if the number of additional informations has been
     * changed to \a count.
     */
    void additionalInfoCountChanged(int count);

    /**
     * Is emitted if the item \a item should be triggered.
     * Usually triggering on a directory opens the directory, triggering
     * on a file opens the corresponding application. The item is null
     * (see KFileItem::isNull()), when clicking on the viewport itself.
     */
    void itemTriggered(const KFileItem& item);

    /**
     * Is emitted if the mouse cursor has entered the item
     * given by \a index.
     */
    void itemEntered(const KFileItem& item);

    /**
     * Is emitted if the mouse cursor has entered
     * the viewport. */
    void viewportEntered();

    /** Is emitted if the view should zoom in. */
    void zoomIn();

    /** Is emitted if the view should zoom out. */
    void zoomOut();

private:
    bool m_showHiddenFiles;
    bool m_showPreview;
    bool m_zoomInPossible;
    bool m_zoomOutPossible;
    int m_additionalInfoCount;
    KUrl m_url;
};

inline const KUrl& DolphinController::url() const
{
    return m_url;
}

inline bool DolphinController::showHiddenFiles() const
{
    return m_showHiddenFiles;
}

inline bool DolphinController::showPreview() const
{
    return m_showPreview;
}

inline bool DolphinController::additionalInfoCount() const
{
    return m_additionalInfoCount;
}

inline void DolphinController::setZoomInPossible(bool possible)
{
    m_zoomInPossible = possible;
}

inline bool DolphinController::isZoomInPossible() const
{
    return m_zoomInPossible;
}

inline void DolphinController::setZoomOutPossible(bool possible)
{
    m_zoomOutPossible = possible;
}

inline bool DolphinController::isZoomOutPossible() const
{
    return m_zoomOutPossible;
}

#endif
