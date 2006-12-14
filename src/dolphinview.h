/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
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


#ifndef _DOLPHINVIEW_H_
#define _DOLPHINVIEW_H_

#include <qwidget.h>
//Added by qt3to4:
#include <QDropEvent>
#include <Q3ValueList>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <kparts/part.h>
#include <kfileitem.h>
#include <kfileiconview.h>
#include <kio/job.h>
#include <urlnavigator.h>

#include <QListView>

class QPainter;
class KUrl;
class KDirModel;
class QLineEdit;
class UrlNavigator;
class QTimer;
class Q3IconViewItem;
class Q3ListViewItem;
class Q3VBoxLayout;
class DolphinMainWindow;
class DolphinDirLister;
class DolphinStatusBar;
class DolphinIconsView;
class DolphinDetailsView;
class DolphinSortFilterProxyModel;
class ViewProperties;
class KProgress;
class FilterBar;

class QModelIndex;

/**
 * @short Represents a view for the directory content
 * including the navigation bar and status bar.
 *
 * View modes for icons, details and previews are supported. Currently
 * Dolphin allows to have up to two views inside the main window.
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 * @see UrlNavigator
 * @see DolphinStatusBar
 *
 * @author Peter Penz <peter.penz@gmx.at>
 */
class DolphinView : public QWidget
{
    Q_OBJECT

public:
	/**
     * Defines the view mode for a directory. The view mode
     * can be defined when constructing a DolphinView. The
     * view mode is automatically updated if the directory itself
     * defines a view mode (see class ViewProperties for details).
     */
    enum Mode
    {
        /**
         * The directory items are shown as icons including an
         * icon name. */
        IconsView = 0,

        /**
         * The icon, the name and at least the size of the directory
         * items are shown in a table. It is possible to add columns
         * for date, group and permissions.
         */
        DetailsView = 1,
        MaxModeEnum = DetailsView
    };

    /** Defines the sort order for the items of a directory. */
    enum Sorting
    {
        SortByName = 0,
        SortBySize = 1,
        SortByDate = 2,
        MaxSortEnum = SortByDate
    };

    DolphinView(DolphinMainWindow* mainwindow,
                QWidget *parent,
                const KUrl& url,
                Mode mode = IconsView,
                bool showHiddenFiles = false);

    virtual ~DolphinView();

    /**
     * Sets the current active Url.
     * The signals UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void setUrl(const KUrl& url);

    /** Returns the current active Url. */
    const KUrl& url() const;

    void requestActivation();
    bool isActive() const;

    void setMode(Mode mode);
    Mode mode() const;
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    void setViewProperties(const ViewProperties& props);

    /**
     * Triggers the renaming of the currently selected items, where
     * the user must input a new name for the items.
     */
    void renameSelectedItems();

    /**
     * Selects all items.
     * @see DolphinView::selectedItems()
     */
    void selectAll();

    /**
     * Inverts the current selection: selected items get unselected,
     * unselected items get selected.
     * @see DolphinView::selectedItems()
     */
    void invertSelection();

    /**
     * Goes back one step in the Url history. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goBack();

    /**
     * Goes forward one step in the Url history. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goForward();

    /**
     * Goes up one step of the Url path. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goUp();

    /**
     * Goes to the home Url. The signals UrlNavigator::urlChanged
     * and UrlNavigator::historyChanged are submitted.
     */
    void goHome();

    /**
     * Sets the Url of the navigation bar to an editable state
     * if \a editable is true. If \a editable is false, each part of
     * the location is presented by a button for a fast navigation.
     */
    void setUrlEditable(bool editable);

    /**
     * Returns the complete Url history. The index 0 indicates the oldest
     * history element.
     * @param index     Output parameter which indicates the current
     *                  index of the location.
     */
    const Q3ValueList<UrlNavigator::HistoryElem> urlHistory(int& index) const;

    /**
     * Returns true, if at least one item is selected.
     */
    bool hasSelection() const;

    /**
     * Returns the selected items. The list is empty if no item has been
     * selected.
     * @see DolphinView::selectedUrls()
     */
    KFileItemList selectedItems() const;

    /**
     * Returns a list of Urls for all selected items. An empty list
     * is returned, if no item is selected.
     * @see DolphinView::selectedItems()
     */
    KUrl::List selectedUrls() const;

    /**
     * Returns the current item, where the cursor is. 0 is returned, if there is no
     * current item (e. g. if the view is empty). Note that the current item must
     * not be a selected item.
     * @see DolphinView::selectedItems()
     */
    const KFileItem* currentFileItem() const;

    /**
     * Opens the context menu for the item indicated by \a fileInfo
     * on the position \a pos. If 0 is passed for the file info, a context
     * menu for the viewport is opened.
     */
    void openContextMenu(KFileItem* fileInfo, const QPoint& pos);

    /**
     * Renames the filename of the source Url by the new file name.
     * If the new file name already exists, a dialog is opened which
     * asks the user to enter a new name.
     */
    void rename(const KUrl& source, const QString& newName);

    /** Returns the status bar of the view. */
    DolphinStatusBar* statusBar() const;

    /**
     * Returns the x-position of the view content.
     * The content of the view might be larger than the visible area
     * and hence a scrolling must be done.
     */
    int contentsX() const;

    /**
     * Returns the y-position of the view content.
     * The content of the view might be larger than the visible area
     * and hence a scrolling must be done.
     */
    int contentsY() const;

    /**
     * Returns true, if the Url shown by the navigation bar is editable.
     * @see UrlNavigator
     */
    bool isUrlEditable() const;

