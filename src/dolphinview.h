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


#ifndef DOLPHINVIEW_H
#define DOLPHINVIEW_H

#include <config-nepomuk.h>

#include "libdolphin_export.h"

#include <kparts/part.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>
#include <kio/fileundomanager.h>
#include <kio/job.h>

#include <QBoxLayout>
#include <QKeyEvent>
#include <QLinkedList>
#include <QListView>
#include <QSet>
#include <QWidget>

typedef KIO::FileUndoManager::CommandType CommandType;

class DolphinController;
class DolphinColumnView;
class DolphinDetailsView;
class DolphinFileItemDelegate;
class DolphinIconsView;
class DolphinMainWindow;
class DolphinModel;
class DolphinSortFilterProxyModel;
class KFilePreviewGenerator;
class KAction;
class KActionCollection;
class KDirLister;
class KUrl;
class KToggleAction;
class ToolTipManager;
class QModelIndex;
class ViewProperties;

/**
 * @short Represents a view for the directory content.
 *
 * View modes for icons, details and columns are supported. It's
 * possible to adjust:
 * - sort order
 * - sort type
 * - show hidden files
 * - show previews
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 * @see DolphinColumnView
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinView : public QWidget
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
         * icon name.
         */
        IconsView = 0,

        /**
         * The icon, the name and at least the size of the directory
         * items are shown in a table. It is possible to add columns
         * for date, group and permissions.
         */
        DetailsView = 1,

        /**
         * Each folder is shown in a separate column.
         */
        ColumnView = 2,
        MaxModeEnum = ColumnView
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
        SortByType,
        SortByRating,
        SortByTags,
        MaxSortEnum = SortByTags
    };

    /**
     * @param parent           Parent widget of the view.
     * @param url              Specifies the content which should be shown.
     * @param dirLister        Used directory lister. The lister is not owned
     *                         by the view and won't get deleted.
     * @param dolphinModel     Used directory model. The model is not owned
     *                         by the view and won't get deleted.
     * @param proxyModel       Used proxy model which specifies the sorting. The
     *                         model is not owned by the view and won't get
     *                         deleted.
     */
    DolphinView(QWidget* parent,
                const KUrl& url,
                KDirLister* dirLister,
                DolphinModel* dolphinModel,
                DolphinSortFilterProxyModel* proxyModel);

    virtual ~DolphinView();

    /**
     * Returns the current active URL, where all actions are applied.
     * The URL navigator is synchronized with this URL.
     */
    const KUrl& url() const;

    /**
     * Returns the root URL of the view, which is defined as the first
     * visible path of DolphinView::url(). Usually the root URL is
     * equal to DolphinView::url(), but in the case of the column view
     * when 2 columns are shown, the root URL might be:
     * /home/peter/Documents
     * and DolphinView::url() might return
     * /home/peter/Documents/Music/
     */
    KUrl rootUrl() const;

    /**
     * If \a active is true, the view will marked as active. The active
     * view is defined as view where all actions are applied to.
     */
    void setActive(bool active);
    bool isActive() const;

    /**
     * Changes the view mode for the current directory to \a mode.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * changed view mode will be stored automatically.
     */
    void setMode(Mode mode);
    Mode mode() const;

    /** See setShowPreview */
    bool showPreview() const;

    /** See setShowHiddenFiles */
    bool showHiddenFiles() const;

    /** See setCategorizedSorting */
    bool categorizedSorting() const;

    /**
     * Returns true, if the categorized sorting is supported by the current
     * used mode (see DolphinView::setMode()). Currently only DolphinView::IconsView
     * supports categorizations. To check whether the categorized
     * sorting is set, use DolphinView::categorizedSorting().
     */
    bool supportsCategorizedSorting() const;

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

    /** Returns true, if at least one item is selected. */
    bool hasSelection() const;

    void clearSelection();

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
     * Returns the number of selected items (this is faster than
     * invoking selectedItems().count()).
     */
    int selectedItemsCount() const;

    /**
     * Sets the upper left position of the view content
     * to (x,y). The content of the view might be larger than the visible area
     * and hence a scrolling must be done.
     */
    void setContentsPosition(int x, int y);

    /** Returns the upper left position of the view content. */
    QPoint contentsPosition() const;

    /**
     * Sets the zoom level to \a level. It is assured that the used
     * level is adjusted to be inside the range ZoomLevelInfo::minimumLevel() and
     * ZoomLevelInfo::maximumLevel().
     */
    void setZoomLevel(int level);
    int zoomLevel() const;

    /**
     * Returns true, if zooming in is possible. If false is returned,
     * the maximum zooming level has been reached.
     */
    bool isZoomInPossible() const;

    /**
     * Returns true, if zooming out is possible. If false is returned,
     * the minimum zooming level has been reached.
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

    /** Sets a separate sorting with folders first (true) or a mixed sorting of files and folders (false). */
    void setSortFoldersFirst(bool foldersFirst);

    /** Returns if files and folders are sorted separately or not. */
    bool sortFoldersFirst() const;

    /** Sets the additional information which should be shown for the items. */
    void setAdditionalInfo(KFileItemDelegate::InformationList info);

    /** Returns the additional information which should be shown for the items. */
    KFileItemDelegate::InformationList additionalInfo() const;

    /** Reloads the current directory. */
    void reload();

    /**
     * Refreshes the view to get synchronized with the (updated) Dolphin settings.
     * This method only needs to get invoked if the view settings for the Icons View,
     * Details View or Columns View have been changed.
     */
    void refresh();

    /**
     * Changes the directory of the view to \a url. If \a rootUrl is empty, the view
     * properties from \a url are used for adjusting the view mode and the other properties.
     * If \a rootUrl is not empty, the view properties from the root URL are considered
     * instead. Specifying a root URL is only required if a view having a different root URL
     * (e. g. the column view) should be restored. Usually using DolphinView::setUrl()
     * is enough for changing the current URL.
     */
    void updateView(const KUrl& url, const KUrl& rootUrl);

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);

    /**
     * Calculates the number of currently shown files into
     * \a fileCount and the number of folders into \a folderCount.
     * The size of all files is written into \a totalFileSize.
     * It is recommend using this method instead of asking the
     * directory lister or the model directly, as it takes
     * filtering and hierarchical previews into account.
     */
    void calculateItemCount(int& fileCount, int& folderCount, KIO::filesize_t& totalFileSize) const;

    /**
     * Returns a textual representation of the state of the current
     * folder or selected items, suitable for use in the status bar.
     */
    QString statusBarText() const;

    /**
     * Updates the state of the 'Additional Information' actions in \a collection.
     */
    void updateAdditionalInfoActions(KActionCollection* collection);

    /**
     * Returns the state of the paste action:
     * first is whether the action should be enabled
     * second is the text for the action
     */
    QPair<bool, QString> pasteInfo() const;

    /**
     * If \a tabsForFiles is true, the signal tabRequested() will also
     * emitted also for files. Per default tabs for files is disabled
     * and hence the signal tabRequested() will only be emitted for
     * directories.
     */
    void setTabsForFilesEnabled(bool tabsForFiles);
    bool isTabsForFilesEnabled() const;

    /**
     * Marks the item \a url as active item as soon as it has
     * been loaded by the directory lister. This is useful mark
     * the previously visited directory as active when going
     * back in history (the URL is known, but the item is not
     * loaded yet).
     */
    void activateItem(const KUrl& url);

    /**
     * Returns true if the current view allows folders to be expanded,
     * i.e. presents a hierarchical view to the user.
     */
    bool itemsExpandable() const;

