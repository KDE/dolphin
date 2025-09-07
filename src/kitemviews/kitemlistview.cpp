/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistview.h"

#ifndef QT_NO_ACCESSIBILITY
#include "accessibility/kitemlistcontaineraccessible.h"
#include "accessibility/kitemlistdelegateaccessible.h"
#include "accessibility/kitemlistviewaccessible.h"
#endif
#include "dolphindebug.h"
#include "kitemlistcontainer.h"
#include "kitemlistcontroller.h"
#include "kitemlistheader.h"
#include "kitemlistselectionmanager.h"
#include "kstandarditemlistwidget.h"

#include "private/kitemlistheaderwidget.h"
#include "private/kitemlistrubberband.h"
#include "private/kitemlistsizehintresolver.h"
#include "private/kitemlistviewlayouter.h"

#include <optional>

#include <QElapsedTimer>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QStyleOptionRubberBand>
#include <QTimer>
#include <QVariantAnimation>

namespace
{
// Time in ms until reaching the autoscroll margin triggers
// an initial autoscrolling
const int InitialAutoScrollDelay = 700;

// Delay in ms for triggering the next autoscroll
const int RepeatingAutoScrollDelay = 1000 / 60;

// Copied from the Kirigami.Units.shortDuration
const int RubberFadeSpeed = 150;

const char *RubberPropertyName = "_kitemviews_rubberBandPosition";
}

#ifndef QT_NO_ACCESSIBILITY
QAccessibleInterface *accessibleInterfaceFactory(const QString &key, QObject *object)
{
    Q_UNUSED(key)

    if (KItemListContainer *container = qobject_cast<KItemListContainer *>(object)) {
        if (auto controller = container->controller(); controller) {
            if (KItemListView *view = controller->view(); view && view->accessibleParent()) {
                return view->accessibleParent();
            }
        }
        return new KItemListContainerAccessible(container);
    } else if (KItemListView *view = qobject_cast<KItemListView *>(object)) {
        return new KItemListViewAccessible(view, view->accessibleParent());
    }

    return nullptr;
}
#endif

KItemListView::KItemListView(QGraphicsWidget *parent)
    : QGraphicsWidget(parent)
    , m_enabledSelectionToggles(false)
    , m_grouped(false)
    , m_highlightEntireRow(false)
    , m_alternateBackgrounds(false)
    , m_supportsItemExpanding(false)
    , m_editingRole(false)
    , m_activeTransactions(0)
    , m_endTransactionAnimationHint(Animation)
    , m_itemSize()
    , m_controller(nullptr)
    , m_model(nullptr)
    , m_visibleRoles()
    , m_widgetCreator(nullptr)
    , m_groupHeaderCreator(nullptr)
    , m_styleOption()
    , m_visibleItems()
    , m_visibleGroups()
    , m_visibleCells()
    , m_scrollBarExtent(0)
    , m_layouter(nullptr)
    , m_animation(nullptr)
    , m_oldScrollOffset(0)
    , m_oldMaximumScrollOffset(0)
    , m_oldItemOffset(0)
    , m_oldMaximumItemOffset(0)
    , m_skipAutoScrollForRubberBand(false)
    , m_rubberBand(nullptr)
    , m_tapAndHoldIndicator(nullptr)
    , m_mousePos()
    , m_autoScrollIncrement(0)
    , m_autoScrollTimer(nullptr)
    , m_header(nullptr)
    , m_headerWidget(nullptr)
    , m_indicatorAnimation(nullptr)
    , m_statusBarOffset(0)
    , m_dropIndicator()
    , m_sizeHintResolver(nullptr)
{
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);

    m_sizeHintResolver = new KItemListSizeHintResolver(this);

    m_layouter = new KItemListViewLayouter(m_sizeHintResolver, this);

    m_animation = new KItemListViewAnimation(this);
    connect(m_animation, &KItemListViewAnimation::finished, this, &KItemListView::slotAnimationFinished);

    m_rubberBand = new KItemListRubberBand(this);
    connect(m_rubberBand, &KItemListRubberBand::activationChanged, this, &KItemListView::slotRubberBandActivationChanged);

    m_tapAndHoldIndicator = new KItemListRubberBand(this);
    m_indicatorAnimation = new QPropertyAnimation(m_tapAndHoldIndicator, "endPosition", this);
    connect(m_tapAndHoldIndicator, &KItemListRubberBand::activationChanged, this, [this](bool active) {
        if (active) {
            m_indicatorAnimation->setDuration(150);
            m_indicatorAnimation->setStartValue(QPointF(1, 1));
            m_indicatorAnimation->setEndValue(QPointF(40, 40));
            m_indicatorAnimation->start();
        }
        update();
    });
    connect(m_tapAndHoldIndicator, &KItemListRubberBand::endPositionChanged, this, [this]() {
        if (m_tapAndHoldIndicator->isActive()) {
            update();
        }
    });

    m_headerWidget = new KItemListHeaderWidget(this);
    m_headerWidget->setVisible(false);

    m_header = new KItemListHeader(this);

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(accessibleInterfaceFactory);
#endif
}

KItemListView::~KItemListView()
{
    // The group headers are children of the widgets created by
    // widgetCreator(). So it is mandatory to delete the group headers
    // first.
    delete m_groupHeaderCreator;
    m_groupHeaderCreator = nullptr;

    delete m_widgetCreator;
    m_widgetCreator = nullptr;

    delete m_sizeHintResolver;
    m_sizeHintResolver = nullptr;
}

void KItemListView::setScrollOffset(qreal offset)
{
    if (offset < 0) {
        offset = 0;
    }

    const qreal previousOffset = m_layouter->scrollOffset();
    if (offset == previousOffset) {
        return;
    }

    m_layouter->setScrollOffset(offset);
    m_animation->setScrollOffset(offset);

    // Don't check whether the m_layoutTimer is active: Changing the
    // scroll offset must always trigger a synchronous layout, otherwise
    // the smooth-scrolling might get jerky.
    doLayout(NoAnimation);
    onScrollOffsetChanged(offset, previousOffset);
}

qreal KItemListView::scrollOffset() const
{
    return m_layouter->scrollOffset();
}

qreal KItemListView::maximumScrollOffset() const
{
    return m_layouter->maximumScrollOffset() + m_statusBarOffset;
}

void KItemListView::setItemOffset(qreal offset)
{
    if (m_layouter->itemOffset() == offset) {
        return;
    }

    m_layouter->setItemOffset(offset);
    if (m_headerWidget->isVisible()) {
        m_headerWidget->setOffset(offset);
    }

    // Don't check whether the m_layoutTimer is active: Changing the
    // item offset must always trigger a synchronous layout, otherwise
    // the smooth-scrolling might get jerky.
    doLayout(NoAnimation);
}

qreal KItemListView::itemOffset() const
{
    return m_layouter->itemOffset();
}

qreal KItemListView::maximumItemOffset() const
{
    return m_layouter->maximumItemOffset();
}

int KItemListView::maximumVisibleItems() const
{
    return m_layouter->maximumVisibleItems();
}

void KItemListView::setVisibleRoles(const QList<QByteArray> &roles)
{
    const QList<QByteArray> previousRoles = m_visibleRoles;
    m_visibleRoles = roles;
    onVisibleRolesChanged(roles, previousRoles);

    m_sizeHintResolver->clearCache();
    m_layouter->markAsDirty();

    if (m_itemSize.isEmpty()) {
        m_headerWidget->setColumns(roles);
        updatePreferredColumnWidths();
        if (!m_headerWidget->automaticColumnResizing()) {
            // The column-width of new roles are still 0. Apply the preferred
            // column-width as default with.
            for (const QByteArray &role : std::as_const(m_visibleRoles)) {
                if (m_headerWidget->columnWidth(role) == 0) {
                    const qreal width = m_headerWidget->preferredColumnWidth(role);
                    m_headerWidget->setColumnWidth(role, width);
                }
            }

            applyColumnWidthsFromHeader();
        }
    }

    const bool alternateBackgroundsChanged =
        m_itemSize.isEmpty() && ((roles.count() > 1 && previousRoles.count() <= 1) || (roles.count() <= 1 && previousRoles.count() > 1));

    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        KItemListWidget *widget = it.value();
        widget->setVisibleRoles(roles);
        if (alternateBackgroundsChanged) {
            updateAlternateBackgroundForWidget(widget);
        }
    }

    doLayout(NoAnimation);
}

QList<QByteArray> KItemListView::visibleRoles() const
{
    return m_visibleRoles;
}

void KItemListView::setAutoScroll(bool enabled)
{
    if (enabled && !m_autoScrollTimer) {
        m_autoScrollTimer = new QTimer(this);
        m_autoScrollTimer->setSingleShot(true);
        connect(m_autoScrollTimer, &QTimer::timeout, this, &KItemListView::triggerAutoScrolling);
        m_autoScrollTimer->start(InitialAutoScrollDelay);
    } else if (!enabled && m_autoScrollTimer) {
        delete m_autoScrollTimer;
        m_autoScrollTimer = nullptr;
    }
}

bool KItemListView::autoScroll() const
{
    return m_autoScrollTimer != nullptr;
}

void KItemListView::setEnabledSelectionToggles(bool enabled)
{
    if (m_enabledSelectionToggles != enabled) {
        m_enabledSelectionToggles = enabled;

        QHashIterator<int, KItemListWidget *> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            it.value()->setEnabledSelectionToggle(enabled);
        }
    }
}

bool KItemListView::enabledSelectionToggles() const
{
    return m_enabledSelectionToggles;
}

KItemListController *KItemListView::controller() const
{
    return m_controller;
}

KItemModelBase *KItemListView::model() const
{
    return m_model;
}

void KItemListView::setWidgetCreator(KItemListWidgetCreatorBase *widgetCreator)
{
    delete m_widgetCreator;
    m_widgetCreator = widgetCreator;
}

KItemListWidgetCreatorBase *KItemListView::widgetCreator() const
{
    if (!m_widgetCreator) {
        m_widgetCreator = defaultWidgetCreator();
    }
    return m_widgetCreator;
}

void KItemListView::setGroupHeaderCreator(KItemListGroupHeaderCreatorBase *groupHeaderCreator)
{
    delete m_groupHeaderCreator;
    m_groupHeaderCreator = groupHeaderCreator;
}

KItemListGroupHeaderCreatorBase *KItemListView::groupHeaderCreator() const
{
    if (!m_groupHeaderCreator) {
        m_groupHeaderCreator = defaultGroupHeaderCreator();
    }
    return m_groupHeaderCreator;
}

#ifndef QT_NO_ACCESSIBILITY
void KItemListView::setAccessibleParentsObject(KItemListContainer *accessibleParentsObject)
{
    Q_ASSERT(!m_accessibleParent);
    m_accessibleParent = new KItemListContainerAccessible(accessibleParentsObject);
}
KItemListContainerAccessible *KItemListView::accessibleParent()
{
    Q_CHECK_PTR(m_accessibleParent); // We always want the accessibility tree/hierarchy to be complete.
    return m_accessibleParent;
}
#endif

QSizeF KItemListView::itemSize() const
{
    return m_itemSize;
}

const KItemListStyleOption &KItemListView::styleOption() const
{
    return m_styleOption;
}

void KItemListView::setGeometry(const QRectF &rect)
{
    QGraphicsWidget::setGeometry(rect);

    if (!m_model) {
        return;
    }

    const QSizeF newSize = rect.size();
    if (m_itemSize.isEmpty()) {
        m_headerWidget->resize(rect.width(), m_headerWidget->size().height());
        if (m_headerWidget->automaticColumnResizing()) {
            applyAutomaticColumnWidths();
        } else {
            const qreal requiredWidth = m_headerWidget->leftPadding() + columnWidthsSum() + m_headerWidget->rightPadding();
            const QSizeF dynamicItemSize(qMax(newSize.width(), requiredWidth), m_itemSize.height());
            m_layouter->setItemSize(dynamicItemSize);
        }
    }

    m_layouter->setSize(newSize);
    // We don't animate the moving of the items here because
    // it would look like the items are slow to find their position.
    doLayout(NoAnimation);
}

