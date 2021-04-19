/*
 * SPDX-FileCopyrightText: 2006-2009 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Gregor Kališnik <gregor@podnapisi.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinview.h"

#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphinitemlistview.h"
#include "dolphinnewfilemenuobserver.h"
#include "draganddrophelper.h"
#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistheader.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/private/kitemlistroleeditor.h"
#include "versioncontrol/versioncontrolobserver.h"
#include "viewproperties.h"
#include "views/tooltips/tooltipmanager.h"
#include "zoomlevelinfo.h"

#ifdef HAVE_BALOO
#include <Baloo/IndexerConfig>
#endif
#include <KColorScheme>
#include <KDesktopFile>
#include <KDirModel>
#include <KFileItemListProperties>
#include <KFormat>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/DropJob>
#include <KIO/JobUiDelegate>
#include <KIO/Paste>
#include <KIO/PasteJob>
#include <KIO/PreviewJob>
#include <KIO/RenameFileDialog>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolManager>

#include <QAbstractItemView>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDropEvent>
#include <QGraphicsOpacityEffect>
#include <QGraphicsSceneDragDropEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeDatabase>
#include <QPixmapCache>
#include <QPointer>
#include <QScrollBar>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>

DolphinView::DolphinView(const QUrl& url, QWidget* parent) :
    QWidget(parent),
    m_active(true),
    m_tabsForFiles(false),
    m_assureVisibleCurrentIndex(false),
    m_isFolderWritable(true),
    m_dragging(false),
    m_loading(false),
    m_url(url),
    m_viewPropertiesContext(),
    m_mode(DolphinView::IconsView),
    m_visibleRoles(),
    m_topLayout(nullptr),
    m_model(nullptr),
    m_view(nullptr),
    m_container(nullptr),
    m_toolTipManager(nullptr),
    m_selectionChangedTimer(nullptr),
    m_currentItemUrl(),
    m_scrollToCurrentItem(false),
    m_restoredContentsPosition(),
    m_selectedUrls(),
    m_clearSelectionBeforeSelectingNewItems(false),
    m_markFirstNewlySelectedItemAsCurrent(false),
    m_versionControlObserver(nullptr),
    m_twoClicksRenamingTimer(nullptr),
    m_placeholderLabel(nullptr)
{
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setContentsMargins(0, 0, 0, 0);

    // When a new item has been created by the "Create New..." menu, the item should
    // get selected and it must be assured that the item will get visible. As the
    // creation is done asynchronously, several signals must be checked:
    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::itemCreated,
            this, &DolphinView::observeCreatedItem);

    m_selectionChangedTimer = new QTimer(this);
    m_selectionChangedTimer->setSingleShot(true);
    m_selectionChangedTimer->setInterval(300);
    connect(m_selectionChangedTimer, &QTimer::timeout,
            this, &DolphinView::emitSelectionChangedSignal);

    m_model = new KFileItemModel(this);
    m_view = new DolphinItemListView();
    m_view->setEnabledSelectionToggles(GeneralSettings::showSelectionToggle());
    m_view->setVisibleRoles({"text"});
    applyModeToView();

    KItemListController* controller = new KItemListController(m_model, m_view, this);
    const int delay = GeneralSettings::autoExpandFolders() ? 750 : -1;
    controller->setAutoActivationDelay(delay);

    // The EnlargeSmallPreviews setting can only be changed after the model
    // has been set in the view by KItemListController.
    m_view->setEnlargeSmallPreviews(GeneralSettings::enlargeSmallPreviews());

    m_container = new KItemListContainer(controller, this);
    m_container->installEventFilter(this);
    setFocusProxy(m_container);
    connect(m_container->horizontalScrollBar(), &QScrollBar::valueChanged, this, [=] { hideToolTip(); });
    connect(m_container->verticalScrollBar(), &QScrollBar::valueChanged, this, [=] { hideToolTip(); });

    // Show some placeholder text for empty folders
    // This is made using a heavily-modified QLabel rather than a KTitleWidget
    // because KTitleWidget can't be told to turn off mouse-selectable text
    m_placeholderLabel = new QLabel(this);
    QFont placeholderLabelFont;
    // To match the size of a level 2 Heading/KTitleWidget
    placeholderLabelFont.setPointSize(qRound(placeholderLabelFont.pointSize() * 1.3));
    m_placeholderLabel->setFont(placeholderLabelFont);
    m_placeholderLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_placeholderLabel->setWordWrap(true);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    // Match opacity of QML placeholder label component
    auto *effect = new QGraphicsOpacityEffect(m_placeholderLabel);
    effect->setOpacity(0.5);
    m_placeholderLabel->setGraphicsEffect(effect);
    // Set initial text and visibility
    updatePlaceholderLabel();

    auto *centeringLayout = new QVBoxLayout(m_container);
    centeringLayout->addWidget(m_placeholderLabel);
    centeringLayout->setAlignment(m_placeholderLabel, Qt::AlignCenter);

    controller->setSelectionBehavior(KItemListController::MultiSelection);
    connect(controller, &KItemListController::itemActivated, this, &DolphinView::slotItemActivated);
    connect(controller, &KItemListController::itemsActivated, this, &DolphinView::slotItemsActivated);
    connect(controller, &KItemListController::itemMiddleClicked, this, &DolphinView::slotItemMiddleClicked);
    connect(controller, &KItemListController::itemContextMenuRequested, this, &DolphinView::slotItemContextMenuRequested);
    connect(controller, &KItemListController::viewContextMenuRequested, this, &DolphinView::slotViewContextMenuRequested);
    connect(controller, &KItemListController::headerContextMenuRequested, this, &DolphinView::slotHeaderContextMenuRequested);
    connect(controller, &KItemListController::mouseButtonPressed, this, &DolphinView::slotMouseButtonPressed);
    connect(controller, &KItemListController::itemHovered, this, &DolphinView::slotItemHovered);
    connect(controller, &KItemListController::itemUnhovered, this, &DolphinView::slotItemUnhovered);
    connect(controller, &KItemListController::itemDropEvent, this, &DolphinView::slotItemDropEvent);
    connect(controller, &KItemListController::escapePressed, this, &DolphinView::stopLoading);
    connect(controller, &KItemListController::modelChanged, this, &DolphinView::slotModelChanged);
    connect(controller, &KItemListController::selectedItemTextPressed, this, &DolphinView::slotSelectedItemTextPressed);
    connect(controller, &KItemListController::increaseZoom, this, &DolphinView::slotIncreaseZoom);
    connect(controller, &KItemListController::decreaseZoom, this, &DolphinView::slotDecreaseZoom);
    connect(controller, &KItemListController::swipeUp, this, &DolphinView::slotSwipeUp);

    connect(m_model, &KFileItemModel::directoryLoadingStarted,       this, &DolphinView::slotDirectoryLoadingStarted);
    connect(m_model, &KFileItemModel::directoryLoadingCompleted,     this, &DolphinView::slotDirectoryLoadingCompleted);
    connect(m_model, &KFileItemModel::directoryLoadingCanceled,      this, &DolphinView::slotDirectoryLoadingCanceled);
    connect(m_model, &KFileItemModel::directoryLoadingProgress,   this, &DolphinView::directoryLoadingProgress);
    connect(m_model, &KFileItemModel::directorySortingProgress,   this, &DolphinView::directorySortingProgress);
    connect(m_model, &KFileItemModel::itemsChanged,
            this, &DolphinView::slotItemsChanged);
    connect(m_model, &KFileItemModel::itemsRemoved,    this, &DolphinView::itemCountChanged);
    connect(m_model, &KFileItemModel::itemsInserted,   this, &DolphinView::itemCountChanged);
    connect(m_model, &KFileItemModel::infoMessage,            this, &DolphinView::infoMessage);
    connect(m_model, &KFileItemModel::errorMessage,           this, &DolphinView::errorMessage);
    connect(m_model, &KFileItemModel::directoryRedirection, this, &DolphinView::slotDirectoryRedirection);
    connect(m_model, &KFileItemModel::urlIsFileError,            this, &DolphinView::urlIsFileError);

    connect(this, &DolphinView::itemCountChanged,
            this, &DolphinView::updatePlaceholderLabel);

    m_view->installEventFilter(this);
    connect(m_view, &DolphinItemListView::sortOrderChanged,
            this, &DolphinView::slotSortOrderChangedByHeader);
    connect(m_view, &DolphinItemListView::sortRoleChanged,
            this, &DolphinView::slotSortRoleChangedByHeader);
    connect(m_view, &DolphinItemListView::visibleRolesChanged,
            this, &DolphinView::slotVisibleRolesChangedByHeader);
    connect(m_view, &DolphinItemListView::roleEditingCanceled,
            this, &DolphinView::slotRoleEditingCanceled);
    connect(m_view->header(), &KItemListHeader::columnWidthChangeFinished,
            this, &DolphinView::slotHeaderColumnWidthChangeFinished);

    KItemListSelectionManager* selectionManager = controller->selectionManager();
    connect(selectionManager, &KItemListSelectionManager::selectionChanged,
            this, &DolphinView::slotSelectionChanged);

#ifdef HAVE_BALOO
    m_toolTipManager = new ToolTipManager(this);
    connect(m_toolTipManager, &ToolTipManager::urlActivated, this, &DolphinView::urlActivated);
#endif

    m_versionControlObserver = new VersionControlObserver(this);
    m_versionControlObserver->setView(this);
    m_versionControlObserver->setModel(m_model);
    connect(m_versionControlObserver, &VersionControlObserver::infoMessage, this, &DolphinView::infoMessage);
    connect(m_versionControlObserver, &VersionControlObserver::errorMessage, this, &DolphinView::errorMessage);
    connect(m_versionControlObserver, &VersionControlObserver::operationCompletedMessage, this, &DolphinView::operationCompletedMessage);

    m_twoClicksRenamingTimer = new QTimer(this);
    m_twoClicksRenamingTimer->setSingleShot(true);
    connect(m_twoClicksRenamingTimer, &QTimer::timeout, this, &DolphinView::slotTwoClicksRenamingTimerTimeout);

    applyViewProperties();
    m_topLayout->addWidget(m_container);

    loadDirectory(url);
}

DolphinView::~DolphinView()
{
}

QUrl DolphinView::url() const
{
    return m_url;
}

void DolphinView::setActive(bool active)
{
    if (active == m_active) {
        return;
    }

    m_active = active;

    updatePalette();

    if (active) {
        m_container->setFocus();
        Q_EMIT activated();
        Q_EMIT writeStateChanged(m_isFolderWritable);
    }
}

bool DolphinView::isActive() const
{
    return m_active;
}

void DolphinView::setMode(Mode mode)
{
    if (mode != m_mode) {
        ViewProperties props(viewPropertiesUrl());
        props.setViewMode(mode);

        // We pass the new ViewProperties to applyViewProperties, rather than
        // storing them on disk and letting applyViewProperties() read them
        // from there, to prevent that changing the view mode fails if the
        // .directory file is not writable (see bug 318534).
        applyViewProperties(props);
    }
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

void DolphinView::setPreviewsShown(bool show)
{
    if (previewsShown() == show) {
        return;
    }

    ViewProperties props(viewPropertiesUrl());
    props.setPreviewsShown(show);

    const int oldZoomLevel = m_view->zoomLevel();
    m_view->setPreviewsShown(show);
    Q_EMIT previewsShownChanged(show);

    const int newZoomLevel = m_view->zoomLevel();
    if (newZoomLevel != oldZoomLevel) {
        Q_EMIT zoomLevelChanged(newZoomLevel, oldZoomLevel);
    }
}

bool DolphinView::previewsShown() const
{
    return m_view->previewsShown();
}

void DolphinView::setHiddenFilesShown(bool show)
{
    if (m_model->showHiddenFiles() == show) {
        return;
    }

    const KFileItemList itemList = selectedItems();
    m_selectedUrls.clear();
    m_selectedUrls = itemList.urlList();

    ViewProperties props(viewPropertiesUrl());
    props.setHiddenFilesShown(show);

    m_model->setShowHiddenFiles(show);
    Q_EMIT hiddenFilesShownChanged(show);
}

bool DolphinView::hiddenFilesShown() const
{
    return m_model->showHiddenFiles();
}

void DolphinView::setGroupedSorting(bool grouped)
{
    if (grouped == groupedSorting()) {
        return;
    }

    ViewProperties props(viewPropertiesUrl());
    props.setGroupedSorting(grouped);
    props.save();

    m_container->controller()->model()->setGroupedSorting(grouped);

    Q_EMIT groupedSortingChanged(grouped);
}

bool DolphinView::groupedSorting() const
{
    return m_model->groupedSorting();
}

KFileItemList DolphinView::items() const
{
    KFileItemList list;
    const int itemCount = m_model->count();
    list.reserve(itemCount);

    for (int i = 0; i < itemCount; ++i) {
        list.append(m_model->fileItem(i));
    }

    return list;
}

int DolphinView::itemsCount() const
{
    return m_model->count();
}

KFileItemList DolphinView::selectedItems() const
{
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

    KFileItemList selectedItems;
    const auto items = selectionManager->selectedItems();
    selectedItems.reserve(items.count());
    for (int index : items) {
        selectedItems.append(m_model->fileItem(index));
    }
    return selectedItems;
}

int DolphinView::selectedItemsCount() const
{
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    return selectionManager->selectedItems().count();
}

void DolphinView::markUrlsAsSelected(const QList<QUrl>& urls)
{
    m_selectedUrls = urls;
}

void DolphinView::markUrlAsCurrent(const QUrl &url)
{
    m_currentItemUrl = url;
    m_scrollToCurrentItem = true;
}

void DolphinView::selectItems(const QRegularExpression &regexp, bool enabled)
{
    const KItemListSelectionManager::SelectionMode mode = enabled
                                                        ? KItemListSelectionManager::Select
                                                        : KItemListSelectionManager::Deselect;
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

    for (int index = 0; index < m_model->count(); index++) {
        const KFileItem item = m_model->fileItem(index);
        if (regexp.match(item.text()).hasMatch()) {
            // An alternative approach would be to store the matching items in a KItemSet and
            // select them in one go after the loop, but we'd need a new function
            // KItemListSelectionManager::setSelected(KItemSet, SelectionMode mode)
            // for that.
            selectionManager->setSelected(index, 1, mode);
        }
    }
}

void DolphinView::setZoomLevel(int level)
{
    const int oldZoomLevel = zoomLevel();
    m_view->setZoomLevel(level);
    if (zoomLevel() != oldZoomLevel) {
        hideToolTip();
        Q_EMIT zoomLevelChanged(zoomLevel(), oldZoomLevel);
    }
}

int DolphinView::zoomLevel() const
{
    return m_view->zoomLevel();
}

void DolphinView::setSortRole(const QByteArray& role)
{
    if (role != sortRole()) {
        updateSortRole(role);
    }
}

QByteArray DolphinView::sortRole() const
{
    const KItemModelBase* model = m_container->controller()->model();
    return model->sortRole();
}

void DolphinView::setSortOrder(Qt::SortOrder order)
{
    if (sortOrder() != order) {
        updateSortOrder(order);
    }
}

Qt::SortOrder DolphinView::sortOrder() const
{
    return m_model->sortOrder();
}

void DolphinView::setSortFoldersFirst(bool foldersFirst)
{
    if (sortFoldersFirst() != foldersFirst) {
        updateSortFoldersFirst(foldersFirst);
    }
}

bool DolphinView::sortFoldersFirst() const
{
    return m_model->sortDirectoriesFirst();
}

void DolphinView::setVisibleRoles(const QList<QByteArray>& roles)
{
    const QList<QByteArray> previousRoles = roles;

    ViewProperties props(viewPropertiesUrl());
    props.setVisibleRoles(roles);

    m_visibleRoles = roles;
    m_view->setVisibleRoles(roles);

    Q_EMIT visibleRolesChanged(m_visibleRoles, previousRoles);
}

QList<QByteArray> DolphinView::visibleRoles() const
{
    return m_visibleRoles;
}

void DolphinView::reload()
{
    QByteArray viewState;
    QDataStream saveStream(&viewState, QIODevice::WriteOnly);
    saveState(saveStream);

    setUrl(url());
    loadDirectory(url(), true);

    QDataStream restoreStream(viewState);
    restoreState(restoreStream);
}

void DolphinView::readSettings()
{
    const int oldZoomLevel = m_view->zoomLevel();

    GeneralSettings::self()->load();
    m_view->readSettings();
    applyViewProperties();

    const int delay = GeneralSettings::autoExpandFolders() ? 750 : -1;
    m_container->controller()->setAutoActivationDelay(delay);

    const int newZoomLevel = m_view->zoomLevel();
    if (newZoomLevel != oldZoomLevel) {
        Q_EMIT zoomLevelChanged(newZoomLevel, oldZoomLevel);
    }
}

void DolphinView::writeSettings()
{
    GeneralSettings::self()->save();
    m_view->writeSettings();
}

void DolphinView::setNameFilter(const QString& nameFilter)
{
    m_model->setNameFilter(nameFilter);
}

QString DolphinView::nameFilter() const
{
    return m_model->nameFilter();
}

void DolphinView::setMimeTypeFilters(const QStringList& filters)
{
    return m_model->setMimeTypeFilters(filters);
}

QStringList DolphinView::mimeTypeFilters() const
{
    return m_model->mimeTypeFilters();
}

void DolphinView::requestStatusBarText()
{
    if (m_statJobForStatusBarText) {
        // Kill the pending request.
        m_statJobForStatusBarText->kill();
    }

    if (m_container->controller()->selectionManager()->hasSelection()) {
        int folderCount = 0;
        int fileCount = 0;
        KIO::filesize_t totalFileSize = 0;

        // Give a summary of the status of the selected files
        const KFileItemList list = selectedItems();
        for (const KFileItem& item : list) {
            if (item.isDir()) {
                ++folderCount;
            } else {
                ++fileCount;
                totalFileSize += item.size();
            }
        }

        if (folderCount + fileCount == 1) {
            // If only one item is selected, show info about it
            Q_EMIT statusBarTextChanged(list.first().getStatusBarInfo());
        } else {
            // At least 2 items are selected
            emitStatusBarText(folderCount, fileCount, totalFileSize, HasSelection);
        }
    } else { // has no selection
        if (!m_model->rootItem().url().isValid()) {
            return;
        }

        m_statJobForStatusBarText = KIO::statDetails(m_model->rootItem().url(),
                        KIO::StatJob::SourceSide, KIO::StatRecursiveSize, KIO::HideProgressInfo);
        connect(m_statJobForStatusBarText, &KJob::result,
                this, &DolphinView::slotStatJobResult);
        m_statJobForStatusBarText->start();
    }
}

void DolphinView::emitStatusBarText(const int folderCount, const int fileCount,
                                    KIO::filesize_t totalFileSize, const Selection selection)
{
    QString foldersText;
    QString filesText;
    QString summary;

    if (selection == HasSelection) {
        // At least 2 items are selected because the case of 1 selected item is handled in
        // DolphinView::requestStatusBarText().
        foldersText = i18ncp("@info:status", "1 Folder selected", "%1 Folders selected", folderCount);
        filesText = i18ncp("@info:status", "1 File selected", "%1 Files selected", fileCount);
    } else {
        foldersText = i18ncp("@info:status", "1 Folder", "%1 Folders", folderCount);
        filesText = i18ncp("@info:status", "1 File", "%1 Files", fileCount);
    }

    if (fileCount > 0 && folderCount > 0) {
        summary = i18nc("@info:status folders, files (size)", "%1, %2 (%3)",
                        foldersText, filesText,
                        KFormat().formatByteSize(totalFileSize));
    } else if (fileCount > 0) {
        summary = i18nc("@info:status files (size)", "%1 (%2)",
                        filesText,
                        KFormat().formatByteSize(totalFileSize));
    } else if (folderCount > 0) {
        summary = foldersText;
    } else {
        summary = i18nc("@info:status", "0 Folders, 0 Files");
    }
    Q_EMIT statusBarTextChanged(summary);
}

QList<QAction*> DolphinView::versionControlActions(const KFileItemList& items) const
{
    QList<QAction*> actions;

    if (items.isEmpty()) {
        const KFileItem item = m_model->rootItem();
        if (!item.isNull()) {
            actions = m_versionControlObserver->actions(KFileItemList() << item);
        }
    } else {
        actions = m_versionControlObserver->actions(items);
    }

    return actions;
}

void DolphinView::setUrl(const QUrl& url)
{
    if (url == m_url) {
        return;
    }

    clearSelection();

    m_url = url;

    hideToolTip();

    disconnect(m_view, &DolphinItemListView::roleEditingFinished,
               this, &DolphinView::slotRoleEditingFinished);

    // It is important to clear the items from the model before
    // applying the view properties, otherwise expensive operations
    // might be done on the existing items although they get cleared
    // anyhow afterwards by loadDirectory().
    m_model->clear();
    applyViewProperties();
    loadDirectory(url);

    Q_EMIT urlChanged(url);
}

void DolphinView::selectAll()
{
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    selectionManager->setSelected(0, m_model->count());
}

void DolphinView::invertSelection()
{
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    selectionManager->setSelected(0, m_model->count(), KItemListSelectionManager::Toggle);
}

void DolphinView::clearSelection()
{
    m_selectedUrls.clear();
    m_container->controller()->selectionManager()->clearSelection();
}

void DolphinView::renameSelectedItems()
{
    const KFileItemList items = selectedItems();
    if (items.isEmpty()) {
        return;
    }

    if (items.count() == 1 && GeneralSettings::renameInline()) {
        const int index = m_model->index(items.first());

        QMetaObject::Connection * const connection = new QMetaObject::Connection;
        *connection = connect(m_view, &KItemListView::scrollingStopped, this, [=](){
            QObject::disconnect(*connection);
            delete connection;

            m_view->editRole(index, "text");

            hideToolTip();

            connect(m_view, &DolphinItemListView::roleEditingFinished,
                    this, &DolphinView::slotRoleEditingFinished);
        });
        m_view->scrollToItem(index);

    } else {
        KIO::RenameFileDialog* dialog = new KIO::RenameFileDialog(items, this);
        connect(dialog, &KIO::RenameFileDialog::renamingFinished,
                this, &DolphinView::slotRenameDialogRenamingFinished);

        dialog->open();
    }

    // Assure that the current index remains visible when KFileItemModel
    // will notify the view about changed items (which might result in
    // a changed sorting).
    m_assureVisibleCurrentIndex = true;
}

void DolphinView::trashSelectedItems()
{
    const QList<QUrl> list = simplifiedSelectedUrls();
    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(window());
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Trash, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job* job = KIO::trash(list);
        KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Trash, list, QUrl(QStringLiteral("trash:/")), job);
        KJobWidgets::setWindow(job, this);
        connect(job, &KIO::Job::result,
                this, &DolphinView::slotTrashFileFinished);
    }
}

void DolphinView::deleteSelectedItems()
{
    const QList<QUrl> list = simplifiedSelectedUrls();

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(window());
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job* job = KIO::del(list);
        KJobWidgets::setWindow(job, this);
        connect(job, &KIO::Job::result,
                this, &DolphinView::slotDeleteFileFinished);
    }
}

void DolphinView::cutSelectedItemsToClipboard()
{
    QMimeData* mimeData = selectionMimeData();
    KIO::setClipboardDataCut(mimeData, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinView::copySelectedItemsToClipboard()
{
    QMimeData* mimeData = selectionMimeData();
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinView::copySelectedItems(const KFileItemList &selection, const QUrl &destinationUrl)
{
    KIO::CopyJob* job = KIO::copy(selection.urlList(), destinationUrl, KIO::DefaultFlags);
    KJobWidgets::setWindow(job, this);

    connect(job, &KIO::DropJob::result, this, &DolphinView::slotJobResult);
    connect(job, &KIO::CopyJob::copyingDone, this, &DolphinView::slotCopyingDone);
    KIO::FileUndoManager::self()->recordCopyJob(job);
}

void DolphinView::moveSelectedItems(const KFileItemList &selection, const QUrl &destinationUrl)
{
    KIO::CopyJob* job = KIO::move(selection.urlList(), destinationUrl, KIO::DefaultFlags);
    KJobWidgets::setWindow(job, this);

    connect(job, &KIO::DropJob::result, this, &DolphinView::slotJobResult);
    connect(job, &KIO::CopyJob::copyingDone, this, &DolphinView::slotCopyingDone);
    KIO::FileUndoManager::self()->recordCopyJob(job);

}

void DolphinView::paste()
{
    pasteToUrl(url());
}

void DolphinView::pasteIntoFolder()
{
    const KFileItemList items = selectedItems();
    if ((items.count() == 1) && items.first().isDir()) {
        pasteToUrl(items.first().url());
    }
}

void DolphinView::duplicateSelectedItems()
{
    const KFileItemList itemList = selectedItems();
    if (itemList.isEmpty()) {
        return;
    }

    const QMimeDatabase db;

    // Duplicate all selected items and append "copy" to the end of the file name
    // but before the filename extension, if present
    QList<QUrl> newSelection;
    for (const auto &item : itemList) {
        const QUrl originalURL  = item.url();
        const QString originalDirectoryPath = originalURL.adjusted(QUrl::RemoveFilename).path();
        const QString originalFileName = item.name();

        QString extension = db.suffixForFileName(originalFileName);

        QUrl duplicateURL = originalURL;

        // No extension; new filename is "<oldfilename> copy"
        if (extension.isEmpty()) {
            duplicateURL.setPath(originalDirectoryPath + i18nc("<filename> copy", "%1 copy", originalFileName));
        // There's an extension; new filename is "<oldfilename> copy.<extension>"
        } else {
            // Need to add a dot since QMimeDatabase::suffixForFileName() doesn't include it
            extension = QLatin1String(".") + extension;
            const QString originalFilenameWithoutExtension = originalFileName.chopped(extension.size());
            // Preserve file's original filename extension in case the casing differs
            // from what QMimeDatabase::suffixForFileName() returned
            const QString originalExtension = originalFileName.right(extension.size());
            duplicateURL.setPath(originalDirectoryPath + i18nc("<filename> copy", "%1 copy", originalFilenameWithoutExtension) + originalExtension);
        }

        KIO::CopyJob* job = KIO::copyAs(originalURL, duplicateURL);
        KJobWidgets::setWindow(job, this);

        if (job) {
            newSelection << duplicateURL;
            KIO::FileUndoManager::self()->recordCopyJob(job);
        }
    }

    forceUrlsSelection(newSelection.first(), newSelection);
}

void DolphinView::stopLoading()
{
    m_model->cancelDirectoryLoading();
}

void DolphinView::updatePalette()
{
    QColor color = KColorScheme(isActiveWindow() ? QPalette::Active : QPalette::Inactive, KColorScheme::View).background().color();
    if (!m_active) {
        color.setAlpha(150);
    }

    QWidget* viewport = m_container->viewport();
    if (viewport) {
        QPalette palette;
        palette.setColor(viewport->backgroundRole(), color);
        viewport->setPalette(palette);
    }

    update();
}

void DolphinView::abortTwoClicksRenaming()
{
    m_twoClicksRenamingItemUrl.clear();
    m_twoClicksRenamingTimer->stop();
}

bool DolphinView::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::PaletteChange:
        updatePalette();
        QPixmapCache::clear();
        break;

    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        updatePalette();
        break;

    case QEvent::KeyPress:
        hideToolTip(ToolTipManager::HideBehavior::Instantly);
        if (GeneralSettings::useTabForSwitchingSplitView()) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Tab && keyEvent->modifiers() == Qt::NoModifier) {
                Q_EMIT toggleActiveViewRequested();
                return true;
            }
        }
        break;
    case QEvent::FocusIn:
        if (watched == m_container) {
            setActive(true);
        }
        break;

    case QEvent::GraphicsSceneDragEnter:
        if (watched == m_view) {
            m_dragging = true;
            abortTwoClicksRenaming();
        }
        break;

    case QEvent::GraphicsSceneDragLeave:
        if (watched == m_view) {
            m_dragging = false;
        }
        break;

    case QEvent::GraphicsSceneDrop:
        if (watched == m_view) {
            m_dragging = false;
        }
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

void DolphinView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const QPoint numDegrees = event->angleDelta() / 8;
        const QPoint numSteps = numDegrees / 15;

        setZoomLevel(zoomLevel() + numSteps.y());
        event->accept();
    } else {
        event->ignore();
    }
}

void DolphinView::hideEvent(QHideEvent* event)
{
    hideToolTip();
    QWidget::hideEvent(event);
}

bool DolphinView::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        /* See Bug 297355
         * Dolphin leaves file preview tooltips open even when is not visible.
         *
         * Hide tool-tip when Dolphin loses focus.
         */
        hideToolTip();
        abortTwoClicksRenaming();
    }

    return QWidget::event(event);
}

