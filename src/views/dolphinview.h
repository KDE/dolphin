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

#include "dolphintabwidget.h"
#include "dolphin_export.h"
#include "tooltips/tooltipmanager.h"

#include <KFileItem>
#include <KIO/Job>
#include <config-baloo.h>
#include <kio/fileundomanager.h>
#include <kparts/part.h>

#include <QMimeData>
#include <QUrl>
#include <QWidget>

typedef KIO::FileUndoManager::CommandType CommandType;
class QVBoxLayout;
class DolphinItemListView;
class KFileItemModel;
class KItemListContainer;
class KItemModelBase;
class KItemSet;
class ToolTipManager;
class VersionControlObserver;
class ViewProperties;
class QGraphicsSceneDragDropEvent;
class QRegularExpression;

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
class DOLPHIN_EXPORT DolphinView : public QWidget
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

    /**
     * @param url              Specifies the content which should be shown.
     * @param parent           Parent widget of the view.
     */
    DolphinView(const QUrl& url, QWidget* parent);

    ~DolphinView() override;

    /**
     * Returns the current active URL, where all actions are applied.
     * The URL navigator is synchronized with this URL.
     */
    QUrl url() const;

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

    /**
     * Turns on the file preview for the all files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * preview setting will be stored automatically.
     */
    void setPreviewsShown(bool show);
    bool previewsShown() const;

    /**
     * Shows all hidden files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * show hidden file setting will be stored automatically.
     */
    void setHiddenFilesShown(bool show);
    bool hiddenFilesShown() const;

    /**
     * Turns on sorting by groups if \a enable is true.
     */
    void setGroupedSorting(bool grouped);
    bool groupedSorting() const;

    /**
     * Returns the items of the view.
     */
    KFileItemList items() const;

    /**
     * @return The number of items. itemsCount() is faster in comparison
     *         to items().count().
     */
    int itemsCount() const;

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
    void markUrlsAsSelected(const QList<QUrl> &urls);

    /**
     * Marks the item indicated by \p url to be scrolled to and as the
     * current item after directory DolphinView::url() has been loaded.
     */
    void markUrlAsCurrent(const QUrl& url);

    /**
     * All items that match the regular expression \a regexp will get selected
     * if \a enabled is true and deselected if \a enabled is false.
     *
     * Note that to match the whole string the pattern should be anchored:
     * - you can anchor the pattern with QRegularExpression::anchoredPattern()
     * - if you use QRegularExpresssion::wildcardToRegularExpression(), don't use
     *   QRegularExpression::anchoredPattern() as the former already returns an
     *   anchored pattern
     */
    void selectItems(const QRegularExpression &regexp, bool enabled);

    /**
     * Sets the zoom level to \a level. It is assured that the used
     * level is adjusted to be inside the range ZoomLevelInfo::minimumLevel() and
     * ZoomLevelInfo::maximumLevel().
     */
    void setZoomLevel(int level);
    int zoomLevel() const;

    /**
     * Resets the view's icon size to the default value
     */
    void resetZoomLevel();

    void setSortRole(const QByteArray& role);
    QByteArray sortRole() const;

    void setSortOrder(Qt::SortOrder order);
    Qt::SortOrder sortOrder() const;

    /** Sets a separate sorting with folders first (true) or a mixed sorting of files and folders (false). */
    void setSortFoldersFirst(bool foldersFirst);
    bool sortFoldersFirst() const;

    /** Sets the additional information which should be shown for the items. */
    void setVisibleRoles(const QList<QByteArray>& roles);

    /** Returns the additional information which should be shown for the items. */
    QList<QByteArray> visibleRoles() const;

    void reload();

    /**
     * Refreshes the view to get synchronized with the settings (e.g. icons size,
     * font, ...).
     */
    void readSettings();

    /**
     * Saves the current settings (e.g. icons size, font, ..).
     */
    void writeSettings();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);
    QString nameFilter() const;

    /**
     * Filters the currently shown items by \a filters. All items
     * whose content-type matches those given by the list of filters
     * will be shown.
     */
    void setMimeTypeFilters(const QStringList& filters);
    QStringList mimeTypeFilters() const;

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

    /**
     * Returns the root item which represents the current URL.
     */
    KFileItem rootItem() const;

    /**
     * Sets a context that is used for remembering the view-properties.
     * Per default the context is empty and the path of the currently set URL
     * is used for remembering the view-properties. Setting a custom context
     * makes sense if specific types of URLs (e.g. search-URLs) should
     * share common view-properties.
     */
    void setViewPropertiesContext(const QString& context);
    QString viewPropertiesContext() const;

    /**
     * Checks if the given \a item can be opened as folder (e.g. archives).
     * This function will also adjust the \a url (e.g. change the protocol).
     * @return a valid and adjusted url if the item can be opened as folder,
     * otherwise return an empty url.
     */
    static QUrl openItemAsFolderUrl(const KFileItem& item, const bool browseThroughArchives = true);

    /**
     * Hides tooltip displayed over element.
     */
    void hideToolTip(const ToolTipManager::HideBehavior behavior = ToolTipManager::HideBehavior::Later);