qreal KItemListView::scrollSingleStep() const
{
    /**
     * The scroll distance is supposed to be similar between every scroll area in existence, so users can predict how much scrolling they need to do to see the
     * part of the view they are interested in. We try to conform to the global scroll distance setting which is defined through
     * QApplication::wheelScrollLines. A single line is assumed to be the height of the default font. This single step is what is supposed to be returned here.
     * The issue is that the application the user scrolls in the most (Firefox) does not care about our scroll speed and goes considerably faster. So we
     * amend a random times two (* 2) so users are happy with the scroll speed in Dolphin.
     */
    const QFontMetrics metrics(font());
    return metrics.height() * 2;
}

qreal KItemListView::verticalPageStep() const
{
    qreal headerHeight = 0;
    if (m_headerWidget->isVisible()) {
        headerHeight = m_headerWidget->size().height();
    }
    return size().height() - headerHeight;
}

std::optional<int> KItemListView::itemAt(const QPointF &pos) const
{
    if (headerBoundaries().contains(pos)) {
        return std::nullopt;
    }

    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();

        const KItemListWidget *widget = it.value();
        const QPointF mappedPos = widget->mapFromItem(this, pos);
        if (widget->contains(mappedPos)) {
            return it.key();
        }
    }

    return std::nullopt;
}

bool KItemListView::isAboveSelectionToggle(int index, const QPointF &pos) const
{
    if (!m_enabledSelectionToggles) {
        return false;
    }

    const KItemListWidget *widget = m_visibleItems.value(index);
    if (widget) {
        const QRectF selectionToggleRect = widget->selectionToggleRect();
        if (!selectionToggleRect.isEmpty()) {
            const QPointF mappedPos = widget->mapFromItem(this, pos);
            return selectionToggleRect.contains(mappedPos);
        }
    }
    return false;
}

bool KItemListView::isAboveExpansionToggle(int index, const QPointF &pos) const
{
    const KItemListWidget *widget = m_visibleItems.value(index);
    if (widget) {
        const QRectF expansionToggleRect = widget->expansionToggleRect();
        if (!expansionToggleRect.isEmpty()) {
            const QPointF mappedPos = widget->mapFromItem(this, pos);
            return expansionToggleRect.contains(mappedPos);
        }
    }
    return false;
}

bool KItemListView::isAboveText(int index, const QPointF &pos) const
{
    const KItemListWidget *widget = m_visibleItems.value(index);
    if (widget) {
        const QRectF &textRect = widget->textRect();
        if (!textRect.isEmpty()) {
            const QPointF mappedPos = widget->mapFromItem(this, pos);
            return textRect.contains(mappedPos);
        }
    }
    return false;
}

int KItemListView::firstVisibleIndex() const
{
    return m_layouter->firstVisibleIndex();
}

int KItemListView::lastVisibleIndex() const
{
    return m_layouter->lastVisibleIndex();
}

void KItemListView::calculateItemSizeHints(QVector<std::pair<qreal, bool>> &logicalHeightHints, qreal &logicalWidthHint) const
{
    widgetCreator()->calculateItemSizeHints(logicalHeightHints, logicalWidthHint, this);
}

void KItemListView::setSupportsItemExpanding(bool supportsExpanding)
{
    if (m_supportsItemExpanding != supportsExpanding) {
        m_supportsItemExpanding = supportsExpanding;
        updateSiblingsInformation();
        onSupportsItemExpandingChanged(supportsExpanding);
    }
}

bool KItemListView::supportsItemExpanding() const
{
    return m_supportsItemExpanding;
}

void KItemListView::setHighlightEntireRow(bool highlightEntireRow)
{
    if (m_highlightEntireRow != highlightEntireRow) {
        m_highlightEntireRow = highlightEntireRow;
        onHighlightEntireRowChanged(highlightEntireRow);
    }
}

bool KItemListView::highlightEntireRow() const
{
    return m_highlightEntireRow;
}

void KItemListView::setAlternateBackgrounds(bool alternate)
{
    if (m_alternateBackgrounds != alternate) {
        m_alternateBackgrounds = alternate;
        updateAlternateBackgrounds();
    }
}

bool KItemListView::alternateBackgrounds() const
{
    return m_alternateBackgrounds;
}

QRectF KItemListView::itemRect(int index) const
{
    return m_layouter->itemRect(index);
}

QRectF KItemListView::itemContextRect(int index) const
{
    QRectF contextRect;

    const KItemListWidget *widget = m_visibleItems.value(index);
    if (widget) {
        contextRect = widget->selectionRectCore();
        contextRect.translate(itemRect(index).topLeft());
    }

    return contextRect;
}

bool KItemListView::isElided(int index) const
{
    return m_sizeHintResolver->isElided(index);
}

void KItemListView::scrollToItem(int index, ViewItemPosition viewItemPosition)
{
    QRectF viewGeometry = geometry();
    if (m_headerWidget->isVisible()) {
        const qreal headerHeight = m_headerWidget->size().height();
        viewGeometry.adjust(0, headerHeight, 0, 0);
    }
    if (m_statusBarOffset != 0) {
        viewGeometry.adjust(0, 0, 0, -m_statusBarOffset);
    }
    QRectF currentRect = itemRect(index);

    if (layoutDirection() == Qt::RightToLeft && scrollOrientation() == Qt::Horizontal) {
        currentRect.moveLeft(m_layouter->size().width() - currentRect.right());
    }

    // Fix for Bug 311099 - View the underscore when using Ctrl + PageDown
    currentRect.adjust(-m_styleOption.horizontalMargin, -m_styleOption.verticalMargin, m_styleOption.horizontalMargin, m_styleOption.verticalMargin);

    qreal offset = 0;
    switch (scrollOrientation()) {
    case Qt::Vertical:
        if (currentRect.top() < viewGeometry.top() || currentRect.bottom() > viewGeometry.bottom()) {
            switch (viewItemPosition) {
            case Beginning:
                offset = currentRect.top() - viewGeometry.top();
                break;
            case Middle:
                offset = 0.5 * (currentRect.top() + currentRect.bottom() - (viewGeometry.top() + viewGeometry.bottom()));
                break;
            case End:
                offset = currentRect.bottom() - viewGeometry.bottom();
                break;
            case Nearest:
                if (currentRect.top() < viewGeometry.top()) {
                    offset = currentRect.top() - viewGeometry.top();
                }
                if (currentRect.bottom() > viewGeometry.bottom() + offset) {
                    offset += currentRect.bottom() - viewGeometry.bottom() - offset;
                }
                break;
            default:
                Q_UNREACHABLE();
            }
        }
        break;
    case Qt::Horizontal:
        if (currentRect.left() < viewGeometry.left() || currentRect.right() > viewGeometry.right()) {
            switch (viewItemPosition) {
            case Beginning:
                if (layoutDirection() == Qt::RightToLeft) {
                    offset = currentRect.right() - viewGeometry.right();
                } else {
                    offset = currentRect.left() - viewGeometry.left();
                }
                break;
            case Middle:
                offset = 0.5 * (currentRect.left() + currentRect.right() - (viewGeometry.left() + viewGeometry.right()));
                break;
            case End:
                if (layoutDirection() == Qt::RightToLeft) {
                    offset = currentRect.left() - viewGeometry.left();
                } else {
                    offset = currentRect.right() - viewGeometry.right();
                }
                break;
            case Nearest:
                if (layoutDirection() == Qt::RightToLeft) {
                    if (currentRect.left() < viewGeometry.left()) {
                        offset = currentRect.left() - viewGeometry.left();
                    }
                    if (currentRect.right() > viewGeometry.right() + offset) {
                        offset += currentRect.right() - viewGeometry.right() - offset;
                    }
                } else {
                    if (currentRect.right() > viewGeometry.right()) {
                        offset = currentRect.right() - viewGeometry.right();
                    }
                    if (currentRect.left() < viewGeometry.left() + offset) {
                        offset += currentRect.left() - viewGeometry.left() - offset;
                    }
                }
                break;
            default:
                Q_UNREACHABLE();
            }
        }
        break;
    default:
        Q_UNREACHABLE();
    }

    if (!qFuzzyIsNull(offset)) {
        Q_EMIT scrollTo(scrollOffset() + offset);
        return;
    }

    Q_EMIT scrollingStopped();
}

void KItemListView::beginTransaction()
{
    ++m_activeTransactions;
    if (m_activeTransactions == 1) {
        onTransactionBegin();
    }
}

void KItemListView::endTransaction()
{
    --m_activeTransactions;
    if (m_activeTransactions < 0) {
        m_activeTransactions = 0;
        qCWarning(DolphinDebug) << "Mismatch between beginTransaction()/endTransaction()";
    }

    if (m_activeTransactions == 0) {
        onTransactionEnd();
        doLayout(m_endTransactionAnimationHint);
        m_endTransactionAnimationHint = Animation;
    }
}

bool KItemListView::isTransactionActive() const
{
    return m_activeTransactions > 0;
}

void KItemListView::setHeaderVisible(bool visible)
{
    if (visible && !m_headerWidget->isVisible()) {
        QStyleOptionHeader option;
        const QSize headerSize = style()->sizeFromContents(QStyle::CT_HeaderSection, &option, QSize());

        m_headerWidget->setPos(0, 0);
        m_headerWidget->resize(size().width(), headerSize.height());
        m_headerWidget->setModel(m_model);
        m_headerWidget->setColumns(m_visibleRoles);
        m_headerWidget->setZValue(1);

        connect(m_headerWidget, &KItemListHeaderWidget::columnWidthChanged, this, &KItemListView::slotHeaderColumnWidthChanged);
        connect(m_headerWidget, &KItemListHeaderWidget::sidePaddingChanged, this, &KItemListView::slotSidePaddingChanged);
        connect(m_headerWidget, &KItemListHeaderWidget::columnMoved, this, &KItemListView::slotHeaderColumnMoved);
        connect(m_headerWidget, &KItemListHeaderWidget::sortOrderChanged, this, &KItemListView::sortOrderChanged);
        connect(m_headerWidget, &KItemListHeaderWidget::sortRoleChanged, this, &KItemListView::sortRoleChanged);
        connect(m_headerWidget, &KItemListHeaderWidget::columnHovered, this, &KItemListView::columnHovered);
        connect(m_headerWidget, &KItemListHeaderWidget::columnUnHovered, this, &KItemListView::columnUnHovered);

        m_layouter->setHeaderHeight(headerSize.height());
        m_headerWidget->setVisible(true);
    } else if (!visible && m_headerWidget->isVisible()) {
        disconnect(m_headerWidget, &KItemListHeaderWidget::columnWidthChanged, this, &KItemListView::slotHeaderColumnWidthChanged);
        disconnect(m_headerWidget, &KItemListHeaderWidget::sidePaddingChanged, this, &KItemListView::slotSidePaddingChanged);
        disconnect(m_headerWidget, &KItemListHeaderWidget::columnMoved, this, &KItemListView::slotHeaderColumnMoved);
        disconnect(m_headerWidget, &KItemListHeaderWidget::sortOrderChanged, this, &KItemListView::sortOrderChanged);
        disconnect(m_headerWidget, &KItemListHeaderWidget::sortRoleChanged, this, &KItemListView::sortRoleChanged);
        disconnect(m_headerWidget, &KItemListHeaderWidget::columnHovered, this, &KItemListView::columnHovered);
        disconnect(m_headerWidget, &KItemListHeaderWidget::columnUnHovered, this, &KItemListView::columnUnHovered);

        m_layouter->setHeaderHeight(0);
        m_headerWidget->setVisible(false);
    }
}

bool KItemListView::isHeaderVisible() const
{
    return m_headerWidget->isVisible();
}

KItemListHeader *KItemListView::header() const
{
    return m_header;
}

QPixmap KItemListView::createDragPixmap(const KItemSet &indexes) const
{
    QPixmap pixmap;

    if (indexes.count() == 1) {
        KItemListWidget *item = m_visibleItems.value(indexes.first());
        QGraphicsView *graphicsView = scene()->views()[0];
        if (item && graphicsView) {
            pixmap = item->createDragPixmap(nullptr, graphicsView);
        }
    } else {
        // TODO: Not implemented yet. Probably extend the interface
        // from KItemListWidget::createDragPixmap() to return a pixmap
        // that can be used for multiple indexes.
    }

    return pixmap;
}