void DolphinView::activate()
{
    setActive(true);
}

void DolphinView::slotItemActivated(int index)
{
    abortTwoClicksRenaming();

    const KFileItem item = m_model->fileItem(index);
    if (!item.isNull()) {
        Q_EMIT itemActivated(item);
    }
}

void DolphinView::slotItemsActivated(const KItemSet& indexes)
{
    Q_ASSERT(indexes.count() >= 2);

    abortTwoClicksRenaming();

    if (indexes.count() > 5) {
        QString question = i18np("Are you sure you want to open 1 item?", "Are you sure you want to open %1 items?", indexes.count());
        const int answer = KMessageBox::warningYesNo(this, question);
        if (answer != KMessageBox::Yes) {
            return;
        }
    }

    KFileItemList items;
    items.reserve(indexes.count());

    for (int index : indexes) {
        KFileItem item = m_model->fileItem(index);
        const QUrl& url = openItemAsFolderUrl(item);

        if (!url.isEmpty()) { // Open folders in new tabs
            Q_EMIT tabRequested(url);
        } else {
            items.append(item);
        }
    }

    if (items.count() == 1) {
        Q_EMIT itemActivated(items.first());
    } else if (items.count() > 1) {
        Q_EMIT itemsActivated(items);
    }
}

void DolphinView::slotItemMiddleClicked(int index)
{
    const KFileItem& item = m_model->fileItem(index);
    const QUrl& url = openItemAsFolderUrl(item);
    if (!url.isEmpty()) {
        Q_EMIT tabRequested(url);
    } else if (isTabsForFilesEnabled()) {
        Q_EMIT tabRequested(item.url());
    }
}

