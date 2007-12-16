/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
#include "dolphincontroller.h"
#include "dolphinsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "draganddrophelper.h"

#include <kcategorizedsortfilterproxymodel.h>
#include <kdialog.h>
#include <kdirmodel.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QPainter>
#include <QPoint>

DolphinIconsView::DolphinIconsView(QWidget* parent, DolphinController* controller) :
    KCategorizedView(parent),
    m_controller(controller),
    m_categoryDrawer(0),
    m_font(),
    m_decorationSize(),
    m_decorationPosition(QStyleOptionViewItem::Top),
    m_displayAlignment(Qt::AlignHCenter),
    m_itemSize(),
    m_dragging(false),
    m_dropRect()
{
    Q_ASSERT(controller != 0);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setSpacing(KDialog::spacingHint());
    setMovement(QListView::Static);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);

    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick' and to handle the
    // RETURN-key in keyPressEvent().
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    }
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));

    const DolphinView* view = controller->dolphinView();
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));
    connect(view, SIGNAL(additionalInfoChanged(const KFileItemDelegate::InformationList&)),
            this, SLOT(slotAdditionalInfoChanged(const KFileItemDelegate::InformationList&)));

    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));

    // apply the icons mode settings to the widget
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    m_font = QFont(settings->fontFamily(), settings->fontSize());
    m_font.setItalic(settings->italicFont());
    m_font.setBold(settings->boldFont());

    setWordWrap(settings->numberOfTextlines() > 1);
    updateGridSize(view->showPreview(), 0);

    if (settings->arrangement() == QListView::TopToBottom) {
        setFlow(QListView::LeftToRight);
        m_decorationPosition = QStyleOptionViewItem::Top;
        m_displayAlignment = Qt::AlignHCenter;
    } else {
        setFlow(QListView::TopToBottom);
        m_decorationPosition = QStyleOptionViewItem::Left;
        m_displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }

    m_categoryDrawer = new DolphinCategoryDrawer();
    setCategoryDrawer(m_categoryDrawer);

    setFocus();
}

DolphinIconsView::~DolphinIconsView()
{
    delete m_categoryDrawer;
    m_categoryDrawer = 0;
}

QRect DolphinIconsView::visualRect(const QModelIndex& index) const
{
    const bool leftToRightFlow = (flow() == QListView::LeftToRight);

    QRect itemRect = KCategorizedView::visualRect(index);

    const int maxWidth  = m_itemSize.width();
    const int maxHeight = m_itemSize.height();

    if (itemRect.width() > maxWidth) {
        // assure that the maximum item width is not exceeded
        if (leftToRightFlow) {
            const int left = itemRect.left() + (itemRect.width() - maxWidth) / 2;
            itemRect.setLeft(left);
        }
        itemRect.setWidth(maxWidth);
    }

    if (itemRect.height() > maxHeight) {
        // assure that the maximum item height is not exceeded
        if (!leftToRightFlow) {
            const int top = itemRect.top() + (itemRect.height() - maxHeight) / 2;
            itemRect.setTop(top);
        }
        itemRect.setHeight(maxHeight);
    }

    KCategorizedSortFilterProxyModel* proxyModel = dynamic_cast<KCategorizedSortFilterProxyModel*>(model());
    if (leftToRightFlow && !proxyModel->isCategorizedModel()) {
        // TODO: QListView::visualRect() calculates a wrong position of the items under
        // certain circumstances (e. g. if the text is too long). This issue is bypassed
        // by the following code (I'll try create a patch for Qt but as Dolphin must also work with
        // Qt 4.3.0 this workaround must get applied at least for KDE 4.0).
        const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
        const int margin = settings->gridSpacing();
        const int gridWidth = gridSize().width();
        const int gridIndex = (itemRect.left() - margin + 1) / gridWidth;
        const int centerInc = (maxWidth - itemRect.width()) / 2;
        itemRect.moveLeft((gridIndex * gridWidth) + margin + centerInc);
    }

    return itemRect;
}

QStyleOptionViewItem DolphinIconsView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = KCategorizedView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.decorationPosition = m_decorationPosition;
    viewOptions.decorationSize = m_decorationSize;
    viewOptions.displayAlignment = m_displayAlignment;
    viewOptions.showDecorationSelected = true;
    return viewOptions;
}