void KItemListView::editRole(int index, const QByteArray &role)
{
    KStandardItemListWidget *widget = qobject_cast<KStandardItemListWidget *>(m_visibleItems.value(index));
    if (!widget || m_editingRole) {
        return;
    }

    m_editingRole = true;
    m_controller->selectionManager()->setCurrentItem(index);
    widget->setEditedRole(role);

    connect(widget, &KItemListWidget::roleEditingCanceled, this, &KItemListView::slotRoleEditingCanceled);
    connect(widget, &KItemListWidget::roleEditingFinished, this, &KItemListView::slotRoleEditingFinished);

    connect(this, &KItemListView::scrollOffsetChanged, widget, &KStandardItemListWidget::finishRoleEditing);
}

void KItemListView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsWidget::paint(painter, option, widget);

    for (auto animation : std::as_const(m_rubberBandAnimations)) {
        QRectF rubberBandRect = animation->property(RubberPropertyName).toRectF();

        const QPointF topLeft = rubberBandRect.topLeft();
        if (scrollOrientation() == Qt::Vertical) {
            rubberBandRect.moveTo(topLeft.x(), topLeft.y() - scrollOffset());
        } else {
            rubberBandRect.moveTo(topLeft.x() - scrollOffset(), topLeft.y());
        }

        QStyleOptionRubberBand opt;
        initStyleOption(&opt);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = rubberBandRect.toRect();

        painter->save();

        painter->setOpacity(animation->currentValue().toReal());
        style()->drawControl(QStyle::CE_RubberBand, &opt, painter);

        painter->restore();
    }

    if (m_rubberBand->isActive()) {
        QRectF rubberBandRect = QRectF(m_rubberBand->startPosition(), m_rubberBand->endPosition()).normalized();

        const QPointF topLeft = rubberBandRect.topLeft();
        if (scrollOrientation() == Qt::Vertical) {
            rubberBandRect.moveTo(topLeft.x(), topLeft.y() - scrollOffset());
        } else {
            rubberBandRect.moveTo(topLeft.x() - scrollOffset(), topLeft.y());
        }

        QStyleOptionRubberBand opt;
        initStyleOption(&opt);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = rubberBandRect.toRect();
        style()->drawControl(QStyle::CE_RubberBand, &opt, painter);
    }

    if (m_tapAndHoldIndicator->isActive()) {
        const QPointF indicatorSize = m_tapAndHoldIndicator->endPosition();
        const QRectF rubberBandRect =
            QRectF(m_tapAndHoldIndicator->startPosition() - indicatorSize, (m_tapAndHoldIndicator->startPosition()) + indicatorSize).normalized();
        QStyleOptionRubberBand opt;
        initStyleOption(&opt);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = rubberBandRect.toRect();
        style()->drawControl(QStyle::CE_RubberBand, &opt, painter);
    }

    if (!m_dropIndicator.isEmpty()) {
        const QRectF r = m_dropIndicator.toRect();

        QColor color = palette().brush(QPalette::Normal, QPalette::Text).color();
        painter->setPen(color);

        // TODO: The following implementation works only for a vertical scroll-orientation
        // and assumes a height of the m_draggingInsertIndicator of 1.
        Q_ASSERT(r.height() == 1);
        painter->drawLine(r.left() + 1, r.top(), r.right() - 1, r.top());

        color.setAlpha(128);
        painter->setPen(color);
        painter->drawRect(r.left(), r.top() - 1, r.width() - 1, 2);
    }
}

void KItemListView::setStatusBarOffset(int offset)
{
    if (m_statusBarOffset != offset) {
        m_statusBarOffset = offset;
        if (m_layouter) {
            m_layouter->setStatusBarOffset(offset);
        }
    }
}

QVariant KItemListView::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSceneHasChanged && scene()) {
        if (!scene()->views().isEmpty()) {
            m_styleOption.palette = scene()->views().at(0)->palette();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void KItemListView::setItemSize(const QSizeF &size)
{
    const QSizeF previousSize = m_itemSize;
    if (size == previousSize) {
        return;
    }

    // Skip animations when the number of rows or columns
    // are changed in the grid layout. Although the animation
    // engine can handle this usecase, it looks obtrusive.
    const bool animate = !changesItemGridLayout(m_layouter->size(), size, m_layouter->itemMargin());

    const bool alternateBackgroundsChanged = m_alternateBackgrounds && ((m_itemSize.isEmpty() && !size.isEmpty()) || (!m_itemSize.isEmpty() && size.isEmpty()));

    m_itemSize = size;

    if (alternateBackgroundsChanged) {
        // For an empty item size alternate backgrounds are drawn if more than
        // one role is shown. Assure that the backgrounds for visible items are
        // updated when changing the size in this context.
        updateAlternateBackgrounds();
    }

    if (size.isEmpty()) {
        if (m_headerWidget->automaticColumnResizing()) {
            updatePreferredColumnWidths();
        } else {
            // Only apply the changed height and respect the header widths
            // set by the user
            const qreal currentWidth = m_layouter->itemSize().width();
            const QSizeF newSize(currentWidth, size.height());
            m_layouter->setItemSize(newSize);
        }
    } else {
        m_layouter->setItemSize(size);
    }

    m_sizeHintResolver->clearCache();
    doLayout(animate ? Animation : NoAnimation);
    onItemSizeChanged(size, previousSize);
}

void KItemListView::setStyleOption(const KItemListStyleOption &option)
{
    if (m_styleOption == option) {
        return;
    }

    const KItemListStyleOption previousOption = m_styleOption;
    m_styleOption = option;

    bool animate = true;
    const QSizeF margin(option.horizontalMargin, option.verticalMargin);
    if (margin != m_layouter->itemMargin()) {
        // Skip animations when the number of rows or columns
        // are changed in the grid layout. Although the animation
        // engine can handle this usecase, it looks obtrusive.
        animate = !changesItemGridLayout(m_layouter->size(), m_layouter->itemSize(), margin);
        m_layouter->setItemMargin(margin);
    }

    if (m_grouped) {
        updateGroupHeaderHeight();
    }

    if (animate && (previousOption.maxTextLines != option.maxTextLines || previousOption.maxTextWidth != option.maxTextWidth)) {
        // Animating a change of the maximum text size just results in expensive
        // temporary eliding and clipping operations and does not look good visually.
        animate = false;
    }

    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        it.value()->setStyleOption(option);
    }

    m_sizeHintResolver->clearCache();
    m_layouter->markAsDirty();
    doLayout(animate ? Animation : NoAnimation);

    if (m_itemSize.isEmpty()) {
        updatePreferredColumnWidths();
    }

    onStyleOptionChanged(option, previousOption);
}

void KItemListView::setScrollOrientation(Qt::Orientation orientation)
{
    const Qt::Orientation previousOrientation = m_layouter->scrollOrientation();
    if (orientation == previousOrientation) {
        return;
    }

    m_layouter->setScrollOrientation(orientation);
    m_animation->setScrollOrientation(orientation);
    m_sizeHintResolver->clearCache();

    if (m_grouped) {
        QMutableHashIterator<KItemListWidget *, KItemListGroupHeader *> it(m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            it.value()->setScrollOrientation(orientation);
        }
        updateGroupHeaderHeight();
    }

    doLayout(NoAnimation);

    onScrollOrientationChanged(orientation, previousOrientation);
    Q_EMIT scrollOrientationChanged(orientation, previousOrientation);
}

Qt::Orientation KItemListView::scrollOrientation() const
{
    return m_layouter->scrollOrientation();
}

KItemListWidgetCreatorBase *KItemListView::defaultWidgetCreator() const
{
    return nullptr;
}

KItemListGroupHeaderCreatorBase *KItemListView::defaultGroupHeaderCreator() const
{
    return nullptr;
}

void KItemListView::initializeItemListWidget(KItemListWidget *item)
{
    Q_UNUSED(item)
}

bool KItemListView::itemSizeHintUpdateRequired(const QSet<QByteArray> &changedRoles) const
{
    Q_UNUSED(changedRoles)
    return true;
}

void KItemListView::onControllerChanged(KItemListController *current, KItemListController *previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onModelChanged(KItemModelBase *current, KItemModelBase *previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onItemSizeChanged(const QSizeF &current, const QSizeF &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onScrollOffsetChanged(qreal current, qreal previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onVisibleRolesChanged(const QList<QByteArray> &current, const QList<QByteArray> &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onStyleOptionChanged(const KItemListStyleOption &current, const KItemListStyleOption &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListView::onHighlightEntireRowChanged(bool highlightEntireRow)
{
    Q_UNUSED(highlightEntireRow)
}

void KItemListView::onSupportsItemExpandingChanged(bool supportsExpanding)
{
    Q_UNUSED(supportsExpanding)
}

void KItemListView::onTransactionBegin()
{
}

void KItemListView::onTransactionEnd()
{
}

bool KItemListView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::PaletteChange:
        updatePalette();
        break;

    case QEvent::FontChange:
        updateFont();
        break;

    case QEvent::FocusIn:
        focusInEvent(static_cast<QFocusEvent *>(event));
        event->accept();
        return true;
        break;

    case QEvent::FocusOut:
        focusOutEvent(static_cast<QFocusEvent *>(event));
        event->accept();
        return true;
        break;

    default:
        // Forward all other events to the controller and handle them there
        if (!m_editingRole && m_controller && m_controller->processEvent(event, transform())) {
            event->accept();
            return true;
        }
    }

    return QGraphicsWidget::event(event);
}

void KItemListView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_mousePos = transform().map(event->pos());
    event->accept();
}

void KItemListView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsWidget::mouseMoveEvent(event);

    m_mousePos = transform().map(event->pos());
    if (m_autoScrollTimer && !m_autoScrollTimer->isActive()) {
        m_autoScrollTimer->start(InitialAutoScrollDelay);
    }
}

void KItemListView::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(true);
    setAutoScroll(true);
}

void KItemListView::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsWidget::dragMoveEvent(event);

    m_mousePos = transform().map(event->pos());
    if (m_autoScrollTimer && !m_autoScrollTimer->isActive()) {
        m_autoScrollTimer->start(InitialAutoScrollDelay);
    }
}

void KItemListView::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsWidget::dragLeaveEvent(event);
    setAutoScroll(false);
}

void KItemListView::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsWidget::dropEvent(event);
    setAutoScroll(false);
}

QList<KItemListWidget *> KItemListView::visibleItemListWidgets() const
{
    return m_visibleItems.values();
}

void KItemListView::updateFont()
{
    if (scene() && !scene()->views().isEmpty()) {
        KItemListStyleOption option = styleOption();
        option.font = scene()->views().first()->font();
        option.fontMetrics = QFontMetrics(option.font);

        setStyleOption(option);
    }
}

void KItemListView::updatePalette()
{
    KItemListStyleOption option = styleOption();
    option.palette = palette();
    setStyleOption(option);
}