void DolphinView::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    // Force emit of a selection changed signal before we request the
    // context menu, to update the edit-actions first. (See Bug 294013)
    if (m_selectionChangedTimer->isActive()) {
        emitSelectionChangedSignal();
    }

    const KFileItem item = m_model->fileItem(index);
    Q_EMIT requestContextMenu(pos.toPoint(), item, url(), QList<QAction*>());
}

void DolphinView::slotViewContextMenuRequested(const QPointF& pos)
{
    Q_EMIT requestContextMenu(pos.toPoint(), KFileItem(), url(), QList<QAction*>());
}

void DolphinView::slotHeaderContextMenuRequested(const QPointF& pos)
{
    ViewProperties props(viewPropertiesUrl());

    QPointer<QMenu> menu = new QMenu(QApplication::activeWindow());

    KItemListView* view = m_container->controller()->view();
    const QList<QByteArray> visibleRolesSet = view->visibleRoles();

    bool indexingEnabled = false;
#ifdef HAVE_BALOO
    Baloo::IndexerConfig config;
    indexingEnabled = config.fileIndexingEnabled();
#endif

    QString groupName;
    QMenu* groupMenu = nullptr;

    // Add all roles to the menu that can be shown or hidden by the user
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    for (const KFileItemModel::RoleInfo& info : rolesInfo) {
        if (info.role == "text") {
            // It should not be possible to hide the "text" role
            continue;
        }

        const QString text = m_model->roleDescription(info.role);
        QAction* action = nullptr;
        if (info.group.isEmpty()) {
            action = menu->addAction(text);
        } else {
            if (!groupMenu || info.group != groupName) {
                groupName = info.group;
                groupMenu = menu->addMenu(groupName);
            }

            action = groupMenu->addAction(text);
        }

        action->setCheckable(true);
        action->setChecked(visibleRolesSet.contains(info.role));
        action->setData(info.role);

        const bool enable = (!info.requiresBaloo && !info.requiresIndexer) ||
                            (info.requiresBaloo) ||
                            (info.requiresIndexer && indexingEnabled);
        action->setEnabled(enable);
    }

    menu->addSeparator();

    QActionGroup* widthsGroup = new QActionGroup(menu);
    const bool autoColumnWidths = props.headerColumnWidths().isEmpty();

    QAction* autoAdjustWidthsAction = menu->addAction(i18nc("@action:inmenu", "Automatic Column Widths"));
    autoAdjustWidthsAction->setCheckable(true);
    autoAdjustWidthsAction->setChecked(autoColumnWidths);
    autoAdjustWidthsAction->setActionGroup(widthsGroup);

    QAction* customWidthsAction = menu->addAction(i18nc("@action:inmenu", "Custom Column Widths"));
    customWidthsAction->setCheckable(true);
    customWidthsAction->setChecked(!autoColumnWidths);
    customWidthsAction->setActionGroup(widthsGroup);

    QAction* action = menu->exec(pos.toPoint());
    if (menu && action) {
        KItemListHeader* header = view->header();

        if (action == autoAdjustWidthsAction) {
            // Clear the column-widths from the viewproperties and turn on
            // the automatic resizing of the columns
            props.setHeaderColumnWidths(QList<int>());
            header->setAutomaticColumnResizing(true);
        } else if (action == customWidthsAction) {
            // Apply the current column-widths as custom column-widths and turn
            // off the automatic resizing of the columns
            QList<int> columnWidths;
            const auto visibleRoles = view->visibleRoles();
            columnWidths.reserve(visibleRoles.count());
            for (const QByteArray& role : visibleRoles) {
                columnWidths.append(header->columnWidth(role));
            }
            props.setHeaderColumnWidths(columnWidths);
            header->setAutomaticColumnResizing(false);
        } else {
            // Show or hide the selected role
            const QByteArray selectedRole = action->data().toByteArray();

            QList<QByteArray> visibleRoles = view->visibleRoles();
            if (action->isChecked()) {
                visibleRoles.append(selectedRole);
            } else {
                visibleRoles.removeOne(selectedRole);
            }

            view->setVisibleRoles(visibleRoles);
            props.setVisibleRoles(visibleRoles);

            QList<int> columnWidths;
            if (!header->automaticColumnResizing()) {
                const auto visibleRoles = view->visibleRoles();
                columnWidths.reserve(visibleRoles.count());
                for (const QByteArray& role : visibleRoles) {
                    columnWidths.append(header->columnWidth(role));
                }
            }
            props.setHeaderColumnWidths(columnWidths);
        }
    }

    delete menu;
}