public slots:
    /**
     * Changes the directory to \a url. If the current directory is equal to
     * \a url, nothing will be done (use DolphinView::reload() instead).
     */
    void setUrl(const QUrl& url);

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
    void cutSelectedItemsToClipboard();

    /** Copies all selected items to the clipboard. */
    void copySelectedItemsToClipboard();

    /**
     * Copies all selected items to @p destinationUrl.
     */
    void copySelectedItems(const KFileItemList &selection, const QUrl &destinationUrl);

    /**
     * Moves all selected items to @p destinationUrl.
     */
    void moveSelectedItems(const KFileItemList &selection, const QUrl &destinationUrl);

    /** Pastes the clipboard data to this view. */
    void paste();

    /**
     * Pastes the clipboard data into the currently selected
     * folder. If the current selection is not exactly one folder, no
     * paste operation is done.
     */
    void pasteIntoFolder();

    /**
     * Copies the path of the first selected KFileItem into Clipboard.
     */
    void copyPathToClipboard();

    /**
     * Creates duplicates of selected items, appending "copy"
     * to the end.
     */
    void duplicateSelectedItems();

    /**
     * Handles a drop of @p dropEvent onto widget @p dropWidget and destination @p destUrl
     */
    void dropUrls(const QUrl &destUrl, QDropEvent *dropEvent, QWidget *dropWidget);

    void stopLoading();

    /**
     * Applies the state that has been restored by restoreViewState()
     * to the view.
     */
    void updateViewState();

    /** Activates the view if the item list container gets focus. */
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     */
    void activated();

    /** Is emitted if the URL of the view has been changed to \a url. */
    void urlChanged(const QUrl& url);

    /**
     * Is emitted when clicking on an item with the left mouse button.
     */
    void itemActivated(const KFileItem& item);

    /**
     * Is emitted when multiple items have been activated by e. g.
     * context menu open with.
     */
    void itemsActivated(const KFileItemList& items);

    /**
     * Is emitted if items have been added or deleted.
     */
    void itemCountChanged();

    /**
     * Is emitted if a new tab should be opened for the URL \a url.
     */
    void tabRequested(const QUrl& url, DolphinTabWidget::TabPlacement tabPlacement);

    /**
     * Is emitted if the view mode (IconsView, DetailsView,
     * PreviewsView) has been changed.
     */
    void modeChanged(DolphinView::Mode current, DolphinView::Mode previous);

    /** Is emitted if the 'show preview' property has been changed. */
    void previewsShownChanged(bool shown);

    /** Is emitted if the 'show hidden files' property has been changed. */
    void hiddenFilesShownChanged(bool shown);

    /** Is emitted if the 'grouped sorting' property has been changed. */
    void groupedSortingChanged(bool groupedSorting);

    /** Is emitted if the sorting by name, size or date has been changed. */
    void sortRoleChanged(const QByteArray& role);

    /** Is emitted if the sort order (ascending or descending) has been changed. */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if the sorting of files and folders (separate with folders
     * first or mixed) has been changed.
     */
    void sortFoldersFirstChanged(bool foldersFirst);

    /** Is emitted if the additional information shown for this view has been changed. */
    void visibleRolesChanged(const QList<QByteArray>& current,
                             const QList<QByteArray>& previous);

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
    void requestContextMenu(const QPoint& pos,
                            const KFileItem& item,
                            const QUrl& url,
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
     * the current directory is loaded. If this signal is emitted,
     * it is assured that the view contains already the correct root
     * URL and property settings.
     */
    void directoryLoadingStarted();

    /**
     * Is emitted after the directory triggered by DolphinView::setUrl()
     * has been loaded.
     */
    void directoryLoadingCompleted();

    /**
     * Is emitted after the directory loading triggered by DolphinView::setUrl()
     * has been canceled.
     */
    void directoryLoadingCanceled();

    /**
     * Is emitted after DolphinView::setUrl() has been invoked and provides
     * the information how much percent of the current directory have been loaded.
     */
    void directoryLoadingProgress(int percent);

    /**
     * Is emitted if the sorting is done asynchronously and provides the
     * progress information of the sorting.
     */
    void directorySortingProgress(int percent);

    /**
     * Emitted when the file-item-model emits redirection.
     * Testcase: fish://localhost
     */
    void redirection(const QUrl& oldUrl, const QUrl& newUrl);

    /**
     * Is emitted when the URL set by DolphinView::setUrl() represents a file.
     * In this case no signal errorMessage() will be emitted.
     */
    void urlIsFileError(const QUrl& url);

    /**
     * Is emitted when the write state of the folder has been changed. The application
     * should disable all actions like "Create New..." that depend on the write
     * state.
     */
    void writeStateChanged(bool isFolderWritable);

    /**
     * Is emitted if the URL should be changed to the previous URL of the
     * history (e.g. because the "back"-mousebutton has been pressed).
     */
    void goBackRequested();

    /**
     * Is emitted if the URL should be changed to the next URL of the
     * history (e.g. because the "next"-mousebutton has been pressed).
     */
    void goForwardRequested();

    /**
     * Is emitted when the user wants to move the focus to another view.
     */
    void toggleActiveViewRequested();

    /**
     * Is emitted when the user clicks a tag or a link
     * in the metadata widget of a tooltip.
     */
    void urlActivated(const QUrl& url);

protected:
    /** Changes the zoom level if Control is pressed during a wheel event. */
    void wheelEvent(QWheelEvent* event) override;

    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    /**
     * Marks the view as active (DolphinView:isActive() will return true)
     * and emits the 'activated' signal if it is not already active.
     */
    void activate();

    void slotItemActivated(int index);
    void slotItemsActivated(const KItemSet& indexes);
    void slotItemMiddleClicked(int index);
    void slotItemContextMenuRequested(int index, const QPointF& pos);
    void slotViewContextMenuRequested(const QPointF& pos);
    void slotHeaderContextMenuRequested(const QPointF& pos);
    void slotHeaderColumnWidthChangeFinished(const QByteArray& role, qreal current);
    void slotItemHovered(int index);
    void slotItemUnhovered(int index);
    void slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);
    void slotModelChanged(KItemModelBase* current, KItemModelBase* previous);
    void slotMouseButtonPressed(int itemIndex, Qt::MouseButtons buttons);
    void slotRenameDialogRenamingFinished(const QList<QUrl>& urls);
    void slotSelectedItemTextPressed(int index);
    void slotCopyingDone(KIO::Job *, const QUrl &, const QUrl &to);

    /*
     * Is called when new items get pasted or dropped.
     */
    void slotItemCreated(const QUrl &url);
    /*
     * Is called after all pasted or dropped items have been copied to destination.
     */
    void slotJobResult(KJob *job);

    /**
     * Emits the signal \a selectionChanged() with a small delay. This is
     * because getting all file items for the selection can be an expensive
     * operation. Fast selection changes are collected in this case and
     * the signal is emitted only after no selection change has been done
     * within a small delay.
     */
    void slotSelectionChanged(const KItemSet& current, const KItemSet& previous);

    /**
     * Is called by emitDelayedSelectionChangedSignal() and emits the
     * signal \a selectionChanged() with all selected file items as parameter.
     */
    void emitSelectionChangedSignal();

    /**
     * Updates the view properties of the current URL to the
     * sorting given by \a role.
     */
    void updateSortRole(const QByteArray& role);

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
     * Indicates in the status bar that the delete operation
     * of the job \a job has been finished.
     */
    void slotDeleteFileFinished(KJob* job);

    /**
     * Indicates in the status bar that the trash operation
     * of the job \a job has been finished.
     */
    void slotTrashFileFinished(KJob* job);

    /**
     * Invoked when the rename job is done, for error handling.
     */
    void slotRenamingResult(KJob* job);

    /**
     * Invoked when the file item model has started the loading
     * of the directory specified by DolphinView::url().
     */
    void slotDirectoryLoadingStarted();

    /**
     * Invoked when the file item model indicates that the loading of a directory has
     * been completed. Assures that pasted items and renamed items get selected.
     */
    void slotDirectoryLoadingCompleted();

    /**
     * Is invoked when items of KFileItemModel have been changed.
     */
    void slotItemsChanged();

    /**
     * Is invoked when the sort order has been changed by the user by clicking
     * on a header item. The view properties of the directory will get updated.
     */
    void slotSortOrderChangedByHeader(Qt::SortOrder current, Qt::SortOrder previous);

    /**
     * Is invoked when the sort role has been changed by the user by clicking
     * on a header item. The view properties of the directory will get updated.
     */
    void slotSortRoleChangedByHeader(const QByteArray& current, const QByteArray& previous);

    /**
     * Is invoked when the visible roles have been changed by the user by dragging
     * a header item. The view properties of the directory will get updated.
     */
    void slotVisibleRolesChangedByHeader(const QList<QByteArray>& current,
                                         const QList<QByteArray>& previous);

    void slotRoleEditingCanceled();
    void slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value);

    /**
     * Observes the item with the URL \a url. As soon as the directory
     * model indicates that the item is available, the item will
     * get selected and it is assured that the item stays visible.
     */
    void observeCreatedItem(const QUrl &url);

    /**
     * Called when a redirection happens.
     * Testcase: fish://localhost
     */
    void slotDirectoryRedirection(const QUrl& oldUrl, const QUrl& newUrl);

    /**
     * Calculates the number of currently shown files into
     * \a fileCount and the number of folders into \a folderCount.
     * The size of all files is written into \a totalFileSize.
     * It is recommend using this method instead of asking the
     * directory lister or the model directly, as it takes
     * filtering and hierarchical previews into account.
     */
    void calculateItemCount(int& fileCount, int& folderCount, KIO::filesize_t& totalFileSize) const;

    void slotTwoClicksRenamingTimerTimeout();

