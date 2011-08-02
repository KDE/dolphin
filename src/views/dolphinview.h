/***************************************************************************
 *   Copyright (C) 2006-2009 by Peter Penz <peter.penz19@gmail.com>        *
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
#include <KFileItem>
#include <KFileItemDelegate>
#include <kio/fileundomanager.h>
#include <KIO/Job>

#include <QBoxLayout>
#include <QKeyEvent>
#include <QLinkedList>
#include <QListView>
#include <QSet>
#include <QWidget>

typedef KIO::FileUndoManager::CommandType CommandType;

class DolphinDirLister;
class DolphinItemListContainer;
class KAction;
class KActionCollection;
class KFileItemModel;
class KUrl;
class ViewProperties;
class QRegExp;

/**
 * @short Represents a view for the directory content.
 *
 * View modes for icons, compact and details are supported. It's
 * possible to adjust:
 * - sort order
 * - sort type
 * - show hidden files
 * - show previews
 * - enable grouping
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Defines the view mode for a directory. The
     * view mode is automatically updated if the directory itself
     * defines a view mode (see class ViewProperties for details).
     */
    enum Mode
    {
        /**
         * The items are shown as icons with a name-label below.
         */
        IconsView = 0,

        /**
         * The icon, the name and the size of the items are
         * shown per default as a table.
         */
        DetailsView,

        /**
         * The items are shown as icons with the name-label aligned
         * to the right side.
         */
        CompactView
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
        SortByDestination,
        SortByPath
    };

    /** Defines the additional information shown for the items of a directory. */
    enum AdditionalInfo
    {
        NoInfo = 0,
        NameInfo,
        SizeInfo,
        DateInfo,
        PermissionsInfo,
        OwnerInfo,
        GroupInfo,
        TypeInfo,
        DestinationInfo,
        PathInfo
    };

    /**
     * @param url              Specifies the content which should be shown.
     * @param parent           Parent widget of the view.
     */
    DolphinView(const KUrl& url, QWidget* parent);

    virtual ~DolphinView();

    /**
     * Returns the current active URL, where all actions are applied.
     * The URL navigator is synchronized with this URL.
     */
    KUrl url() const;

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

    /** See setPreviewsShown */
    bool previewsShown() const;

    /** See setShowHiddenFiles */
    bool hiddenFilesShown() const;

    /** See setCategorizedSorting */
    bool categorizedSorting() const;

    /**
     * Returns the items of the view.
     */
    KFileItemList items() const;

    /**
     * Returns the selected items. The list is empty if no item has been
     * selected.
     */
    KFileItemList selectedItems() const;

    /**
     * Returns the number of selected items (this is faster than
     * invoking selectedItems().count()).
     */
    int selectedItemsCount() const;

    /**
     * Marks the items indicated by \p urls to get selected after the
     * directory DolphinView::url() has been loaded. Note that nothing
     * gets selected if no loading of a directory has been triggered
     * by DolphinView::setUrl() or DolphinView::reload().
     */
    void markUrlsAsSelected(const QList<KUrl>& urls);

    /**
     * All items that match to the pattern \a pattern will get selected
     * if \a enabled is true and deselected if  \a enabled is false.
     */
    void setItemSelectionEnabled(const QRegExp& pattern, bool enabled);

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

    /** Sets the sorting criterion (e.g., SortByName, SortBySize,...) of the items inside a directory (see DolphinView::Sorting). */
    void setSorting(Sorting sorting);

    /** Returns the sorting criterion (e.g., SortByName, SortBySize,...) of the items inside a directory (see DolphinView::Sorting). */
    Sorting sorting() const;

    /** Sets the sort order (Qt::Ascending or Qt::Descending) for the items. */
    void setSortOrder(Qt::SortOrder order);

    /** Returns the currently used sort order (Qt::Ascending or Qt::Descending). */
    Qt::SortOrder sortOrder() const;

    /** Sets a separate sorting with folders first (true) or a mixed sorting of files and folders (false). */
    void setSortFoldersFirst(bool foldersFirst);

    /** Returns if files and folders are sorted separately or not. */
    bool sortFoldersFirst() const;

    /** Sets the additional information which should be shown for the items. */
    void setAdditionalInfoList(const QList<AdditionalInfo>& info);

    /** Returns the additional information which should be shown for the items. */
    QList<AdditionalInfo> additionalInfoList() const;

    /** Reloads the current directory. */
    void reload();

    void stopLoading();

    /**
     * Refreshes the view to get synchronized with the (updated) Dolphin settings.
     * This method only needs to get invoked if the view settings for the Icons View,
     * Details View or Columns View have been changed.
     */
    void refresh();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);
    QString nameFilter() const;

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
     * Returns the version control actions that are provided for the items \p items.
     * Usually the actions are presented in the context menu.
     */
    QList<QAction*> versionControlActions(const KFileItemList& items) const;

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
     * Returns true if the current view allows folders to be expanded,
     * i.e. presents a hierarchical view to the user.
     */
    bool itemsExpandable() const;

    /**
     * Restores the view state (current item, contents position, details view expansion state)
     */
    void restoreState(QDataStream& stream);

    /**
     * Saves the view state (current item, contents position, details view expansion state)
     */
    void saveState(QDataStream& stream);

    /** Returns true, if at least one item is selected. */
    bool hasSelection() const;

    /**
     * Returns the root item which represents the current URL. Note that the returned
     * item can be null (KFileItem::isNull() will return true) in case that the directory
     * has not been loaded.
     */
    KFileItem rootItem() const;