void DolphinView::slotHeaderColumnWidthChangeFinished(const QByteArray& role, qreal current)
{
    const QList<QByteArray> visibleRoles = m_view->visibleRoles();

    ViewProperties props(viewPropertiesUrl());
    QList<int> columnWidths = props.headerColumnWidths();
    if (columnWidths.count() != visibleRoles.count()) {
        columnWidths.clear();
        columnWidths.reserve(visibleRoles.count());
        const KItemListHeader* header = m_view->header();
        for (const QByteArray& role : visibleRoles) {
            const int width = header->columnWidth(role);
            columnWidths.append(width);
        }
    }

    const int roleIndex = visibleRoles.indexOf(role);
    Q_ASSERT(roleIndex >= 0 && roleIndex < columnWidths.count());
    columnWidths[roleIndex] = current;

    props.setHeaderColumnWidths(columnWidths);
}

void DolphinView::slotItemHovered(int index)
{
    const KFileItem item = m_model->fileItem(index);

    if (GeneralSettings::showToolTips() && !m_dragging) {
        QRectF itemRect = m_container->controller()->view()->itemContextRect(index);
        const QPoint pos = m_container->mapToGlobal(itemRect.topLeft().toPoint());
        itemRect.moveTo(pos);

#ifdef HAVE_BALOO
        m_toolTipManager->showToolTip(item, itemRect, nativeParentWidget()->windowHandle());
#endif
    }

    Q_EMIT requestItemInfo(item);
}