void DolphinIconsView::contextMenuEvent(QContextMenuEvent* event)
{
    KCategorizedView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinIconsView::mousePressEvent(QMouseEvent* event)
{
    m_controller->requestActivation();
    if (!indexAt(event->pos()).isValid()) {
        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if (!(modifier & Qt::ShiftModifier) && !(modifier & Qt::ControlModifier)) {
            clearSelection();
        }
    }

    KCategorizedView::mousePressEvent(event);
}

void DolphinIconsView::startDrag(Qt::DropActions supportedActions)
{
    // TODO: invoking KCategorizedView::startDrag() should not be necessary, we'll
    // fix this in KDE 4.1
    KCategorizedView::startDrag(supportedActions);
    DragAndDropHelper::startDrag(this, supportedActions);
}

void DolphinIconsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    m_dragging = true;
}

void DolphinIconsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    KCategorizedView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void DolphinIconsView::dragMoveEvent(QDragMoveEvent* event)
{
    KCategorizedView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);

    m_dropRect.setSize(QSize()); // set as invalid
    bool destIsDir = false;
    if (index.isValid()) {
        const KFileItem item = itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            m_dropRect = visualRect(index);
            destIsDir = true;
        }
    } else { // dropping on viewport
        destIsDir = true;
    }
    if (destIsDir && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    setDirtyRegion(m_dropRect);
}

void DolphinIconsView::dropEvent(QDropEvent* event)
{
    if (!selectionModel()->isSelected(indexAt(event->pos()))) {
        const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
        if (!urls.isEmpty()) {
            const QModelIndex index = indexAt(event->pos());
            const KFileItem item = itemForIndex(index);
            m_controller->indicateDroppedUrls(urls,
                                              m_controller->url(),
                                              item);
            event->acceptProposedAction();
        }
    }

    KCategorizedView::dropEvent(event);

    m_dragging = false;
}

void DolphinIconsView::paintEvent(QPaintEvent* event)
{
    KCategorizedView::paintEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    if (m_dragging) {
        const QBrush& brush = viewOptions().palette.brush(QPalette::Normal, QPalette::Highlight);
        DragAndDropHelper::drawHoverIndication(this, m_dropRect, brush);
    }
}

void DolphinIconsView::keyPressEvent(QKeyEvent* event)
{
    KCategorizedView::keyPressEvent(event);

    const QItemSelectionModel* selModel = selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();
    const bool trigger = currentIndex.isValid()
                         && (event->key() == Qt::Key_Return)
                         && (selModel->selectedIndexes().count() <= 1);
    if (trigger) {
        triggerItem(currentIndex);
    }
}

void DolphinIconsView::triggerItem(const QModelIndex& index)
{
    m_controller->triggerItem(itemForIndex(index));
}

void DolphinIconsView::slotEntered(const QModelIndex& index)
{
    m_controller->emitItemEntered(itemForIndex(index));
}

void DolphinIconsView::slotShowPreviewChanged()
{
    const DolphinView* view = m_controller->dolphinView();
    updateGridSize(view->showPreview(), additionalInfoCount());
}

void DolphinIconsView::slotAdditionalInfoChanged(const KFileItemDelegate::InformationList& info)
{
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    if (!settings->showAdditionalInfo()) {
        return;
    }

    const bool showPreview = m_controller->dolphinView()->showPreview();
    updateGridSize(showPreview, info.count());
}

void DolphinIconsView::zoomIn()
{
    if (isZoomInPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

        const int oldIconSize = settings->iconSize();
        int newIconSize = oldIconSize;

        const bool showPreview = m_controller->dolphinView()->showPreview();
        if (showPreview) {
            const int previewSize = increasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
        } else {
            newIconSize = increasedIconSize(oldIconSize);
            settings->setIconSize(newIconSize);
            if (settings->previewSize() < newIconSize) {
                // assure that the preview size is always >= the icon size
                settings->setPreviewSize(newIconSize);
            }
        }

        // increase also the grid size
        const int diff = newIconSize - oldIconSize;
        settings->setItemWidth(settings->itemWidth() + diff);
        settings->setItemHeight(settings->itemHeight() + diff);

        updateGridSize(showPreview, additionalInfoCount());
    }
}

