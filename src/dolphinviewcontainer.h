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


#ifndef DOLPHINVIEWCONTAINER_H
#define DOLPHINVIEWCONTAINER_H

#include "dolphinview.h"

#include <kfileitem.h>
#include <kfileitemdelegate.h>
#include <kio/job.h>

#include <kurlnavigator.h>

#include <QtGui/QKeyEvent>
#include <QtCore/QLinkedList>
#include <QtGui/QListView>
#include <QtGui/QBoxLayout>
#include <QtGui/QWidget>

class FilterBar;
class KUrl;
class DolphinModel;
class KUrlNavigator;
class DolphinDirLister;
class DolphinMainWindow;
class DolphinSortFilterProxyModel;
class DolphinStatusBar;
class QModelIndex;

/**
 * @short Represents a view for the directory content
 *        including the navigation bar, filter bar and status bar.
 *
 * View modes for icons, details and columns are supported. Currently
 * Dolphin allows to have up to two views inside the main window.
 *
 * @see DolphinView
 * @see FilterBar
 * @see KUrlNavigator
 * @see DolphinStatusBar
 */
class DolphinViewContainer : public QWidget
{
    Q_OBJECT

public:
    DolphinViewContainer(DolphinMainWindow* mainwindow,
                         QWidget *parent,
                         const KUrl& url);

    virtual ~DolphinViewContainer();

    /**
     * Sets the current active URL, where all actions are applied. The
     * URL navigator is synchronized with this URL. The signals
     * KUrlNavigator::urlChanged() and KUrlNavigator::historyChanged()
     * are emitted.
     * @see DolphinViewContainer::urlNavigator()
     */
    void setUrl(const KUrl& url);

    /**
     * Returns the current active URL, where all actions are applied.
     * The URL navigator is synchronized with this URL.
     */
    const KUrl& url() const;

    /**
     * If \a active is true, the view container will marked as active. The active
     * view container is defined as view where all actions are applied to.
     */
    void setActive(bool active);
    bool isActive() const;

    const DolphinStatusBar* statusBar() const;
    DolphinStatusBar* statusBar();

    /**
     * Returns true, if the URL shown by the navigation bar is editable.
     * @see KUrlNavigator
     */
    bool isUrlEditable() const;

    const KUrlNavigator* urlNavigator() const;
    KUrlNavigator* urlNavigator();

    const DolphinView* view() const;
    DolphinView* view();

    /** Returns true, if the filter bar is visible. */
    bool isFilterBarVisible() const;

public slots:
    /**
     * Popups the filter bar above the status bar if \a show is true.
     */
    void showFilterBar(bool show);

    /**
     * Updates the number of items (= number of files + number of
     * directories) in the statusbar. If files are selected, the number
     * of selected files and the sum of the filesize is shown.
     */
    void updateStatusBar();

signals:
    /**
     * Is emitted whenever the filter bar has changed its visibility state.
     */
    void showFilterBarChanged(bool shown);

private slots:
    void updateProgress(int percent);

    /**
     * Assures that the viewport position is restored and updates the
     * statusbar to reflect the current content.
     */
    void slotDirListerCompleted();

    /**
     * Handles clicking on an item
     */
    void slotItemTriggered(const KFileItem& item);

    /**
     * Shows the information for the item \a item inside the statusbar. If the
     * item is null, the default statusbar information is shown.
     */
    void showItemInfo(const KFileItem& item);

    /** Shows the information \a msg inside the statusbar. */
    void showInfoMessage(const QString& msg);

    /** Shows the error message \a msg inside the statusbar. */
    void showErrorMessage(const QString& msg);

    /** Shows the "operation completed" message \a msg inside the statusbar. */
    void showOperationCompletedMessage(const QString& msg);

    void closeFilterBar();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);

    /**
     * Opens the context menu on the current mouse position.
     * @item  File item context. If item is 0, the context menu
     *        should be applied to \a url.
     * @url   URL which contains \a item.
     */
    void openContextMenu(const KFileItem& item, const KUrl& url);

    /**
     * Saves the position of the contents to the
     * current history element.
     */
    void saveContentsPos(int x, int y);

    /**
     * Restores the contents position of the view, if the view
     * is part of the history.
     */
    void restoreContentsPos();

    /**
     * Marks the view container as active
     * (see DolphinViewContainer::setActive()).
     */
    void activate();

    /**
     * Restores the current view to show \a url and assures
     * that the root URL of the view is respected.
     */
    void restoreView(const KUrl& url);

    /**
     * Saves the root URL of the current URL \a url
     * into the URL navigator.
     */
    void saveRootUrl(const KUrl& url);

    /**
     * Is invoked when a redirection is done and changes the
     * URL of the URL navigator to \a newUrl without triggering
     * a reloading of the directory.
     */
    void redirect(const KUrl& oldUrl, const KUrl& newUrl);
    
private:
    /**
     * Returns the default text of the status bar, if no item is
     * selected.
     */
    QString defaultStatusBarText() const;

    /**
     * Returns the text for the status bar, if at least one item
     * is selected.
     */
    QString selectionStatusBarText() const;

private:
    bool m_showProgress;

    DolphinMainWindow* m_mainWindow;
    QVBoxLayout* m_topLayout;
    KUrlNavigator* m_urlNavigator;

    DolphinView* m_view;

    FilterBar* m_filterBar;
    DolphinStatusBar* m_statusBar;

    DolphinModel* m_dolphinModel;
    DolphinDirLister* m_dirLister;
    DolphinSortFilterProxyModel* m_proxyModel;
};

inline const DolphinStatusBar* DolphinViewContainer::statusBar() const
{
    return m_statusBar;
}

inline DolphinStatusBar* DolphinViewContainer::statusBar()
{
    return m_statusBar;
}

inline const KUrlNavigator* DolphinViewContainer::urlNavigator() const
{
    return m_urlNavigator;
}

inline KUrlNavigator* DolphinViewContainer::urlNavigator()
{
    return m_urlNavigator;
}

inline const DolphinView* DolphinViewContainer::view() const
{
    return m_view;
}

inline DolphinView* DolphinViewContainer::view()
{
    return m_view;
}

#endif // DOLPHINVIEWCONTAINER_H
