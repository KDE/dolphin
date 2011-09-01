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

#include "dolphinview.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QItemSelection>
#include <QBoxLayout>
#include <QTimer>
#include <QScrollBar>

#include <KActionCollection>
#include <KColorScheme>
#include <KDirLister>
#include <KDirModel>
#include <KIconEffect>
#include <KFileItem>
#include <KFileItemListProperties>
#include <KLocale>
#include <kitemviews/kfileitemmodel.h>
#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kitemlistselectionmanager.h>
#include <kitemviews/kitemlistview.h>
#include <kitemviews/kitemlistcontroller.h>
#include <KIO/DeleteJob>
#include <KIO/NetAccess>
#include <KIO/PreviewJob>
#include <KJob>
#include <KMenu>
#include <KMessageBox>
#include <konq_fileitemcapabilities.h>
#include <konq_operations.h>
#include <konqmimedata.h>
#include <KToggleAction>
#include <KUrl>

#include "additionalinfoaccessor.h"
#include "dolphindirlister.h"
#include "dolphinnewfilemenuobserver.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphinitemlistcontainer.h"
#include "renamedialog.h"
#include "settings/dolphinsettings.h"
#include "viewmodecontroller.h"
#include "viewproperties.h"
#include "views/tooltips/tooltipmanager.h"
#include "zoomlevelinfo.h"

namespace {
    const int MaxModeEnum = DolphinView::CompactView;
    const int MaxSortingEnum = DolphinView::SortByPath;
};

