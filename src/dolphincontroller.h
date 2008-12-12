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

class QAbstractItemView;
class DolphinView;
class KUrl;
class QBrush;
class QPoint;
class QRect;
class QWidget;

// TODO: get rid of all the state duplications in the controller and allow read access
// to the Dolphin view for all view implementations

/**
 * @brief Acts as mediator between the abstract Dolphin view and the view
 *        implementations.
 *
 * The abstract Dolphin view (see DolphinView) represents the parent of the controller.
 * The lifetime of the controller is equal to the lifetime of the Dolphin view.
 * The controller is passed to the current view implementation
 * (see DolphinIconsView, DolphinDetailsView and DolphinColumnView)
 * by passing it in the constructor and informing the controller about the change
 * of the view implementation:
 *
 * \code
 * QAbstractItemView* view = new DolphinIconsView(parent, controller);
 * controller->setItemView(view);
 * \endcode
 *
 * The communication of the view implementations to the abstract view is done by:
 * - triggerContextMenuRequest()
 * - requestActivation()
 * - triggerUrlChangeRequest()
 * - indicateDroppedUrls()
 * - indicateSortingChange()
 * - indicateSortOrderChanged()
 * - triggerItem()
 * - requestTab()
 * - handleKeyPressEvent()
 * - emitItemEntered()
 * - emitViewportEntered()
 * - replaceUrlByClipboard()
 * - hideToolTip()
 *
 * The communication of the abstract view to the view implementations is done by:
 * - setUrl()
 * - setShowHiddenFiles()
 * - setShowPreview()
 * - indicateActivationChange()
 * - setZoomLevel()
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinController : public QObject
{
    Q_OBJECT

public:
    explicit DolphinController(DolphinView* dolphinView);
    virtual ~DolphinController();

    /**
     * Allows read access for the view implementation to the abstract
     * Dolphin view.
     */
    const DolphinView* dolphinView() const;

    /**
     * Sets the URL to \a url and emits the signal urlChanged() if
     * \a url is different for the current URL. This method should
     * be invoked by the abstract Dolphin view whenever the current
     * URL has been changed.
     */
    void setUrl(const KUrl& url);
    const KUrl& url() const;

    /**
     * Changes the current item view where the controller is working. This
     * is only necessary for views like the tree view, where internally
     * several QAbstractItemView instances are used.
     */
    void setItemView(QAbstractItemView* view);

    QAbstractItemView* itemView() const;

    /**
     * Allows a view implementation to request an URL change to \a url.
     * The signal requestUrlChange() is emitted and the abstract Dolphin view
     * will assure that the URL of the Dolphin Controller will be updated
     * later. Invoking this method makes only sense if the view implementation
     * shows a hierarchy of URLs and allows to change the URL within
     * the view (e. g. this is the case in the column view).
     */
    void triggerUrlChangeRequest(const KUrl& url);

    /**
     * Requests a context menu for the position \a pos. This method
     * should be invoked by the view implementation when a context
     * menu should be opened. The abstract Dolphin view itself
     * takes care itself to get the selected items depending from
     * \a pos.
     */
    void triggerContextMenuRequest(const QPoint& pos);

    /**
     * Requests an activation of the view and emits the signal
     * activated(). This method should be invoked by the view implementation
     * if e. g. a mouse click on the view has been done.
     * After the activation has been changed, the view implementation
     * might listen to the activationChanged() signal.
     */
    void requestActivation();

    /**
     * Indicates that URLs are dropped above a destination. This method
     * should be invoked by the view implementation. The abstract Dolphin view
     * will start the corresponding action (copy, move, link).
     * @param destItem  Item of the destination (can be null, see KFileItem::isNull()).
     * @param destPath  Path of the destination.
     * @param event     Drop event
     */
    void indicateDroppedUrls(const KFileItem& destItem,
                             const KUrl& destPath,
                             QDropEvent* event);

    /**
     * Informs the abstract Dolphin view about a sorting change done inside
     * the view implementation. This method should be invoked by the view
     * implementation (e. g. the details view uses this method in combination
     * with the details header).
     */
    void indicateSortingChange(DolphinView::Sorting sorting);

    /**
     * Informs the abstract Dolphin view about a sort order change done inside
     * the view implementation. This method should be invoked by the view
     * implementation (e. g. the details view uses this method in combination
     * with the details header).
     */
    void indicateSortOrderChange(Qt::SortOrder order);

    /**
     * Informs the abstract Dolphin view about an additional information change
     * done inside the view implementation. This method should be invoked by the
     * view implementation (e. g. the details view uses this method in combination
     * with the details header).
     */
    void indicateAdditionalInfoChange(const KFileItemDelegate::InformationList& info);

    /**
     * Informs the view implementation about a change of the activation
     * state and is invoked by the abstract Dolphin view. The signal
     * activationChanged() is emitted.
     */
    void indicateActivationChange(bool active);

    /**
     * Sets the zoom level to \a level and emits the signal zoomLevelChanged().
     * It must be assured that the used level is inside the range
     * DolphinController::zoomLevelMinimum() and
     * DolphinController::zoomLevelMaximum().
     * Is invoked by the abstract Dolphin view.
     */
    void setZoomLevel(int level);
    int zoomLevel() const;

    /**
     * Tells the view implementation to zoom out by emitting the signal zoomOut()
     * and is invoked by the abstract Dolphin view.
     */
    void triggerZoomOut();

    /**
     * Should be invoked in each view implementation whenever a key has been
     * pressed. If the selection model of \a view is not empty and
     * the return key has been pressed, the selected items will get triggered.
     */
    void handleKeyPressEvent(QKeyEvent* event);

    /**
     * Replaces the URL of the abstract Dolphin view  with the content
     * of the clipboard as URL. If the clipboard contains no text,
     * nothing will be done.
     */
    void replaceUrlByClipboard();

    /** Emits the signal hideToolTip(). */
    void emitHideToolTip();

    /**
     * Returns the file item for the proxy index \a index of the view \a view.
     */
    KFileItem itemForIndex(const QModelIndex& index) const;

