/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef DOLPHINVIEWCONTROLLER_H
#define DOLPHINVIEWCONTROLLER_H

#include <views/dolphinview.h>
#include <kurl.h>
#include <QtCore/QObject>
#include <libdolphin_export.h>

class QAbstractItemView;
class DolphinView;
class KUrl;
class QPoint;

/**
 * @brief Allows the view mode implementations (DolphinIconsView, DolphinDetailsView, DolphinColumnView)
 *        to do a limited control of the DolphinView.
 *
 * The DolphinView connects to the signals of DolphinViewController to react on changes
 * that have been triggered by the view mode implementations. The view mode implementations
 * have read access to the whole DolphinView by DolphinViewController::view(). Limited control of the
 * DolphinView by the view mode implementations are defined by the public DolphinViewController methods.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinViewController : public QObject
{
    Q_OBJECT

public:
    explicit DolphinViewController(DolphinView* dolphinView);
    virtual ~DolphinViewController();

    /**
     * Allows read access for the view mode implementation
     * to the DolphinView.
     */
    const DolphinView* view() const;

    /**
     * Requests the DolphinView to change the URL to \p url. The signal
     * urlChangeRequested will be emitted.
     */
    void requestUrlChange(const KUrl& url);

    /**
     * Changes the current view mode implementation where the controller is working.
     * This is only necessary for views like the column view, where internally
     * several QAbstractItemView instances are used.
     */
    void setItemView(QAbstractItemView* view);
    QAbstractItemView* itemView() const;

    /**
     * Requests a context menu for the position \a pos. DolphinView
     * takes care itself to get the selected items depending from
     * \a pos. It is possible to define a custom list of actions for
     * the context menu by \a customActions.
     */
    void triggerContextMenuRequest(const QPoint& pos,
                                   const QList<QAction*>& customActions = QList<QAction*>());

    /**
     * Requests an activation of the DolphinView and emits the signal
     * activated(). This method should be invoked by the view mode implementation
     * if e. g. a mouse click on the view has been done.
     */
    void requestActivation();

    /**
     * Indicates that URLs are dropped above a destination. The DolphinView
     * will start the corresponding action (copy, move, link).
     * @param destItem  Item of the destination (can be null, see KFileItem::isNull()).
     * @param event     Drop event
     */
    void indicateDroppedUrls(const KFileItem& destItem, QDropEvent* event);

    /**
     * Informs the DolphinView about a sorting change done inside
     * the view mode implementation.
     */
    void indicateSortingChange(DolphinView::Sorting sorting);

    /**
     * Informs the DolphinView about a sort order change done inside
     * the view mode implementation.
     */
    void indicateSortOrderChange(Qt::SortOrder order);

    /**
     * Informs the DolphinView about a change between separate sorting
     * (with folders first) and mixed sorting of files and folders done inside
     * the view mode implementation.
     */
    void indicateSortFoldersFirstChange(bool foldersFirst);

    /**
     * Informs the DolphinView about an additional information change
     * done inside the view mode implementation.
     */
    void indicateAdditionalInfoChange(const KFileItemDelegate::InformationList& info);

    /**
     * Sets the available version control actions. Is called by the view
     * mode implementation as soon as the DolphinView has requested them by the signal
     * requestVersionControlActions().
     */
    void setVersionControlActions(QList<QAction*> actions);

    /**
     * Emits the signal requestVersionControlActions(). The view mode implementation
     * listens to the signal and will invoke a DolphinViewController::setVersionControlActions()
     * and the result will be returned.
     */
    QList<QAction*> versionControlActions(const KFileItemList& items);

    /**
     * Must be be invoked in each view mode implementation whenever a key has been
     * pressed. If the selection model of \a view is not empty and
     * the return key has been pressed, the selected items will get triggered.
     */
    void handleKeyPressEvent(QKeyEvent* event);

    /**
     * Replaces the URL of the DolphinView  with the content
     * of the clipboard as URL. If the clipboard contains no text,
     * nothing will be done.
     */
    void replaceUrlByClipboard();

    /**
     * Requests the view mode implementation to hide tooltips.
     */
    void requestToolTipHiding();

    /**
     * Emits the signal itemTriggered() for the item \a item.
     * The method can be used by the view mode implementations to
     * trigger an item directly without mouse interaction.
     * If the item triggering is done by the mouse, it is recommended
     * to use DolphinViewController::triggerItem(), as this will check
     * the used mouse buttons to execute the correct action.
     */
    void emitItemTriggered(const KFileItem& item);

    /**
     * Returns the file item for the proxy index \a index of the DolphinView.
     */
    KFileItem itemForIndex(const QModelIndex& index) const;