DolphinView::DolphinView(const KUrl& url, QWidget* parent) :
    QWidget(parent),
    m_active(true),
    m_tabsForFiles(false),
    m_assureVisibleCurrentIndex(false),
    m_isFolderWritable(true),
    m_url(url),
    m_mode(DolphinView::IconsView),
    m_additionalInfoList(),
    m_topLayout(0),
    m_dirLister(0),
    m_container(0),
    m_toolTipManager(0),
    m_selectionChangedTimer(0),
    m_currentItemIndex(-1),
    m_restoredContentsPosition(),
    m_createdItemUrl(),
    m_selectedItems()
{
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    //m_dolphinViewController = new DolphinViewController(this);

    //m_viewModeController = new ViewModeController(this);
    //m_viewModeController->setUrl(url);

    /*connect(m_viewModeController, SIGNAL(urlChanged(KUrl)),
            this, SIGNAL(urlChanged(KUrl)));

    connect(m_dolphinViewController, SIGNAL(requestContextMenu(QPoint,QList<QAction*>)),
            this, SLOT(openContextMenu(QPoint,QList<QAction*>)));
    connect(m_dolphinViewController, SIGNAL(urlsDropped(KFileItem,KUrl,QDropEvent*)),
            this, SLOT(dropUrls(KFileItem,KUrl,QDropEvent*)));
    connect(m_dolphinViewController, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(updateSorting(DolphinView::Sorting)));
    connect(m_dolphinViewController, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(updateSortOrder(Qt::SortOrder)));
    connect(m_dolphinViewController, SIGNAL(sortFoldersFirstChanged(bool)),
            this, SLOT(updateSortFoldersFirst(bool)));
    connect(m_dolphinViewController, SIGNAL(additionalInfoChanged(QList<DolphinView::AdditionalInfo>)),
            this, SLOT(updateAdditionalInfo(QList<DolphinView::AdditionalInfo>)));*/
    //connect(m_dolphinViewController, SIGNAL(itemActivated(KFileItem)),
    //        this, SLOT(triggerItem(KFileItem)));
    //connect(m_dolphinViewController, SIGNAL(tabRequested(KUrl)),
    //        this, SIGNAL(tabRequested(KUrl)));
    /*connect(m_dolphinViewController, SIGNAL(activated()),
            this, SLOT(activate()));
    connect(m_dolphinViewController, SIGNAL(itemEntered(KFileItem)),
            this, SLOT(showHoverInformation(KFileItem)));
    connect(m_dolphinViewController, SIGNAL(viewportEntered()),
            this, SLOT(clearHoverInformation()));
    connect(m_dolphinViewController, SIGNAL(urlChangeRequested(KUrl)),
            this, SLOT(slotUrlChangeRequested(KUrl)));*/

    // When a new item has been created by the "Create New..." menu, the item should
    // get selected and it must be assured that the item will get visible. As the
    // creation is done asynchronously, several signals must be checked:
    connect(&DolphinNewFileMenuObserver::instance(), SIGNAL(itemCreated(KUrl)),
            this, SLOT(observeCreatedItem(KUrl)));

    m_selectionChangedTimer = new QTimer(this);
    m_selectionChangedTimer->setSingleShot(true);
    m_selectionChangedTimer->setInterval(300);
    connect(m_selectionChangedTimer, SIGNAL(timeout()),
            this, SLOT(emitSelectionChangedSignal()));

    m_dirLister = new DolphinDirLister(this);
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setDelayedMimeTypes(true);

    connect(m_dirLister, SIGNAL(redirection(KUrl,KUrl)), this, SLOT(slotRedirection(KUrl,KUrl)));
    connect(m_dirLister, SIGNAL(started(KUrl)),          this, SLOT(slotDirListerStarted(KUrl)));
    connect(m_dirLister, SIGNAL(completed()),            this, SLOT(slotDirListerCompleted()));
    connect(m_dirLister, SIGNAL(refreshItems(QList<QPair<KFileItem,KFileItem> >)),
            this, SLOT(slotRefreshItems()));

    connect(m_dirLister, SIGNAL(clear()),                     this, SIGNAL(itemCountChanged()));
    connect(m_dirLister, SIGNAL(newItems(KFileItemList)),     this, SIGNAL(itemCountChanged()));
    connect(m_dirLister, SIGNAL(infoMessage(QString)),        this, SIGNAL(infoMessage(QString)));
    connect(m_dirLister, SIGNAL(errorMessage(QString)),       this, SIGNAL(infoMessage(QString)));
    connect(m_dirLister, SIGNAL(percent(int)),                this, SIGNAL(pathLoadingProgress(int)));
    connect(m_dirLister, SIGNAL(urlIsFileError(KUrl)),        this, SIGNAL(urlIsFileError(KUrl)));
    connect(m_dirLister, SIGNAL(itemsDeleted(KFileItemList)), this, SIGNAL(itemCountChanged()));

    m_container = new DolphinItemListContainer(m_dirLister, this);
    QHash<QByteArray, int> visibleRoles;
    visibleRoles.insert("name", 0);
    m_container->setVisibleRoles(visibleRoles);

    KItemListController* controller = m_container->controller();
    controller->setSelectionBehavior(KItemListController::MultiSelection);
    connect(controller, SIGNAL(itemActivated(int)),
            this, SLOT(slotItemActivated(int)));
    connect(controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));
    connect(controller, SIGNAL(contextMenuRequested(int,QPointF)), this, SLOT(slotContextMenuRequested(int,QPointF)));
    connect(controller, SIGNAL(itemExpansionToggleClicked(int)), this, SLOT(slotItemExpansionToggleClicked(int)));
    connect(controller, SIGNAL(itemHovered(int)), this, SLOT(slotItemHovered(int)));
    connect(controller, SIGNAL(itemUnhovered(int)), this, SLOT(slotItemUnhovered(int)));

    KItemListSelectionManager* selectionManager = controller->selectionManager();
    connect(selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)),
            this, SLOT(slotSelectionChanged(QSet<int>,QSet<int>)));

    m_toolTipManager = new ToolTipManager(this);

    applyViewProperties();
    m_topLayout->addWidget(m_container);

    loadDirectory(url);
}

DolphinView::~DolphinView()
{
}

KUrl DolphinView::url() const
{
    return m_url;
}

void DolphinView::setActive(bool active)
{
    if (active == m_active) {
        return;
    }

    m_active = active;

    QColor color = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
    if (!active) {
        color.setAlpha(150);
    }

    /*QAbstractItemView* view = m_viewAccessor.itemView();
    QWidget* viewport = view ? view->viewport() : 0;
    if (viewport) {
        QPalette palette;
        palette.setColor(viewport->backgroundRole(), color);
        viewport->setPalette(palette);
    }*/

    update();

    if (active) {
        //if (view) {
        //    view->setFocus();
        //}
        emit activated();
        emit writeStateChanged(m_isFolderWritable);
    }

    //m_viewModeController->indicateActivationChange(active);
}

bool DolphinView::isActive() const
{
    return m_active;
}

