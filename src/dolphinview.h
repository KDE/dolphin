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

#include <kparts/part.h>
#include <kfileitem.h>
#include <kfileiconview.h>
#include <kio/job.h>

#include <urlnavigator.h>

#include <QDropEvent>
#include <QLinkedList>
#include <QListView>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWidget>

class DolphinController;
class FilterBar;
class KUrl;
class KDirModel;
class UrlNavigator;
class DolphinDetailsView;
class DolphinDirLister;
class DolphinIconsView;
class DolphinMainWindow;
class DolphinSortFilterProxyModel;
class DolphinStatusBar;
class QModelIndex;
class QPainter;
class QTimer;
class ViewProperties;

/**
 * @short Represents a view for the directory content
 *        including the navigation bar, filter bar and status bar.
 *
 * View modes for icons and details are supported. Currently
 * Dolphin allows to have up to two views inside the main window.
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 * @see UrlNavigator
 * @see DolphinStatusBar
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
        SortBySize,
        SortByDate,
        SortByPermissions,
        SortByOwner,
        SortByGroup,
        MaxSortEnum = SortByGroup
    };

    DolphinView(DolphinMainWindow* mainwindow,
                QWidget *parent,
                const KUrl& url,
                Mode mode = IconsView,
                bool showHiddenFiles = false);

    virtual ~DolphinView();

    /**
     * Sets the current active URL.
     * The signals UrlNavigator::urlChanged() and UrlNavigator::historyChanged()
     * are emitted.
     */
    void setUrl(const KUrl& url);

    /** Returns the current active URL. */
    const KUrl& url() const;

    /**
     * Returns true if the view is active and hence all actions are
     * applied to this view.
     */
    bool isActive() const;

    /**
     * Changes the view mode for the current directory to \a mode.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * changed view mode will be be stored automatically.
     */
    void setMode(Mode mode);
    Mode mode() const;

    /**
     * Turns on the file preview for the all files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * preview setting will be be stored automatically.
     */
    void setShowPreview(bool show);
    bool showPreview() const;

    /**
     * Shows all hidden files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * show hidden file setting will be be stored automatically.
     */
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

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
     * Goes back one step in the URL history. The signals
     * UrlNavigator::urlChanged() and UrlNavigator::historyChanged()
     * are submitted.
     */
    void goBack();

    /**
     * Goes forward one step in the Url history. The signals
     * UrlNavigator::urlChanged() and UrlNavigator::historyChanged()
     * are submitted.
     */
    void goForward();

    /**
     * Goes up one step of the Url path. The signals
     * UrlNavigator::urlChanged() and UrlNavigator::historyChanged()
     * are submitted.
     */
    void goUp();

    /**
     * Goes to the home URL. The signals UrlNavigator::urlChanged()
     * and UrlNavigator::historyChanged() are submitted.
     */
    void goHome();

    /**
     * Sets the URL of the navigation bar to an editable state
     * if \a editable is true. If \a editable is false, each part of
     * the location is presented by a button for a fast navigation.
     */
    void setUrlEditable(bool editable);

    /**
     * Returns the complete URL history. The index 0 indicates the oldest
     * history element.
     * @param index     Output parameter which indicates the current
     *                  index of the location.
     */
    const QLinkedList<UrlNavigator::HistoryElem> urlHistory(int& index) const;

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
     * Returns a list of URLs for all selected items. An empty list
     * is returned, if no item is selected.
     * @see DolphinView::selectedItems()
     */
    KUrl::List selectedUrls() const;

    /**
     * Returns the file item for the given model index \a index.
     */
    KFileItem* fileItem(const QModelIndex index) const;

    /**
     * Renames the filename of the source URL by the new file name.
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
     * Returns true, if the URL shown by the navigation bar is editable.
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

    /** Returns the UrlNavigator of the view for read access. */
    const UrlNavigator* urlNavigator() const { return m_urlNavigator; }

    /**
     * Triggers to request user information for the item given
     * by the URL \a url. The signal requestItemInfo is emitted,
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

    /** Reloads the current directory. */
    void reload();

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

    /**
     * Requests the main window to set this view as active view, which
     * means that all actions are applied to this view.
     */
    void requestActivation();

    /** Applies an item effect to all cut items of the clipboard. */
    void updateCutItems();