public slots:
    /**
     * Changes the directory to \a url. If the current directory is equal to
     * \a url, nothing will be done (use DolphinView::reload() instead).
     */
    void setUrl(const KUrl& url);

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

    void clearSelection();

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
     * the items as cut.
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
    void setPreviewsShown(bool show);

    /**
     * Shows all hidden files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * show hidden file setting will be stored automatically.
     */
    void setHiddenFilesShown(bool show);

    /**
     * Summarizes all sorted items by their category \a categorized
     * is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * categorized sorting setting will be stored automatically.
     */
    void setCategorizedSorting(bool categorized);

signals:
    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     */
    void activated();

    /**
     * Is emitted if the URL of the view will be changed to \a url.
     * After the URL has been changed the signal urlChanged() will
     * be emitted.
     */
    void urlAboutToBeChanged(const KUrl& url);

    /** Is emitted if the URL of the view has been changed to \a url. */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted when clicking on an item with the left mouse button.
     */
    void itemTriggered(const KFileItem& item);

    /**
     * Is emitted if items have been added or deleted.
     */
    void itemCountChanged();

    /**
     * Is emitted if a new tab should be opened for the URL \a url.
     */
    void tabRequested(const KUrl& url);

    /**
     * Is emitted if the view mode (IconsView, DetailsView,
     * PreviewsView) has been changed.
     */
    void modeChanged(Mode current, Mode previous);

    /** Is emitted if the 'show preview' property has been changed. */
    void previewsShownChanged(bool shown);

    /** Is emitted if the 'show hidden files' property has been changed. */
    void hiddenFilesShownChanged(bool shown);

    /** Is emitted if the 'categorized sorting' property has been changed. */
    void categorizedSortingChanged(bool sortCategorized);

    /** Is emitted if the sorting by name, size or date has been changed. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order (ascending or descending) has been changed. */
    void sortOrderChanged(Qt::SortOrder order);

    /** Is emitted if the sorting of files and folders (separate with folders first or mixed) has been changed. */
    void sortFoldersFirstChanged(bool foldersFirst);

    /** Is emitted if the additional information shown for this view has been changed. */
    void additionalInfoListChanged(const QList<DolphinView::AdditionalInfo>& current,
                                   const QList<DolphinView::AdditionalInfo>& previous);

    /** Is emitted if the zoom level has been changed by zooming in or out. */
    void zoomLevelChanged(int current, int previous);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the panel.
     * If item is null, no item information request is pending.
     */
    void requestItemInfo(const KFileItem& item);

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
     * Is emitted after the path triggered by DolphinView::setUrl()
     * has been loaded.
     */
    void finishedPathLoading(const KUrl& url);

    /**
     * Is emitted after DolphinView::setUrl() has been invoked and provides
     * the information how much percent of the current path have been loaded.
     */
    void pathLoadingProgress(int percent);

    /**
     * Is emitted if the DolphinView::setUrl() is invoked but the URL is not
     * a directory.
     */
    void urlIsFileError(const KUrl& file);

    /**
     * Emitted when KDirLister emits redirection.
     * Testcase: fish://localhost
     */
    void redirection(const KUrl& oldUrl, const KUrl& newUrl);

    /**
     * Is emitted when the write state of the folder has been changed. The application
     * should disable all actions like "Create New..." that depend on the write
     * state.
     */
    void writeStateChanged(bool isFolderWritable);