void DolphinView::setMode(Mode mode)
{
    if (mode != m_mode) {
        ViewProperties props(url());
        props.setViewMode(mode);
        props.save();

        applyViewProperties();
    }
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

bool DolphinView::previewsShown() const
{
    return m_container->previewsShown();
}

bool DolphinView::hiddenFilesShown() const
{
    return m_dirLister->showingDotFiles();
}

bool DolphinView::categorizedSorting() const
{
    return false; //m_storedCategorizedSorting;
}

KFileItemList DolphinView::items() const
{
    return m_dirLister->items();
}

KFileItemList DolphinView::selectedItems() const
{
    const KFileItemModel* model = fileItemModel();
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    const QSet<int> selectedIndexes = selectionManager->selectedItems();

    KFileItemList selectedItems;
    QSetIterator<int> it(selectedIndexes);
    while (it.hasNext()) {
        const int index = it.next();
        selectedItems.append(model->fileItem(index));
    }
    return selectedItems;
}

int DolphinView::selectedItemsCount() const
{
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    return selectionManager->selectedItems().count();
}

void DolphinView::markUrlsAsSelected(const QList<KUrl>& urls)
{
    foreach (const KUrl& url, urls) {
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
        m_selectedItems.append(item);
    }
}

void DolphinView::setItemSelectionEnabled(const QRegExp& pattern, bool enabled)
{
    const KItemListSelectionManager::SelectionMode mode = enabled
                                                        ? KItemListSelectionManager::Select
                                                        : KItemListSelectionManager::Deselect;
    const KFileItemModel* model = fileItemModel();
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

    for (int index = 0; index < model->count(); index++) {
        const KFileItem item = model->fileItem(index);
        if (pattern.exactMatch(item.name())) {
            // An alternative approach would be to store the matching items in a QSet<int> and
            // select them in one go after the loop, but we'd need a new function
            // KItemListSelectionManager::setSelected(QSet<int>, SelectionMode mode)
            // for that.
            selectionManager->setSelected(index, 1, mode);
        }
    }
}

void DolphinView::setZoomLevel(int level)
{
    const int oldZoomLevel = zoomLevel();
    m_container->setZoomLevel(level);
    if (zoomLevel() != oldZoomLevel) {
        emit zoomLevelChanged(zoomLevel(), oldZoomLevel);
    }
}

int DolphinView::zoomLevel() const
{
    return m_container->zoomLevel();
}

void DolphinView::setSorting(Sorting sorting)
{
    if (sorting != this->sorting()) {
        updateSorting(sorting);
    }
}

DolphinView::Sorting DolphinView::sorting() const
{
    return DolphinView::SortByName;
    //return m_viewAccessor.proxyModel()->sorting();
}

void DolphinView::setSortOrder(Qt::SortOrder order)
{
    if (sortOrder() != order) {
        updateSortOrder(order);
    }
}

Qt::SortOrder DolphinView::sortOrder() const
{
    return Qt::AscendingOrder; // m_viewAccessor.proxyModel()->sortOrder();
}

void DolphinView::setSortFoldersFirst(bool foldersFirst)
{
    if (sortFoldersFirst() != foldersFirst) {
        updateSortFoldersFirst(foldersFirst);
    }
}

bool DolphinView::sortFoldersFirst() const
{
    return true; // m_viewAccessor.proxyModel()->sortFoldersFirst();
}

void DolphinView::setAdditionalInfoList(const QList<AdditionalInfo>& info)
{
    const QList<AdditionalInfo> previousList = info;

    ViewProperties props(url());
    props.setAdditionalInfoList(info);

    m_additionalInfoList = info;
    applyAdditionalInfoListToView();

    emit additionalInfoListChanged(m_additionalInfoList, previousList);
}

QList<DolphinView::AdditionalInfo> DolphinView::additionalInfoList() const
{
    return m_additionalInfoList;
}

void DolphinView::reload()
{
    QByteArray viewState;
    QDataStream saveStream(&viewState, QIODevice::WriteOnly);
    saveState(saveStream);
    m_selectedItems= selectedItems();

    setUrl(url());
    loadDirectory(url(), true);

    QDataStream restoreStream(viewState);
    restoreState(restoreStream);
}

void DolphinView::stopLoading()
{
    m_dirLister->stop();
}

void DolphinView::refresh()
{
    const bool oldActivationState = m_active;
    const int oldZoomLevel = zoomLevel();
    m_active = true;

    applyViewProperties();
    reload();

    setActive(oldActivationState);
    updateZoomLevel(oldZoomLevel);
}

void DolphinView::setNameFilter(const QString& nameFilter)
{
    Q_UNUSED(nameFilter);
    //m_viewModeController->setNameFilter(nameFilter);
}

QString DolphinView::nameFilter() const
{
    return QString(); //m_viewModeController->nameFilter();
}

void DolphinView::calculateItemCount(int& fileCount,
                                     int& folderCount,
                                     KIO::filesize_t& totalFileSize) const
{
    foreach (const KFileItem& item, m_dirLister->items()) {
        if (item.isDir()) {
            ++folderCount;
        } else {
            ++fileCount;
            totalFileSize += item.size();
        }
    }
}

QString DolphinView::statusBarText() const
{
    QString text;
    int folderCount = 0;
    int fileCount = 0;
    KIO::filesize_t totalFileSize = 0;

    if (hasSelection()) {
        // give a summary of the status of the selected files
        const KFileItemList list = selectedItems();
        if (list.isEmpty()) {
            // when an item is triggered, it is temporary selected but selectedItems()
            // will return an empty list
            return text;
        }

        KFileItemList::const_iterator it = list.begin();
        const KFileItemList::const_iterator end = list.end();
        while (it != end) {
            const KFileItem& item = *it;
            if (item.isDir()) {
                ++folderCount;
            } else {
                ++fileCount;
                totalFileSize += item.size();
            }
            ++it;
        }

        if (folderCount + fileCount == 1) {
            // if only one item is selected, show the filename
            const QString name = list.first().text();
            text = (folderCount == 1) ? i18nc("@info:status", "<filename>%1</filename> selected", name) :
                                        i18nc("@info:status", "<filename>%1</filename> selected (%2)",
                                              name, KIO::convertSize(totalFileSize));
        } else {
            // at least 2 items are selected
            const QString foldersText = i18ncp("@info:status", "1 Folder selected", "%1 Folders selected", folderCount);
            const QString filesText = i18ncp("@info:status", "1 File selected", "%1 Files selected", fileCount);
            if ((folderCount > 0) && (fileCount > 0)) {
                text = i18nc("@info:status folders, files (size)", "%1, %2 (%3)",
                             foldersText, filesText, KIO::convertSize(totalFileSize));
            } else if (fileCount > 0) {
                text = i18nc("@info:status files (size)", "%1 (%2)", filesText, KIO::convertSize(totalFileSize));
            } else {
                Q_ASSERT(folderCount > 0);
                text = foldersText;
            }
        }
    } else {
        calculateItemCount(fileCount, folderCount, totalFileSize);
        text = KIO::itemsSummaryString(fileCount + folderCount,
                                       fileCount, folderCount,
                                       totalFileSize, true);
    }

    return text;
}

QList<QAction*> DolphinView::versionControlActions(const KFileItemList& items) const
{
    Q_UNUSED(items);
    return QList<QAction*>(); //m_dolphinViewController->versionControlActions(items);
}

void DolphinView::setUrl(const KUrl& url)
{
    if (url == m_url) {
        return;
    }

    emit urlAboutToBeChanged(url);
    m_url = url;

    if (GeneralSettings::showToolTips()) {
        m_toolTipManager->hideToolTip();
    }

    // It is important to clear the items from the model before
    // applying the view properties, otherwise expensive operations
    // might be done on the existing items although they get cleared
    // anyhow afterwards by loadDirectory().
    fileItemModel()->clear();
    applyViewProperties();
    loadDirectory(url);

    emit urlChanged(url);
}

void DolphinView::selectAll()
{
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    selectionManager->setSelected(0, fileItemModel()->count());
}

void DolphinView::invertSelection()
{
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    selectionManager->setSelected(0, fileItemModel()->count(), KItemListSelectionManager::Toggle);
}

void DolphinView::clearSelection()
{
    m_container->controller()->selectionManager()->clearSelection();
}

void DolphinView::renameSelectedItems()
{
    KFileItemList items = selectedItems();
    const int itemCount = items.count();
    if (itemCount < 1) {
        return;
    }

    /*if ((itemCount == 1) && DolphinSettings::instance().generalSettings()->renameInline()) {
        const QModelIndex dirIndex = m_viewAccessor.dirModel()->indexForItem(items.first());
        const QModelIndex proxyIndex = m_viewAccessor.proxyModel()->mapFromSource(dirIndex);
        m_viewAccessor.itemView()->edit(proxyIndex);
    } else {*/
        RenameDialog* dialog = new RenameDialog(this, items);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    //}

    // assure that the current index remains visible when KDirLister
    // will notify the view about changed items
    m_assureVisibleCurrentIndex = true;
}

void DolphinView::trashSelectedItems()
{
    const KUrl::List list = simplifiedSelectedUrls();
    KonqOperations::del(this, KonqOperations::TRASH, list);
}

void DolphinView::deleteSelectedItems()
{
    const KUrl::List list = simplifiedSelectedUrls();
    const bool del = KonqOperations::askDeleteConfirmation(list,
                     KonqOperations::DEL,
                     KonqOperations::DEFAULT_CONFIRMATION,
                     this);

    if (del) {
        KIO::Job* job = KIO::del(list);
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotDeleteFileFinished(KJob*)));
    }
}