void DolphinView::slotItemUnhovered(int index)
{
    Q_UNUSED(index)
    hideToolTip();
    Q_EMIT requestItemInfo(KFileItem());
}

void DolphinView::slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event)
{
    QUrl destUrl;
    KFileItem destItem = m_model->fileItem(index);
    if (destItem.isNull() || (!destItem.isDir() && !destItem.isDesktopFile())) {
        // Use the URL of the view as drop target if the item is no directory
        // or desktop-file
        destItem = m_model->rootItem();
        destUrl = url();
    } else {
        // The item represents a directory or desktop-file
        destUrl = destItem.mostLocalUrl();
    }

    QDropEvent dropEvent(event->pos().toPoint(),
                         event->possibleActions(),
                         event->mimeData(),
                         event->buttons(),
                         event->modifiers());
    dropUrls(destUrl, &dropEvent, this);

    setActive(true);
}

void DolphinView::dropUrls(const QUrl &destUrl, QDropEvent *dropEvent, QWidget *dropWidget)
{
    KIO::DropJob* job = DragAndDropHelper::dropUrls(destUrl, dropEvent, dropWidget);

    if (job) {
        connect(job, &KIO::DropJob::result, this, &DolphinView::slotJobResult);

        if (destUrl == url()) {
            // Mark the dropped urls as selected.
            m_clearSelectionBeforeSelectingNewItems = true;
            m_markFirstNewlySelectedItemAsCurrent = true;
            connect(job, &KIO::DropJob::itemCreated, this, &DolphinView::slotItemCreated);
        }
    }
}

void DolphinView::slotModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    if (previous != nullptr) {
        Q_ASSERT(qobject_cast<KFileItemModel*>(previous));
        KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(previous);
        disconnect(fileItemModel, &KFileItemModel::directoryLoadingCompleted, this, &DolphinView::slotDirectoryLoadingCompleted);
        m_versionControlObserver->setModel(nullptr);
    }

    if (current) {
        Q_ASSERT(qobject_cast<KFileItemModel*>(current));
        KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(current);
        connect(fileItemModel, &KFileItemModel::directoryLoadingCompleted, this, &DolphinView::slotDirectoryLoadingCompleted);
        m_versionControlObserver->setModel(fileItemModel);
    }
}

void DolphinView::slotMouseButtonPressed(int itemIndex, Qt::MouseButtons buttons)
{
    Q_UNUSED(itemIndex)

    hideToolTip();

    if (buttons & Qt::BackButton) {
        Q_EMIT goBackRequested();
    } else if (buttons & Qt::ForwardButton) {
        Q_EMIT goForwardRequested();
    }
}

void DolphinView::slotSelectedItemTextPressed(int index)
{
    if (GeneralSettings::renameInline() && !m_view->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        const KFileItem item = m_model->fileItem(index);
        const KFileItemListProperties capabilities(KFileItemList() << item);
        if (capabilities.supportsMoving()) {
            m_twoClicksRenamingItemUrl = item.url();
            m_twoClicksRenamingTimer->start(QApplication::doubleClickInterval());
        }
    }
}

void DolphinView::slotCopyingDone(KIO::Job *, const QUrl &, const QUrl &to)
{
    slotItemCreated(to);
}

void DolphinView::slotItemCreated(const QUrl& url)
{
    if (m_markFirstNewlySelectedItemAsCurrent) {
        markUrlAsCurrent(url);
        m_markFirstNewlySelectedItemAsCurrent = false;
    }
    m_selectedUrls << url;
}

void DolphinView::slotJobResult(KJob *job)
{
    if (job->error()) {
        Q_EMIT errorMessage(job->errorString());
    }
    if (!m_selectedUrls.isEmpty()) {
        m_selectedUrls = KDirModel::simplifiedUrlList(m_selectedUrls);
    }
}

void DolphinView::slotSelectionChanged(const KItemSet& current, const KItemSet& previous)
{
    const int currentCount = current.count();
    const int previousCount = previous.count();
    const bool selectionStateChanged = (currentCount == 0 && previousCount  > 0) ||
                                       (currentCount >  0 && previousCount == 0);

    // If nothing has been selected before and something got selected (or if something
    // was selected before and now nothing is selected) the selectionChangedSignal must
    // be emitted asynchronously as fast as possible to update the edit-actions.
    m_selectionChangedTimer->setInterval(selectionStateChanged ? 0 : 300);
    m_selectionChangedTimer->start();
}

void DolphinView::emitSelectionChangedSignal()
{
    m_selectionChangedTimer->stop();
    Q_EMIT selectionChanged(selectedItems());
}

void DolphinView::slotStatJobResult(KJob *job)
{
    int folderCount = 0;
    int fileCount = 0;
    KIO::filesize_t totalFileSize = 0;
    bool countFileSize = true;

    const auto entry =  static_cast<KIO::StatJob *>(job)->statResult();
    if (entry.contains(KIO::UDSEntry::UDS_RECURSIVE_SIZE)) {
        // We have a precomputed value.
        totalFileSize = static_cast<KIO::filesize_t>(
                                entry.numberValue(KIO::UDSEntry::UDS_RECURSIVE_SIZE));
        countFileSize = false;
    }

    const int itemCount = m_model->count();
    for (int i = 0; i < itemCount; ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (item.isDir()) {
            ++folderCount;
        } else {
            ++fileCount;
            if (countFileSize) {
                totalFileSize += item.size();
            }
        }
    }
    emitStatusBarText(folderCount, fileCount, totalFileSize, NoSelection);
}