void KItemListView::slotItemsInserted(const KItemRangeList &itemRanges)
{
    if (m_itemSize.isEmpty()) {
        updatePreferredColumnWidths(itemRanges);
    }

    const bool hasMultipleRanges = (itemRanges.count() > 1);
    if (hasMultipleRanges) {
        beginTransaction();
    }

    m_layouter->markAsDirty();

    m_sizeHintResolver->itemsInserted(itemRanges);

    int previouslyInsertedCount = 0;
    for (const KItemRange &range : itemRanges) {
        // range.index is related to the model before anything has been inserted.
        // As in each loop the current item-range gets inserted the index must
        // be increased by the already previously inserted items.
        const int index = range.index + previouslyInsertedCount;
        const int count = range.count;
        if (index < 0 || count <= 0) {
            qCWarning(DolphinDebug) << "Invalid item range (index:" << index << ", count:" << count << ")";
            continue;
        }
        previouslyInsertedCount += count;

        // Determine which visible items must be moved
        QList<int> itemsToMove;
        QHashIterator<int, KItemListWidget *> it(m_visibleItems);
        while (it.hasNext()) {
            it.next();
            const int visibleItemIndex = it.key();
            if (visibleItemIndex >= index) {
                itemsToMove.append(visibleItemIndex);
            }
        }

        // Update the indexes of all KItemListWidget instances that are located
        // after the inserted items. It is important to adjust the indexes in the order
        // from the highest index to the lowest index to prevent overlaps when setting the new index.
        std::sort(itemsToMove.begin(), itemsToMove.end());
        for (int i = itemsToMove.count() - 1; i >= 0; --i) {
            KItemListWidget *widget = m_visibleItems.value(itemsToMove[i]);
            Q_ASSERT(widget);
            const int newIndex = widget->index() + count;
            if (hasMultipleRanges) {
                setWidgetIndex(widget, newIndex);
            } else {
                // Try to animate the moving of the item
                moveWidgetToIndex(widget, newIndex);
            }
        }

        if (m_model->count() == count && m_activeTransactions == 0) {
            // Check whether a scrollbar is required to show the inserted items. In this case
            // the size of the layouter will be decreased before calling doLayout(): This prevents
            // an unnecessary temporary animation due to the geometry change of the inserted scrollbar.
            const bool verticalScrollOrientation = (scrollOrientation() == Qt::Vertical);
            const bool decreaseLayouterSize = (verticalScrollOrientation && maximumScrollOffset() > size().height())
                || (!verticalScrollOrientation && maximumScrollOffset() > size().width());
            if (decreaseLayouterSize) {
                const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

                int scrollbarSpacing = 0;
                if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) {
                    scrollbarSpacing = style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing);
                }

                QSizeF layouterSize = m_layouter->size();
                if (verticalScrollOrientation) {
                    layouterSize.rwidth() -= scrollBarExtent + scrollbarSpacing;
                } else {
                    layouterSize.rheight() -= scrollBarExtent + scrollbarSpacing;
                }
                m_layouter->setSize(layouterSize);
            }
        }

        if (!hasMultipleRanges) {
            doLayout(animateChangedItemCount(count) ? Animation : NoAnimation, index, count);
            updateSiblingsInformation();
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsInserted(itemRanges);
    }

    if (hasMultipleRanges) {
        m_endTransactionAnimationHint = NoAnimation;
        endTransaction();

        updateSiblingsInformation();
    }

    if (m_grouped && (hasMultipleRanges || itemRanges.first().count < m_model->count())) {
        // In case if items of the same group have been inserted before an item that
        // currently represents the first item of the group, the group header of
        // this item must be removed.
        updateVisibleGroupHeaders();
    }

    if (useAlternateBackgrounds()) {
        updateAlternateBackgrounds();
    }
}

void KItemListView::slotItemsRemoved(const KItemRangeList &itemRanges)
{
    if (m_itemSize.isEmpty()) {
        // Don't pass the item-range: The preferred column-widths of
        // all items must be adjusted when removing items.
        updatePreferredColumnWidths();
    }

    const bool hasMultipleRanges = (itemRanges.count() > 1);
    if (hasMultipleRanges) {
        beginTransaction();
    }

    m_layouter->markAsDirty();

    m_sizeHintResolver->itemsRemoved(itemRanges);

    for (int i = itemRanges.count() - 1; i >= 0; --i) {
        const KItemRange &range = itemRanges[i];
        const int index = range.index;
        const int count = range.count;
        if (index < 0 || count <= 0) {
            qCWarning(DolphinDebug) << "Invalid item range (index:" << index << ", count:" << count << ")";
            continue;
        }

        const int firstRemovedIndex = index;
        const int lastRemovedIndex = index + count - 1;

        // Remember which items have to be moved because they are behind the removed range.
        QVector<int> itemsToMove;

        // Remove all KItemListWidget instances that got deleted
        // Iterate over a const copy because the container is mutated within the loop
        // directly and in `recycleWidget()` (https://bugs.kde.org/show_bug.cgi?id=428374)
        const auto visibleItems = m_visibleItems;
        for (KItemListWidget *widget : visibleItems) {
            const int i = widget->index();
            if (i < firstRemovedIndex) {
                continue;
            } else if (i > lastRemovedIndex) {
                itemsToMove.append(i);
                continue;
            }

            m_animation->stop(widget);
            // Stopping the animation might lead to recycling the widget if
            // it is invisible (see slotAnimationFinished()).
            // Check again whether it is still visible:
            if (!m_visibleItems.contains(i)) {
                continue;
            }

            if (m_model->count() == 0 || hasMultipleRanges || !animateChangedItemCount(count)) {
                // Remove the widget without animation
                recycleWidget(widget);
            } else {
                // Animate the removing of the items. Special case: When removing an item there
                // is no valid model index available anymore. For the
                // remove-animation the item gets removed from m_visibleItems but the widget
                // will stay alive until the animation has been finished and will
                // be recycled (deleted) in KItemListView::slotAnimationFinished().
                m_visibleItems.remove(i);
                widget->setIndex(-1);
                m_animation->start(widget, KItemListViewAnimation::DeleteAnimation);
            }
        }

        // Update the indexes of all KItemListWidget instances that are located
        // after the deleted items. It is important to update them in ascending
        // order to prevent overlaps when setting the new index.
        std::sort(itemsToMove.begin(), itemsToMove.end());
        for (int i : std::as_const(itemsToMove)) {
            KItemListWidget *widget = m_visibleItems.value(i);
            Q_ASSERT(widget);
            const int newIndex = i - count;
            if (hasMultipleRanges) {
                setWidgetIndex(widget, newIndex);
            } else {
                // Try to animate the moving of the item
                moveWidgetToIndex(widget, newIndex);
            }
        }

        if (!hasMultipleRanges) {
            // The decrease-layout-size optimization in KItemListView::slotItemsInserted()
            // assumes an updated geometry. If items are removed during an active transaction,
            // the transaction will be temporary deactivated so that doLayout() triggers a
            // geometry update if necessary.
            const int activeTransactions = m_activeTransactions;
            m_activeTransactions = 0;
            doLayout(animateChangedItemCount(count) ? Animation : NoAnimation, index, -count);
            m_activeTransactions = activeTransactions;
            updateSiblingsInformation();
        }
    }

    if (m_controller) {
        m_controller->selectionManager()->itemsRemoved(itemRanges);
    }

    if (hasMultipleRanges) {
        m_endTransactionAnimationHint = NoAnimation;
        endTransaction();
        updateSiblingsInformation();
    }

    if (m_grouped && (hasMultipleRanges || m_model->count() > 0)) {
        // In case if the first item of a group has been removed, the group header
        // must be applied to the next visible item.
        updateVisibleGroupHeaders();
    }

    if (useAlternateBackgrounds()) {
        updateAlternateBackgrounds();
    }
}

void KItemListView::slotItemsMoved(const KItemRange &itemRange, const QList<int> &movedToIndexes)
{
    m_sizeHintResolver->itemsMoved(itemRange, movedToIndexes);
    m_layouter->markAsDirty();

    if (m_controller) {
        m_controller->selectionManager()->itemsMoved(itemRange, movedToIndexes);
    }

    const int firstVisibleMovedIndex = qMax(firstVisibleIndex(), itemRange.index);
    const int lastVisibleMovedIndex = qMin(lastVisibleIndex(), itemRange.index + itemRange.count - 1);

    /// Represents an item that was moved while being edited.
    struct MovedEditedItem {
        int movedToIndex;
        QByteArray editedRole;
    };
    std::optional<MovedEditedItem> movedEditedItem;
    for (int index = firstVisibleMovedIndex; index <= lastVisibleMovedIndex; ++index) {
        KItemListWidget *widget = m_visibleItems.value(index);
        if (widget) {
            if (m_editingRole && !widget->editedRole().isEmpty()) {
                movedEditedItem = {movedToIndexes[index - itemRange.index], widget->editedRole()};
                disconnectRoleEditingSignals(index);
                m_editingRole = false;
            }
            updateWidgetProperties(widget, index);
            initializeItemListWidget(widget);
        }
    }

    doLayout(NoAnimation);
    updateSiblingsInformation();

    if (movedEditedItem) {
        editRole(movedEditedItem->movedToIndex, movedEditedItem->editedRole);
    }
}

void KItemListView::slotItemsChanged(const KItemRangeList &itemRanges, const QSet<QByteArray> &roles)
{
    const bool updateSizeHints = itemSizeHintUpdateRequired(roles);
    if (updateSizeHints && m_itemSize.isEmpty()) {
        updatePreferredColumnWidths(itemRanges);
    }

    for (const KItemRange &itemRange : itemRanges) {
        const int index = itemRange.index;
        const int count = itemRange.count;

        if (updateSizeHints) {
            m_sizeHintResolver->itemsChanged(index, count, roles);
            m_layouter->markAsDirty();
        }

        // Apply the changed roles to the visible item-widgets
        const int lastIndex = index + count - 1;
        for (int i = index; i <= lastIndex; ++i) {
            KItemListWidget *widget = m_visibleItems.value(i);
            if (widget) {
                widget->setData(m_model->data(i), roles);
            }
        }

        if (m_grouped && roles.contains(m_model->sortRole())) {
            // The sort-role has been changed which might result
            // in modified group headers
            updateVisibleGroupHeaders();
            doLayout(NoAnimation);
        }
    }
    doLayout(NoAnimation);
}

void KItemListView::slotGroupsChanged()
{
    updateVisibleGroupHeaders();
    doLayout(NoAnimation);
    updateSiblingsInformation();
}

void KItemListView::slotGroupedSortingChanged(bool current)
{
    m_grouped = current;
    m_layouter->markAsDirty();

    if (m_grouped) {
        updateGroupHeaderHeight();
    } else {
        // Clear all visible headers. Note that the QHashIterator takes a copy of
        // m_visibleGroups. Therefore, it remains valid even if items are removed
        // from m_visibleGroups in recycleGroupHeaderForWidget().
        QHashIterator<KItemListWidget *, KItemListGroupHeader *> it(m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            recycleGroupHeaderForWidget(it.key());
        }
        Q_ASSERT(m_visibleGroups.isEmpty());
    }

    if (useAlternateBackgrounds()) {
        // Changing the group mode requires to update the alternate backgrounds
        // as with the enabled group mode the altering is done on base of the first
        // group item.
        updateAlternateBackgrounds();
    }
    updateSiblingsInformation();
    doLayout(NoAnimation);
}

void KItemListView::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    if (m_grouped) {
        updateVisibleGroupHeaders();
        doLayout(NoAnimation);
    }
}

void KItemListView::slotSortRoleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    if (m_grouped) {
        updateVisibleGroupHeaders();
        doLayout(NoAnimation);
    }
}

void KItemListView::slotCurrentChanged(int current, int previous)
{
    // In SingleSelection mode (e.g., in the Places Panel), the current item is
    // always the selected item. It is not necessary to highlight the current item then.
    if (m_controller->selectionBehavior() != KItemListController::SingleSelection) {
        KItemListWidget *previousWidget = m_visibleItems.value(previous, nullptr);
        if (previousWidget) {
            previousWidget->setCurrent(false);
        }

        KItemListWidget *currentWidget = m_visibleItems.value(current, nullptr);
        if (currentWidget) {
            currentWidget->setCurrent(true);
        }
    }
#ifndef QT_NO_ACCESSIBILITY
    if (current != previous && QAccessible::isActive()) {
        static_cast<KItemListViewAccessible *>(QAccessible::queryAccessibleInterface(this))->announceCurrentItem();
    }
#endif
}

void KItemListView::slotSelectionChanged(const KItemSet &current, const KItemSet &previous)
{
    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        const int index = it.key();
        KItemListWidget *widget = it.value();
        const bool isSelected(current.contains(index));
        widget->setSelected(isSelected);

#ifndef QT_NO_ACCESSIBILITY
        if (!QAccessible::isActive()) {
            continue;
        }
        // Let the screen reader announce "selected" or "not selected" for the active item.
        const bool wasSelected(previous.contains(index));
        if (isSelected != wasSelected) {
            QAccessibleEvent accessibleSelectionChangedEvent(this, QAccessible::SelectionAdd);
            accessibleSelectionChangedEvent.setChild(index);
            QAccessible::updateAccessibility(&accessibleSelectionChangedEvent);
        }
    }
#else
    }
    Q_UNUSED(previous)
#endif
}