public slots:
    /**
     * Emits the signal itemTriggered() if the file item for the index \a index
     * is not null and the left mouse button has been pressed. If the item is
     * null, the signal itemEntered() is emitted.
     * The method should be invoked by the view mode implementations whenever the
     * user has triggered an item with the mouse (see
     * QAbstractItemView::clicked() or QAbstractItemView::doubleClicked()).
     */
    void triggerItem(const QModelIndex& index);

    /**
     * Emits the signal tabRequested(), if the file item for the index \a index
     * represents a directory and when the middle mouse button has been pressed.
     */
    void requestTab(const QModelIndex& index);

    /**
     * Emits the signal itemEntered() if the file item for the index \a index
     * is not null. The method should be invoked by the view mode implementation
     * whenever the mouse cursor is above an item.
     */
    void emitItemEntered(const QModelIndex& index);

    /**
     * Emits the signal viewportEntered(). The method should be invoked by
     * the view mode implementation whenever the mouse cursor is above the viewport.
     */
    void emitViewportEntered();

signals:
    void urlChangeRequested(const KUrl& url);

    /**
     * Is emitted if a context menu should be opened (see triggerContextMenuRequest()).
     * @param pos           Position relative to the view widget where the
     *                      context menu should be opened. It is recommended
     *                      to get the corresponding model index from
     *                      this position.
     * @param customActions List of actions that is added to the context menu when
     *                      the menu is opened above the viewport.
     */
    void requestContextMenu(const QPoint& pos, QList<QAction*> customActions);

    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
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
     * the view mode implementation (see indicateSortingChanged().
     * The DolphinView connects to
     * this signal to update its menu action.
     */
    void sortingChanged(DolphinView::Sorting sorting);

    /**
     * Is emitted if the sort order has been changed to \a order
     * by the view mode implementation (see indicateSortOrderChanged().
     * The DolphinView connects
     * to this signal to update its menu actions.
     */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if 'sort folders first' has been changed to \a foldersFirst
     * by the view mode implementation (see indicateSortOrderChanged().
     * The DolphinView connects
     * to this signal to update its menu actions.
     */
    void sortFoldersFirstChanged(bool foldersFirst);

    /**
     * Is emitted if the additional info has been changed to \a info
     * by the view mode implementation. The DolphinView connects
     * to this signal to update its menu actions.
     */
    void additionalInfoChanged(const KFileItemDelegate::InformationList& info);

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
     */
    void itemEntered(const KFileItem& item);

    /**
     * Is emitted if a new tab should be opened for the URL \a url.
     */
    void tabRequested(const KUrl& url);

    /**
     * Is emitted if the mouse cursor has entered
     * the viewport (see emitViewportEntered()).
     */
    void viewportEntered();

    /**
     * Is emitted, if DolphinViewController::requestToolTipHiding() is invoked
     * and requests to hide all tooltips.
     */
    void hideToolTip();

    /**
     * Is emitted if pending previews should be canceled (e. g. because of an URL change).
     */
    void cancelPreviews();

    /**
     * Requests the view mode implementation to invoke DolphinViewController::setVersionControlActions(),
     * so that they can be returned with DolphinViewController::versionControlActions() for
     * the DolphinView.
     */
    void requestVersionControlActions(const KFileItemList& items);

private slots:
    void updateMouseButtonState();

private:
    static Qt::MouseButtons m_mouseButtons; // TODO: this is a workaround until  Qt-issue 176832 has been fixed

    DolphinView* m_dolphinView;
    QAbstractItemView* m_itemView;
    QList<QAction*> m_versionControlActions;
};

#endif