protected:
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
    /**
     * Marks the view as active (DolphinView:isActive() will return true)
     * and emits the 'activated' signal if it is not already active.
     */
    void activate();

    void slotItemClicked(int index, Qt::MouseButton button);

    void slotItemExpansionToggleClicked(int index);

    /**
     * Emits the signal \a selectionChanged() with a small delay. This is
     * because getting all file items for the signal can be an expensive
     * operation. Fast selection changes are collected in this case and
     * the signal is emitted only after no selection change has been done
     * within a small delay.
     */
    void slotSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

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
     * Invoked when the directory lister has been started the
     * loading of \a url.
     */
    void slotDirListerStarted(const KUrl& url);

    /**
     * Invoked when the directory lister has completed the loading of
     * items. Assures that pasted items and renamed items get seleced.
     */
    void slotDirListerCompleted();

    /**
     * Invoked when the loading of the directory is finished.
     * Restores the active item and the scroll position if possible.
     */
    void slotLoadingCompleted();

    /**
     * Is invoked when the KDirLister indicates refreshed items.
     */
    void slotRefreshItems();

    /**
     * Observes the item with the URL \a url. As soon as the directory
     * model indicates that the item is available, the item will
     * get selected and it is assured that the item stays visible.
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
     * Called when a redirection happens.
     * Testcase: fish://localhost
     */
    void slotRedirection(const KUrl& oldUrl, const KUrl& newUrl);

    /**
     * Restores the contents position, if history information about the old position is available.
     */
    void restoreContentsPosition();

    //void slotUrlChangeRequested(const KUrl& url);

private:
    KFileItemModel* fileItemModel() const;

    void loadDirectory(const KUrl& url, bool reload = false);

    /**
     * Applies the view properties which are defined by the current URL
     * to the DolphinView properties.
     */
    void applyViewProperties();

    void applyAdditionalInfoListToView();

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

    /**
     * Helper method for DolphinView::setItemSelectionEnabled(): Returns the selection for
     * all items of \p parent that match with the regular expression defined by \p pattern.
     */
    QItemSelection childrenMatchingPattern(const QModelIndex& parent, const QRegExp& pattern) const;

    /**
     * Updates m_isFolderWritable dependent on whether the folder represented by
     * the current URL is writable. If the state has changed, the signal
     * writeableStateChanged() will be emitted.
     */
    void updateWritableState();

    QByteArray sortRoleForSorting(Sorting sorting) const;

private:
    bool m_active : 1;
    bool m_tabsForFiles : 1;
    bool m_assureVisibleCurrentIndex : 1;
    bool m_expanderActive : 1;
    bool m_isFolderWritable : 1;

    KUrl m_url;
    Mode m_mode;
    QList<AdditionalInfo> m_additionalInfoList;

    QVBoxLayout* m_topLayout;

    DolphinDirLister* m_dirLister;
    DolphinItemListContainer* m_container;

    QTimer* m_selectionChangedTimer;

    KUrl m_activeItemUrl;
    QPoint m_restoredContentsPosition;
    KUrl m_createdItemUrl; // URL for a new item that got created by the "Create New..." menu
    KFileItemList m_selectedItems; // this is used for making the View to remember selections after F5

    /**
     * Remembers the filenames that have been added by a paste operation
     * or a drag & drop operation. Allows to select the items in
     * slotDirListerCompleted().
     */
    QSet<QString> m_newFileNames;

    // For unit tests
    friend class TestBase;
    friend class DolphinDetailsViewTest;
};

/// Allow using DolphinView::Mode in QVariant
Q_DECLARE_METATYPE(DolphinView::Mode)

#endif // DOLPHINVIEW_H