private:
    void loadDirectory(const QUrl& url, bool reload = false);

    /**
     * Applies the view properties which are defined by the current URL
     * to the DolphinView properties. The view properties are read from a
     * .directory file either in the current directory, or in the
     * share/apps/dolphin/view_properties/ subfolder of the user's .kde folder.
     */
    void applyViewProperties();

    /**
     * Applies the given view properties to the DolphinView.
     */
    void applyViewProperties(const ViewProperties& props);

    /**
     * Applies the m_mode property to the corresponding
     * itemlayout-property of the KItemListView.
     */
    void applyModeToView();

    /**
     * Helper method for DolphinView::paste() and DolphinView::pasteIntoFolder().
     * Pastes the clipboard data into the URL \a url.
     */
    void pasteToUrl(const QUrl& url);

    /**
     * Returns a list of URLs for all selected items. The list is
     * simplified, so that when the URLs are part of different tree
     * levels, only the parent is returned.
     */
    QList<QUrl> simplifiedSelectedUrls() const;

    /**
     * Returns the MIME data for all selected items.
     */
    QMimeData* selectionMimeData() const;

    /**
     * Updates m_isFolderWritable dependent on whether the folder represented by
     * the current URL is writable. If the state has changed, the signal
     * writeableStateChanged() will be emitted.
     */
    void updateWritableState();

    /**
     * @return The current URL if no viewproperties-context is given (see
     *         DolphinView::viewPropertiesContext(), otherwise the context
     *         is returned.
     */
    QUrl viewPropertiesUrl() const;

    /**
     * Clears the selection and updates current item and selection according to the parameters
     *
     * @param current URL to be set as current
     * @param selected list of selected items
     */
    void forceUrlsSelection(const QUrl& current, const QList<QUrl>& selected);

    void abortTwoClicksRenaming();