void KItemListView::slotAnimationFinished(QGraphicsWidget *widget, KItemListViewAnimation::AnimationType type)
{
    KItemListWidget *itemListWidget = qobject_cast<KItemListWidget *>(widget);
    Q_ASSERT(itemListWidget);

    if (type == KItemListViewAnimation::DeleteAnimation) {
        // As we recycle the widget in this case it is important to assure that no
        // other animation has been started. This is a convention in KItemListView and
        // not a requirement defined by KItemListViewAnimation.
        Q_ASSERT(!m_animation->isStarted(itemListWidget));

        // All KItemListWidgets that are animated by the DeleteAnimation are not maintained
        // by m_visibleWidgets and must be deleted manually after the animation has
        // been finished.
        recycleGroupHeaderForWidget(itemListWidget);
        widgetCreator()->recycle(itemListWidget);
    } else {
        const int index = itemListWidget->index();
        const bool invisible = (index < m_layouter->firstVisibleIndex()) || (index > m_layouter->lastVisibleIndex());
        if (invisible && !m_animation->isStarted(itemListWidget)) {
            recycleWidget(itemListWidget);
        }
    }
}

void KItemListView::slotRubberBandPosChanged()
{
    update();
}

void KItemListView::slotRubberBandActivationChanged(bool active)
{
    if (active) {
        connect(m_rubberBand, &KItemListRubberBand::startPositionChanged, this, &KItemListView::slotRubberBandPosChanged);
        connect(m_rubberBand, &KItemListRubberBand::endPositionChanged, this, &KItemListView::slotRubberBandPosChanged);
        m_skipAutoScrollForRubberBand = true;
    } else {
        QRectF rubberBandRect = QRectF(m_rubberBand->startPosition(), m_rubberBand->endPosition()).normalized();

        auto animation = new QVariantAnimation(this);
        animation->setStartValue(1.0);
        animation->setEndValue(0.0);
        animation->setDuration(RubberFadeSpeed);
        animation->setProperty(RubberPropertyName, rubberBandRect);

        QEasingCurve curve;
        curve.setType(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(1.0, 1.0), QPointF(1.0, 1.0));
        animation->setEasingCurve(curve);

        connect(animation, &QVariantAnimation::valueChanged, this, [=, this](const QVariant &) {
            update();
        });
        connect(animation, &QVariantAnimation::finished, this, [=, this]() {
            m_rubberBandAnimations.removeAll(animation);
            delete animation;
        });
        animation->start();
        m_rubberBandAnimations << animation;

        disconnect(m_rubberBand, &KItemListRubberBand::startPositionChanged, this, &KItemListView::slotRubberBandPosChanged);
        disconnect(m_rubberBand, &KItemListRubberBand::endPositionChanged, this, &KItemListView::slotRubberBandPosChanged);
        m_skipAutoScrollForRubberBand = false;
    }

    update();
}

void KItemListView::slotHeaderColumnWidthChanged(const QByteArray &role, qreal currentWidth, qreal previousWidth)
{
    Q_UNUSED(role)
    Q_UNUSED(currentWidth)
    Q_UNUSED(previousWidth)

    m_headerWidget->setAutomaticColumnResizing(false);
    applyColumnWidthsFromHeader();
    doLayout(NoAnimation);
}

void KItemListView::slotSidePaddingChanged(qreal width)
{
    Q_UNUSED(width)
    if (m_headerWidget->automaticColumnResizing()) {
        applyAutomaticColumnWidths();
    }
    applyColumnWidthsFromHeader();
    doLayout(NoAnimation);
}

void KItemListView::slotHeaderColumnMoved(const QByteArray &role, int currentIndex, int previousIndex)
{
    Q_ASSERT(m_visibleRoles[previousIndex] == role);

    const QList<QByteArray> previous = m_visibleRoles;

    QList<QByteArray> current = m_visibleRoles;
    current.removeAt(previousIndex);
    current.insert(currentIndex, role);

    setVisibleRoles(current);

    Q_EMIT visibleRolesChanged(current, previous);
}

void KItemListView::triggerAutoScrolling()
{
    if (!m_autoScrollTimer) {
        return;
    }

    int pos = 0;
    int visibleSize = 0;
    if (scrollOrientation() == Qt::Vertical) {
        pos = m_mousePos.y();
        visibleSize = size().height();
    } else {
        pos = m_mousePos.x();
        visibleSize = size().width();
    }

    if (m_autoScrollTimer->interval() == InitialAutoScrollDelay) {
        m_autoScrollIncrement = 0;
    }

    m_autoScrollIncrement = calculateAutoScrollingIncrement(pos, visibleSize, m_autoScrollIncrement);
    if (m_autoScrollIncrement == 0) {
        // The mouse position is not above an autoscroll margin (the autoscroll timer
        // will be restarted in mouseMoveEvent())
        m_autoScrollTimer->stop();
        return;
    }

    if (m_rubberBand->isActive() && m_skipAutoScrollForRubberBand) {
        // If a rubberband selection is ongoing the autoscrolling may only get triggered
        // if the direction of the rubberband is similar to the autoscroll direction. This
        // prevents that starting to create a rubberband within the autoscroll margins starts
        // an autoscrolling.

        const qreal minDiff = 4; // Ignore any autoscrolling if the rubberband is very small
        const qreal diff = (scrollOrientation() == Qt::Vertical) ? m_rubberBand->endPosition().y() - m_rubberBand->startPosition().y()
                                                                 : m_rubberBand->endPosition().x() - m_rubberBand->startPosition().x();
        if (qAbs(diff) < minDiff || (m_autoScrollIncrement < 0 && diff > 0) || (m_autoScrollIncrement > 0 && diff < 0)) {
            // The rubberband direction is different from the scroll direction (e.g. the rubberband has
            // been moved up although the autoscroll direction might be down)
            m_autoScrollTimer->stop();
            return;
        }
    }

    // As soon as the autoscrolling has been triggered at least once despite having an active rubberband,
    // the autoscrolling may not get skipped anymore until a new rubberband is created
    m_skipAutoScrollForRubberBand = false;

    const qreal maxVisibleOffset = qMax(qreal(0), maximumScrollOffset() - visibleSize);
    const qreal newScrollOffset = qMin(scrollOffset() + m_autoScrollIncrement, maxVisibleOffset);
    setScrollOffset(newScrollOffset);

    // Trigger the autoscroll timer which will periodically call
    // triggerAutoScrolling()
    m_autoScrollTimer->start(RepeatingAutoScrollDelay);
}

void KItemListView::slotGeometryOfGroupHeaderParentChanged()
{
    KItemListWidget *widget = qobject_cast<KItemListWidget *>(sender());
    Q_ASSERT(widget);
    KItemListGroupHeader *groupHeader = m_visibleGroups.value(widget);
    Q_ASSERT(groupHeader);
    updateGroupHeaderLayout(widget);
}

void KItemListView::slotRoleEditingCanceled(int index, const QByteArray &role, const QVariant &value)
{
    disconnectRoleEditingSignals(index);

    m_editingRole = false;
    Q_EMIT roleEditingCanceled(index, role, value);
}

void KItemListView::slotRoleEditingFinished(int index, const QByteArray &role, const QVariant &value)
{
    disconnectRoleEditingSignals(index);

    m_editingRole = false;
    Q_EMIT roleEditingFinished(index, role, value);
}

void KItemListView::setController(KItemListController *controller)
{
    if (m_controller != controller) {
        KItemListController *previous = m_controller;
        if (previous) {
            KItemListSelectionManager *selectionManager = previous->selectionManager();
            disconnect(selectionManager, &KItemListSelectionManager::currentChanged, this, &KItemListView::slotCurrentChanged);
            disconnect(selectionManager, &KItemListSelectionManager::selectionChanged, this, &KItemListView::slotSelectionChanged);
        }

        m_controller = controller;

        if (controller) {
            KItemListSelectionManager *selectionManager = controller->selectionManager();
            connect(selectionManager, &KItemListSelectionManager::currentChanged, this, &KItemListView::slotCurrentChanged);
            connect(selectionManager, &KItemListSelectionManager::selectionChanged, this, &KItemListView::slotSelectionChanged);
        }

        onControllerChanged(controller, previous);
    }
}

void KItemListView::setModel(KItemModelBase *model)
{
    if (m_model == model) {
        return;
    }

    KItemModelBase *previous = m_model;

    if (m_model) {
        disconnect(m_model, &KItemModelBase::itemsChanged, this, &KItemListView::slotItemsChanged);
        disconnect(m_model, &KItemModelBase::itemsInserted, this, &KItemListView::slotItemsInserted);
        disconnect(m_model, &KItemModelBase::itemsRemoved, this, &KItemListView::slotItemsRemoved);
        disconnect(m_model, &KItemModelBase::itemsMoved, this, &KItemListView::slotItemsMoved);
        disconnect(m_model, &KItemModelBase::groupsChanged, this, &KItemListView::slotGroupsChanged);
        disconnect(m_model, &KItemModelBase::groupedSortingChanged, this, &KItemListView::slotGroupedSortingChanged);
        disconnect(m_model, &KItemModelBase::sortOrderChanged, this, &KItemListView::slotSortOrderChanged);
        disconnect(m_model, &KItemModelBase::sortRoleChanged, this, &KItemListView::slotSortRoleChanged);

        m_sizeHintResolver->itemsRemoved(KItemRangeList() << KItemRange(0, m_model->count()));
    }

    m_model = model;
    m_layouter->setModel(model);
    m_grouped = model->groupedSorting();

    if (m_model) {
        connect(m_model, &KItemModelBase::itemsChanged, this, &KItemListView::slotItemsChanged);
        connect(m_model, &KItemModelBase::itemsInserted, this, &KItemListView::slotItemsInserted);
        connect(m_model, &KItemModelBase::itemsRemoved, this, &KItemListView::slotItemsRemoved);
        connect(m_model, &KItemModelBase::itemsMoved, this, &KItemListView::slotItemsMoved);
        connect(m_model, &KItemModelBase::groupsChanged, this, &KItemListView::slotGroupsChanged);
        connect(m_model, &KItemModelBase::groupedSortingChanged, this, &KItemListView::slotGroupedSortingChanged);
        connect(m_model, &KItemModelBase::sortOrderChanged, this, &KItemListView::slotSortOrderChanged);
        connect(m_model, &KItemModelBase::sortRoleChanged, this, &KItemListView::slotSortRoleChanged);

        const int itemCount = m_model->count();
        if (itemCount > 0) {
            slotItemsInserted(KItemRangeList() << KItemRange(0, itemCount));
        }
    }

    onModelChanged(model, previous);
}

KItemListRubberBand *KItemListView::rubberBand() const
{
    return m_rubberBand;
}

