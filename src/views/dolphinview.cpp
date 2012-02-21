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
#include <QBoxLayout>
#include <QClipboard>
#include <QDropEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QKeyEvent>
#include <QItemSelection>
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
#include "draganddrophelper.h"
#include "renamedialog.h"
#include "versioncontrol/versioncontrolobserver.h"
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
    m_currentItemUrl(),
    m_restoredContentsPosition(),
    m_createdItemUrl(),
    m_selectedUrls(),
    m_versionControlObserver(0)
{
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

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
    m_container->setVisibleRoles(QList<QByteArray>() << "name");
    m_container->installEventFilter(this);
    setFocusProxy(m_container);
    connect(m_container->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hideToolTip()));
    connect(m_container->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hideToolTip()));

    KItemListController* controller = m_container->controller();
    controller->setSelectionBehavior(KItemListController::MultiSelection);
    connect(controller, SIGNAL(itemActivated(int)), this, SLOT(slotItemActivated(int)));
    connect(controller, SIGNAL(itemsActivated(QSet<int>)), this, SLOT(slotItemsActivated(QSet<int>)));
    connect(controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));
    connect(controller, SIGNAL(itemContextMenuRequested(int,QPointF)), this, SLOT(slotItemContextMenuRequested(int,QPointF)));
    connect(controller, SIGNAL(viewContextMenuRequested(QPointF)), this, SLOT(slotViewContextMenuRequested(QPointF)));
    connect(controller, SIGNAL(headerContextMenuRequested(QPointF)), this, SLOT(slotHeaderContextMenuRequested(QPointF)));
    connect(controller, SIGNAL(itemPressed(int,Qt::MouseButton)), this, SLOT(hideToolTip()));
    connect(controller, SIGNAL(itemHovered(int)), this, SLOT(slotItemHovered(int)));
    connect(controller, SIGNAL(itemUnhovered(int)), this, SLOT(slotItemUnhovered(int)));
    connect(controller, SIGNAL(itemDropEvent(int,QGraphicsSceneDragDropEvent*)), this, SLOT(slotItemDropEvent(int,QGraphicsSceneDragDropEvent*)));
    connect(controller, SIGNAL(modelChanged(KItemModelBase*,KItemModelBase*)), this, SLOT(slotModelChanged(KItemModelBase*,KItemModelBase*)));

    KFileItemModel* model = fileItemModel();
    if (model) {
        connect(model, SIGNAL(loadingCompleted()), this, SLOT(slotLoadingCompleted()));
    }

    KItemListView* view = controller->view();
    connect(view, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
            this, SLOT(slotSortOrderChangedByHeader(Qt::SortOrder,Qt::SortOrder)));
    connect(view, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
            this, SLOT(slotSortRoleChangedByHeader(QByteArray,QByteArray)));

    KItemListSelectionManager* selectionManager = controller->selectionManager();
    connect(selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)),
            this, SLOT(slotSelectionChanged(QSet<int>,QSet<int>)));

    m_toolTipManager = new ToolTipManager(this);

    m_versionControlObserver = new VersionControlObserver(this);
    m_versionControlObserver->setModel(model);
    connect(m_versionControlObserver, SIGNAL(infoMessage(QString)), this, SIGNAL(infoMessage(QString)));
    connect(m_versionControlObserver, SIGNAL(errorMessage(QString)), this, SIGNAL(errorMessage(QString)));
    connect(m_versionControlObserver, SIGNAL(operationCompletedMessage(QString)), this, SIGNAL(operationCompletedMessage(QString)));

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

    QWidget* viewport = m_container->viewport();
    if (viewport) {
        QPalette palette;
        palette.setColor(viewport->backgroundRole(), color);
        viewport->setPalette(palette);
    }

    update();

    if (active) {
        m_container->setFocus();
        emit activated();
        emit writeStateChanged(m_isFolderWritable);
    }
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

bool DolphinView::previewsShown() const
{
    return m_container->previewsShown();
}

void DolphinView::setHiddenFilesShown(bool show)
{
    if (m_dirLister->showingDotFiles() == show) {
        return;
    }

    const KFileItemList itemList = selectedItems();
    m_selectedUrls.clear();
    m_selectedUrls = itemList.urlList();

    ViewProperties props(url());
    props.setHiddenFilesShown(show);

    fileItemModel()->setShowHiddenFiles(show);
    emit hiddenFilesShownChanged(show);
}

