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
#include "dolphinviewcontroller.h"
#include "dolphindirlister.h"
#include "dolphinfileitemdelegate.h"
#include "dolphinsortfilterproxymodel.h"
#include "settings/dolphinsettings.h"
#include "dolphinviewautoscroller.h"
#include "dolphin_columnmodesettings.h"
#include "dolphin_generalsettings.h"
#include "draganddrophelper.h"
#include "folderexpander.h"
#include "tooltips/tooltipmanager.h"
#include "viewextensionsfactory.h"
#include "viewmodecontroller.h"
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
#include <QHeaderView>
#include <QPainter>
#include <QPoint>
#include <QScrollBar>

DolphinColumnView::DolphinColumnView(QWidget* parent,
                                     DolphinColumnViewContainer* container,
                                     const KUrl& url) :
    DolphinTreeView(parent),
    m_active(false),
    m_container(container),
    m_extensionsFactory(0),
    m_url(url),
    m_childUrl(),
    m_font(),
    m_decorationSize(),
    m_dirLister(0),
    m_dolphinModel(0),
    m_proxyModel(0)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setUniformRowHeights(true);
    setSelectionBehavior(SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    const ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    } else {
        m_font = QFont(settings->fontFamily(),
                       qRound(settings->fontSize()),
                       settings->fontWeight(),
                       settings->italicFont());
        m_font.setPointSizeF(settings->fontSize());
    }

    connect(this, SIGNAL(viewportEntered()),
            m_container->m_dolphinViewController, SLOT(emitViewportEntered()));
    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));

    const DolphinView* dolphinView = m_container->m_dolphinViewController->view();
    connect(dolphinView, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(window());
    m_dirLister->setDelayedMimeTypes(true);
    const bool showHiddenFiles = m_container->m_dolphinViewController->view()->showHiddenFiles();
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

    const ViewModeController* viewModeController = m_container->m_viewModeController;
    connect(viewModeController, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(setZoomLevel(int)));
    const QString nameFilter = viewModeController->nameFilter();
    if (!nameFilter.isEmpty()) {
        m_proxyModel->setFilterFixedString(nameFilter);
    }

    updateDecorationSize(dolphinView->showPreview());
    updateBackground();
    
    DolphinViewController* dolphinViewController = m_container->m_dolphinViewController;
    m_extensionsFactory = new ViewExtensionsFactory(this, dolphinViewController, viewModeController);
    m_extensionsFactory->fileItemDelegate()->setMinimizedNameColumn(true);

    m_dirLister->openUrl(url, KDirLister::NoFlags);
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

void DolphinColumnView::setSelectionModel(QItemSelectionModel* model)
{
    // If a change of the selection is done although the view is not active
    // (e. g. by the selection markers), the column must be activated. This
    // is done by listening to the current selectionChanged() signal.
    if (selectionModel() != 0) {
        disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
                   this, SLOT(requestActivation()));
    }

    DolphinTreeView::setSelectionModel(model);

    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(requestActivation()));
}

QStyleOptionViewItem DolphinColumnView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = DolphinTreeView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.fontMetrics = QFontMetrics(m_font);
    viewOptions.decorationSize = m_decorationSize;
    viewOptions.showDecorationSelected = true;
    return viewOptions;
}

bool DolphinColumnView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        // Hide all columns except of the 'Name' column
        for (int i = DolphinModel::Name + 1; i < DolphinModel::ExtraColumnCount; ++i) {
            hideColumn(i);
        }
        header()->hide();
    }

    return DolphinTreeView::event(event);
}

void DolphinColumnView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::instance().startDrag(this, supportedActions, m_container->m_dolphinViewController);
    DolphinTreeView::startDrag(supportedActions);
}

void DolphinColumnView::dragEnterEvent(QDragEnterEvent* event)
{
    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        event->acceptProposedAction();
        requestActivation();
    }
    DolphinTreeView::dragEnterEvent(event);
}

void DolphinColumnView::dragMoveEvent(QDragMoveEvent* event)
{
    DolphinTreeView::dragMoveEvent(event);

    if (DragAndDropHelper::instance().isMimeDataSupported(event->mimeData())) {
        // accept url drops, independently from the destination item
        event->acceptProposedAction();
    }
}