void KItemListView::doLayout(LayoutAnimationHint hint, int changedIndex, int changedCount)
{
    if (m_activeTransactions > 0) {
        if (hint == NoAnimation) {
            // As soon as at least one property change should be done without animation,
            // the whole transaction will be marked as not animated.
            m_endTransactionAnimationHint = NoAnimation;
        }
        return;
    }

    if (!m_model || m_model->count() < 0) {
        return;
    }

    int firstVisibleIndex = m_layouter->firstVisibleIndex();
    if (firstVisibleIndex < 0) {
        emitOffsetChanges();
        return;
    }

    // Do a sanity check of the scroll-offset property: When properties of the itemlist-view have been changed
    // it might be possible that the maximum offset got changed too. Assure that the full visible range
    // is still shown if the maximum offset got decreased.
    const qreal visibleOffsetRange = (scrollOrientation() == Qt::Horizontal) ? size().width() : size().height();
    const qreal maxOffsetToShowFullRange = maximumScrollOffset() - visibleOffsetRange;
    if (scrollOffset() > maxOffsetToShowFullRange) {
        m_layouter->setScrollOffset(qMax(qreal(0), maxOffsetToShowFullRange));
        firstVisibleIndex = m_layouter->firstVisibleIndex();
    }

    const int lastVisibleIndex = m_layouter->lastVisibleIndex();

    int firstSibblingIndex = -1;
    int lastSibblingIndex = -1;
    const bool supportsExpanding = supportsItemExpanding();

    QList<int> reusableItems = recycleInvisibleItems(firstVisibleIndex, lastVisibleIndex, hint);

    // Assure that for each visible item a KItemListWidget is available. KItemListWidget
    // instances from invisible items are reused. If no reusable items are
    // found then new KItemListWidget instances get created.
    const bool animate = (hint == Animation);
    for (int i = firstVisibleIndex; i <= lastVisibleIndex; ++i) {
        bool applyNewPos = true;

        const QRectF itemBounds = m_layouter->itemRect(i);
        const QPointF newPos = itemBounds.topLeft();
        KItemListWidget *widget = m_visibleItems.value(i);
        if (!widget) {
            if (!reusableItems.isEmpty()) {
                // Reuse a KItemListWidget instance from an invisible item
                const int oldIndex = reusableItems.takeLast();
                widget = m_visibleItems.value(oldIndex);
                setWidgetIndex(widget, i);
                updateWidgetProperties(widget, i);
                initializeItemListWidget(widget);
            } else {
                // No reusable KItemListWidget instance is available, create a new one
                widget = createWidget(i);
            }
            widget->resize(itemBounds.size());

            if (animate && changedCount < 0) {
                // Items have been deleted.
                if (i >= changedIndex) {
                    // The item is located behind the removed range. Move the
                    // created item to the imaginary old position outside the
                    // view. It will get animated to the new position later.
                    const int previousIndex = i - changedCount;
                    const QRectF itemRect = m_layouter->itemRect(previousIndex);
                    if (itemRect.isEmpty()) {
                        const QPointF invisibleOldPos = (scrollOrientation() == Qt::Vertical) ? QPointF(0, size().height()) : QPointF(size().width(), 0);
                        widget->setPos(invisibleOldPos);
                    } else {
                        widget->setPos(itemRect.topLeft());
                    }
                    applyNewPos = false;
                }
            }

            if (supportsExpanding && changedCount == 0) {
                if (firstSibblingIndex < 0) {
                    firstSibblingIndex = i;
                }
                lastSibblingIndex = i;
            }
        }

        if (animate) {
            if (m_animation->isStarted(widget, KItemListViewAnimation::MovingAnimation)) {
                m_animation->start(widget, KItemListViewAnimation::MovingAnimation, newPos);
                applyNewPos = false;
            }

            const bool itemsRemoved = (changedCount < 0);
            const bool itemsInserted = (changedCount > 0);
            if (itemsRemoved && (i >= changedIndex)) {
                // The item is located after the removed items. Animate the moving of the position.
                applyNewPos = !moveWidget(widget, newPos);
            } else if (itemsInserted && i >= changedIndex) {
                // The item is located after the first inserted item
                if (i <= changedIndex + changedCount - 1) {
                    // The item is an inserted item. Animate the appearing of the item.
                    // For performance reasons no animation is done when changedCount is equal
                    // to all available items.
                    if (changedCount < m_model->count()) {
                        m_animation->start(widget, KItemListViewAnimation::CreateAnimation);
                    }
                } else if (!m_animation->isStarted(widget, KItemListViewAnimation::CreateAnimation)) {
                    // The item was already there before, so animate the moving of the position.
                    // No moving animation is done if the item is animated by a create animation: This
                    // prevents a "move animation mess" when inserting several ranges in parallel.
                    applyNewPos = !moveWidget(widget, newPos);
                }
            }
        } else {
            m_animation->stop(widget);
        }

        if (applyNewPos) {
            widget->setPos(newPos);
        }

        Q_ASSERT(widget->index() == i);
        widget->setVisible(true);

        bool animateIconResizing = animate;

        if (widget->size() != itemBounds.size()) {
            // Resize the widget for the item to the changed size.
            if (animate) {
                // If a dynamic item size is used then no animation is done in the direction
                // of the dynamic size.
                if (m_itemSize.width() <= 0) {
                    // The width is dynamic, apply the new width without animation.
                    widget->resize(itemBounds.width(), widget->size().height());
                } else if (m_itemSize.height() <= 0) {
                    // The height is dynamic, apply the new height without animation.
                    widget->resize(widget->size().width(), itemBounds.height());
                }
                m_animation->start(widget, KItemListViewAnimation::ResizeAnimation, itemBounds.size());
            } else {
                widget->resize(itemBounds.size());
            }
        } else {
            animateIconResizing = false;
        }

        const int newIconSize = widget->styleOption().iconSize;
        if (widget->iconSize() != newIconSize) {
            if (animateIconResizing) {
                m_animation->start(widget, KItemListViewAnimation::IconResizeAnimation, newIconSize);
            } else {
                widget->setIconSize(newIconSize);
            }
        }

        // Updating the cell-information must be done as last step: The decision whether the
        // moving-animation should be started at all is based on the previous cell-information.
        const Cell cell(m_layouter->itemColumn(i), m_layouter->itemRow(i));
        m_visibleCells.insert(i, cell);
    }

    // Delete invisible KItemListWidget instances that have not been reused
    for (int index : std::as_const(reusableItems)) {
        recycleWidget(m_visibleItems.value(index));
    }

    if (supportsExpanding && firstSibblingIndex >= 0) {
        Q_ASSERT(lastSibblingIndex >= 0);
        updateSiblingsInformation(firstSibblingIndex, lastSibblingIndex);
    }

    if (m_grouped) {
        // Update the layout of all visible group headers
        QHashIterator<KItemListWidget *, KItemListGroupHeader *> it(m_visibleGroups);
        while (it.hasNext()) {
            it.next();
            updateGroupHeaderLayout(it.key());
        }
    }

    emitOffsetChanges();
}

QList<int> KItemListView::recycleInvisibleItems(int firstVisibleIndex, int lastVisibleIndex, LayoutAnimationHint hint)
{
    // Determine all items that are completely invisible and might be
    // reused for items that just got (at least partly) visible. If the
    // animation hint is set to 'Animation' items that do e.g. an animated
    // moving of their position are not marked as invisible: This assures
    // that a scrolling inside the view can be done without breaking an animation.

    QList<int> items;

    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();

        KItemListWidget *widget = it.value();
        const int index = widget->index();
        const bool invisible = (index < firstVisibleIndex) || (index > lastVisibleIndex);

        if (invisible) {
            if (m_animation->isStarted(widget)) {
                if (hint == NoAnimation) {
                    // Stopping the animation will call KItemListView::slotAnimationFinished()
                    // and the widget will be recycled if necessary there.
                    m_animation->stop(widget);
                }
            } else {
                widget->setVisible(false);
                items.append(index);

                if (m_grouped) {
                    recycleGroupHeaderForWidget(widget);
                }
            }
        }
    }

    return items;
}

bool KItemListView::moveWidget(KItemListWidget *widget, const QPointF &newPos)
{
    if (widget->pos() == newPos) {
        return false;
    }

    bool startMovingAnim = false;

    if (m_itemSize.isEmpty()) {
        // The items are not aligned in a grid but either as columns or rows.
        startMovingAnim = true;
    } else {
        // When having a grid the moving-animation should only be started, if it is done within
        // one row in the vertical scroll-orientation or one column in the horizontal scroll-orientation.
        // Otherwise instead of a moving-animation a create-animation on the new position will be used
        // instead. This is done to prevent overlapping (and confusing) moving-animations.
        const int index = widget->index();
        const Cell cell = m_visibleCells.value(index);
        if (cell.column >= 0 && cell.row >= 0) {
            if (scrollOrientation() == Qt::Vertical) {
                startMovingAnim = (cell.row == m_layouter->itemRow(index));
            } else {
                startMovingAnim = (cell.column == m_layouter->itemColumn(index));
            }
        }
    }

    if (startMovingAnim) {
        m_animation->start(widget, KItemListViewAnimation::MovingAnimation, newPos);
        return true;
    }

    m_animation->stop(widget);
    m_animation->start(widget, KItemListViewAnimation::CreateAnimation);
    return false;
}

void KItemListView::emitOffsetChanges()
{
    const qreal newScrollOffset = m_layouter->scrollOffset();
    if (m_oldScrollOffset != newScrollOffset) {
        Q_EMIT scrollOffsetChanged(newScrollOffset, m_oldScrollOffset);
        m_oldScrollOffset = newScrollOffset;
    }

    const qreal newMaximumScrollOffset = m_layouter->maximumScrollOffset();
    if (m_oldMaximumScrollOffset != newMaximumScrollOffset) {
        Q_EMIT maximumScrollOffsetChanged(newMaximumScrollOffset, m_oldMaximumScrollOffset);
        m_oldMaximumScrollOffset = newMaximumScrollOffset;
    }

    const qreal newItemOffset = m_layouter->itemOffset();
    if (m_oldItemOffset != newItemOffset) {
        Q_EMIT itemOffsetChanged(newItemOffset, m_oldItemOffset);
        m_oldItemOffset = newItemOffset;
    }

    const qreal newMaximumItemOffset = m_layouter->maximumItemOffset();
    if (m_oldMaximumItemOffset != newMaximumItemOffset) {
        Q_EMIT maximumItemOffsetChanged(newMaximumItemOffset, m_oldMaximumItemOffset);
        m_oldMaximumItemOffset = newMaximumItemOffset;
    }
}

KItemListWidget *KItemListView::createWidget(int index)
{
    KItemListWidget *widget = widgetCreator()->create(this);
    widget->setFlag(QGraphicsItem::ItemStacksBehindParent);

    m_visibleItems.insert(index, widget);
    m_visibleCells.insert(index, Cell());
    updateWidgetProperties(widget, index);
    initializeItemListWidget(widget);
    return widget;
}

void KItemListView::recycleWidget(KItemListWidget *widget)
{
    if (m_grouped) {
        recycleGroupHeaderForWidget(widget);
    }

    const int index = widget->index();
    m_visibleItems.remove(index);
    m_visibleCells.remove(index);

    widgetCreator()->recycle(widget);
}

void KItemListView::setWidgetIndex(KItemListWidget *widget, int index)
{
    const int oldIndex = widget->index();
    m_visibleItems.remove(oldIndex);
    m_visibleCells.remove(oldIndex);

    m_visibleItems.insert(index, widget);
    m_visibleCells.insert(index, Cell());

    widget->setIndex(index);
}

void KItemListView::moveWidgetToIndex(KItemListWidget *widget, int index)
{
    const int oldIndex = widget->index();
    const Cell oldCell = m_visibleCells.value(oldIndex);

    setWidgetIndex(widget, index);

    const Cell newCell(m_layouter->itemColumn(index), m_layouter->itemRow(index));
    const bool vertical = (scrollOrientation() == Qt::Vertical);
    const bool updateCell = (vertical && oldCell.row == newCell.row) || (!vertical && oldCell.column == newCell.column);
    if (updateCell) {
        m_visibleCells.insert(index, newCell);
    }
}

void KItemListView::setLayouterSize(const QSizeF &size, SizeType sizeType)
{
    switch (sizeType) {
    case LayouterSize:
        m_layouter->setSize(size);
        break;
    case ItemSize:
        m_layouter->setItemSize(size);
        break;
    default:
        break;
    }
}

void KItemListView::updateWidgetProperties(KItemListWidget *widget, int index)
{
    widget->setVisibleRoles(m_visibleRoles);
    updateWidgetColumnWidths(widget);
    widget->setStyleOption(m_styleOption);

    const KItemListSelectionManager *selectionManager = m_controller->selectionManager();

    // In SingleSelection mode (e.g., in the Places Panel), the current item is
    // always the selected item. It is not necessary to highlight the current item then.
    if (m_controller->selectionBehavior() != KItemListController::SingleSelection) {
        widget->setCurrent(index == selectionManager->currentItem());
    }
    widget->setSelected(selectionManager->isSelected(index));
    widget->setHovered(false);
    widget->setEnabledSelectionToggle(enabledSelectionToggles());
    widget->setIndex(index);
    widget->setData(m_model->data(index));
    widget->setSiblingsInformation(QBitArray());
    updateAlternateBackgroundForWidget(widget);

    if (m_grouped) {
        updateGroupHeaderForWidget(widget);
    }
}