void DolphinView::cutSelectedItems()
{
    QMimeData* mimeData = selectionMimeData();
    KonqMimeData::addIsCutSelection(mimeData, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinView::copySelectedItems()
{
    QMimeData* mimeData = selectionMimeData();
    QApplication::clipboard()->setMimeData(mimeData);
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

void DolphinView::setPreviewsShown(bool show)
{
    if (previewsShown() == show) {
        return;
    }

    ViewProperties props(url());
    props.setPreviewsShown(show);

    m_container->setPreviewsShown(show);
    emit previewsShownChanged(show);
}

void DolphinView::setHiddenFilesShown(bool show)
{
    if (m_dirLister->showingDotFiles() == show) {
        return;
    }

    m_selectedItems = selectedItems();

    ViewProperties props(url());
    props.setHiddenFilesShown(show);

    m_dirLister->setShowingDotFiles(show);
    m_dirLister->emitChanges();
    emit hiddenFilesShownChanged(show);
}

void DolphinView::setCategorizedSorting(bool categorized)
{
    if (categorized == categorizedSorting()) {
        return;
    }

    ViewProperties props(url());
    props.setCategorizedSorting(categorized);
    props.save();

    //m_viewAccessor.proxyModel()->setCategorizedModel(categorized);

    emit categorizedSortingChanged(categorized);
}

void DolphinView::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    setActive(true);
}

void DolphinView::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);

    const QPoint pos = m_container->mapFromGlobal(QCursor::pos());
    const KItemListView* view = m_container->controller()->view();
    if (view->itemAt(pos) < 0) {
        // Only open the context-menu if the cursor is above the viewport
        // (the context-menu for items is handled in slotContextMenuRequested())
        requestContextMenu(KFileItem(), url(), QList<QAction*>());
    }
}