void DolphinView::updateSortRole(const QByteArray& role)
{
    ViewProperties props(viewPropertiesUrl());
    props.setSortRole(role);

    KItemModelBase* model = m_container->controller()->model();
    model->setSortRole(role);

    Q_EMIT sortRoleChanged(role);
}

void DolphinView::updateSortOrder(Qt::SortOrder order)
{
    ViewProperties props(viewPropertiesUrl());
    props.setSortOrder(order);

    m_model->setSortOrder(order);

    Q_EMIT sortOrderChanged(order);
}

void DolphinView::updateSortFoldersFirst(bool foldersFirst)
{
    ViewProperties props(viewPropertiesUrl());
    props.setSortFoldersFirst(foldersFirst);

    m_model->setSortDirectoriesFirst(foldersFirst);

    Q_EMIT sortFoldersFirstChanged(foldersFirst);
}

QPair<bool, QString> DolphinView::pasteInfo() const
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    QPair<bool, QString> info;
    info.second = KIO::pasteActionText(mimeData, &info.first, rootItem());
    return info;
}

void DolphinView::setTabsForFilesEnabled(bool tabsForFiles)
{
    m_tabsForFiles = tabsForFiles;
}

bool DolphinView::isTabsForFilesEnabled() const
{
    return m_tabsForFiles;
}

bool DolphinView::itemsExpandable() const
{
    return m_mode == DetailsView;
}

void DolphinView::restoreState(QDataStream& stream)
{
    // Read the version number of the view state and check if the version is supported.
    quint32 version = 0;
    stream >> version;
    if (version != 1) {
        // The version of the view state isn't supported, we can't restore it.
        return;
    }

    // Restore the current item that had the keyboard focus
    stream >> m_currentItemUrl;

    // Restore the previously selected items
    stream >> m_selectedUrls;

    // Restore the view position
    stream >> m_restoredContentsPosition;

    // Restore expanded folders (only relevant for the details view - will be ignored by the view in other view modes)
    QSet<QUrl> urls;
    stream >> urls;
    m_model->restoreExpandedDirectories(urls);
}

void DolphinView::saveState(QDataStream& stream)
{
    stream << quint32(1); // View state version

    // Save the current item that has the keyboard focus
    const int currentIndex = m_container->controller()->selectionManager()->currentItem();
    if (currentIndex != -1) {
        KFileItem item = m_model->fileItem(currentIndex);
        Q_ASSERT(!item.isNull()); // If the current index is valid a item must exist
        QUrl currentItemUrl = item.url();
        stream << currentItemUrl;
    } else {
        stream << QUrl();
    }

    // Save the selected urls
    stream << selectedItems().urlList();

    // Save view position
    const qreal x = m_container->horizontalScrollBar()->value();
    const qreal y = m_container->verticalScrollBar()->value();
    stream << QPoint(x, y);

    // Save expanded folders (only relevant for the details view - the set will be empty in other view modes)
    stream << m_model->expandedDirectories();
}

KFileItem DolphinView::rootItem() const
{
    return m_model->rootItem();
}

void DolphinView::setViewPropertiesContext(const QString& context)
{
    m_viewPropertiesContext = context;
}

QString DolphinView::viewPropertiesContext() const
{
    return m_viewPropertiesContext;
}

QUrl DolphinView::openItemAsFolderUrl(const KFileItem& item, const bool browseThroughArchives)
{
    if (item.isNull()) {
        return QUrl();
    }

    QUrl url = item.targetUrl();

    if (item.isDir()) {
        return url;
    }

    if (item.isMimeTypeKnown()) {
        const QString& mimetype = item.mimetype();

        if (browseThroughArchives && item.isFile() && url.isLocalFile()) {
            // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
            // zip:/<path>/ when clicking on a zip file, etc.
            // The .protocol file specifies the mimetype that the kioslave handles.
            // Note that we don't use mimetype inheritance since we don't want to
            // open OpenDocument files as zip folders...
            const QString& protocol = KProtocolManager::protocolForArchiveMimetype(mimetype);
            if (!protocol.isEmpty()) {
                url.setScheme(protocol);
                return url;
            }
        }

        if (mimetype == QLatin1String("application/x-desktop")) {
            // Redirect to the URL in Type=Link desktop files, unless it is a http(s) URL.
            KDesktopFile desktopFile(url.toLocalFile());
            if (desktopFile.hasLinkType()) {
                const QString linkUrl = desktopFile.readUrl();
                if (!linkUrl.startsWith(QLatin1String("http"))) {
                    return QUrl::fromUserInput(linkUrl);
                }
            }
        }
    }

    return QUrl();
}

void DolphinView::resetZoomLevel()
{
    ViewModeSettings::ViewMode mode;

    switch (m_mode) {
    case IconsView:     mode = ViewModeSettings::IconsMode;   break;
    case CompactView:   mode = ViewModeSettings::CompactMode; break;
    case DetailsView:   mode = ViewModeSettings::DetailsMode; break;
    }
    const ViewModeSettings settings(mode);
    const QSize iconSize = QSize(settings.iconSize(), settings.iconSize());
    setZoomLevel(ZoomLevelInfo::zoomLevelForIconSize(iconSize));
}

void DolphinView::observeCreatedItem(const QUrl& url)
{
    if (m_active) {
        forceUrlsSelection(url, {url});
    }
}

void DolphinView::slotDirectoryRedirection(const QUrl& oldUrl, const QUrl& newUrl)
{
    if (oldUrl.matches(url(), QUrl::StripTrailingSlash)) {
        Q_EMIT redirection(oldUrl, newUrl);
        m_url = newUrl; // #186947
    }
}

void DolphinView::updateViewState()
{
    if (m_currentItemUrl != QUrl()) {
        KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

        // if there is a selection already, leave it that way
        if (!selectionManager->hasSelection()) {
            const int currentIndex = m_model->index(m_currentItemUrl);
            if (currentIndex != -1) {
                selectionManager->setCurrentItem(currentIndex);

                // scroll to current item and reset the state
                if (m_scrollToCurrentItem) {
                    m_view->scrollToItem(currentIndex);
                    m_scrollToCurrentItem = false;
                }
            } else {
                selectionManager->setCurrentItem(0);
            }
        }

        m_currentItemUrl = QUrl();
    }

    if (!m_restoredContentsPosition.isNull()) {
        const int x = m_restoredContentsPosition.x();
        const int y = m_restoredContentsPosition.y();
        m_restoredContentsPosition = QPoint();

        m_container->horizontalScrollBar()->setValue(x);
        m_container->verticalScrollBar()->setValue(y);
    }

    if (!m_selectedUrls.isEmpty()) {
        KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

        // if there is a selection already, leave it that way
        if (!selectionManager->hasSelection()) {
            if (m_clearSelectionBeforeSelectingNewItems) {
                selectionManager->clearSelection();
                m_clearSelectionBeforeSelectingNewItems = false;
            }

            KItemSet selectedItems = selectionManager->selectedItems();

            QList<QUrl>::iterator it = m_selectedUrls.begin();
            while (it != m_selectedUrls.end()) {
                const int index = m_model->index(*it);
                if (index >= 0) {
                    selectedItems.insert(index);
                    it = m_selectedUrls.erase(it);
                } else {
                    ++it;
                }
            }

            selectionManager->beginAnchoredSelection(selectionManager->currentItem());
            selectionManager->setSelectedItems(selectedItems);
        }
    }
}

void DolphinView::hideToolTip(const ToolTipManager::HideBehavior behavior)
{
#ifdef HAVE_BALOO
    if (GeneralSettings::showToolTips()) {
        m_toolTipManager->hideToolTip(behavior);
    }
#else
        Q_UNUSED(behavior)
#endif
}