public slots:
    /**
     * Changes the directory to \a url. If the current directory is equal to
     * \a url, nothing will be done (use DolphinView::reload() instead).
     */
    void setUrl(const KUrl& url);

    /**
     * Request of a selection change. The view will do its best to accommodate
     * the request, but it is not guaranteed that all items in \a selection
     * will actually get selected. The view will e.g. not select items which
     * are not in the currently displayed folder.
     */
    void changeSelection(const KFileItemList& selection);

    /**
     * Triggers the renaming of the currently selected items, where
     * the user must input a new name for the items.
     */
    void renameSelectedItems();

    /**
     * Moves all selected items to the trash.
     */
    void trashSelectedItems();

    /**
     * Deletes all selected items.
     */
    void deleteSelectedItems();

    /**
     * Copies all selected items to the clipboard and marks
     * the items as cutted.
     */
    void cutSelectedItems();

    /** Copies all selected items to the clipboard. */
    void copySelectedItems();

    /** Pastes the clipboard data to this view. */
    void paste();

    /**
     * Pastes the clipboard data into the currently selected
     * folder. If the current selection is not exactly one folder, no
     * paste operation is done.
     */
    void pasteIntoFolder();

    /**
     * Turns on the file preview for the all files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * preview setting will be stored automatically.
     */
    void setShowPreview(bool show);

    /**
     * Shows all hidden files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * show hidden file setting will be stored automatically.
     */
    void setShowHiddenFiles(bool show);

    /**
     * Summarizes all sorted items by their category \a categorized
     * is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * categorized sorting setting will be stored automatically.
     */
    void setCategorizedSorting(bool categorized);

    /** Switches between an ascending and descending sorting order. */
    void toggleSortOrder();

    /** Switches between a separate sorting (with folders first) and a mixed sorting of files and folders. */
    void toggleSortFoldersFirst();

    /**
     * Switches on or off the displaying of additional information
     * as specified by \a action.
     */
    void toggleAdditionalInfo(QAction* action);