void DolphinView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const int numDegrees = event->delta() / 8;
        const int numSteps = numDegrees / 15;

        setZoomLevel(zoomLevel() + numSteps);
    }
    event->accept();
}

void DolphinView::activate()
{
    setActive(true);
}

void DolphinView::slotItemActivated(int index)
{
    Q_UNUSED(index);

    const KFileItemList items = selectedItems();
    if (items.isEmpty()) {
        return;
    }

    if (items.count() == 1) {
        emit itemActivated(items.at(0)); // caught by DolphinViewContainer or DolphinPart
    } else {
        foreach (const KFileItem& item, items) {
            if (item.isDir()) {
                emit tabRequested(item.url());
            } else {
                emit itemActivated(item);
            }
        }
    }
}

void DolphinView::slotItemMiddleClicked(int index)
{
    const KFileItem item = fileItemModel()->fileItem(index);
    if (item.isDir() || isTabsForFilesEnabled()) {
        emit tabRequested(item.url());
    }
}

void DolphinView::slotContextMenuRequested(int index, const QPointF& pos)
{
    Q_UNUSED(pos);
    if (GeneralSettings::showToolTips()) {
        m_toolTipManager->hideToolTip();
    }
    const KFileItem item = fileItemModel()->fileItem(index);
    emit requestContextMenu(item, url(), QList<QAction*>());
}

void DolphinView::slotItemExpansionToggleClicked(int index)
{
    // TODO: When doing a model->setExpanded(false) it should
    // be checked here whether the current index is part of the
    // closed sub-tree. If this is the case, the current index
    // should be adjusted to the parent index.
    KFileItemModel* model = fileItemModel();
    const bool expanded = model->isExpanded(index);
    model->setExpanded(index, !expanded);
}

void DolphinView::slotItemHovered(int index)
{
    const KFileItem item = fileItemModel()->fileItem(index);

    if (GeneralSettings::showToolTips()) {
        QRectF itemRect = m_container->controller()->view()->itemBoundingRect(index);
        const QPoint pos = m_container->mapToGlobal(itemRect.topLeft().toPoint());
        itemRect.moveTo(pos);

        m_toolTipManager->showToolTip(item, itemRect);
    }

    emit requestItemInfo(item);
}

void DolphinView::slotItemUnhovered(int index)
{
    Q_UNUSED(index);
    if (GeneralSettings::showToolTips()) {
        m_toolTipManager->hideToolTip();
    }
    emit requestItemInfo(KFileItem());
}

void DolphinView::slotSelectionChanged(const QSet<int>& current, const QSet<int>& previous)
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
    emit selectionChanged(selectedItems());
}