void KItemListView::updateGroupHeaderForWidget(KItemListWidget *widget)
{
    Q_ASSERT(m_grouped);

    const int index = widget->index();
    if (!m_layouter->isFirstGroupItem(index)) {
        // The widget does not represent the first item of a group
        // and hence requires no header
        recycleGroupHeaderForWidget(widget);
        return;
    }

    const QList<QPair<int, QVariant>> groups = model()->groups();
    if (groups.isEmpty() || !groupHeaderCreator()) {
        return;
    }

    KItemListGroupHeader *groupHeader = m_visibleGroups.value(widget);
    if (!groupHeader) {
        groupHeader = groupHeaderCreator()->create(this);
        groupHeader->setParentItem(widget);
        m_visibleGroups.insert(widget, groupHeader);
        connect(widget, &KItemListWidget::geometryChanged, this, &KItemListView::slotGeometryOfGroupHeaderParentChanged);
    }
    Q_ASSERT(groupHeader->parentItem() == widget);

    const int groupIndex = groupIndexForItem(index);
    Q_ASSERT(groupIndex >= 0);
    groupHeader->setData(groups.at(groupIndex).second);
    groupHeader->setRole(model()->sortRole());
    groupHeader->setStyleOption(m_styleOption);
    groupHeader->setScrollOrientation(scrollOrientation());
    groupHeader->setItemIndex(index);

    groupHeader->show();
}

void KItemListView::updateGroupHeaderLayout(KItemListWidget *widget)
{
    KItemListGroupHeader *groupHeader = m_visibleGroups.value(widget);
    Q_ASSERT(groupHeader);

    const int index = widget->index();
    const QRectF groupHeaderRect = m_layouter->groupHeaderRect(index);
    const QRectF itemRect = m_layouter->itemRect(index);

    // The group-header is a child of the itemlist widget. Translate the
    // group header position to the relative position.
    if (scrollOrientation() == Qt::Vertical) {
        // In the vertical scroll orientation the group header should always span
        // the whole width no matter which temporary position the parent widget
        // has. In this case the x-position and width will be adjusted manually.
        const qreal x = -widget->x() - itemOffset();
        const qreal width = maximumItemOffset();
        groupHeader->setPos(x, -groupHeaderRect.height());
        groupHeader->resize(width, groupHeaderRect.size().height());
    } else {
        groupHeader->setPos(groupHeaderRect.x() - itemRect.x(), -widget->y());
        groupHeader->resize(groupHeaderRect.size());
    }
}

void KItemListView::recycleGroupHeaderForWidget(KItemListWidget *widget)
{
    KItemListGroupHeader *header = m_visibleGroups.value(widget);
    if (header) {
        header->setParentItem(nullptr);
        groupHeaderCreator()->recycle(header);
        m_visibleGroups.remove(widget);
        disconnect(widget, &KItemListWidget::geometryChanged, this, &KItemListView::slotGeometryOfGroupHeaderParentChanged);
    }
}

void KItemListView::updateVisibleGroupHeaders()
{
    Q_ASSERT(m_grouped);
    m_layouter->markAsDirty();

    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        updateGroupHeaderForWidget(it.value());
    }
}

int KItemListView::groupIndexForItem(int index) const
{
    Q_ASSERT(m_grouped);

    const QList<QPair<int, QVariant>> groups = model()->groups();
    if (groups.isEmpty()) {
        return -1;
    }

    int min = 0;
    int max = groups.count() - 1;
    int mid = 0;
    do {
        mid = (min + max) / 2;
        if (index > groups[mid].first) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (groups[mid].first != index && min <= max);

    if (min > max) {
        while (groups[mid].first > index && mid > 0) {
            --mid;
        }
    }

    return mid;
}

void KItemListView::updateAlternateBackgrounds()
{
    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        updateAlternateBackgroundForWidget(it.value());
    }
}

void KItemListView::updateAlternateBackgroundForWidget(KItemListWidget *widget)
{
    bool enabled = useAlternateBackgrounds();
    if (enabled) {
        const int index = widget->index();
        enabled = (index & 0x1) > 0;
        if (m_grouped) {
            const int groupIndex = groupIndexForItem(index);
            if (groupIndex >= 0) {
                const QList<QPair<int, QVariant>> groups = model()->groups();
                const int indexOfFirstGroupItem = groups[groupIndex].first;
                const int relativeIndex = index - indexOfFirstGroupItem;
                enabled = (relativeIndex & 0x1) > 0;
            }
        }
    }
    widget->setAlternateBackground(enabled);
}

bool KItemListView::useAlternateBackgrounds() const
{
    return m_alternateBackgrounds && m_itemSize.isEmpty();
}

QHash<QByteArray, qreal> KItemListView::preferredColumnWidths(const KItemRangeList &itemRanges) const
{
    QElapsedTimer timer;
    timer.start();

    QHash<QByteArray, qreal> widths;

    // Calculate the minimum width for each column that is required
    // to show the headline unclipped.
    const QFontMetricsF fontMetrics(m_headerWidget->font());
    const int gripMargin = m_headerWidget->style()->pixelMetric(QStyle::PM_HeaderGripMargin);
    const int headerMargin = m_headerWidget->style()->pixelMetric(QStyle::PM_HeaderMargin);
    for (const QByteArray &visibleRole : std::as_const(m_visibleRoles)) {
        const QString headerText = m_model->roleDescription(visibleRole);
        const qreal headerWidth = fontMetrics.horizontalAdvance(headerText) + gripMargin + headerMargin * 2;
        widths.insert(visibleRole, headerWidth);
    }

    // Calculate the preferred column widths for each item and ignore values
    // smaller than the width for showing the headline unclipped.
    const KItemListWidgetCreatorBase *creator = widgetCreator();
    int calculatedItemCount = 0;
    bool maxTimeExceeded = false;
    for (const KItemRange &itemRange : itemRanges) {
        const int startIndex = itemRange.index;
        const int endIndex = startIndex + itemRange.count - 1;

        for (int i = startIndex; i <= endIndex; ++i) {
            for (const QByteArray &visibleRole : std::as_const(m_visibleRoles)) {
                qreal maxWidth = widths.value(visibleRole, 0);
                const qreal width = creator->preferredRoleColumnWidth(visibleRole, i, this);
                maxWidth = qMax(width, maxWidth);
                widths.insert(visibleRole, maxWidth);
            }

            if (calculatedItemCount > 100 && timer.elapsed() > 200) {
                // When having several thousands of items calculating the sizes can get
                // very expensive. We accept a possibly too small role-size in favour
                // of having no blocking user interface.
                maxTimeExceeded = true;
                break;
            }
            ++calculatedItemCount;
        }
        if (maxTimeExceeded) {
            break;
        }
    }

    return widths;
}

void KItemListView::applyColumnWidthsFromHeader()
{
    // Apply the new size to the layouter
    const qreal requiredWidth = m_headerWidget->leftPadding() + columnWidthsSum() + m_headerWidget->rightPadding();
    const QSizeF dynamicItemSize(qMax(size().width(), requiredWidth), m_itemSize.height());
    m_layouter->setItemSize(dynamicItemSize);

    // Update the role sizes for all visible widgets
    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        updateWidgetColumnWidths(it.value());
    }
}

void KItemListView::updateWidgetColumnWidths(KItemListWidget *widget)
{
    for (const QByteArray &role : std::as_const(m_visibleRoles)) {
        widget->setColumnWidth(role, m_headerWidget->columnWidth(role));
    }
    widget->setSidePadding(m_headerWidget->leftPadding(), m_headerWidget->rightPadding());
}

void KItemListView::updatePreferredColumnWidths(const KItemRangeList &itemRanges)
{
    Q_ASSERT(m_itemSize.isEmpty());
    const int itemCount = m_model->count();
    int rangesItemCount = 0;
    for (const KItemRange &range : itemRanges) {
        rangesItemCount += range.count;
    }

    if (itemCount == rangesItemCount) {
        const QHash<QByteArray, qreal> preferredWidths = preferredColumnWidths(itemRanges);
        for (const QByteArray &role : std::as_const(m_visibleRoles)) {
            m_headerWidget->setPreferredColumnWidth(role, preferredWidths.value(role));
        }
    } else {
        // Only a sub range of the roles need to be determined.
        // The chances are good that the widths of the sub ranges
        // already fit into the available widths and hence no
        // expensive update might be required.
        bool changed = false;

        const QHash<QByteArray, qreal> updatedWidths = preferredColumnWidths(itemRanges);
        QHashIterator<QByteArray, qreal> it(updatedWidths);
        while (it.hasNext()) {
            it.next();
            const QByteArray &role = it.key();
            const qreal updatedWidth = it.value();
            const qreal currentWidth = m_headerWidget->preferredColumnWidth(role);
            if (updatedWidth > currentWidth) {
                m_headerWidget->setPreferredColumnWidth(role, updatedWidth);
                changed = true;
            }
        }

        if (!changed) {
            // All the updated sizes are smaller than the current sizes and no change
            // of the stretched roles-widths is required
            return;
        }
    }

    if (m_headerWidget->automaticColumnResizing()) {
        applyAutomaticColumnWidths();
    }
}

void KItemListView::updatePreferredColumnWidths()
{
    if (m_model) {
        updatePreferredColumnWidths(KItemRangeList() << KItemRange(0, m_model->count()));
    }
}

void KItemListView::applyAutomaticColumnWidths()
{
    Q_ASSERT(m_itemSize.isEmpty());
    Q_ASSERT(m_headerWidget->automaticColumnResizing());
    if (m_visibleRoles.isEmpty()) {
        return;
    }

    // Calculate the maximum size of an item by considering the
    // visible role sizes and apply them to the layouter. If the
    // size does not use the available view-size the size of the
    // first role will get stretched.

    for (const QByteArray &role : std::as_const(m_visibleRoles)) {
        const qreal preferredWidth = m_headerWidget->preferredColumnWidth(role);
        m_headerWidget->setColumnWidth(role, preferredWidth);
    }

    const QByteArray firstRole = m_visibleRoles.first();
    qreal firstColumnWidth = m_headerWidget->columnWidth(firstRole);
    QSizeF dynamicItemSize = m_itemSize;

    qreal requiredWidth = m_headerWidget->leftPadding() + columnWidthsSum() + m_headerWidget->rightPadding();
    // By default we want the same padding symmetrically on both sides of the view. This improves UX, looks better and increases the chances of users figuring
    // out that the padding area can be used for deselecting and dropping files.
    const qreal availableWidth = size().width();
    if (requiredWidth < availableWidth) {
        // Stretch the first column to use the whole remaining width
        firstColumnWidth += availableWidth - requiredWidth;
        m_headerWidget->setColumnWidth(firstRole, firstColumnWidth);
    } else if (requiredWidth > availableWidth && m_visibleRoles.count() > 1) {
        // Shrink the first column to be able to show as much other
        // columns as possible
        qreal shrinkedFirstColumnWidth = firstColumnWidth - requiredWidth + availableWidth;

        // TODO: A proper calculation of the minimum width depends on the implementation
        // of KItemListWidget. Probably a kind of minimum size-hint should be introduced
        // later.
        const qreal minWidth = qMin(firstColumnWidth, qreal(m_styleOption.iconSize * 2 + 200));
        if (shrinkedFirstColumnWidth < minWidth) {
            shrinkedFirstColumnWidth = minWidth;
        }

        m_headerWidget->setColumnWidth(firstRole, shrinkedFirstColumnWidth);
        requiredWidth -= firstColumnWidth - shrinkedFirstColumnWidth;
    }

    dynamicItemSize.rwidth() = qMax(requiredWidth, availableWidth);

    m_layouter->setItemSize(dynamicItemSize);

    // Update the role sizes for all visible widgets
    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        updateWidgetColumnWidths(it.value());
    }
}

qreal KItemListView::columnWidthsSum() const
{
    qreal widthsSum = 0;
    for (const QByteArray &role : std::as_const(m_visibleRoles)) {
        widthsSum += m_headerWidget->columnWidth(role);
    }
    return widthsSum;
}

QRectF KItemListView::headerBoundaries() const
{
    return m_headerWidget->isVisible() ? m_headerWidget->geometry() : QRectF();
}