signals:
    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     */
    void activated();

    /** Is emitted if URL of the view has been changed to \a url. */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if the view requests a changing of the current
     * URL to \a url (see DolphinController::triggerUrlChangeRequest()).
     */
    void requestUrlChange(const KUrl& url);

    /**
     * Is emitted when clicking on an item with the left mouse button.
     */
    void itemTriggered(const KFileItem& item);

    /**
     * Is emitted if a new tab should be opened for the URL \a url.
     */
    void tabRequested(const KUrl& url);

    /**
     * Is emitted if the view mode (IconsView, DetailsView,
     * PreviewsView) has been changed.
     */
    void modeChanged();

    /** Is emitted if the 'show preview' property has been changed. */
    void showPreviewChanged();

    /** Is emitted if the 'show hidden files' property has been changed. */
    void showHiddenFilesChanged();

    /** Is emitted if the 'categorized sorting' property has been changed. */
    void categorizedSortingChanged();

    /** Is emitted if the sorting by name, size or date has been changed. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order (ascending or descending) has been changed. */
    void sortOrderChanged(Qt::SortOrder order);

    /** Is emitted if the sorting of files and folders (separate with folders first or mixed) has been changed. */
    void sortFoldersFirstChanged(bool foldersFirst);

    /** Is emitted if the additional information shown for this view has been changed. */
    void additionalInfoChanged();

    /** Is emitted if the zoom level has been changed by zooming in or out. */
    void zoomLevelChanged(int level);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the panel.
     * If item is null, no item information request is pending.
     */
    void requestItemInfo(const KFileItem& item);

    /** Is emitted if the contents has been moved to \a x, \a y. */
    void contentsMoved(int x, int y);

    /**
     * Is emitted whenever the selection has been changed.
     */
    void selectionChanged(const KFileItemList& selection);

    /**
     * Is emitted if a context menu is requested for the item \a item,
     * which is part of \a url. If the item is null, the context menu
     * for the URL should be shown and the custom actions \a customActions
     * will be added.
     */
    void requestContextMenu(const KFileItem& item,
                            const KUrl& url,
                            const QList<QAction*>& customActions);

    /**
     * Is emitted if an information message with the content \a msg
     * should be shown.
     */
    void infoMessage(const QString& msg);

    /**
     * Is emitted if an error message with the content \a msg
     * should be shown.
     */
    void errorMessage(const QString& msg);

    /**
     * Is emitted if an "operation completed" message with the content \a msg
     * should be shown.
     */
    void operationCompletedMessage(const QString& msg);

    /**
     * Is emitted after DolphinView::setUrl() has been invoked and
     * the path \a url is currently loaded. If this signal is emitted,
     * it is assured that the view contains already the correct root
     * URL and property settings.
     */
    void startedPathLoading(const KUrl& url);

    /**
     * Emitted when KDirLister emits redirection.
     * Testcase: fish://localhost
     */
    void redirection(const KUrl& oldUrl, const KUrl& newUrl);