void DolphinView::openContextMenu(const QPoint& pos,
                                  const QList<QAction*>& customActions)
{
    KFileItem item;
    const int index = m_container->controller()->view()->itemAt(pos);
    if (index >= 0) {
        item = fileItemModel()->fileItem(index);
    }

    emit requestContextMenu(item, url(), customActions);
}

void DolphinView::dropUrls(const KFileItem& destItem,
                           const KUrl& destPath,
                           QDropEvent* event)
{
    Q_UNUSED(destItem);
    Q_UNUSED(destPath);
    markPastedUrlsAsSelected(event->mimeData());
    //DragAndDropHelper::instance().dropUrls(destItem, destPath, event, this);
}

void DolphinView::updateSorting(DolphinView::Sorting sorting)
{
    ViewProperties props(url());
    props.setSorting(sorting);

    KItemModelBase* model = m_container->controller()->model();
    model->setSortRole(sortRoleForSorting(sorting));

    emit sortingChanged(sorting);
}

void DolphinView::updateSortOrder(Qt::SortOrder order)
{
    ViewProperties props(url());
    props.setSortOrder(order);

    //m_viewAccessor.proxyModel()->setSortOrder(order);

    emit sortOrderChanged(order);
}

void DolphinView::updateSortFoldersFirst(bool foldersFirst)
{
    ViewProperties props(url());
    props.setSortFoldersFirst(foldersFirst);

    //m_viewAccessor.proxyModel()->setSortFoldersFirst(foldersFirst);

    emit sortFoldersFirstChanged(foldersFirst);
}

QPair<bool, QString> DolphinView::pasteInfo() const
{
    return KonqOperations::pasteInfo(url());
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
    // Restore the current item that had the keyboard focus
    stream >> m_currentItemIndex;

    // Restore the view position
    stream >> m_restoredContentsPosition;

    // Restore expanded folders (only relevant for the details view - will be ignored by the view in other view modes)
    QSet<KUrl> urlsToExpand;
    stream >> urlsToExpand;
    /*const DolphinDetailsViewExpander* expander = m_viewAccessor.setExpandedUrls(urlsToExpand);
    if (expander) {
        m_expanderActive = true;
        connect (expander, SIGNAL(completed()), this, SLOT(slotLoadingCompleted()));
    }
    else {
        m_expanderActive = false;
    }*/
}

void DolphinView::saveState(QDataStream& stream)
{
    // Save the current item that has the keyboard focus
    stream << m_container->controller()->selectionManager()->currentItem();

    // Save view position
    const qreal x = m_container->horizontalScrollBar()->value();
    const qreal y = m_container->verticalScrollBar()->value();
    stream << QPoint(x, y);

    // Save expanded folders (only relevant for the details view - the set will be empty in other view modes)
    //stream << m_viewAccessor.expandedUrls();
}

bool DolphinView::hasSelection() const
{
    return m_container->controller()->selectionManager()->hasSelection();
}

KFileItem DolphinView::rootItem() const
{
    return m_dirLister->rootItem();
}

void DolphinView::observeCreatedItem(const KUrl& url)
{
    m_createdItemUrl = url;
    //connect(m_dirModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
    //        this, SLOT(selectAndScrollToCreatedItem()));
}

void DolphinView::selectAndScrollToCreatedItem()
{
    /*const QModelIndex dirIndex = m_viewAccessor.dirModel()->indexForUrl(m_createdItemUrl);
    if (dirIndex.isValid()) {
        const QModelIndex proxyIndex = m_viewAccessor.proxyModel()->mapFromSource(dirIndex);
        QAbstractItemView* view = m_viewAccessor.itemView();
        if (view) {
            view->setCurrentIndex(proxyIndex);
        }
    }

    disconnect(m_viewAccessor.dirModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
               this, SLOT(selectAndScrollToCreatedItem()));*/
    m_createdItemUrl = KUrl();
}

void DolphinView::slotRedirection(const KUrl& oldUrl, const KUrl& newUrl)
{
    if (oldUrl.equals(url(), KUrl::CompareWithoutTrailingSlash)) {
        emit redirection(oldUrl, newUrl);
        m_url = newUrl; // #186947
    }
}