bool KItemListView::changesItemGridLayout(const QSizeF &newGridSize, const QSizeF &newItemSize, const QSizeF &newItemMargin) const
{
    if (newItemSize.isEmpty() || newGridSize.isEmpty()) {
        return false;
    }

    if (m_layouter->scrollOrientation() == Qt::Vertical) {
        const qreal itemWidth = m_layouter->itemSize().width();
        if (itemWidth > 0) {
            const int newColumnCount = itemsPerSize(newGridSize.width(), newItemSize.width(), newItemMargin.width());
            if (m_model->count() > newColumnCount) {
                const int oldColumnCount = itemsPerSize(m_layouter->size().width(), itemWidth, m_layouter->itemMargin().width());
                return oldColumnCount != newColumnCount;
            }
        }
    } else {
        const qreal itemHeight = m_layouter->itemSize().height();
        if (itemHeight > 0) {
            const int newRowCount = itemsPerSize(newGridSize.height(), newItemSize.height(), newItemMargin.height());
            if (m_model->count() > newRowCount) {
                const int oldRowCount = itemsPerSize(m_layouter->size().height(), itemHeight, m_layouter->itemMargin().height());
                return oldRowCount != newRowCount;
            }
        }
    }

    return false;
}

bool KItemListView::animateChangedItemCount(int changedItemCount) const
{
    if (m_itemSize.isEmpty()) {
        // We have only columns or only rows, but no grid: An animation is usually
        // welcome when inserting or removing items.
        return !supportsItemExpanding();
    }

    if (m_layouter->size().isEmpty() || m_layouter->itemSize().isEmpty()) {
        return false;
    }

    const int maximum = (scrollOrientation() == Qt::Vertical) ? m_layouter->size().width() / m_layouter->itemSize().width()
                                                              : m_layouter->size().height() / m_layouter->itemSize().height();
    // Only animate if up to 2/3 of a row or column are inserted or removed
    return changedItemCount <= maximum * 2 / 3;
}

bool KItemListView::scrollBarRequired(const QSizeF &size) const
{
    const QSizeF oldSize = m_layouter->size();

    m_layouter->setSize(size);
    const qreal maxOffset = m_layouter->maximumScrollOffset();
    m_layouter->setSize(oldSize);

    return m_layouter->scrollOrientation() == Qt::Vertical ? maxOffset > size.height() : maxOffset > size.width();
}

int KItemListView::showDropIndicator(const QPointF &pos)
{
    QHashIterator<int, KItemListWidget *> it(m_visibleItems);
    while (it.hasNext()) {
        it.next();
        const KItemListWidget *widget = it.value();

        const QPointF mappedPos = widget->mapFromItem(this, pos);
        const QRectF rect = itemRect(widget->index());
        if (mappedPos.y() >= 0 && mappedPos.y() <= rect.height()) {
            if (m_model->supportsDropping(widget->index())) {
                // Keep 30% of the rectangle as the gap instead of always having a fixed gap
                const int gap = qMax(qreal(4.0), qreal(0.3) * rect.height());
                if (mappedPos.y() >= gap && mappedPos.y() <= rect.height() - gap) {
                    return -1;
                }
            }

            const bool isAboveItem = (mappedPos.y() < rect.height() / 2);
            const qreal y = isAboveItem ? rect.top() : rect.bottom();

            const QRectF draggingInsertIndicator(rect.left(), y, rect.width(), 1);
            if (m_dropIndicator != draggingInsertIndicator) {
                m_dropIndicator = draggingInsertIndicator;
                update();
            }

            int index = widget->index();
            if (!isAboveItem) {
                ++index;
            }
            return index;
        }
    }

    const QRectF firstItemRect = itemRect(firstVisibleIndex());
    return (pos.y() <= firstItemRect.top()) ? 0 : -1;
}

void KItemListView::hideDropIndicator()
{
    if (!m_dropIndicator.isNull()) {
        m_dropIndicator = QRectF();
        update();
    }
}

void KItemListView::updateGroupHeaderHeight()
{
    qreal groupHeaderHeight = m_styleOption.fontMetrics.height();
    qreal groupHeaderMargin = 0;

    if (scrollOrientation() == Qt::Horizontal) {
        // The vertical margin above and below the header should be
        // equal to the horizontal margin, not the vertical margin
        // from m_styleOption.
        groupHeaderHeight += 2 * m_styleOption.horizontalMargin;
        groupHeaderMargin = m_styleOption.horizontalMargin;
    } else if (m_itemSize.isEmpty()) {
        groupHeaderHeight += 4 * m_styleOption.padding;
        groupHeaderMargin = m_styleOption.iconSize / 2;
    } else {
        groupHeaderHeight += 2 * m_styleOption.padding + m_styleOption.verticalMargin;
        groupHeaderMargin = m_styleOption.iconSize / 4;
    }
    m_layouter->setGroupHeaderHeight(groupHeaderHeight);
    m_layouter->setGroupHeaderMargin(groupHeaderMargin);

    updateVisibleGroupHeaders();
}

void KItemListView::updateSiblingsInformation(int firstIndex, int lastIndex)
{
    if (!supportsItemExpanding() || !m_model) {
        return;
    }

    if (firstIndex < 0 || lastIndex < 0) {
        firstIndex = m_layouter->firstVisibleIndex();
        lastIndex = m_layouter->lastVisibleIndex();
    } else {
        const bool isRangeVisible = (firstIndex <= m_layouter->lastVisibleIndex() && lastIndex >= m_layouter->firstVisibleIndex());
        if (!isRangeVisible) {
            return;
        }
    }

    int previousParents = 0;
    QBitArray previousSiblings;

    // The rootIndex describes the first index where the siblings get
    // calculated from. For the calculation the upper most parent item
    // is required. For performance reasons it is checked first whether
    // the visible items before or after the current range already
    // contain a siblings information which can be used as base.
    int rootIndex = firstIndex;

    KItemListWidget *widget = m_visibleItems.value(firstIndex - 1);
    if (!widget) {
        // There is no visible widget before the range, check whether there
        // is one after the range:
        widget = m_visibleItems.value(lastIndex + 1);
        if (widget) {
            // The sibling information of the widget may only be used if
            // all items of the range have the same number of parents.
            const int parents = m_model->expandedParentsCount(lastIndex + 1);
            for (int i = lastIndex; i >= firstIndex; --i) {
                if (m_model->expandedParentsCount(i) != parents) {
                    widget = nullptr;
                    break;
                }
            }
        }
    }

    if (widget) {
        // Performance optimization: Use the sibling information of the visible
        // widget beside the given range.
        previousSiblings = widget->siblingsInformation();
        if (previousSiblings.isEmpty()) {
            return;
        }
        previousParents = previousSiblings.count() - 1;
        previousSiblings.truncate(previousParents);
    } else {
        // Potentially slow path: Go back to the upper most parent of firstIndex
        // to be able to calculate the initial value for the siblings.
        while (rootIndex > 0 && m_model->expandedParentsCount(rootIndex) > 0) {
            --rootIndex;
        }
    }

    Q_ASSERT(previousParents >= 0);
    for (int i = rootIndex; i <= lastIndex; ++i) {
        // Update the parent-siblings in case if the current item represents
        // a child or an upper parent.
        const int currentParents = m_model->expandedParentsCount(i);
        Q_ASSERT(currentParents >= 0);
        if (previousParents < currentParents) {
            previousParents = currentParents;
            previousSiblings.resize(currentParents);
            previousSiblings.setBit(currentParents - 1, hasSiblingSuccessor(i - 1));
        } else if (previousParents > currentParents) {
            previousParents = currentParents;
            previousSiblings.truncate(currentParents);
        }

        if (i >= firstIndex) {
            // The index represents a visible item. Apply the parent-siblings
            // and update the sibling of the current item.
            KItemListWidget *widget = m_visibleItems.value(i);
            if (!widget) {
                continue;
            }

            QBitArray siblings = previousSiblings;
            siblings.resize(siblings.count() + 1);
            siblings.setBit(siblings.count() - 1, hasSiblingSuccessor(i));

            widget->setSiblingsInformation(siblings);
        }
    }
}

bool KItemListView::hasSiblingSuccessor(int index) const
{
    bool hasSuccessor = false;
    const int parentsCount = m_model->expandedParentsCount(index);
    int successorIndex = index + 1;

    // Search the next sibling
    const int itemCount = m_model->count();
    while (successorIndex < itemCount) {
        const int currentParentsCount = m_model->expandedParentsCount(successorIndex);
        if (currentParentsCount == parentsCount) {
            hasSuccessor = true;
            break;
        } else if (currentParentsCount < parentsCount) {
            break;
        }
        ++successorIndex;
    }

    if (m_grouped && hasSuccessor) {
        // If the sibling is part of another group, don't mark it as
        // successor as the group header is between the sibling connections.
        for (int i = index + 1; i <= successorIndex; ++i) {
            if (m_layouter->isFirstGroupItem(i)) {
                hasSuccessor = false;
                break;
            }
        }
    }

    return hasSuccessor;
}

void KItemListView::disconnectRoleEditingSignals(int index)
{
    KStandardItemListWidget *widget = qobject_cast<KStandardItemListWidget *>(m_visibleItems.value(index));
    if (!widget) {
        return;
    }

    disconnect(widget, &KItemListWidget::roleEditingCanceled, this, nullptr);
    disconnect(widget, &KItemListWidget::roleEditingFinished, this, nullptr);
    disconnect(this, &KItemListView::scrollOffsetChanged, widget, nullptr);
}

int KItemListView::calculateAutoScrollingIncrement(int pos, int range, int oldInc)
{
    int inc = 0;

    const int minSpeed = 4;
    const int maxSpeed = 128;
    const int speedLimiter = 96;
    const int autoScrollBorder = 64;

    // Limit the increment that is allowed to be added in comparison to 'oldInc'.
    // This assures that the autoscrolling speed grows gradually.
    const int incLimiter = 1;

    if (pos < autoScrollBorder) {
        inc = -minSpeed + qAbs(pos - autoScrollBorder) * (pos - autoScrollBorder) / speedLimiter;
        inc = qMax(inc, -maxSpeed);
        inc = qMax(inc, oldInc - incLimiter);
    } else if (pos > range - autoScrollBorder) {
        inc = minSpeed + qAbs(pos - range + autoScrollBorder) * (pos - range + autoScrollBorder) / speedLimiter;
        inc = qMin(inc, maxSpeed);
        inc = qMin(inc, oldInc + incLimiter);
    }

    return inc;
}

int KItemListView::itemsPerSize(qreal size, qreal itemSize, qreal itemMargin)
{
    const qreal availableSize = size - itemMargin;
    const int count = availableSize / (itemSize + itemMargin);
    return count;
}

KItemListCreatorBase::~KItemListCreatorBase()
{
    qDeleteAll(m_recycleableWidgets);
    qDeleteAll(m_createdWidgets);
}

void KItemListCreatorBase::addCreatedWidget(QGraphicsWidget *widget)
{
    m_createdWidgets.insert(widget);
}

void KItemListCreatorBase::pushRecycleableWidget(QGraphicsWidget *widget)
{
    Q_ASSERT(m_createdWidgets.contains(widget));
    m_createdWidgets.remove(widget);

    if (m_recycleableWidgets.count() < 100) {
        m_recycleableWidgets.append(widget);
        widget->setVisible(false);
    } else {
        delete widget;
    }
}

QGraphicsWidget *KItemListCreatorBase::popRecycleableWidget()
{
    if (m_recycleableWidgets.isEmpty()) {
        return nullptr;
    }

    QGraphicsWidget *widget = m_recycleableWidgets.takeLast();
    m_createdWidgets.insert(widget);
    return widget;
}

KItemListWidgetCreatorBase::~KItemListWidgetCreatorBase()
{
}

void KItemListWidgetCreatorBase::recycle(KItemListWidget *widget)
{
    widget->setParentItem(nullptr);
    widget->setOpacity(1.0);
    pushRecycleableWidget(widget);
}

KItemListGroupHeaderCreatorBase::~KItemListGroupHeaderCreatorBase()
{
}

void KItemListGroupHeaderCreatorBase::recycle(KItemListGroupHeader *header)
{
    header->setOpacity(1.0);
    pushRecycleableWidget(header);
}

#include "moc_kitemlistview.cpp"
