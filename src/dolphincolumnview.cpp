/***************************************************************************
 *   Copyright (C) 2007-2009 by Peter Penz <peter.penz@gmx.at>             *
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

#include "dolphincolumnview.h"

#include "dolphinmodel.h"
#include "dolphincolumnviewcontainer.h"
#include "dolphincontroller.h"
#include "dolphindirlister.h"
#include "dolphinsortfilterproxymodel.h"
#include "settings/dolphinsettings.h"
#include "dolphinviewautoscroller.h"
#include "dolphin_columnmodesettings.h"
#include "dolphin_generalsettings.h"
#include "draganddrophelper.h"
#include "folderexpander.h"
#include "tooltips/tooltipmanager.h"
#include "versioncontrolobserver.h"
#include "viewextensionsfactory.h"
#include "zoomlevelinfo.h"

#include <kcolorscheme.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio/previewjob.h>
#include <kiconeffect.h>
#include <kjob.h>
#include <konqmimedata.h>

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QPoint>
#include <QScrollBar>

DolphinColumnView::DolphinColumnView(QWidget* parent,
                                     DolphinColumnViewContainer* container,
                                     const KUrl& url) :
    QListView(parent),
    m_active(false),
    m_container(container),
    m_extensionsFactory(0),
    m_url(url),
    m_childUrl(),
    m_font(),
    m_decorationSize(),
    m_dirLister(0),
    m_dolphinModel(0),
    m_proxyModel(0),
    m_dropRect()
{
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setSelectionBehavior(SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setSelectionRectVisible(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollMode(QListView::ScrollPerPixel);

    // apply the column mode settings to the widget
    const ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    } else {
        m_font = QFont(settings->fontFamily(),
                       settings->fontSize(),
                       settings->fontWeight(),
                       settings->italicFont());
    }

    activate();

    connect(this, SIGNAL(viewportEntered()),
            m_container->m_controller, SLOT(emitViewportEntered()));
    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));

    const DolphinView* dolphinView = m_container->m_controller->dolphinView();
    connect(dolphinView, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(window());
    m_dirLister->setDelayedMimeTypes(true);
    const bool showHiddenFiles = m_container->m_controller->dolphinView()->showHiddenFiles();
    m_dirLister->setShowingDotFiles(showHiddenFiles);

    m_dolphinModel = new DolphinModel(this);
    m_dolphinModel->setDirLister(m_dirLister);
    m_dolphinModel->setDropsAllowed(DolphinModel::DropOnDirectory);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dolphinModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_proxyModel->setSorting(dolphinView->sorting());
    m_proxyModel->setSortOrder(dolphinView->sortOrder());
    m_proxyModel->setSortFoldersFirst(dolphinView->sortFoldersFirst());

    setModel(m_proxyModel);

    connect(KGlobalSettings::self(), SIGNAL(kdisplayFontChanged()),
            this, SLOT(updateFont()));

    /*FolderExpander* folderExpander = new FolderExpander(this, m_proxyModel);
    folderExpander->setEnabled(DolphinSettings::instance().generalSettings()->autoExpandFolders());
    connect (folderExpander, SIGNAL(enterDir(const QModelIndex&)),
             m_container->m_controller, SLOT(triggerItem(const QModelIndex&)));

    new VersionControlObserver(this);*/

    DolphinController* controller = m_container->m_controller;
    connect(controller, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(setZoomLevel(int)));

    const QString nameFilter = controller->nameFilter();
    if (!nameFilter.isEmpty()) {
        m_proxyModel->setFilterRegExp(nameFilter);
    }
    connect(controller, SIGNAL(nameFilterChanged(const QString&)),
            this, SLOT(setNameFilter(const QString&)));

    m_extensionsFactory = new ViewExtensionsFactory(this, controller);
    updateDecorationSize(dolphinView->showPreview());
}

DolphinColumnView::~DolphinColumnView()
{
    delete m_proxyModel;
    m_proxyModel = 0;
    delete m_dolphinModel;
    m_dolphinModel = 0;
    m_dirLister = 0; // deleted by m_dolphinModel
}

void DolphinColumnView::setActive(bool active)
{
    if (active && (m_container->focusProxy() != this)) {
        m_container->setFocusProxy(this);
    }

    if (m_active != active) {
        m_active = active;

        if (active) {
            activate();
        } else {
            deactivate();
        }
    }
}