void DolphinView::updateViewState()
{
    if (m_currentItemIndex >= 0) {
        KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
        selectionManager->setCurrentItem(m_currentItemIndex);
        m_currentItemIndex =-1;
    }

    if (!m_restoredContentsPosition.isNull()) {
        const int x = m_restoredContentsPosition.x();
        const int y = m_restoredContentsPosition.y();
        m_restoredContentsPosition = QPoint();

        m_container->horizontalScrollBar()->setValue(x);
        m_container->verticalScrollBar()->setValue(y);
    }

    if (!m_selectedItems.isEmpty()) {
        KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
        QSet<int> selectedItems = selectionManager->selectedItems();
        const KFileItemModel* model = fileItemModel();

        foreach (const KFileItem& selectedItem, m_selectedItems) {
            const int index = model->index(selectedItem);
            if (index >= 0) {
                selectedItems.insert(index);
            }
        }

        selectionManager->setSelectedItems(selectedItems);
        m_selectedItems.clear();
    }
}

void DolphinView::showHoverInformation(const KFileItem& item)
{
    emit requestItemInfo(item);
}

void DolphinView::clearHoverInformation()
{
    emit requestItemInfo(KFileItem());
}

void DolphinView::slotDeleteFileFinished(KJob* job)
{
    if (job->error() == 0) {
        emit operationCompletedMessage(i18nc("@info:status", "Delete operation completed."));
    } else if (job->error() != KIO::ERR_USER_CANCELED) {
        emit errorMessage(job->errorString());
    }
}

void DolphinView::slotDirListerStarted(const KUrl& url)
{
    // Disable the writestate temporary until it can be determined in a fast way
    // in DolphinView::slotDirListerCompleted()
    if (m_isFolderWritable) {
        m_isFolderWritable = false;
        emit writeStateChanged(m_isFolderWritable);
    }

    emit startedPathLoading(url);
}

void DolphinView::slotDirListerCompleted()
{
    // Update the view-state. This has to be done using a Qt::QueuedConnection
    // because the view might not be in its final state yet (the view also
    // listens to the completed()-signal from KDirLister and the order of
    // of slots is undefined).
    QTimer::singleShot(0, this, SLOT(updateViewState()));

    emit finishedPathLoading(url());

    updateWritableState();
}

void DolphinView::slotRefreshItems()
{
    if (m_assureVisibleCurrentIndex) {
        m_assureVisibleCurrentIndex = false;
        //QAbstractItemView* view = m_viewAccessor.itemView();
        //if (view) {
        //    m_viewAccessor.itemView()->scrollTo(m_viewAccessor.itemView()->currentIndex());
        //}
    }
}

KFileItemModel* DolphinView::fileItemModel() const
{
    return static_cast<KFileItemModel*>(m_container->controller()->model());
}

void DolphinView::loadDirectory(const KUrl& url, bool reload)
{
    if (!url.isValid()) {
        const QString location(url.pathOrUrl());
        if (location.isEmpty()) {
            emit errorMessage(i18nc("@info:status", "The location is empty."));
        } else {
            emit errorMessage(i18nc("@info:status", "The location '%1' is invalid.", location));
        }
        return;
    }

    m_dirLister->openUrl(url, reload ? KDirLister::Reload : KDirLister::NoFlags);
}

