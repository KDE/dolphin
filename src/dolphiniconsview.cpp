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
#include "dolphinfileitemdelegate.h"
#include "dolphinsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "draganddrophelper.h"
#include "selectionmanager.h"

#include <kcategorizedsortfilterproxymodel.h>
#include <kdialog.h>
#include <kdirmodel.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QPainter>
#include <QPoint>
#include <QScrollBar>

DolphinIconsView::DolphinIconsView(QWidget* parent, DolphinController* controller) :
    KCategorizedView(parent),
    m_controller(controller),
    m_categoryDrawer(0),
    m_font(),
    m_decorationSize(),
    m_decorationPosition(QStyleOptionViewItem::Top),
    m_displayAlignment(Qt::AlignHCenter),
    m_itemSize(),
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

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick' and to handle the
    // RETURN-key in keyPressEvent().
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
        if (DolphinSettings::instance().generalSettings()->showSelectionToggle()) {
            SelectionManager* selManager = new SelectionManager(this);
            connect(selManager, SIGNAL(selectionChanged()),
                    this, SLOT(requestActivation()));
            connect(m_controller, SIGNAL(urlChanged(const KUrl&)),
                    selManager, SLOT(reset()));
        }
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
    }
    connect(this, SIGNAL(entered(const QModelIndex&)),
            controller, SLOT(emitItemEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));

    const DolphinView* view = controller->dolphinView();
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
                       settings->fontSize(),
                       settings->fontWeight(),
                       settings->italicFont());
    }

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

    connect(KGlobalSettings::self(), SIGNAL(kdisplayFontChanged()),
            this, SLOT(updateFont()));
}

DolphinIconsView::~DolphinIconsView()
{
    delete m_categoryDrawer;
    m_categoryDrawer = 0;
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
        const KFileItem item = m_controller->itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            m_dropRect = visualRect(index);
        } else {
            m_dropRect.setSize(QSize()); // set as invalid
        }
    }
    if (event->mimeData()->hasUrls()) {
        // accept url drops, independently from the destination item
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
            const KFileItem item = m_controller->itemForIndex(index);
            m_controller->indicateDroppedUrls(urls,
                                              m_controller->url(),
                                              item);
            event->acceptProposedAction();
        }
    }

    KCategorizedView::dropEvent(event);
}

void DolphinIconsView::keyPressEvent(QKeyEvent* event)
{
    KCategorizedView::keyPressEvent(event);
    m_controller->handleKeyPressEvent(event);
}

void DolphinIconsView::wheelEvent(QWheelEvent* event)
{
    // let Ctrl+wheel events propagate to the DolphinView for icon zooming
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }
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

void DolphinIconsView::slotShowPreviewChanged()
{
    const DolphinView* view = m_controller->dolphinView();
    updateGridSize(view->showPreview(), additionalInfoCount());
}

void DolphinIconsView::slotAdditionalInfoChanged()
{
    const DolphinView* view = m_controller->dolphinView();
    const bool showPreview = view->showPreview();
    updateGridSize(showPreview, view->additionalInfo().count());
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

void DolphinIconsView::requestActivation()
{
    m_controller->requestActivation();
}

void DolphinIconsView::updateFont()
{
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
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
        setIconSize(QSize(itemWidth, size));
    } else {
        m_decorationSize = QSize(size, size);
        setIconSize(QSize(size, size));
    }

    m_itemSize = QSize(itemWidth, itemHeight);

    const int spacing = settings->gridSpacing();
    setGridSize(QSize(itemWidth + spacing * 2, itemHeight + spacing));

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

    DolphinFileItemDelegate* delegate = qobject_cast<DolphinFileItemDelegate*>(itemDelegate());
    if (delegate != 0) {
        delegate->setMaximumSize(m_itemSize);
    }
}

int DolphinIconsView::additionalInfoCount() const
{
    const DolphinView* view = m_controller->dolphinView();
    return view->additionalInfo().count();
}

#include "dolphiniconsview.moc"