void DolphinColumnView::updateBackground()
{
    // TODO: The alpha-value 150 is copied from DolphinView::setActive(). When
    // cleaning up the cut-indication of DolphinColumnView with the code from
    // DolphinView a common helper-class should be available which can be shared
    // by all view implementations -> no hardcoded value anymore
    const QPalette::ColorRole role = viewport()->backgroundRole();
    QColor color = viewport()->palette().color(role);
    color.setAlpha((m_active && m_container->m_active) ? 255 : 150);

    QPalette palette = viewport()->palette();
    palette.setColor(role, color);
    viewport()->setPalette(palette);

    update();
}

KFileItem DolphinColumnView::itemAt(const QPoint& pos) const
{
    KFileItem item;
    const QModelIndex index = indexAt(pos);
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
        item = m_dolphinModel->itemForIndex(dolphinModelIndex);
    }
    return item;
}

QStyleOptionViewItem DolphinColumnView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = QListView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.decorationSize = m_decorationSize;
    viewOptions.showDecorationSelected = true;
    return viewOptions;
}

void DolphinColumnView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::instance().startDrag(this, supportedActions, m_container->m_controller);
}

void DolphinColumnView::dragEnterEvent(QDragEnterEvent* event)
{
    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        event->acceptProposedAction();
        requestActivation();
    }
}

void DolphinColumnView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QListView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void DolphinColumnView::dragMoveEvent(QDragMoveEvent* event)
{
    QListView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);

    m_dropRect.setSize(QSize()); // set as invalid
    if (index.isValid()) {
        m_container->m_controller->setItemView(this);
        const KFileItem item = m_container->m_controller->itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            m_dropRect = visualRect(index);
        }
    }
    setDirtyRegion(m_dropRect);

    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        // accept url drops, independently from the destination item
        event->acceptProposedAction();
    }
}

void DolphinColumnView::dropEvent(QDropEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    m_container->m_controller->setItemView(this);
    const KFileItem item = m_container->m_controller->itemForIndex(index);
    m_container->m_controller->indicateDroppedUrls(item, url(), event);
    QListView::dropEvent(event);
}

void DolphinColumnView::paintEvent(QPaintEvent* event)
{
    if (!m_childUrl.isEmpty()) {
        // indicate the shown URL of the next column by highlighting the shown folder item
        const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_childUrl);
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        if (proxyIndex.isValid() && !selectionModel()->isSelected(proxyIndex)) {
            const QRect itemRect = visualRect(proxyIndex);
            QPainter painter(viewport());
            QColor color = KColorScheme(QPalette::Active, KColorScheme::View).foreground().color();
            color.setAlpha(32);
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRect(itemRect);
        }
    }

    QListView::paintEvent(event);
}

void DolphinColumnView::mousePressEvent(QMouseEvent* event)
{
    requestActivation();
    if (!indexAt(event->pos()).isValid()) {
        if (QApplication::mouseButtons() & Qt::MidButton) {
            m_container->m_controller->replaceUrlByClipboard();
        }
    } else if (event->button() == Qt::LeftButton) {
        // TODO: see comment in DolphinIconsView::mousePressEvent()
        setState(QAbstractItemView::DraggingState);
    }
    QListView::mousePressEvent(event);
}

void DolphinColumnView::keyPressEvent(QKeyEvent* event)
{
    QListView::keyPressEvent(event);
    requestActivation();

    DolphinController* controller = m_container->m_controller;
    controller->handleKeyPressEvent(event);
    switch (event->key()) {
    case Qt::Key_Right: {
        // Special key handling for the column: A Key_Right should
        // open a new column for the currently selected folder.
        const QModelIndex index = currentIndex();
        const KFileItem item = controller->itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            controller->emitItemTriggered(item);
        }
        break;
    }

    case Qt::Key_Escape:
        selectionModel()->setCurrentIndex(selectionModel()->currentIndex(),
                                          QItemSelectionModel::Current |
                                          QItemSelectionModel::Clear);
        break;

    default:
        break;
    }
}