void DolphinView::applyViewProperties()
{
    m_container->beginTransaction();

    const ViewProperties props(url());

    const Mode mode = props.viewMode();
    if (m_mode != mode) {
        const Mode previousMode = m_mode;
        m_mode = mode;

        // Changing the mode might result in changing
        // the zoom level. Remember the old zoom level so
        // that zoomLevelChanged() can get emitted.
        const int oldZoomLevel = m_container->zoomLevel();

        switch (m_mode) {
        case IconsView:   m_container->setItemLayout(KFileItemListView::IconsLayout); break;
        case CompactView: m_container->setItemLayout(KFileItemListView::CompactLayout); break;
        case DetailsView: m_container->setItemLayout(KFileItemListView::DetailsLayout); break;
        default: Q_ASSERT(false); break;
        }

        emit modeChanged(m_mode, previousMode);

        if (m_container->zoomLevel() != oldZoomLevel) {
            emit zoomLevelChanged(m_container->zoomLevel(), oldZoomLevel);
        }
    }

    const bool hiddenFilesShown = props.hiddenFilesShown();
    if (hiddenFilesShown != m_dirLister->showingDotFiles()) {
        m_dirLister->setShowingDotFiles(hiddenFilesShown);
        m_dirLister->emitChanges();
        emit hiddenFilesShownChanged(hiddenFilesShown);
    }

/*    m_storedCategorizedSorting = props.categorizedSorting();
    const bool categorized = m_storedCategorizedSorting && supportsCategorizedSorting();
    if (categorized != m_viewAccessor.proxyModel()->isCategorizedModel()) {
        m_viewAccessor.proxyModel()->setCategorizedModel(categorized);
        emit categorizedSortingChanged();
    }*/

    const DolphinView::Sorting sorting = props.sorting();
    KItemModelBase* model = m_container->controller()->model();
    const QByteArray newSortRole = sortRoleForSorting(sorting);
    if (newSortRole != model->sortRole()) {
        model->setSortRole(newSortRole);
        emit sortingChanged(sorting);
    }
/*
    const Qt::SortOrder sortOrder = props.sortOrder();
    if (sortOrder != m_viewAccessor.proxyModel()->sortOrder()) {
        m_viewAccessor.proxyModel()->setSortOrder(sortOrder);
        emit sortOrderChanged(sortOrder);
    }

    const bool sortFoldersFirst = props.sortFoldersFirst();
    if (sortFoldersFirst != m_viewAccessor.proxyModel()->sortFoldersFirst()) {
        m_viewAccessor.proxyModel()->setSortFoldersFirst(sortFoldersFirst);
        emit sortFoldersFirstChanged(sortFoldersFirst);
    }
*/
    const QList<DolphinView::AdditionalInfo> infoList = props.additionalInfoList();
    if (infoList != m_additionalInfoList) {
        const QList<DolphinView::AdditionalInfo> previousList = m_additionalInfoList;
        m_additionalInfoList = infoList;
        applyAdditionalInfoListToView();
        emit additionalInfoListChanged(m_additionalInfoList, previousList);
    }

    const bool previewsShown = props.previewsShown();
    if (previewsShown != m_container->previewsShown()) {
        const int oldZoomLevel = zoomLevel();

        m_container->setPreviewsShown(previewsShown);
        emit previewsShownChanged(previewsShown);

        // Changing the preview-state might result in a changed zoom-level
        if (oldZoomLevel != zoomLevel()) {
            emit zoomLevelChanged(zoomLevel(), oldZoomLevel);
        }
    }

    m_container->endTransaction();
}

void DolphinView::applyAdditionalInfoListToView()
{
    const AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();

    QHash<QByteArray, int> visibleRoles;
    visibleRoles.insert("name", 0);

    int index = 1;
    foreach (AdditionalInfo info, m_additionalInfoList) {
        visibleRoles.insert(infoAccessor.role(info), index);
        ++index;
    }

    m_container->setVisibleRoles(visibleRoles);
}

void DolphinView::pasteToUrl(const KUrl& url)
{
    markPastedUrlsAsSelected(QApplication::clipboard()->mimeData());
    KonqOperations::doPaste(this, url);
}

void DolphinView::updateZoomLevel(int oldZoomLevel)
{
    Q_UNUSED(oldZoomLevel);
 /*   const int newZoomLevel = ZoomLevelInfo::zoomLevelForIconSize(m_viewAccessor.itemView()->iconSize());
    if (oldZoomLevel != newZoomLevel) {
        m_viewModeController->setZoomLevel(newZoomLevel);
        emit zoomLevelChanged(newZoomLevel);
    }*/
}

KUrl::List DolphinView::simplifiedSelectedUrls() const
{
    KUrl::List urls;

    const KFileItemList items = selectedItems();
    foreach (const KFileItem &item, items) {
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
    const KFileItemModel* model = fileItemModel();
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    const QSet<int> selectedIndexes = selectionManager->selectedItems();

    return model->createMimeData(selectedIndexes);
}

void DolphinView::markPastedUrlsAsSelected(const QMimeData* mimeData)
{
    const KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    markUrlsAsSelected(urls);
}

void DolphinView::updateWritableState()
{
    const bool wasFolderWritable = m_isFolderWritable;
    m_isFolderWritable = true;

    const KFileItem item = m_dirLister->rootItem();
    if (!item.isNull()) {
        KFileItemListProperties capabilities(KFileItemList() << item);
        m_isFolderWritable = capabilities.supportsWriting();
    }
    if (m_isFolderWritable != wasFolderWritable) {
        emit writeStateChanged(m_isFolderWritable);
    }
}

QByteArray DolphinView::sortRoleForSorting(Sorting sorting) const
{
    switch (sorting) {
    case SortByName:        return "name";
    case SortBySize:        return "size";
    case SortByDate:        return "date";
    case SortByPermissions: return "permissions";
    case SortByOwner:       return "owner";
    case SortByGroup:       return "group";
    case SortByType:        return "type";
    case SortByDestination: return "destination";
    case SortByPath:        return "path";
    default: break;
    }

    return QByteArray();
}

#include "dolphinview.moc"