void DolphinView::slotTwoClicksRenamingTimerTimeout()
{
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

    // verify that only one item is selected
    if (selectionManager->selectedItems().count() == 1) {
        const int index = selectionManager->currentItem();
        const QUrl fileItemUrl = m_model->fileItem(index).url();

        // check if the selected item was the same item that started the twoClicksRenaming
        if (fileItemUrl.isValid() && m_twoClicksRenamingItemUrl == fileItemUrl) {
            renameSelectedItems();
        }
    }
}

void DolphinView::slotTrashFileFinished(KJob* job)
{
    if (job->error() == 0) {
        Q_EMIT operationCompletedMessage(i18nc("@info:status", "Trash operation completed."));
    } else if (job->error() != KIO::ERR_USER_CANCELED) {
        Q_EMIT errorMessage(job->errorString());
    }
}

void DolphinView::slotDeleteFileFinished(KJob* job)
{
    if (job->error() == 0) {
        Q_EMIT operationCompletedMessage(i18nc("@info:status", "Delete operation completed."));
    } else if (job->error() != KIO::ERR_USER_CANCELED) {
        Q_EMIT errorMessage(job->errorString());
    }
}

void DolphinView::slotRenamingResult(KJob* job)
{
    if (job->error()) {
        KIO::CopyJob *copyJob = qobject_cast<KIO::CopyJob *>(job);
        Q_ASSERT(copyJob);
        const QUrl newUrl = copyJob->destUrl();
        const int index = m_model->index(newUrl);
        if (index >= 0) {
            QHash<QByteArray, QVariant> data;
            const QUrl oldUrl = copyJob->srcUrls().at(0);
            data.insert("text", oldUrl.fileName());
            m_model->setData(index, data);
        }
    }
}

void DolphinView::slotDirectoryLoadingStarted()
{
    m_loading = true;
    updatePlaceholderLabel();

    // Disable the writestate temporary until it can be determined in a fast way
    // in DolphinView::slotDirectoryLoadingCompleted()
    if (m_isFolderWritable) {
        m_isFolderWritable = false;
        Q_EMIT writeStateChanged(m_isFolderWritable);
    }

    Q_EMIT directoryLoadingStarted();
}

void DolphinView::slotDirectoryLoadingCompleted()
{
    m_loading = false;

    // Update the view-state. This has to be done asynchronously
    // because the view might not be in its final state yet.
    QTimer::singleShot(0, this, &DolphinView::updateViewState);

    // Update the placeholder label in case we found that the folder was empty
    // after loading it

    Q_EMIT directoryLoadingCompleted();

    updatePlaceholderLabel();
    updateWritableState();
}

void DolphinView::slotDirectoryLoadingCanceled()
{
    m_loading = false;

    updatePlaceholderLabel();

    Q_EMIT directoryLoadingCanceled();
}

void DolphinView::slotItemsChanged()
{
    m_assureVisibleCurrentIndex = false;
}

void DolphinView::slotSortOrderChangedByHeader(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(previous)
    Q_ASSERT(m_model->sortOrder() == current);

    ViewProperties props(viewPropertiesUrl());
    props.setSortOrder(current);

    Q_EMIT sortOrderChanged(current);
}

void DolphinView::slotSortRoleChangedByHeader(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous)
    Q_ASSERT(m_model->sortRole() == current);

    ViewProperties props(viewPropertiesUrl());
    props.setSortRole(current);

    Q_EMIT sortRoleChanged(current);
}

void DolphinView::slotVisibleRolesChangedByHeader(const QList<QByteArray>& current,
                                                  const QList<QByteArray>& previous)
{
    Q_UNUSED(previous)
    Q_ASSERT(m_container->controller()->view()->visibleRoles() == current);

    const QList<QByteArray> previousVisibleRoles = m_visibleRoles;

    m_visibleRoles = current;

    ViewProperties props(viewPropertiesUrl());
    props.setVisibleRoles(m_visibleRoles);

    Q_EMIT visibleRolesChanged(m_visibleRoles, previousVisibleRoles);
}

void DolphinView::slotRoleEditingCanceled()
{
    disconnect(m_view, &DolphinItemListView::roleEditingFinished,
               this, &DolphinView::slotRoleEditingFinished);
}

void DolphinView::slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value)
{
    disconnect(m_view, &DolphinItemListView::roleEditingFinished,
               this, &DolphinView::slotRoleEditingFinished);

    const KFileItemList items = selectedItems();
    if (items.count() != 1) {
        return;
    }

    if (role == "text") {
        const KFileItem oldItem = items.first();
        const EditResult retVal = value.value<EditResult>();
        const QString newName = retVal.newName;
        if (!newName.isEmpty() && newName != oldItem.text() && newName != QLatin1Char('.') && newName != QLatin1String("..")) {
            const QUrl oldUrl = oldItem.url();

            QUrl newUrl = oldUrl.adjusted(QUrl::RemoveFilename);
            newUrl.setPath(newUrl.path() + KIO::encodeFileName(newName));

#ifndef Q_OS_WIN
            //Confirm hiding file/directory by renaming inline
            if (!hiddenFilesShown() && newName.startsWith(QLatin1Char('.')) && !oldItem.name().startsWith(QLatin1Char('.'))) {
                KGuiItem yesGuiItem(KStandardGuiItem::yes());
                yesGuiItem.setText(i18nc("@action:button", "Rename and Hide"));

                const auto code = KMessageBox::questionYesNo(this,
                                                             oldItem.isFile() ? i18n("Adding a dot to the beginning of this file's name will hide it from view.\n"
                                                                                     "Do you still want to rename it?")
                                                                              : i18n("Adding a dot to the beginning of this folder's name will hide it from view.\n"
                                                                                     "Do you still want to rename it?"),
                                                             oldItem.isFile() ? i18n("Hide this File?") : i18n("Hide this Folder?"),
                                                             yesGuiItem,
                                                             KStandardGuiItem::cancel(),
                                                             QStringLiteral("ConfirmHide")
                                                            );

                if (code == KMessageBox::No) {
                   return;
                }
            }
#endif

            const bool newNameExistsAlready = (m_model->index(newUrl) >= 0);
            if (!newNameExistsAlready && m_model->index(oldUrl) == index) {
                // Only change the data in the model if no item with the new name
                // is in the model yet. If there is an item with the new name
                // already, calling KIO::CopyJob will open a dialog
                // asking for a new name, and KFileItemModel will update the
                // data when the dir lister signals that the file name has changed.
                QHash<QByteArray, QVariant> data;
                data.insert(role, retVal.newName);
                m_model->setData(index, data);
            }

            KIO::Job * job = KIO::moveAs(oldUrl, newUrl);
            KJobWidgets::setWindow(job, this);
            KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Rename, {oldUrl}, newUrl, job);
            job->uiDelegate()->setAutoErrorHandlingEnabled(true);

            forceUrlsSelection(newUrl, {newUrl});

            if (!newNameExistsAlready) {
                // Only connect the result signal if there is no item with the new name
                // in the model yet, see bug 328262.
                connect(job, &KJob::result, this, &DolphinView::slotRenamingResult);
            }
        }
        if (retVal.direction != EditDone) {
            const short indexShift = retVal.direction == EditNext ? 1 : -1;
            m_container->controller()->selectionManager()->setSelected(index, 1, KItemListSelectionManager::Deselect);
            m_container->controller()->selectionManager()->setSelected(index + indexShift, 1,
                    KItemListSelectionManager::Select);
            renameSelectedItems();
        }
    }
}

void DolphinView::loadDirectory(const QUrl& url, bool reload)
{
    if (!url.isValid()) {
        const QString location(url.toDisplayString(QUrl::PreferLocalFile));
        if (location.isEmpty()) {
            Q_EMIT errorMessage(i18nc("@info:status", "The location is empty."));
        } else {
            Q_EMIT errorMessage(i18nc("@info:status", "The location '%1' is invalid.", location));
        }
        return;
    }

    if (reload) {
        m_model->refreshDirectory(url);
    } else {
        m_model->loadDirectory(url);
    }
}

void DolphinView::applyViewProperties()
{
    const ViewProperties props(viewPropertiesUrl());
    applyViewProperties(props);
}