protected:
    /** @see QWidget::mouseReleaseEvent */
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    /**
     * Marks the view as active (DolphinView:isActive() will return true)
     * and emits the 'activated' signal if it is not already active.
     */
    void activate();

    /**
     * If the item \a item is a directory, then this
     * directory will be loaded. If the  item is a file, the corresponding
     * application will get started.
     */
    void triggerItem(const KFileItem& index);

    /**
     * Emits the signal \a selectionChanged() with a small delay. This is
     * because getting all file items for the signal can be an expensive
     * operation. Fast selection changes are collected in this case and
     * the signal is emitted only after no selection change has been done
     * within a small delay.
     */
    void emitDelayedSelectionChangedSignal();

    /**
     * Is called by emitDelayedSelectionChangedSignal() and emits the
     * signal \a selectionChanged() with all selected file items as parameter.
     */
    void emitSelectionChangedSignal();

    /**
     * Opens the context menu on position \a pos. The position
     * is used to check whether the context menu is related to an
     * item or to the viewport.
     */
    void openContextMenu(const QPoint& pos, const QList<QAction*>& customActions);

    /**
     * Drops dragged URLs to the destination path \a destPath. If
     * the URLs are dropped above an item inside the destination path,
     * the item is indicated by \a destItem.
     */
    void dropUrls(const KFileItem& destItem,
                  const KUrl& destPath,
                  QDropEvent* event);

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
     * Updates the view properties of the current URL to the
     * sorting of files and folders (separate with folders first or mixed) given by \a foldersFirst.
     */
    void updateSortFoldersFirst(bool foldersFirst);

    /**
     * Updates the view properties of the current URL to the
     * additional information given by \a info.
     */
    void updateAdditionalInfo(const KFileItemDelegate::InformationList& info);

    /**
     * Emits the signal contentsMoved with the current coordinates
     * of the viewport as parameters.
     */
    void emitContentsMoved();

    /**
     * Updates the status bar to show hover information for the
     * item \a item. If currently other items are selected,
     * no hover information is shown.
     * @see DolphinView::clearHoverInformation()
     */
    void showHoverInformation(const KFileItem& item);

    /**
     * Clears the hover information shown in the status bar.
     * @see DolphinView::showHoverInformation().
     */
    void clearHoverInformation();

    /**
     * Indicates in the status bar that the delete operation
     * of the job \a job has been finished.
     */
    void slotDeleteFileFinished(KJob* job);

    /**
     * Is emitted if the controller requests a changing of the current
     * URL to \a url
     */
    void slotRequestUrlChange(const KUrl& url);

    /**
     * Invoked when the directory lister has completed the loading of
     * items. Assures that pasted items and renamed items get seleced.
     */
    void slotDirListerCompleted();

    /**
     * Is invoked when the KDirLister indicates refreshed items.
     */
    void slotRefreshItems();

    /**
     * If \a view can be positively identified as not being the source for the
     * current drag operation, deleteLater() it immediately.  Else stores
     * it for later deletion.
     */
    void deleteWhenNotDragSource(QAbstractItemView* view);

    /**
     * Observes the item with the URL \a url. As soon as the directory
     * model indicates that the item is available, the item will
     * get selected and it is assure that the item stays visible.
     *
     * @see selectAndScrollToCreatedItem()
     */
    void observeCreatedItem(const KUrl& url);

    /**
     * Selects and scrolls to the item that got observed
     * by observeCreatedItem().
     */
    void selectAndScrollToCreatedItem();

    /**
     * Restore selection after view refresh.
     */
    void restoreSelection();

    /**
     * Called when a redirection happens.
     * Testcase: fish://localhost
     */
    void slotRedirection(const KUrl& oldUrl, const KUrl& newUrl);

