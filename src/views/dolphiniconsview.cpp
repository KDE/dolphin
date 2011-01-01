/***************************************************************************
 *   Copyright (C) 2006-2009 by Peter Penz <peter.penz@gmx.at>             *
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

#include "dolphiniconsview.h"

#include "dolphincategorydrawer.h"
#include "dolphinviewcontroller.h"
#include "settings/dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "draganddrophelper.h"
#include "selectionmanager.h"
#include "viewextensionsfactory.h"
#include "viewmodecontroller.h"
#include "zoomlevelinfo.h"

#include <kcategorizedsortfilterproxymodel.h>
#include <kdialog.h>
#include <kfileitemdelegate.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QScrollBar>

DolphinIconsView::DolphinIconsView(QWidget* parent,
                                   DolphinViewController* dolphinViewController,
                                   const ViewModeController* viewModeController,
                                   DolphinSortFilterProxyModel* proxyModel) :
    KCategorizedView(parent),
    m_dolphinViewController(dolphinViewController),
    m_viewModeController(viewModeController),
    m_categoryDrawer(new DolphinCategoryDrawer(this)),
    m_extensionsFactory(0),
    m_font(),
    m_decorationSize(),
    m_decorationPosition(QStyleOptionViewItem::Top),
    m_displayAlignment(Qt::AlignHCenter),
    m_itemSize(),
    m_dropRect()
{
    Q_ASSERT(dolphinViewController != 0);
    Q_ASSERT(viewModeController != 0);

    setModel(proxyModel);
    setLayoutDirection(Qt::LeftToRight);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    viewport()->setAcceptDrops(true);

    setMouseTracking(true);

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            dolphinViewController, SLOT(requestTab(const QModelIndex&)));
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                dolphinViewController, SLOT(triggerItem(const QModelIndex&)));
    }

    connect(this, SIGNAL(entered(const QModelIndex&)),
            dolphinViewController, SLOT(emitItemEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            dolphinViewController, SLOT(emitViewportEntered()));
    connect(viewModeController, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(setZoomLevel(int)));

    const DolphinView* view = dolphinViewController->view();
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));
    connect(view, SIGNAL(additionalInfoChanged()),
            this, SLOT(slotAdditionalInfoChanged()));

    // apply the icons mode settings to the widget
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
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

    setWordWrap(settings->numberOfTextlines() > 1);

    if (settings->arrangement() == QListView::TopToBottom) {
        setFlow(QListView::LeftToRight);
        m_decorationPosition = QStyleOptionViewItem::Top;
        m_displayAlignment = Qt::AlignHCenter;
    } else {
        setFlow(QListView::TopToBottom);
        m_decorationPosition = QStyleOptionViewItem::Left;
        m_displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }

    connect(m_categoryDrawer, SIGNAL(actionRequested(int,QModelIndex)), this, SLOT(categoryDrawerActionRequested(int,QModelIndex)));
    setCategoryDrawer(m_categoryDrawer);

    connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)),
            this, SLOT(slotGlobalSettingsChanged(int)));

    updateGridSize(view->showPreview(), 0);
    m_extensionsFactory = new ViewExtensionsFactory(this, dolphinViewController, viewModeController);
}

DolphinIconsView::~DolphinIconsView()
{
}

void DolphinIconsView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    KCategorizedView::dataChanged(topLeft, bottomRight);

    KCategorizedSortFilterProxyModel* proxyModel = dynamic_cast<KCategorizedSortFilterProxyModel*>(model());
    if (!proxyModel->isCategorizedModel()) {
        // bypass a QListView issue that items are not layout correctly if the decoration size of
        // an index changes
        scheduleDelayedItemsLayout();
    }
}

QStyleOptionViewItem DolphinIconsView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = KCategorizedView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.fontMetrics = QFontMetrics(m_font);
    viewOptions.decorationPosition = m_decorationPosition;
    viewOptions.decorationSize = m_decorationSize;
    viewOptions.displayAlignment = m_displayAlignment;
    viewOptions.showDecorationSelected = true;
    return viewOptions;
}

void DolphinIconsView::contextMenuEvent(QContextMenuEvent* event)
{
    KCategorizedView::contextMenuEvent(event);
    m_dolphinViewController->triggerContextMenuRequest(event->pos());
}

void DolphinIconsView::mousePressEvent(QMouseEvent* event)
{
    m_dolphinViewController->requestActivation();
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid() && (event->button() == Qt::LeftButton)) {
        // TODO: It should not be necessary to manually set the dragging state, but I could
        // not reproduce this issue with a Qt-only example yet to find the root cause.
        // Issue description: start Dolphin, split the view and drag an item from the
        // inactive view to the active view by a very fast mouse movement. Result:
        // the item gets selected instead of being dragged...
        setState(QAbstractItemView::DraggingState);
    }

    if (!index.isValid() && (QApplication::mouseButtons() & Qt::MidButton)) {
         m_dolphinViewController->replaceUrlByClipboard();
    }

    KCategorizedView::mousePressEvent(event);
}

void DolphinIconsView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::instance().startDrag(this, supportedActions, m_dolphinViewController);
}

void DolphinIconsView::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void DolphinIconsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    KCategorizedView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void DolphinIconsView::dragMoveEvent(QDragMoveEvent* event)
{
    KCategorizedView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);

    m_dropRect.setSize(QSize()); // set as invalid
    if (index.isValid()) {
        const KFileItem item = m_dolphinViewController->itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            m_dropRect = visualRect(index);
        } else {
            m_dropRect.setSize(QSize()); // set as invalid
        }
    }
    event->acceptProposedAction();

    setDirtyRegion(m_dropRect);
}

void DolphinIconsView::dropEvent(QDropEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    const KFileItem item = m_dolphinViewController->itemForIndex(index);
    m_dolphinViewController->indicateDroppedUrls(item, m_viewModeController->url(), event);
    // don't call KCategorizedView::dropEvent(event), as it moves
    // the items which is not wanted
}

QModelIndex DolphinIconsView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    const QModelIndex oldCurrent = currentIndex();

    QModelIndex newCurrent = KCategorizedView::moveCursor(cursorAction, modifiers);
    if (newCurrent != oldCurrent) {
        return newCurrent;
    }

    // The cursor has not been moved by the base implementation. Provide a
    // wrap behavior, so that the cursor will go to the next item when reaching
    // the border.
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    if (settings->arrangement() == QListView::LeftToRight) {
        switch (cursorAction) {
        case MoveUp:
            if (newCurrent.row() == 0) {
                return newCurrent;
            }
            newCurrent = KCategorizedView::moveCursor(MoveLeft, modifiers);
            selectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::NoUpdate);
            newCurrent = KCategorizedView::moveCursor(MovePageDown, modifiers);
            break;

        case MoveDown:
            if (newCurrent.row() == (model()->rowCount() - 1)) {
                return newCurrent;
            }
            newCurrent = KCategorizedView::moveCursor(MovePageUp, modifiers);
            selectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::NoUpdate);
            newCurrent = KCategorizedView::moveCursor(MoveRight, modifiers);
            break;

        default:
            break;
        }
    } else {
        QModelIndex current = oldCurrent;
        switch (cursorAction) {
        case MoveLeft:
            if (newCurrent.row() == 0) {
                return newCurrent;
            }
            newCurrent = KCategorizedView::moveCursor(MoveUp, modifiers);
            do {
                selectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::NoUpdate);
                current = newCurrent;
                newCurrent = KCategorizedView::moveCursor(MoveRight, modifiers);
            } while (newCurrent != current);
            break;

        case MoveRight:
            if (newCurrent.row() == (model()->rowCount() - 1)) {
                return newCurrent;
            }
            do {
                selectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::NoUpdate);
                current = newCurrent;
                newCurrent = KCategorizedView::moveCursor(MoveLeft, modifiers);
            } while (newCurrent != current);
            newCurrent = KCategorizedView::moveCursor(MoveDown, modifiers);
            break;

        default:
            break;
        }
    }

    // Revert all changes of the current item to make sure that item selection works correctly
    selectionModel()->setCurrentIndex(oldCurrent, QItemSelectionModel::NoUpdate);
    return newCurrent;
}

void DolphinIconsView::keyPressEvent(QKeyEvent* event)
{
    KCategorizedView::keyPressEvent(event);
    m_dolphinViewController->handleKeyPressEvent(event);
}

void DolphinIconsView::wheelEvent(QWheelEvent* event)
{
    horizontalScrollBar()->setSingleStep(m_itemSize.width() / 5);
    verticalScrollBar()->setSingleStep(m_itemSize.height() / 5);

    KCategorizedView::wheelEvent(event);
    // if the icons are aligned left to right, the vertical wheel event should
    // be applied to the horizontal scrollbar
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    const bool scrollHorizontal = (event->orientation() == Qt::Vertical) &&
                                  (settings->arrangement() == QListView::LeftToRight);
    if (scrollHorizontal) {
        QWheelEvent horizEvent(event->pos(),
                               event->delta(),
                               event->buttons(),
                               event->modifiers(),
                               Qt::Horizontal);
        QApplication::sendEvent(horizontalScrollBar(), &horizEvent);
    }
}

void DolphinIconsView::showEvent(QShowEvent* event)
{
    KFileItemDelegate* delegate = dynamic_cast<KFileItemDelegate*>(itemDelegate());
    delegate->setMaximumSize(m_itemSize);

    KCategorizedView::showEvent(event);
}

void DolphinIconsView::leaveEvent(QEvent* event)
{
    KCategorizedView::leaveEvent(event);
    // if the mouse is above an item and moved very fast outside the widget,
    // no viewportEntered() signal might be emitted although the mouse has been moved
    // above the viewport
    m_dolphinViewController->emitViewportEntered();
}

void DolphinIconsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    KCategorizedView::currentChanged(current, previous);
    m_extensionsFactory->handleCurrentIndexChange(current, previous);
}

void DolphinIconsView::resizeEvent(QResizeEvent* event)
{
    KCategorizedView::resizeEvent(event);
    const DolphinView* view = m_dolphinViewController->view();
    updateGridSize(view->showPreview(), view->additionalInfo().count());
}

void DolphinIconsView::slotShowPreviewChanged()
{
    const DolphinView* view = m_dolphinViewController->view();
    updateGridSize(view->showPreview(), additionalInfoCount());
}

void DolphinIconsView::slotAdditionalInfoChanged()
{
    const DolphinView* view = m_dolphinViewController->view();
    const bool showPreview = view->showPreview();
    updateGridSize(showPreview, view->additionalInfo().count());
}

void DolphinIconsView::setZoomLevel(int level)
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

    const int oldIconSize = settings->iconSize();
    int newIconSize = oldIconSize;

    const bool showPreview = m_dolphinViewController->view()->showPreview();
    if (showPreview) {
        const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(level);
        settings->setPreviewSize(previewSize);
    } else {
        newIconSize = ZoomLevelInfo::iconSizeForZoomLevel(level);
        settings->setIconSize(newIconSize);
    }

    // increase also the grid size
    const int diff = newIconSize - oldIconSize;
    settings->setItemWidth(settings->itemWidth() + diff);
    settings->setItemHeight(settings->itemHeight() + diff);

    updateGridSize(showPreview, additionalInfoCount());
}

void DolphinIconsView::requestActivation()
{
    m_dolphinViewController->requestActivation();
}

void DolphinIconsView::slotGlobalSettingsChanged(int category)
{
    Q_UNUSED(category);

    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);
    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    }

    disconnect(this, SIGNAL(clicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    disconnect(this, SIGNAL(doubleClicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    } else {
        connect(this, SIGNAL(doubleClicked(QModelIndex)), m_dolphinViewController, SLOT(triggerItem(QModelIndex)));
    }
}

void DolphinIconsView::categoryDrawerActionRequested(int action, const QModelIndex &index)
{
    const QSortFilterProxyModel *model = dynamic_cast<const QSortFilterProxyModel*>(index.model());
    const QModelIndex topLeft = model->index(index.row(), modelColumn());
    QModelIndex bottomRight = topLeft;
    const QString category = model->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
    QModelIndex current = topLeft;
    while (true) {
        current = model->index(current.row() + 1, modelColumn());
        const QString curCategory = model->data(model->index(current.row(), index.column()), KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
        if (!current.isValid() || category != curCategory) {
            break;
        }
        bottomRight = current;
    }
    switch (action) {
        case DolphinCategoryDrawer::SelectAll:
            selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
            break;
        case DolphinCategoryDrawer::UnselectAll:
            selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Deselect);
            break;
        default:
            break;
    }
}

void DolphinIconsView::updateGridSize(bool showPreview, int additionalInfoCount)
{
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    int itemWidth = settings->itemWidth();
    int itemHeight = settings->itemHeight();
    int size = settings->iconSize();

    if (showPreview) {
        const int previewSize = settings->previewSize();
        const int diff = previewSize - size;
        itemWidth  += diff;
        itemHeight += diff;

        size = previewSize;
    }

    Q_ASSERT(additionalInfoCount >= 0);
    itemHeight += additionalInfoCount * QFontMetrics(m_font).height();

    // Optimize the item size of the grid in a way to prevent large gaps on the
    // right border (= row arrangement) or the bottom border (= column arrangement).
    // There is no public API in QListView to find out the used width of the viewport
    // for the layout. The following calculation of 'contentWidth'/'contentHeight'
    // is based on QListViewPrivate::prepareItemsLayout() (Copyright (C) 2009 Nokia Corporation).
    int frameAroundContents = 0;
    if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) {
        frameAroundContents = style()->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2;
    }
    const int spacing = settings->gridSpacing();
    if (settings->arrangement() == QListView::TopToBottom) {
        const int contentWidth = viewport()->width() - 1
                                 - frameAroundContents
                                 - style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, horizontalScrollBar());
        const int gridWidth = itemWidth + spacing * 2;
        const int horizItemCount = contentWidth / gridWidth;
        if (horizItemCount > 0) {
            itemWidth += (contentWidth - horizItemCount * gridWidth) / horizItemCount;
        }

        // The decoration width indirectly defines the maximum
        // width for the text wrapping. To use the maximum item width
        // for text wrapping, it is used as decoration width.
        m_decorationSize = QSize(itemWidth, size);
        setIconSize(QSize(itemWidth, size));
    } else {
        const int contentHeight = viewport()->height() - 1
                                  - frameAroundContents
                                  - style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, verticalScrollBar());
        const int gridHeight = itemHeight + spacing;
        const int vertItemCount = contentHeight / gridHeight;
        if (vertItemCount > 0) {
            itemHeight += (contentHeight - vertItemCount * gridHeight) / vertItemCount;
        }

        m_decorationSize = QSize(size, size);
        setIconSize(QSize(size, size));
    }

    m_itemSize = QSize(itemWidth, itemHeight);
    setGridSizeOwn(QSize(itemWidth + spacing * 2, itemHeight + spacing));

    KFileItemDelegate* delegate = dynamic_cast<KFileItemDelegate*>(itemDelegate());
    if (delegate != 0) {
        delegate->setMaximumSize(m_itemSize);
    }
}

int DolphinIconsView::additionalInfoCount() const
{
    const DolphinView* view = m_dolphinViewController->view();
    return view->additionalInfo().count();
}

#include "dolphiniconsview.moc"
