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

#include <kparts/part.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>
#include <kio/job.h>

#include <kurlnavigator.h>

#include <QtGui/QKeyEvent>
#include <QtCore/QLinkedList>
#include <QtGui/QListView>
#include <QtGui/QBoxLayout>
#include <QtGui/QWidget>

class DolphinController;
class FilterBar;
class KFileItemDelegate;
class KUrl;
class KDirModel;
class KUrlNavigator;
class DolphinColumnView;
class DolphinDetailsView;
class DolphinDirLister;
class DolphinIconsView;
class DolphinMainWindow;
class DolphinSortFilterProxyModel;
class DolphinStatusBar;
class QModelIndex;
class ViewProperties;

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
                         const KUrl& url,
                         DolphinView::Mode mode = DolphinView::IconsView,
                         bool showHiddenFiles = false);

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

    /**
     * Triggers the renaming of the currently selected items, where
     * the user must input a new name for the items.
     */
    void renameSelectedItems();

    KFileItem* fileItem(const QModelIndex index) const;

    DolphinStatusBar* statusBar() const;

    /**
     * Returns true, if the URL shown by the navigation bar is editable.
     * @see KUrlNavigator
     */
    bool isUrlEditable() const;

    inline KUrlNavigator* urlNavigator() const;

    inline DolphinView* view() const;

    /** Returns true, if the filter bar is visible. */
    bool isFilterBarVisible() const;

    /**
     * Return the DolphinMainWindow this View belongs to. It is guaranteed
     * that we have one.
     */
    DolphinMainWindow* mainWindow() const ;

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
     * Updates the number of items (= number of directories + number of files)
     * and shows this information in the statusbar.
     */
    void updateItemCount();

    /**
     * Shows the item information for the URL \a url inside the statusbar. If the
     * URL is empty, the default statusbar information is shown.
     */
    void showItemInfo(const KUrl& url);

    /** Shows the information \a msg inside the statusbar. */
    void showInfoMessage(const QString& msg);

    /** Shows the error message \a msg inside the statusbar. */
    void showErrorMessage(const QString& msg);

    void closeFilterBar();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void changeNameFilter(const QString& nameFilter);

    /**
     * Opens the context menu on the current mouse postition.
     * @item  File item context. If item is 0, the context menu
     *        should be applied to \a url.
     * @url   URL which contains \a item.
     */
    void openContextMenu(KFileItem* item, const KUrl& url);

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

    int m_iconSize;
    int m_folderCount;
    int m_fileCount;

    DolphinMainWindow* m_mainWindow;
    QVBoxLayout* m_topLayout;
    KUrlNavigator* m_urlNavigator;

    DolphinView* m_view;

    FilterBar* m_filterBar;
    DolphinStatusBar* m_statusBar;

    KDirModel* m_dirModel;
    DolphinDirLister* m_dirLister;
    DolphinSortFilterProxyModel* m_proxyModel;
};

KUrlNavigator* DolphinViewContainer::urlNavigator() const
{
    return m_urlNavigator;
}

DolphinView* DolphinViewContainer::view() const
{
    return m_view;
}

#endif // DOLPHINVIEWCONTAINER_H