bool DolphinView::hiddenFilesShown() const
{
    return m_dirLister->showingDotFiles();
}

void DolphinView::setGroupedSorting(bool grouped)
{
    if (grouped == groupedSorting()) {
        return;
    }

    ViewProperties props(url());
    props.setGroupedSorting(grouped);
    props.save();

    m_container->controller()->model()->setGroupedSorting(grouped);

    emit groupedSortingChanged(grouped);
}

bool DolphinView::groupedSorting() const
{
    return fileItemModel()->groupedSorting();
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
    m_selectedUrls = urls;
}

void DolphinView::markUrlAsCurrent(const KUrl& url)
{
    m_currentItemUrl = url;
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
        if (pattern.exactMatch(item.text())) {
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
    KItemModelBase* model = m_container->controller()->model();
    return sortingForSortRole(model->sortRole());
}

void DolphinView::setSortOrder(Qt::SortOrder order)
{
    if (sortOrder() != order) {
        updateSortOrder(order);
    }
}

Qt::SortOrder DolphinView::sortOrder() const
{
    KItemModelBase* model = fileItemModel();
    return model->sortOrder();
}

void DolphinView::setSortFoldersFirst(bool foldersFirst)
{
    if (sortFoldersFirst() != foldersFirst) {
        updateSortFoldersFirst(foldersFirst);
    }
}

bool DolphinView::sortFoldersFirst() const
{
    KFileItemModel* model = fileItemModel();
    return model->sortFoldersFirst();
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

    const KFileItemList itemList = selectedItems();
    m_selectedUrls.clear();
    m_selectedUrls = itemList.urlList();

    setUrl(url());
    loadDirectory(url(), true);

    QDataStream restoreStream(viewState);
    restoreState(restoreStream);
}

void DolphinView::stopLoading()
{
    m_dirLister->stop();
}

void DolphinView::readSettings()
{
    const int oldZoomLevel = m_container->zoomLevel();
    
    GeneralSettings::self()->readConfig();
    m_container->readSettings();
    applyViewProperties();
    
    const int newZoomLevel = m_container->zoomLevel();
    if (newZoomLevel != oldZoomLevel) {
        emit zoomLevelChanged(newZoomLevel, oldZoomLevel);
    }
}

void DolphinView::writeSettings()
{
    GeneralSettings::self()->writeConfig();
    m_container->writeSettings();
}

void DolphinView::setNameFilter(const QString& nameFilter)
{
    fileItemModel()->setNameFilter(nameFilter);
}

QString DolphinView::nameFilter() const
{
    return fileItemModel()->nameFilter();
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
    QString summary;
    QString foldersText;
    QString filesText;

    int folderCount = 0;
    int fileCount = 0;
    KIO::filesize_t totalFileSize = 0;

    if (hasSelection()) {
        // Give a summary of the status of the selected files
        const KFileItemList list = selectedItems();
        foreach (const KFileItem& item, list) {
            if (item.isDir()) {
                ++folderCount;
            } else {
                ++fileCount;
                totalFileSize += item.size();
            }
        }

        if (folderCount + fileCount == 1) {
            // If only one item is selected, show the filename
            filesText = i18nc("@info:status", "<filename>%1</filename> selected", list.first().text());
        } else {
            // At least 2 items are selected
            foldersText = i18ncp("@info:status", "1 Folder selected", "%1 Folders selected", folderCount);
            filesText = i18ncp("@info:status", "1 File selected", "%1 Files selected", fileCount);
        }
    } else {
        calculateItemCount(fileCount, folderCount, totalFileSize);
        foldersText = i18ncp("@info:status", "1 Folder", "%1 Folders", folderCount);
        filesText = i18ncp("@info:status", "1 File", "%1 Files", fileCount);
    }

    if (fileCount > 0 && folderCount > 0) {
        summary = i18nc("@info:status folders, files (size)", "%1, %2 (%3)",
                        foldersText, filesText, fileSizeText(totalFileSize));
    } else if (fileCount > 0) {
        summary = i18nc("@info:status files (size)", "%1 (%2)", filesText, fileSizeText(totalFileSize));
    } else if (folderCount > 0) {
        summary = foldersText;
    }

    return summary;
}

QList<QAction*> DolphinView::versionControlActions(const KFileItemList& items) const
{
    QList<QAction*> actions;

    if (items.isEmpty()) {
        const KFileItem item = fileItemModel()->rootItem();
        actions = m_versionControlObserver->actions(KFileItemList() << item);
    } else {
        actions = m_versionControlObserver->actions(items);
    }

    return actions;
}

void DolphinView::setUrl(const KUrl& url)
{
    if (url == m_url) {
        return;
    }

    emit urlAboutToBeChanged(url);
    m_url = url;

    hideToolTip();

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

    // TODO: The new view-engine introduced with Dolphin 2.0 does not support inline
    // renaming yet.
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

bool DolphinView::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        if (watched == m_container) {
            setActive(true);
        }
        break;

    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

void DolphinView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const int numDegrees = event->delta() / 8;
        const int numSteps = numDegrees / 15;

        setZoomLevel(zoomLevel() + numSteps);
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

void DolphinView::activate()
{
    setActive(true);
}

void DolphinView::slotItemActivated(int index)
{
    const KFileItem item = fileItemModel()->fileItem(index);
    if (!item.isNull()) {
        emit itemActivated(item);
    }
}

void DolphinView::slotItemsActivated(const QSet<int>& indexes)
{
    Q_ASSERT(indexes.count() >= 2);

    KFileItemList items;

    KFileItemModel* model = fileItemModel();
    QSetIterator<int> it(indexes);
    while (it.hasNext()) {
        const int index = it.next();
        items.append(model->fileItem(index));
    }

    foreach (const KFileItem& item, items) {
        if (item.isDir()) {
            emit tabRequested(item.url());
        } else {
            emit itemActivated(item);
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

void DolphinView::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    const KFileItem item = fileItemModel()->fileItem(index);
    emit requestContextMenu(pos.toPoint(), item, url(), QList<QAction*>());
}

void DolphinView::slotViewContextMenuRequested(const QPointF& pos)
{
    emit requestContextMenu(pos.toPoint(), KFileItem(), url(), QList<QAction*>());
}

void DolphinView::slotHeaderContextMenuRequested(const QPointF& pos)
{
    QWeakPointer<KMenu> menu = new KMenu(QApplication::activeWindow());

    KItemListView* view = m_container->controller()->view();
    const QSet<QByteArray> visibleRolesSet = view->visibleRoles().toSet();

    // Add all roles to the menu that can be shown or hidden by the user
    const AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
    const QList<DolphinView::AdditionalInfo> keys = infoAccessor.keys();
    foreach (const DolphinView::AdditionalInfo info, keys) {
        const QByteArray& role = infoAccessor.role(info);
        if (role != "name") {
            const QString text = fileItemModel()->roleDescription(role);

            QAction* action = menu.data()->addAction(text);
            action->setCheckable(true);
            action->setChecked(visibleRolesSet.contains(role));
            action->setData(info);
        }
    }

    QAction* action = menu.data()->exec(pos.toPoint());
    if (action) {
        // Show or hide the selected role
        const DolphinView::AdditionalInfo info =
            static_cast<DolphinView::AdditionalInfo>(action->data().toInt());

        ViewProperties props(url());
        QList<DolphinView::AdditionalInfo> infoList = props.additionalInfoList();

        const QByteArray selectedRole = infoAccessor.role(info);
        QList<QByteArray> visibleRoles = view->visibleRoles();

        if (action->isChecked()) {
            const int index = keys.indexOf(info) + 1;
            visibleRoles.insert(index, selectedRole);
            infoList.insert(index, info);
        } else {
            visibleRoles.removeOne(selectedRole);
            infoList.removeOne(info);
        }

        view->setVisibleRoles(visibleRoles);
        props.setAdditionalInfoList(infoList);
    }

    delete menu.data();
}

void DolphinView::slotItemHovered(int index)
{
    const KFileItem item = fileItemModel()->fileItem(index);

    if (GeneralSettings::showToolTips() && QApplication::mouseButtons() == Qt::NoButton) {
        QRectF itemRect = m_container->controller()->view()->itemContextRect(index);
        const QPoint pos = m_container->mapToGlobal(itemRect.topLeft().toPoint());
        itemRect.moveTo(pos);

        m_toolTipManager->showToolTip(item, itemRect);
    }

    emit requestItemInfo(item);
}

void DolphinView::slotItemUnhovered(int index)
{
    Q_UNUSED(index);
    hideToolTip();
    emit requestItemInfo(KFileItem());
}

void DolphinView::slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event)
{
    KUrl destUrl;
    KFileItem destItem = fileItemModel()->fileItem(index);
    if (destItem.isNull() || (!destItem.isDir() && !destItem.isDesktopFile())) {
        // Use the URL of the view as drop target if the item is no directory
        // or desktop-file
        destItem = fileItemModel()->rootItem();
        destUrl = url();
    } else {
        // The item represents a directory or desktop-file
        destUrl = destItem.url();
    }

    QDropEvent dropEvent(event->pos().toPoint(),
                         event->possibleActions(),
                         event->mimeData(),
                         event->buttons(),
                         event->modifiers());

    const QString error = DragAndDropHelper::dropUrls(destItem, destUrl, &dropEvent);
    if (!error.isEmpty()) {
        emit errorMessage(error);
    }
}

void DolphinView::slotModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    if (previous != 0) {
        disconnect(previous, SIGNAL(loadingCompleted()), this, SLOT(slotLoadingCompleted()));
    }

    Q_ASSERT(qobject_cast<KFileItemModel*>(current));
    connect(current, SIGNAL(loadingCompleted()), this, SLOT(slotLoadingCompleted()));

    KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(current);
    m_versionControlObserver->setModel(fileItemModel);
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

    KItemModelBase* model = fileItemModel();
    model->setSortOrder(order);

    emit sortOrderChanged(order);
}

void DolphinView::updateSortFoldersFirst(bool foldersFirst)
{
    ViewProperties props(url());
    props.setSortFoldersFirst(foldersFirst);

    KFileItemModel* model = fileItemModel();
    model->setSortFoldersFirst(foldersFirst);

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
    stream >> m_currentItemUrl;

    // Restore the view position
    stream >> m_restoredContentsPosition;

    // Restore expanded folders (only relevant for the details view - will be ignored by the view in other view modes)
    QSet<KUrl> urls;
    stream >> urls;
    fileItemModel()->restoreExpandedUrls(urls);
}

void DolphinView::saveState(QDataStream& stream)
{
    // Save the current item that has the keyboard focus
    const int currentIndex = m_container->controller()->selectionManager()->currentItem();
    if (currentIndex != -1) {
        KFileItem item = fileItemModel()->fileItem(currentIndex);
        Q_ASSERT(!item.isNull()); // If the current index is valid a item must exist
        KUrl currentItemUrl = item.url();
        stream << currentItemUrl;
    } else {
        stream << KUrl();
    }

    // Save view position
    const qreal x = m_container->horizontalScrollBar()->value();
    const qreal y = m_container->verticalScrollBar()->value();
    stream << QPoint(x, y);

    // Save expanded folders (only relevant for the details view - the set will be empty in other view modes)
    stream << fileItemModel()->expandedUrls();
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
    if (m_currentItemUrl != KUrl()) {
        KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
        const int currentIndex = fileItemModel()->index(m_currentItemUrl);
        if (currentIndex != -1) {
            selectionManager->setCurrentItem(currentIndex);
        } else {
            selectionManager->setCurrentItem(0);
        }
        m_currentItemUrl = KUrl();
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
        QSet<int> selectedItems = selectionManager->selectedItems();
        const KFileItemModel* model = fileItemModel();

        foreach (const KUrl& url, m_selectedUrls) {
            const int index = model->index(url);
            if (index >= 0) {
                selectedItems.insert(index);
            }
        }

        selectionManager->setSelectedItems(selectedItems);
        m_selectedUrls.clear();
    }
}

void DolphinView::hideToolTip()
{
    if (GeneralSettings::showToolTips()) {
        m_toolTipManager->hideToolTip();
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
    // in DolphinView::slotLoadingCompleted()
    if (m_isFolderWritable) {
        m_isFolderWritable = false;
        emit writeStateChanged(m_isFolderWritable);
    }

    emit startedPathLoading(url);
}

void DolphinView::slotLoadingCompleted()
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

void DolphinView::slotSortOrderChangedByHeader(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(previous);
    Q_ASSERT(fileItemModel()->sortOrder() == current);

    ViewProperties props(url());
    props.setSortOrder(current);

    emit sortOrderChanged(current);
}

void DolphinView::slotSortRoleChangedByHeader(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous);
    Q_ASSERT(fileItemModel()->sortRole() == current);

    ViewProperties props(url());
    const Sorting sorting = sortingForSortRole(current);
    props.setSorting(sorting);

    emit sortingChanged(sorting);
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
    KFileItemModel* model = fileItemModel();

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
    if (hiddenFilesShown != model->showHiddenFiles()) {
        model->setShowHiddenFiles(hiddenFilesShown);
        emit hiddenFilesShownChanged(hiddenFilesShown);
    }

    const bool groupedSorting = props.groupedSorting();
    if (groupedSorting != model->groupedSorting()) {
        model->setGroupedSorting(groupedSorting);
        emit groupedSortingChanged(groupedSorting);
    }

    const DolphinView::Sorting sorting = props.sorting();
    const QByteArray newSortRole = sortRoleForSorting(sorting);
    if (newSortRole != model->sortRole()) {
        model->setSortRole(newSortRole);
        emit sortingChanged(sorting);
    }

    const Qt::SortOrder sortOrder = props.sortOrder();
    if (sortOrder != model->sortOrder()) {
        model->setSortOrder(sortOrder);
        emit sortOrderChanged(sortOrder);
    }

    const bool sortFoldersFirst = props.sortFoldersFirst();
    if (sortFoldersFirst != model->sortFoldersFirst()) {
        model->setSortFoldersFirst(sortFoldersFirst);
        emit sortFoldersFirstChanged(sortFoldersFirst);
    }

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

    QList<QByteArray> visibleRoles;
    visibleRoles.reserve(m_additionalInfoList.count() + 1);
    visibleRoles.append("name");

    foreach (AdditionalInfo info, m_additionalInfoList) {
        visibleRoles.append(infoAccessor.role(info));
    }

    m_container->setVisibleRoles(visibleRoles);
}

void DolphinView::pasteToUrl(const KUrl& url)
{
    markPastedUrlsAsSelected(QApplication::clipboard()->mimeData());
    KonqOperations::doPaste(this, url);
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

DolphinView::Sorting DolphinView::sortingForSortRole(const QByteArray& sortRole) const
{
    static QHash<QByteArray, DolphinView::Sorting> sortHash;
    if (sortHash.isEmpty()) {
        sortHash.insert("name", SortByName);
        sortHash.insert("size", SortBySize);
        sortHash.insert("date", SortByDate);
        sortHash.insert("permissions", SortByPermissions);
        sortHash.insert("owner", SortByOwner);
        sortHash.insert("group", SortByGroup);
        sortHash.insert("type", SortByType);
        sortHash.insert("destination", SortByDestination);
        sortHash.insert("path", SortByPath);
    }
    return sortHash.value(sortRole);
}

QString DolphinView::fileSizeText(KIO::filesize_t fileSize)
{
    const KLocale* locale = KGlobal::locale();
    const unsigned int multiplier = (locale->binaryUnitDialect() == KLocale::MetricBinaryDialect)
                                    ? 1000 : 1024;

    QString text;
    if (fileSize < multiplier) {
        // Show the size in bytes
        text = locale->formatByteSize(fileSize, 0, KLocale::DefaultBinaryDialect, KLocale::UnitByte);
    } else if (fileSize < multiplier * multiplier) {
        // Show the size in kilobytes and always round up. This is done
        // for consistency with the values shown e.g. in the "Size" column
        // of the details-view.
        fileSize += (multiplier / 2) - 1;
        text = locale->formatByteSize(fileSize, 0, KLocale::DefaultBinaryDialect, KLocale::UnitKiloByte);
    } else {
        // Show the size in the best fitting unit having one decimal
        text = locale->formatByteSize(fileSize, 1);
    }
    return text;
}

#include "dolphinview.moc"