void DolphinIconsView::zoomOut()
{
    if (isZoomOutPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

        const int oldIconSize = settings->iconSize();
        int newIconSize = oldIconSize;

        const bool showPreview = m_controller->dolphinView()->showPreview();
        if (showPreview) {
            const int previewSize = decreasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
            if (settings->iconSize() > previewSize) {
                // assure that the icon size is always <= the preview size
                newIconSize = previewSize;
                settings->setIconSize(newIconSize);
            }
        } else {
            newIconSize = decreasedIconSize(settings->iconSize());
            settings->setIconSize(newIconSize);
        }

        // decrease also the grid size
        const int diff = oldIconSize - newIconSize;
        settings->setItemWidth(settings->itemWidth() - diff);
        settings->setItemHeight(settings->itemHeight() - diff);

        updateGridSize(showPreview, additionalInfoCount());
    }
}

bool DolphinIconsView::isZoomInPossible() const
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    const bool showPreview = m_controller->dolphinView()->showPreview();
    const int size = showPreview ? settings->previewSize() : settings->iconSize();
    return size < KIconLoader::SizeEnormous;
}

bool DolphinIconsView::isZoomOutPossible() const
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    const bool showPreview = m_controller->dolphinView()->showPreview();
    const int size = showPreview ? settings->previewSize() : settings->iconSize();
    return size > KIconLoader::SizeSmall;
}

int DolphinIconsView::increasedIconSize(int size) const
{
    int incSize = 0;
    switch (size) {
    case KIconLoader::SizeSmall:       incSize = KIconLoader::SizeSmallMedium; break;
    case KIconLoader::SizeSmallMedium: incSize = KIconLoader::SizeMedium; break;
    case KIconLoader::SizeMedium:      incSize = KIconLoader::SizeLarge; break;
    case KIconLoader::SizeLarge:       incSize = KIconLoader::SizeHuge; break;
    case KIconLoader::SizeHuge:        incSize = KIconLoader::SizeEnormous; break;
    default: Q_ASSERT(false); break;
    }
    return incSize;
}

int DolphinIconsView::decreasedIconSize(int size) const
{
    int decSize = 0;
    switch (size) {
    case KIconLoader::SizeSmallMedium: decSize = KIconLoader::SizeSmall; break;
    case KIconLoader::SizeMedium: decSize = KIconLoader::SizeSmallMedium; break;
    case KIconLoader::SizeLarge: decSize = KIconLoader::SizeMedium; break;
    case KIconLoader::SizeHuge: decSize = KIconLoader::SizeLarge; break;
    case KIconLoader::SizeEnormous: decSize = KIconLoader::SizeHuge; break;
    default: Q_ASSERT(false); break;
    }
    return decSize;
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
        Q_ASSERT(diff >= 0);
        itemWidth  += diff;
        itemHeight += diff;

        size = previewSize;
    }

    Q_ASSERT(additionalInfoCount >= 0);
    itemHeight += additionalInfoCount * m_font.pointSize() * 2;

    if (settings->arrangement() == QListView::TopToBottom) {
        // The decoration width indirectly defines the maximum
        // width for the text wrapping. To use the maximum item width
        // for text wrapping, it is used as decoration width.
        m_decorationSize = QSize(itemWidth, size);
    } else {
        m_decorationSize = QSize(size, size);
    }

    m_itemSize = QSize(itemWidth, itemHeight);

    const int spacing = settings->gridSpacing();
    setGridSize(QSize(itemWidth + spacing * 2, itemHeight + spacing));

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());
}

KFileItem DolphinIconsView::itemForIndex(const QModelIndex& index) const
{
    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    const QModelIndex dirIndex = proxyModel->mapToSource(index);
    return dirModel->itemForIndex(dirIndex);
}

int DolphinIconsView::additionalInfoCount() const
{
    const DolphinView* view = m_controller->dolphinView();
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    return settings->showAdditionalInfo() ? view->additionalInfo().count() : 0;
}

#include "dolphiniconsview.moc"