    /** Increases the size of the current set view mode. */
    void zoomIn();

    /** Decreases the size of the current set view mode. */
    void zoomOut();

    /**
     * Returns true, if zooming in is possible. If false is returned,
     * the minimal zoom size is possible.
     */
    bool isZoomInPossible() const;

    /**
     * Returns true, if zooming out is possible. If false is returned,
     * the maximum zoom size is possible.
     */
    bool isZoomOutPossible() const;

    /** Sets the sort order of the items inside a directory (see DolphinView::Sorting). */
    void setSorting(Sorting sorting);

    /** Returns the sort order of the items inside a directory (see DolphinView::Sorting). */
    Sorting sorting() const;

    /** Sets the sort order (Qt::Ascending or Qt::Descending) for the items. */
    void setSortOrder(Qt::SortOrder order);

    /** Returns the current used sort order (Qt::Ascending or Qt::Descending). */
    Qt::SortOrder sortOrder() const;

    /** Refreshs the view settings by reading out the stored settings. */
    void refreshSettings();

    /**
     * Updates the number of items (= number of files + number of
     * directories) in the statusbar. If files are selected, the number
     * of selected files and the sum of the filesize is shown.
     */
    void updateStatusBar();

    /** Returns the UrlNavigator of the view for read access. */
    const UrlNavigator* urlNavigator() const { return m_urlNavigator; }

    /**
     * Triggers to request user information for the item given
     * by the Url \a url. The signal requestItemInfo is emitted,
     * which provides a way for widgets to get an indication to update
     * the item information.
     */
    void emitRequestItemInfo(const KUrl& url);

    /** Returns true, if the filter bar is visible. */
    bool isFilterBarVisible() const;

    /**
     * Return the DolphinMainWindow this View belongs to. It is guranteed
     * that we have one.
     */
    DolphinMainWindow* mainWindow() const ;

public slots:
    void reload();
    void slotUrlListDropped(QDropEvent* event,
                            const KUrl::List& urls,
                            const KUrl& url);

    /**
     * Slot that popups the filter bar like FireFox popups his Search bar.
     */
    void slotShowFilterBar(bool show);

    /**
     * Declare this View as the activeview of the mainWindow()
     */
    void declareViewActive();

signals:
    /** Is emitted if Url of the view has been changed to \a url. */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if the view mode (IconsView, DetailsView,
     * PreviewsView) has been changed.
     */
    void modeChanged();

    /** Is emitted if the 'show hidden files' property has been changed. */
    void showHiddenFilesChanged();

    /** Is emitted if the sorting by name, size or date has been changed. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order (ascending or descending) has been changed. */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the sidebar.
     * It the Url is empty, no item information request is pending.
     */
    void requestItemInfo(const KUrl& url);

    /** Is emitted if the contents has been moved to \a x, \a y. */
    void contentsMoved(int x, int y);

    /**
     * Is emitted whenever the selection has been changed. The current selection can
     * be retrieved by mainWindow()->activeView()->selectedItems() or by
     * mainWindow()->activeView()->selectedUrls().
     */
    void selectionChanged();

    /**
     * Is emitted whenever the filter bar has been turned show or hidden.
     */
    void showFilterBarChanged(bool shown);

protected:
    /** @see QWidget::mouseReleaseEvent */
    virtual void mouseReleaseEvent(QMouseEvent* event);

private slots:
    void loadDirectory(const KUrl& kurl);
    void triggerIconsViewItem(Q3IconViewItem *item);
    void triggerItem(const QModelIndex& index);
    void updateUrl();

    void slotPercent(int percent);
    void slotClear();
    void slotDeleteItem(KFileItem* item);
    void slotCompleted();
    void slotInfoMessage(const QString& msg);
    void slotErrorMessage(const QString& msg);
    void slotGrabActivation();
    void emitSelectionChangedSignal();
    void closeFilterBar();

    /**
     * Is invoked shortly before the contents of a view implementation
     * has been moved and emits the signal contentsMoved. Note that no
     * signal is emitted when the contents moving is only temporary by
     * e. g. reloading a directory.
     */
    void slotContentsMoving(int x, int y);

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void slotChangeNameFilter(const QString& nameFilter);

private:
    void startDirLister(const KUrl& url, bool reload = false);

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

    /**
     * Returns the string representation for the index \a index
     * for renaming \itemCount items.
     */
    QString renameIndexPresentation(int index, int itemCount) const;

    /**
     * Applies the current view mode m_mode to the
     * view implementation.
     */
    void applyModeToView();

    /**
     * Returns the column index used in the KDirModel depending on \a sorting.
     */
    int columnIndex(Sorting sorting) const;

    /**
     * Selects all items by using the selection flags \a flags. This is a helper
     * method for the slots DolphinView::selectAll() and DolphinView::invertSelection().
     */
    void selectAll(QItemSelectionModel::SelectionFlags flags);

private:
    bool m_refreshing;
    bool m_showProgress;
    Mode m_mode;

    int m_iconSize;
    int m_folderCount;
    int m_fileCount;

    DolphinMainWindow* m_mainWindow;
    QVBoxLayout* m_topLayout;
    UrlNavigator* m_urlNavigator;
    DolphinIconsView* m_iconsView;
    FilterBar *m_filterBar;
    DolphinStatusBar* m_statusBar;

    DolphinDirLister* m_dirLister;
    DolphinSortFilterProxyModel* m_proxyModel;

};

#endif // _DOLPHINVIEW_H_