void DolphinColumnView::contextMenuEvent(QContextMenuEvent* event)
{
    if (!m_active) {
        m_container->requestActivation(this);
        Q_ASSERT(m_container->m_controller->itemView() == this);
        m_container->m_controller->triggerUrlChangeRequest(m_url);
    }
    Q_ASSERT(m_active);

    QListView::contextMenuEvent(event);

    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        clearSelection();
    }

    const QPoint pos = m_container->viewport()->mapFromGlobal(event->globalPos());
    Q_ASSERT(m_container->m_controller->itemView() == this);
    m_container->m_controller->triggerContextMenuRequest(pos);
}

void DolphinColumnView::wheelEvent(QWheelEvent* event)
{
    // let Ctrl+wheel events propagate to the DolphinView for icon zooming
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }

    const int height = m_decorationSize.height();
    const int step = (height >= KIconLoader::SizeHuge) ? height / 10 : (KIconLoader::SizeHuge - height) / 2;
    verticalScrollBar()->setSingleStep(step);

    QListView::wheelEvent(event);
}

void DolphinColumnView::leaveEvent(QEvent* event)
{
    QListView::leaveEvent(event);
    // if the mouse is above an item and moved very fast outside the widget,
    // no viewportEntered() signal might be emitted although the mouse has been moved
    // above the viewport
    m_container->m_controller->emitViewportEntered();
}

void DolphinColumnView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QListView::selectionChanged(selected, deselected);

    //QItemSelectionModel* selModel = m_container->selectionModel();
    //selModel->select(selected, QItemSelectionModel::Select);
    //selModel->select(deselected, QItemSelectionModel::Deselect);
}

void DolphinColumnView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QListView::currentChanged(current, previous);
    m_extensionsFactory->handleCurrentIndexChange(current, previous);
}

void DolphinColumnView::setNameFilter(const QString& nameFilter)
{
    m_proxyModel->setFilterRegExp(nameFilter);
}

void DolphinColumnView::setZoomLevel(int level)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(level);
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();

    const bool showPreview = m_container->m_controller->dolphinView()->showPreview();
    if (showPreview) {
        settings->setPreviewSize(size);
    } else {
        settings->setIconSize(size);
    }

    updateDecorationSize(showPreview);
}

void DolphinColumnView::slotEntered(const QModelIndex& index)
{
    m_container->m_controller->setItemView(this);
    m_container->m_controller->emitItemEntered(index);
}

void DolphinColumnView::requestActivation()
{
    m_container->m_controller->setItemView(this);
    m_container->m_controller->requestActivation();
    if (!m_active) {
        m_container->requestActivation(this);
        m_container->m_controller->triggerUrlChangeRequest(m_url);
        selectionModel()->clear();
    }
}

void DolphinColumnView::updateFont()
{
    const ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    }
}

void DolphinColumnView::slotShowPreviewChanged()
{
    const DolphinView* view = m_container->m_controller->dolphinView();
    updateDecorationSize(view->showPreview());
}

void DolphinColumnView::activate()
{
    setFocus(Qt::OtherFocusReason);

    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                m_container->m_controller, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                m_container->m_controller, SLOT(triggerItem(const QModelIndex&)));
    }

    if (selectionModel() && selectionModel()->currentIndex().isValid()) {
        selectionModel()->setCurrentIndex(selectionModel()->currentIndex(), QItemSelectionModel::SelectCurrent);
    }

    updateBackground();
}

void DolphinColumnView::deactivate()
{
    clearFocus();
    if (KGlobalSettings::singleClick()) {
        disconnect(this, SIGNAL(clicked(const QModelIndex&)),
                   m_container->m_controller, SLOT(triggerItem(const QModelIndex&)));
    } else {
        disconnect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                   m_container->m_controller, SLOT(triggerItem(const QModelIndex&)));
    }

    const QModelIndex current = selectionModel()->currentIndex();
    selectionModel()->clear();
    selectionModel()->setCurrentIndex(current, QItemSelectionModel::NoUpdate);
    updateBackground();
}

void DolphinColumnView::updateDecorationSize(bool showPreview)
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int iconSize = showPreview ? settings->previewSize() : settings->iconSize();
    const QSize size(iconSize, iconSize);
    setIconSize(size);

    m_decorationSize = size;

    doItemsLayout();
}

#include "dolphincolumnview.moc"