signals:
    /** Is emitted if URL of the view has been changed to \a url. */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if the view mode (IconsView, DetailsView,
     * PreviewsView) has been changed.
     */
    void modeChanged();

    /** Is emitted if the 'show preview' property has been changed. */
    void showPreviewChanged();

    /** Is emitted if the 'show hidden files' property has been changed. */
    void showHiddenFilesChanged();

    /** Is emitted if the sorting by name, size or date has been changed. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order (ascending or descending) has been changed. */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the sidebar.
     * It the URL is empty, no item information request is pending.
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
    void triggerItem(const QModelIndex& index);
    void updateProgress(int percent);

    /**
     * Updates the number of items (= number of directories + number of files)
     * and shows this information in the statusbar.
     */
    void updateItemCount();

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
    void showPreview(const KFileItem* item, const QPixmap& pixmap);

    /**
     * Restores the x- and y-position of the contents if the
     * current view is part of the history.
     */
    void restoreContentsPos();

    /** Shows the information \a msg inside the statusbar. */
    void showInfoMessage(const QString& msg);

    /** Shows the error message \a msg inside the statusbar. */
    void showErrorMessage(const QString& msg);

    void emitSelectionChangedSignal();
    void closeFilterBar();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void changeNameFilter(const QString& nameFilter);

    /**
     * Opens the context menu on position \a pos. The position
     * is used to check whether the context menu is related to an
     * item or to the viewport.
     */
    void openContextMenu(const QPoint& pos);

    /**
     * Drops the URLs \a urls to the index \a index. \a source
     * indicates the widget where the dragging has been started from.
     */
    void dropUrls(const KUrl::List& urls,
                  const QModelIndex& index,
                  QWidget* source);

    /**
     * Drops the URLs \a urls at the
     * destination \a destination.
     */
    void dropUrls(const KUrl::List& urls,
                  const KUrl& destination);
    /**
     * Updates the view properties of the current URL to the
     * sorting given by \a sorting.
     */
    void updateSorting(DolphinView::Sorting sorting);

    /**
     * Updates the view properties of the current URL to the
     * sort order given by \a order.
     */
    void updateSortOrder(Qt::SortOrder order);

    /**
     * Emits the signal contentsMoved with the current coordinates
     * of the viewport as parameters.
     */
    void emitContentsMoved();

    /**
     * Updates the activation state of the view by checking whether
     * the currently active view is this view.
     */
    void updateActivationState();

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
     * Creates a new view representing the given view mode (DolphinView::viewMode()).
     * The current view will get deleted.
     */
    void createView();

    /**
     * Selects all items by using the selection flags \a flags. This is a helper
     * method for the slots DolphinView::selectAll() and DolphinView::invertSelection().
     */
    void selectAll(QItemSelectionModel::SelectionFlags flags);

    /**
     * Returns a pointer to the currently used item view, which is either
     * a ListView or a TreeView.
     */
    QAbstractItemView* itemView() const;

    /**
     * Returns true if the index is valid and represents
     * the column KDirModel::Name.
     */
    bool isValidNameIndex(const QModelIndex& index) const;

    /**
     * Returns true, if the item \a item has been cut into
     * the clipboard.
     */
    bool isCutItem(const KFileItem& item) const;

private:
    bool m_showProgress;
    Mode m_mode;

    int m_iconSize;
    int m_folderCount;
    int m_fileCount;

    DolphinMainWindow* m_mainWindow;
    QVBoxLayout* m_topLayout;
    UrlNavigator* m_urlNavigator;

    DolphinController* m_controller;
    DolphinIconsView* m_iconsView;
    DolphinDetailsView* m_detailsView;

    FilterBar* m_filterBar;
    DolphinStatusBar* m_statusBar;

    KDirModel* m_dirModel;
    DolphinDirLister* m_dirLister;
    DolphinSortFilterProxyModel* m_proxyModel;
};

#endif // _DOLPHINVIEW_H_
