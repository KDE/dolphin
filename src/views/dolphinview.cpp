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

#include <config-nepomuk.h>

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
#include <KDirModel>
#include <KIconEffect>
#include <KFileItem>
#include <KFileItemListProperties>
#include <KLocale>
#include <kitemviews/kfileitemmodel.h>
#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistheader.h>
#include <kitemviews/kitemlistselectionmanager.h>
#include <kitemviews/kitemlistview.h>
#include <kitemviews/kitemlistcontroller.h>
#include <KIO/DeleteJob>
#include <KIO/JobUiDelegate>
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

#include "dolphinnewfilemenuobserver.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphinitemlistview.h"
#include "draganddrophelper.h"
#include "renamedialog.h"
#include "versioncontrol/versioncontrolobserver.h"
#include "viewmodecontroller.h"
#include "viewproperties.h"
#include "views/tooltips/tooltipmanager.h"
#include "zoomlevelinfo.h"

#ifdef HAVE_NEPOMUK
    #include <Nepomuk/ResourceManager>
#endif

namespace {
    const int MaxModeEnum = DolphinView::CompactView;
};

DolphinView::DolphinView(const KUrl& url, QWidget* parent) :
    QWidget(parent),
    m_active(true),
    m_tabsForFiles(false),
    m_assureVisibleCurrentIndex(false),
    m_isFolderWritable(true),
    m_dragging(false),
    m_url(url),
    m_mode(DolphinView::IconsView),
    m_visibleRoles(),
    m_topLayout(0),
    m_model(0),
    m_view(0),
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

    m_model = new KFileItemModel(this);
    m_view = new DolphinItemListView();
    m_view->setEnabledSelectionToggles(GeneralSettings::showSelectionToggle());
    m_view->setEnlargeSmallPreviews(GeneralSettings::enlargeSmallPreviews());
    m_view->setVisibleRoles(QList<QByteArray>() << "text");
    applyModeToView();

    KItemListController* controller = new KItemListController(m_model, m_view, this);
    const int delay = GeneralSettings::autoExpandFolders() ? 750 : -1;
    controller->setAutoActivationDelay(delay);

    m_container = new KItemListContainer(controller, this);
    m_container->installEventFilter(this);
    setFocusProxy(m_container);
    connect(m_container->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hideToolTip()));
    connect(m_container->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hideToolTip()));

    controller->setSelectionBehavior(KItemListController::MultiSelection);
    connect(controller, SIGNAL(itemActivated(int)), this, SLOT(slotItemActivated(int)));
    connect(controller, SIGNAL(itemsActivated(QSet<int>)), this, SLOT(slotItemsActivated(QSet<int>)));
    connect(controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));
    connect(controller, SIGNAL(itemContextMenuRequested(int,QPointF)), this, SLOT(slotItemContextMenuRequested(int,QPointF)));
    connect(controller, SIGNAL(viewContextMenuRequested(QPointF)), this, SLOT(slotViewContextMenuRequested(QPointF)));
    connect(controller, SIGNAL(headerContextMenuRequested(QPointF)), this, SLOT(slotHeaderContextMenuRequested(QPointF)));
    connect(controller, SIGNAL(mouseButtonPressed(int,Qt::MouseButtons)), this, SLOT(slotMouseButtonPressed(int,Qt::MouseButtons)));
    connect(controller, SIGNAL(itemHovered(int)), this, SLOT(slotItemHovered(int)));
    connect(controller, SIGNAL(itemUnhovered(int)), this, SLOT(slotItemUnhovered(int)));
    connect(controller, SIGNAL(itemDropEvent(int,QGraphicsSceneDragDropEvent*)), this, SLOT(slotItemDropEvent(int,QGraphicsSceneDragDropEvent*)));
    connect(controller, SIGNAL(modelChanged(KItemModelBase*,KItemModelBase*)), this, SLOT(slotModelChanged(KItemModelBase*,KItemModelBase*)));

    connect(m_model, SIGNAL(directoryLoadingStarted()),       this, SLOT(slotDirectoryLoadingStarted()));
    connect(m_model, SIGNAL(directoryLoadingCompleted()),     this, SLOT(slotDirectoryLoadingCompleted()));
    connect(m_model, SIGNAL(directoryLoadingProgress(int)),   this, SIGNAL(directoryLoadingProgress(int)));
    connect(m_model, SIGNAL(directorySortingProgress(int)),   this, SIGNAL(directorySortingProgress(int)));
    connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
            this, SLOT(slotItemsChanged()));
    connect(m_model, SIGNAL(itemsRemoved(KItemRangeList)),    this, SIGNAL(itemCountChanged()));
    connect(m_model, SIGNAL(itemsInserted(KItemRangeList)),   this, SIGNAL(itemCountChanged()));
    connect(m_model, SIGNAL(infoMessage(QString)),            this, SIGNAL(infoMessage(QString)));
    connect(m_model, SIGNAL(errorMessage(QString)),           this, SIGNAL(errorMessage(QString)));
    connect(m_model, SIGNAL(directoryRedirection(KUrl,KUrl)), this, SLOT(slotDirectoryRedirection(KUrl,KUrl)));

    m_view->installEventFilter(this);
    connect(m_view, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
            this, SLOT(slotSortOrderChangedByHeader(Qt::SortOrder,Qt::SortOrder)));
    connect(m_view, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
            this, SLOT(slotSortRoleChangedByHeader(QByteArray,QByteArray)));
    connect(m_view, SIGNAL(visibleRolesChanged(QList<QByteArray>,QList<QByteArray>)),
            this, SLOT(slotVisibleRolesChangedByHeader(QList<QByteArray>,QList<QByteArray>)));
    connect(m_view, SIGNAL(roleEditingFinished(int,QByteArray,QVariant)),
            this, SLOT(slotRoleEditingFinished(int,QByteArray,QVariant)));
    connect(m_view->header(), SIGNAL(columnWidthChanged(QByteArray,qreal,qreal)),
            this, SLOT(slotHeaderColumnWidthChanged(QByteArray,qreal,qreal)));

    KItemListSelectionManager* selectionManager = controller->selectionManager();
    connect(selectionManager, SIGNAL(selectionChanged(QSet<int>,QSet<int>)),
            this, SLOT(slotSelectionChanged(QSet<int>,QSet<int>)));

    m_toolTipManager = new ToolTipManager(this);

    m_versionControlObserver = new VersionControlObserver(this);
    m_versionControlObserver->setModel(m_model);
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

    m_view->setPreviewsShown(show);
    emit previewsShownChanged(show);
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

    ViewProperties props(url());
    props.setHiddenFilesShown(show);

    m_model->setShowHiddenFiles(show);
    emit hiddenFilesShownChanged(show);
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

    ViewProperties props(url());
    props.setGroupedSorting(grouped);
    props.save();

    m_container->controller()->model()->setGroupedSorting(grouped);

    emit groupedSortingChanged(grouped);
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
    const QSet<int> selectedIndexes = selectionManager->selectedItems();

    KFileItemList selectedItems;
    QSetIterator<int> it(selectedIndexes);
    while (it.hasNext()) {
        const int index = it.next();
        selectedItems.append(m_model->fileItem(index));
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

void DolphinView::selectItems(const QRegExp& pattern, bool enabled)
{
    const KItemListSelectionManager::SelectionMode mode = enabled
                                                        ? KItemListSelectionManager::Select
                                                        : KItemListSelectionManager::Deselect;
    KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();

    for (int index = 0; index < m_model->count(); index++) {
        const KFileItem item = m_model->fileItem(index);
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
    m_view->setZoomLevel(level);
    if (zoomLevel() != oldZoomLevel) {
        emit zoomLevelChanged(zoomLevel(), oldZoomLevel);
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

    ViewProperties props(url());
    props.setVisibleRoles(roles);

    m_visibleRoles = roles;
    m_view->setVisibleRoles(roles);

    emit visibleRolesChanged(m_visibleRoles, previousRoles);
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
    m_model->cancelDirectoryLoading();
}

void DolphinView::readSettings()
{
    const int oldZoomLevel = m_view->zoomLevel();

    GeneralSettings::self()->readConfig();
    m_view->readSettings();
    applyViewProperties();

    const int delay = GeneralSettings::autoExpandFolders() ? 750 : -1;
    m_container->controller()->setAutoActivationDelay(delay);

    const int newZoomLevel = m_view->zoomLevel();
    if (newZoomLevel != oldZoomLevel) {
        emit zoomLevelChanged(newZoomLevel, oldZoomLevel);
    }
}

void DolphinView::writeSettings()
{
    GeneralSettings::self()->writeConfig();
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

QString DolphinView::statusBarText() const
{
    QString summary;
    QString foldersText;
    QString filesText;

    int folderCount = 0;
    int fileCount = 0;
    KIO::filesize_t totalFileSize = 0;

    if (m_container->controller()->selectionManager()->hasSelection()) {
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
                        foldersText, filesText,
                        KGlobal::locale()->formatByteSize(totalFileSize));
    } else if (fileCount > 0) {
        summary = i18nc("@info:status files (size)", "%1 (%2)",
                        filesText,
                        KGlobal::locale()->formatByteSize(totalFileSize));
    } else if (folderCount > 0) {
        summary = foldersText;
    } else {
        summary = i18nc("@info:status", "0 Folders, 0 Files");
    }

    return summary;
}

QList<QAction*> DolphinView::versionControlActions(const KFileItemList& items) const
{
    QList<QAction*> actions;

    if (items.isEmpty()) {
        const KFileItem item = m_model->rootItem();
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
    m_model->clear();
    applyViewProperties();
    loadDirectory(url);

    emit urlChanged(url);
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
    m_container->controller()->selectionManager()->clearSelection();
}

void DolphinView::renameSelectedItems()
{
    const KFileItemList items = selectedItems();
     if (items.isEmpty()) {
         return;
     }

     if (items.count() == 1) {
         const int index = m_model->index(items.first());
         m_container->controller()->view()->editRole(index, "text");
     } else {
         RenameDialog* dialog = new RenameDialog(this, items);
         dialog->setAttribute(Qt::WA_DeleteOnClose);
         dialog->show();
         dialog->raise();
         dialog->activateWindow();
     }

     // Assure that the current index remains visible when KFileItemModel
     // will notify the view about changed items (which might result in
     // a changed sorting).
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
        if (job->ui()) {
            job->ui()->setWindow(this);
        }
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

    case QEvent::GraphicsSceneDragEnter:
        if (watched == m_view) {
            m_dragging = true;
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
    const KFileItem item = m_model->fileItem(index);
    if (!item.isNull()) {
        emit itemActivated(item);
    }
}

void DolphinView::slotItemsActivated(const QSet<int>& indexes)
{
    Q_ASSERT(indexes.count() >= 2);

    KFileItemList items;

    QSetIterator<int> it(indexes);
    while (it.hasNext()) {
        const int index = it.next();
        items.append(m_model->fileItem(index));
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
    const KFileItem item = m_model->fileItem(index);
    if (item.isDir() || isTabsForFilesEnabled()) {
        emit tabRequested(item.url());
    }
}

void DolphinView::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    const KFileItem item = m_model->fileItem(index);
    emit requestContextMenu(pos.toPoint(), item, url(), QList<QAction*>());
}

void DolphinView::slotViewContextMenuRequested(const QPointF& pos)
{
    emit requestContextMenu(pos.toPoint(), KFileItem(), url(), QList<QAction*>());
}

void DolphinView::slotHeaderContextMenuRequested(const QPointF& pos)
{
    ViewProperties props(url());

    QPointer<KMenu> menu = new KMenu(QApplication::activeWindow());

    KItemListView* view = m_container->controller()->view();
    const QSet<QByteArray> visibleRolesSet = view->visibleRoles().toSet();

    bool nepomukRunning = false;
    bool indexingEnabled = false;
#ifdef HAVE_NEPOMUK
    nepomukRunning = (Nepomuk::ResourceManager::instance()->init() == 0);
    if (nepomukRunning) {
        KConfig config("nepomukserverrc");
        indexingEnabled = config.group("Service-nepomukfileindexer").readEntry("autostart", false);
    }
#endif

    QString groupName;
    QMenu* groupMenu = 0;

    // Add all roles to the menu that can be shown or hidden by the user
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        if (info.role == "text") {
            // It should not be possible to hide the "text" role
            continue;
        }

        const QString text = m_model->roleDescription(info.role);
        QAction* action = 0;
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

        const bool enable = (!info.requiresNepomuk && !info.requiresIndexer) ||
                            (info.requiresNepomuk && nepomukRunning) ||
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
            foreach (const QByteArray& role, view->visibleRoles()) {
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
                foreach (const QByteArray& role, view->visibleRoles()) {
                    columnWidths.append(header->columnWidth(role));
                }
            }
            props.setHeaderColumnWidths(columnWidths);
        }
    }

    delete menu;
}

void DolphinView::slotHeaderColumnWidthChanged(const QByteArray& role, qreal current, qreal previous)
{
    Q_UNUSED(previous);

    const QList<QByteArray> visibleRoles = m_view->visibleRoles();

    ViewProperties props(url());
    QList<int> columnWidths = props.headerColumnWidths();
    if (columnWidths.count() != visibleRoles.count()) {
        columnWidths.clear();
        columnWidths.reserve(visibleRoles.count());
        const KItemListHeader* header = m_view->header();
        foreach (const QByteArray& role, visibleRoles) {
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
    KFileItem destItem = m_model->fileItem(index);
    if (destItem.isNull() || (!destItem.isDir() && !destItem.isDesktopFile())) {
        // Use the URL of the view as drop target if the item is no directory
        // or desktop-file
        destItem = m_model->rootItem();
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

    if (destUrl == url()) {
        // Mark the dropped urls as selected.
        markPastedUrlsAsSelected(event->mimeData());
    }
}

void DolphinView::slotModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    if (previous != 0) {
        disconnect(previous, SIGNAL(directoryLoadingCompleted()), this, SLOT(slotDirectoryLoadingCompleted()));
        m_versionControlObserver->setModel(0);
    }

    if (current) {
        Q_ASSERT(qobject_cast<KFileItemModel*>(current));
        connect(current, SIGNAL(loadingCompleted()), this, SLOT(slotDirectoryLoadingCompleted()));

        KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(current);
        m_versionControlObserver->setModel(fileItemModel);
    }
}

void DolphinView::slotMouseButtonPressed(int itemIndex, Qt::MouseButtons buttons)
{
    hideToolTip();

    if (itemIndex < 0) {
        // Trigger the history navigation only when clicking on the viewport:
        // Above an item the XButtons provide a simple way to select items in
        // the singleClick mode.
        if (buttons & Qt::XButton1) {
            emit goBackRequested();
        } else if (buttons & Qt::XButton2) {
            emit goForwardRequested();
        }
    }
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

void DolphinView::updateSortRole(const QByteArray& role)
{
    ViewProperties props(url());
    props.setSortRole(role);

    KItemModelBase* model = m_container->controller()->model();
    model->setSortRole(role);

    emit sortRoleChanged(role);
}

void DolphinView::updateSortOrder(Qt::SortOrder order)
{
    ViewProperties props(url());
    props.setSortOrder(order);

    m_model->setSortOrder(order);

    emit sortOrderChanged(order);
}

void DolphinView::updateSortFoldersFirst(bool foldersFirst)
{
    ViewProperties props(url());
    props.setSortFoldersFirst(foldersFirst);

    m_model->setSortDirectoriesFirst(foldersFirst);

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
    m_model->restoreExpandedDirectories(urls);
}

void DolphinView::saveState(QDataStream& stream)
{
    // Save the current item that has the keyboard focus
    const int currentIndex = m_container->controller()->selectionManager()->currentItem();
    if (currentIndex != -1) {
        KFileItem item = m_model->fileItem(currentIndex);
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
    stream << m_model->expandedDirectories();
}

KFileItem DolphinView::rootItem() const
{
    return m_model->rootItem();
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

void DolphinView::slotDirectoryRedirection(const KUrl& oldUrl, const KUrl& newUrl)
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
        const int currentIndex = m_model->index(m_currentItemUrl);
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
        clearSelection();

        KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
        QSet<int> selectedItems = selectionManager->selectedItems();

        foreach (const KUrl& url, m_selectedUrls) {
            const int index = m_model->index(url);
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

void DolphinView::calculateItemCount(int& fileCount,
                                     int& folderCount,
                                     KIO::filesize_t& totalFileSize) const
{
    const int itemCount = m_model->count();
    for (int i = 0; i < itemCount; ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (item.isDir()) {
            ++folderCount;
        } else {
            ++fileCount;
            totalFileSize += item.size();
        }
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

void DolphinView::slotDirectoryLoadingStarted()
{
    // Disable the writestate temporary until it can be determined in a fast way
    // in DolphinView::slotLoadingCompleted()
    if (m_isFolderWritable) {
        m_isFolderWritable = false;
        emit writeStateChanged(m_isFolderWritable);
    }

    emit directoryLoadingStarted();
}

void DolphinView::slotDirectoryLoadingCompleted()
{
    // Update the view-state. This has to be done asynchronously
    // because the view might not be in its final state yet.
    QTimer::singleShot(0, this, SLOT(updateViewState()));

    emit directoryLoadingCompleted();

    updateWritableState();
}

void DolphinView::slotItemsChanged()
{
    m_assureVisibleCurrentIndex = false;
}

void DolphinView::slotSortOrderChangedByHeader(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(previous);
    Q_ASSERT(m_model->sortOrder() == current);

    ViewProperties props(url());
    props.setSortOrder(current);

    emit sortOrderChanged(current);
}

void DolphinView::slotSortRoleChangedByHeader(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(previous);
    Q_ASSERT(m_model->sortRole() == current);

    ViewProperties props(url());
    props.setSortRole(current);

    emit sortRoleChanged(current);
}

void DolphinView::slotVisibleRolesChangedByHeader(const QList<QByteArray>& current,
                                                  const QList<QByteArray>& previous)
{
    Q_UNUSED(previous);
    Q_ASSERT(m_container->controller()->view()->visibleRoles() == current);

    const QList<QByteArray> previousVisibleRoles = m_visibleRoles;

    m_visibleRoles = current;

    ViewProperties props(url());
    props.setVisibleRoles(m_visibleRoles);

    emit visibleRolesChanged(m_visibleRoles, previousVisibleRoles);
}

void DolphinView::slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value)
{
    if (role == "text") {
        const KFileItem item = m_model->fileItem(index);
        const QString newName = value.toString();
        if (!newName.isEmpty() && newName != item.text() && newName != QLatin1String(".") && newName != QLatin1String("..")) {
            KonqOperations::rename(this, item.url(), newName);
        }
    }
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

    if (reload) {
        m_model->refreshDirectory(url);
    } else {
        m_model->loadDirectory(url);
    }
}

void DolphinView::applyViewProperties()
{
    m_view->beginTransaction();

    const ViewProperties props(url());

    const Mode mode = props.viewMode();
    if (m_mode != mode) {
        const Mode previousMode = m_mode;
        m_mode = mode;

        // Changing the mode might result in changing
        // the zoom level. Remember the old zoom level so
        // that zoomLevelChanged() can get emitted.
        const int oldZoomLevel = m_view->zoomLevel();
        applyModeToView();

        emit modeChanged(m_mode, previousMode);

        if (m_view->zoomLevel() != oldZoomLevel) {
            emit zoomLevelChanged(m_view->zoomLevel(), oldZoomLevel);
        }
    }

    const bool hiddenFilesShown = props.hiddenFilesShown();
    if (hiddenFilesShown != m_model->showHiddenFiles()) {
        m_model->setShowHiddenFiles(hiddenFilesShown);
        emit hiddenFilesShownChanged(hiddenFilesShown);
    }

    const bool groupedSorting = props.groupedSorting();
    if (groupedSorting != m_model->groupedSorting()) {
        m_model->setGroupedSorting(groupedSorting);
        emit groupedSortingChanged(groupedSorting);
    }

    const QByteArray sortRole = props.sortRole();
    if (sortRole != m_model->sortRole()) {
        m_model->setSortRole(sortRole);
        emit sortRoleChanged(sortRole);
    }

    const Qt::SortOrder sortOrder = props.sortOrder();
    if (sortOrder != m_model->sortOrder()) {
        m_model->setSortOrder(sortOrder);
        emit sortOrderChanged(sortOrder);
    }

    const bool sortFoldersFirst = props.sortFoldersFirst();
    if (sortFoldersFirst != m_model->sortDirectoriesFirst()) {
        m_model->setSortDirectoriesFirst(sortFoldersFirst);
        emit sortFoldersFirstChanged(sortFoldersFirst);
    }

    const QList<QByteArray> visibleRoles = props.visibleRoles();
    if (visibleRoles != m_visibleRoles) {
        const QList<QByteArray> previousVisibleRoles = m_visibleRoles;
        m_visibleRoles = visibleRoles;
        m_view->setVisibleRoles(visibleRoles);
        emit visibleRolesChanged(m_visibleRoles, previousVisibleRoles);
    }

    const bool previewsShown = props.previewsShown();
    if (previewsShown != m_view->previewsShown()) {
        const int oldZoomLevel = zoomLevel();

        m_view->setPreviewsShown(previewsShown);
        emit previewsShownChanged(previewsShown);

        // Changing the preview-state might result in a changed zoom-level
        if (oldZoomLevel != zoomLevel()) {
            emit zoomLevelChanged(zoomLevel(), oldZoomLevel);
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
    const KItemListSelectionManager* selectionManager = m_container->controller()->selectionManager();
    const QSet<int> selectedIndexes = selectionManager->selectedItems();

    return m_model->createMimeData(selectedIndexes);
}

void DolphinView::markPastedUrlsAsSelected(const QMimeData* mimeData)
{
    const KUrl::List sourceUrls = KUrl::List::fromMimeData(mimeData);
    KUrl::List destUrls;
    foreach (const KUrl& source, sourceUrls) {
        KUrl destination(url().url() + "/" + source.fileName());
        destUrls << destination;
    }
    markUrlsAsSelected(destUrls);
}

void DolphinView::updateWritableState()
{
    const bool wasFolderWritable = m_isFolderWritable;
    m_isFolderWritable = true;

    const KFileItem item = m_model->rootItem();
    if (!item.isNull()) {
        KFileItemListProperties capabilities(KFileItemList() << item);
        m_isFolderWritable = capabilities.supportsWriting();
    }
    if (m_isFolderWritable != wasFolderWritable) {
        emit writeStateChanged(m_isFolderWritable);
    }
}

#include "dolphinview.moc"