public slots:
    /**
     * Emits the signal itemTriggered() if the file item for the index \a index
     * is not null and the left mouse button has been pressed. If the item is
     * null, the signal itemEntered() is emitted.
     * The method should be invoked by the controller parent whenever the
     * user has triggered an item.
     */
    void triggerItem(const QModelIndex& index);

    /**
     * Emits the signal tabRequested(), if the file item for the index \a index
     * represents a directory and when the middle mouse button has been pressed.
     * The method should be invoked by the controller parent.
     */
    void requestTab(const QModelIndex& index);

    /**
     * Emits the signal itemEntered() if the file item for the index \a index
     * is not null. The method should be invoked by the controller parent
     * whenever the mouse cursor is above an item.
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
     * Is emitted if the view implementation requests a changing of the current
     * URL to \a url (see triggerUrlChangeRequest()).
     */
    void requestUrlChange(const KUrl& url);

    /**
     * Is emitted if a context menu should be opened (see triggerContextMenuRequest()).
     * The abstract Dolphin view connects to this signal and will open the context menu.
     * @param pos       Position relative to the view widget where the
     *                  context menu should be opened. It is recommended
     *                  to get the corresponding model index from
     *                  this position.
     */
    void requestContextMenu(const QPoint& pos);

    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     * The abstract Dolphin view connects to this signal to know the
     * destination view for the menu actions.
     */
    void activated();

    /**
     * Is emitted if URLs have been dropped to the destination
     * path \a destPath. If the URLs have been dropped above an item of
     * the destination path, the item is indicated by \a destItem
     * (can be null, see KFileItem::isNull()).
     */
    void urlsDropped(const KFileItem& destItem,
                     const KUrl& destPath,
                     QDropEvent* event);

    /**
     * Is emitted if the sorting has been changed to \a sorting by
     * the view implementation (see indicateSortingChanged().
     * The abstract Dolphin view connects to
     * this signal to update its menu action.
     */
    void sortingChanged(DolphinView::Sorting sorting);

    /**
     * Is emitted if the sort order has been changed to \a order
     * by the view implementation (see indicateSortOrderChanged().
     * The abstract Dolphin view connects
     * to this signal to update its menu actions.
     */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if the additional info has been changed to \a info
     * by the view implementation. The abstract Dolphin view connects
     * to this signal to update its menu actions.
     */
    void additionalInfoChanged(const KFileItemDelegate::InformationList& info);

    /**
     * Is emitted if the activation state has been changed to \a active
     * by the abstract Dolphin view.
     * The view implementation might connect to this signal if custom
     * updates are required in this case.
     */
    void activationChanged(bool active);

    /**
     * Is emitted if the item \a item should be triggered. The abstract
     * Dolphin view connects to this signal. If the item represents a directory,
     * the directory is opened. On a file the corresponding application is opened.
     * The item is null (see KFileItem::isNull()), when clicking on the viewport itself.
     */
    void itemTriggered(const KFileItem& item);

    /**
     * Is emitted if the mouse cursor has entered the item
     * given by \a index (see emitItemEntered()).
     * The abstract Dolphin view connects to this signal.
     */
    void itemEntered(const KFileItem& item);

    /**
     * Is emitted if a new tab should be opened for the URL \a url.
     */
    void tabRequested(const KUrl& url);

    /**
     * Is emitted if the mouse cursor has entered
     * the viewport (see emitViewportEntered().
     * The abstract Dolphin view connects to this signal.
     */
    void viewportEntered();

    /**
     * Is emitted if the view should change the zoom to \a level. The view implementation
     * must connect to this signal if it supports zooming.
     */
    void zoomLevelChanged(int level);

    /**
     * Is emitted if the abstract view should hide an open tooltip.
     */
    void hideToolTip();

private slots:
    void updateMouseButtonState();

private:
    int m_zoomLevel;
    Qt::MouseButtons m_mouseButtons; // TODO: this is a workaround until  Qt-issue 176832 has been fixed
    KUrl m_url;
    DolphinView* m_dolphinView;
    QAbstractItemView* m_itemView;
};

inline const DolphinView* DolphinController::dolphinView() const
{
    return m_dolphinView;
}

inline const KUrl& DolphinController::url() const
{
    return m_url;
}

inline QAbstractItemView* DolphinController::itemView() const
{
    return m_itemView;
}

inline int DolphinController::zoomLevel() const
{
    return m_zoomLevel;
}

#endif