void DolphinView::applyViewProperties(const ViewProperties& props)
{
    m_view->beginTransaction();

    const Mode mode = props.viewMode();
    if (m_mode != mode) {
        const Mode previousMode = m_mode;
        m_mode = mode;

        // Changing the mode might result in changing
        // the zoom level. Remember the old zoom level so
        // that zoomLevelChanged() can get emitted.
        const int oldZoomLevel = m_view->zoomLevel();
        applyModeToView();

        Q_EMIT modeChanged(m_mode, previousMode);

        if (m_view->zoomLevel() != oldZoomLevel) {
            Q_EMIT zoomLevelChanged(m_view->zoomLevel(), oldZoomLevel);
        }
    }

    const bool hiddenFilesShown = props.hiddenFilesShown();
    if (hiddenFilesShown != m_model->showHiddenFiles()) {
        m_model->setShowHiddenFiles(hiddenFilesShown);
        Q_EMIT hiddenFilesShownChanged(hiddenFilesShown);
    }

    const bool groupedSorting = props.groupedSorting();
    if (groupedSorting != m_model->groupedSorting()) {
        m_model->setGroupedSorting(groupedSorting);
        Q_EMIT groupedSortingChanged(groupedSorting);
    }

    const QByteArray sortRole = props.sortRole();
    if (sortRole != m_model->sortRole()) {
        m_model->setSortRole(sortRole);
        Q_EMIT sortRoleChanged(sortRole);
    }

    const Qt::SortOrder sortOrder = props.sortOrder();
    if (sortOrder != m_model->sortOrder()) {
        m_model->setSortOrder(sortOrder);
        Q_EMIT sortOrderChanged(sortOrder);
    }

    const bool sortFoldersFirst = props.sortFoldersFirst();
    if (sortFoldersFirst != m_model->sortDirectoriesFirst()) {
        m_model->setSortDirectoriesFirst(sortFoldersFirst);
        Q_EMIT sortFoldersFirstChanged(sortFoldersFirst);
    }

    const QList<QByteArray> visibleRoles = props.visibleRoles();
    if (visibleRoles != m_visibleRoles) {
        const QList<QByteArray> previousVisibleRoles = m_visibleRoles;
        m_visibleRoles = visibleRoles;
        m_view->setVisibleRoles(visibleRoles);
        Q_EMIT visibleRolesChanged(m_visibleRoles, previousVisibleRoles);
    }

    const bool previewsShown = props.previewsShown();
    if (previewsShown != m_view->previewsShown()) {
        const int oldZoomLevel = zoomLevel();

        m_view->setPreviewsShown(previewsShown);
        Q_EMIT previewsShownChanged(previewsShown);

        // Changing the preview-state might result in a changed zoom-level
        if (oldZoomLevel != zoomLevel()) {
            Q_EMIT zoomLevelChanged(zoomLevel(), oldZoomLevel);
        }
    }

    KItemListView* itemListView = m_container->controller()->view();
    if (itemListView->isHeaderVisible()) {
        KItemListHeader* header = itemListView->header();
        const QList<int> headerColumnWidths = props.headerColumnWidths();
        const int rolesCount = m_visibleRoles.count();
        if (headerColumnWidths.count() == rolesCount) {
            header->setAutomaticColumnResizing(false);

            QHash<QByteArray, qreal> columnWidths;
            for (int i = 0; i < rolesCount; ++i) {
                columnWidths.insert(m_visibleRoles[i], headerColumnWidths[i]);
            }
            header->setColumnWidths(columnWidths);
        } else {
            header->setAutomaticColumnResizing(true);
        }
    }

    m_view->endTransaction();
}

void DolphinView::applyModeToView()
{
    switch (m_mode) {
    case IconsView:   m_view->setItemLayout(KFileItemListView::IconsLayout); break;
    case CompactView: m_view->setItemLayout(KFileItemListView::CompactLayout); break;
    case DetailsView: m_view->setItemLayout(KFileItemListView::DetailsLayout); break;
    default: Q_ASSERT(false); break;
    }
}

void DolphinView::pasteToUrl(const QUrl& url)
{
    KIO::PasteJob *job = KIO::paste(QApplication::clipboard()->mimeData(), url);
    KJobWidgets::setWindow(job, this);
    m_clearSelectionBeforeSelectingNewItems = true;
    m_markFirstNewlySelectedItemAsCurrent = true;
    connect(job, &KIO::PasteJob::itemCreated, this, &DolphinView::slotItemCreated);
    connect(job, &KIO::PasteJob::result, this, &DolphinView::slotJobResult);
}

QList<QUrl> DolphinView::simplifiedSelectedUrls() const
{
    QList<QUrl> urls;

    const KFileItemList items = selectedItems();
    urls.reserve(items.count());
    for (const KFileItem& item : items) {
        urls.append(item.url());
    }

    if (itemsExpandable()) {
        // TODO: Check if we still need KDirModel for this in KDE 5.0
        urls = KDirModel::simplifiedUrlList(urls);
    }

    return urls;
}

QMimeData* DolphinView::selectionMimeData() const
{
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    const KItemSet selectedIndexes = selectionManager->selectedItems();

    return m_model->createMimeData(selectedIndexes);
}

void DolphinView::updateWritableState()
{
    const bool wasFolderWritable = m_isFolderWritable;
    m_isFolderWritable = false;

    KFileItem item = m_model->rootItem();
    if (item.isNull()) {
        // Try to find out if the URL is writable even if the "root item" is
        // null, see https://bugs.kde.org/show_bug.cgi?id=330001
        item = KFileItem(url());
        item.setDelayedMimeTypes(true);
    }

    KFileItemListProperties capabilities(KFileItemList() << item);
    m_isFolderWritable = capabilities.supportsWriting();

    if (m_isFolderWritable != wasFolderWritable) {
        Q_EMIT writeStateChanged(m_isFolderWritable);
    }
}

QUrl DolphinView::viewPropertiesUrl() const
{
    if (m_viewPropertiesContext.isEmpty()) {
        return m_url;
    }

    QUrl url;
    url.setScheme(m_url.scheme());
    url.setPath(m_viewPropertiesContext);
    return url;
}

void DolphinView::slotRenameDialogRenamingFinished(const QList<QUrl>& urls)
{
    forceUrlsSelection(urls.first(), urls);
}

void DolphinView::forceUrlsSelection(const QUrl& current, const QList<QUrl>& selected)
{
    clearSelection();
    m_clearSelectionBeforeSelectingNewItems = true;
    markUrlAsCurrent(current);
    markUrlsAsSelected(selected);
}

void DolphinView::copyPathToClipboard()
{
    const KFileItemList list = selectedItems();
    if (list.isEmpty()) {
        return;
    }
    const KFileItem& item = list.at(0);
    QString path = item.localPath();
    if (path.isEmpty()) {
        path = item.url().toDisplayString();
    }
    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard == nullptr) {
        return;
    }
    clipboard->setText(path);
}

void DolphinView::slotIncreaseZoom()
{
    setZoomLevel(zoomLevel() + 1);
}

void DolphinView::slotDecreaseZoom()
{
    setZoomLevel(zoomLevel() - 1);
}

void DolphinView::slotSwipeUp()
{
    Q_EMIT goUpRequested();
}

void DolphinView::updatePlaceholderLabel()
{
    if (m_loading || itemsCount() > 0) {
        m_placeholderLabel->setVisible(false);
        return;
    }

    if (!nameFilter().isEmpty()) {
        m_placeholderLabel->setText(i18n("No items matching the filter"));
    } else if (m_url.scheme() == QLatin1String("baloosearch") || m_url.scheme() == QLatin1String("filenamesearch")) {
        m_placeholderLabel->setText(i18n("No items matching the search"));
    } else if (m_url.scheme() == QLatin1String("trash")) {
        m_placeholderLabel->setText(i18n("Trash is empty"));
    } else if (m_url.scheme() == QLatin1String("tags")) {
        m_placeholderLabel->setText(i18n("No tags"));
    } else if (m_url.scheme() == QLatin1String("recentlyused")) {
        m_placeholderLabel->setText(i18n("No recently used items"));
    } else if (m_url.scheme() == QLatin1String("smb")) {
        m_placeholderLabel->setText(i18n("No shared folders found"));
    } else if (m_url.scheme() == QLatin1String("network")) {
        m_placeholderLabel->setText(i18n("No relevant network resources found"));
    } else if (m_url.scheme() == QLatin1String("mtp")) {
        m_placeholderLabel->setText(i18n("No MTP-compatible devices found"));
    } else if (m_url.scheme() == QLatin1String("bluetooth")) {
        m_placeholderLabel->setText(i18n("No Bluetooth devices found"));
    } else {
        m_placeholderLabel->setText(i18n("Folder is empty"));
    }

    m_placeholderLabel->setVisible(true);
}