private:
    void loadDirectory(const KUrl& url, bool reload = false);

    /**
     * Returns the URL where the view properties should be stored. Usually
     * DolphinView::url() is returned, but in the case of a Column View the
     * view properties are always stored in the directory represented by the
     * first column. It is recommendend whenever using the ViewProperties class
     * to use DolphinView::viewPropertiesUrl() as URL.
     */
    KUrl viewPropertiesUrl() const;

    /**
     * Applies the view properties which are defined by the current URL
     * m_url to the DolphinView properties.
     */
    void applyViewProperties(const KUrl& url);

    /**
     * Creates a new view representing the given view mode (DolphinView::mode()).
     * The current view will get deleted.
     */
    void createView();

    void deleteView();

    /**
     * Returns a pointer to the currently used item view, which is either
     * a ListView or a TreeView.
     */
    QAbstractItemView* itemView() const;

    /**
     * Helper method for DolphinView::paste() and DolphinView::pasteIntoFolder().
     * Pastes the clipboard data into the URL \a url.
     */
    void pasteToUrl(const KUrl& url);

    /**
     * Checks whether the current item view has the same zoom level
     * as \a oldZoomLevel. If this is not the case, the zoom level
     * of the controller is updated and a zoomLevelChanged() signal
     * is emitted.
     */
    void updateZoomLevel(int oldZoomLevel);

    /**
     * Returns a list of URLs for all selected items. The list is
     * simplified, so that when the URLs are part of different tree
     * levels, only the parent is returned.
     */
    KUrl::List simplifiedSelectedUrls() const;

    /**
     * Returns true, if the ColumnView is activated. As the column view
     * requires some special handling for iterating through directories,
     * this method has been introduced for convenience.
     */
    bool isColumnViewActive() const;

    /**
     * Returns the MIME data for all selected items.
     */
    QMimeData* selectionMimeData() const;

    /**
     * Is invoked after a paste operation or a drag & drop
     * operation and adds the filenames of all URLs from \a mimeData to
     * m_newFileNames. This allows to select all newly added
     * items in slotDirListerCompleted().
     */
    void addNewFileNames(const QMimeData* mimeData);

private:
    bool m_active : 1;
    bool m_showPreview : 1;
    bool m_loadingDirectory : 1;
    bool m_storedCategorizedSorting : 1;
    bool m_tabsForFiles : 1;
    bool m_isContextMenuOpen : 1;   // TODO: workaround for Qt-issue 207192
    bool m_ignoreViewProperties : 1;
    bool m_assureVisibleCurrentIndex : 1;

    Mode m_mode;

    DolphinMainWindow* m_mainWindow;
    QVBoxLayout* m_topLayout;

    DolphinController* m_controller;
    DolphinIconsView* m_iconsView;
    DolphinDetailsView* m_detailsView;
    DolphinColumnView* m_columnView;
    DolphinFileItemDelegate* m_fileItemDelegate;

    QItemSelectionModel* m_selectionModel;
    QTimer* m_selectionChangedTimer;

    DolphinModel* m_dolphinModel;
    KDirLister* m_dirLister;
    DolphinSortFilterProxyModel* m_proxyModel;

    KFilePreviewGenerator* m_previewGenerator;
    ToolTipManager* m_toolTipManager;

    KUrl m_rootUrl;
    KUrl m_activeItemUrl;
    KUrl m_createdItemUrl; // URL for a new item that got created by the "Create New..." menu
    KFileItemList m_selectedItems; // this is used for making the View to remember selections after F5

    /**
     * Remembers the filenames that have been added by a paste operation
     * or a drag & drop operation. Allows to select the items in
     * slotDirListerCompleted().
     */
    QSet<QString> m_newFileNames;

    QAbstractItemView* m_expandedDragSource;
};

inline bool DolphinView::isColumnViewActive() const
{
    return m_columnView != 0;
}

/// Allow using DolphinView::Mode in QVariant
Q_DECLARE_METATYPE(DolphinView::Mode)

#endif // DOLPHINVIEW_H