void DolphinColumnView::dropEvent(QDropEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    m_container->m_dolphinViewController->setItemView(this);
    const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dolphinModel->itemForIndex(dolphinModelIndex);
    m_container->m_dolphinViewController->indicateDroppedUrls(item, url(), event);
    DolphinTreeView::dropEvent(event);
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

    DolphinTreeView::paintEvent(event);
}

void DolphinColumnView::mousePressEvent(QMouseEvent* event)
{
    requestActivation();
    if (!indexAt(event->pos()).isValid() && (QApplication::mouseButtons() & Qt::MidButton)) {
        m_container->m_dolphinViewController->replaceUrlByClipboard();
    }

    DolphinTreeView::mousePressEvent(event);
}

void DolphinColumnView::keyPressEvent(QKeyEvent* event)
{   
    DolphinTreeView::keyPressEvent(event);

    DolphinViewController* controller = m_container->m_dolphinViewController;
    controller->handleKeyPressEvent(event);
    switch (event->key()) {
    case Qt::Key_Right: {
        // Special key handling for the column: A Key_Right should
        // open a new column for the currently selected folder.
        const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(currentIndex());
        const KFileItem item = m_dolphinModel->itemForIndex(dolphinModelIndex);
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
    requestActivation();
    DolphinTreeView::contextMenuEvent(event);
    m_container->m_dolphinViewController->triggerContextMenuRequest(event->pos());
}

void DolphinColumnView::wheelEvent(QWheelEvent* event)
{
    const int step = m_decorationSize.height();
    verticalScrollBar()->setSingleStep(step);
    DolphinTreeView::wheelEvent(event);
}

void DolphinColumnView::leaveEvent(QEvent* event)
{
    DolphinTreeView::leaveEvent(event);
    // if the mouse is above an item and moved very fast outside the widget,
    // no viewportEntered() signal might be emitted although the mouse has been moved
    // above the viewport
    m_container->m_dolphinViewController->emitViewportEntered();
}

void DolphinColumnView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    DolphinTreeView::currentChanged(current, previous);
    m_extensionsFactory->handleCurrentIndexChange(current, previous);
}

QRect DolphinColumnView::visualRect(const QModelIndex& index) const
{
    QRect rect = DolphinTreeView::visualRect(index);

    const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dolphinModel->itemForIndex(dolphinModelIndex);
    if (!item.isNull()) {
        const int width = DolphinFileItemDelegate::nameColumnWidth(item.text(), viewOptions());
        rect.setWidth(width);
    }

    return rect;
}

bool DolphinColumnView::acceptsDrop(const QModelIndex& index) const
{
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        // Accept drops above directories
        const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
        const KFileItem item = m_dolphinModel->itemForIndex(dolphinModelIndex);
        return !item.isNull() && item.isDir();
    }

    return false;
}

void DolphinColumnView::setZoomLevel(int level)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(level);
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();

    const bool showPreview = m_container->m_dolphinViewController->view()->showPreview();
    if (showPreview) {
        settings->setPreviewSize(size);
    } else {
        settings->setIconSize(size);
    }

    updateDecorationSize(showPreview);
}

void DolphinColumnView::slotEntered(const QModelIndex& index)
{
    m_container->m_dolphinViewController->setItemView(this);
    m_container->m_dolphinViewController->emitItemEntered(index);
}

void DolphinColumnView::requestActivation()
{
    m_container->m_dolphinViewController->requestActivation();
    if (!m_active) {
        m_container->requestActivation(this);
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
    const DolphinView* view = m_container->m_dolphinViewController->view();
    updateDecorationSize(view->showPreview());
}

void DolphinColumnView::activate()
{
    setFocus(Qt::OtherFocusReason);

    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                m_container->m_dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                m_container->m_dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
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
                   m_container->m_dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    } else {
        disconnect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                   m_container->m_dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    }

    // It is important to disconnect the connection to requestActivation() temporary, otherwise the internal
    // clearing of the selection would result in activating the column again.
    disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
               this, SLOT(requestActivation()));
    const QModelIndex current = selectionModel()->currentIndex();
    selectionModel()->clear();
    selectionModel()->setCurrentIndex(current, QItemSelectionModel::NoUpdate);
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(requestActivation()));
    
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