private:
    void updatePalette();

    bool m_active;
    bool m_tabsForFiles;
    bool m_assureVisibleCurrentIndex;
    bool m_isFolderWritable;
    bool m_dragging; // True if a dragging is done. Required to be able to decide whether a
                     // tooltip may be shown when hovering an item.

    QUrl m_url;
    QString m_viewPropertiesContext;
    Mode m_mode;
    QList<QByteArray> m_visibleRoles;

    QVBoxLayout* m_topLayout;

    KFileItemModel* m_model;
    DolphinItemListView* m_view;
    KItemListContainer* m_container;

    ToolTipManager* m_toolTipManager;

    QTimer* m_selectionChangedTimer;

    QUrl m_currentItemUrl; // Used for making the view to remember the current URL after F5
    bool m_scrollToCurrentItem; // Used for marking we need to scroll to current item or not
    QPoint m_restoredContentsPosition;

    QList<QUrl> m_selectedUrls; // Used for making the view to remember selections after F5
    bool m_clearSelectionBeforeSelectingNewItems;
    bool m_markFirstNewlySelectedItemAsCurrent;

    VersionControlObserver* m_versionControlObserver;

    QTimer* m_twoClicksRenamingTimer;
    QUrl m_twoClicksRenamingItemUrl;

    // For unit tests
    friend class TestBase;
    friend class DolphinDetailsViewTest;
    friend class DolphinPart;                   // Accesses m_model
};

/// Allow using DolphinView::Mode in QVariant
Q_DECLARE_METATYPE(DolphinView::Mode)

#endif // DOLPHINVIEW_H
