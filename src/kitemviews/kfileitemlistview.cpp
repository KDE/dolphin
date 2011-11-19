/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kfileitemlistview.h"

#include "kfileitemlistgroupheader.h"
#include "kfileitemmodelrolesupdater.h"
#include "kfileitemlistwidget.h"
#include "kfileitemmodel.h"
#include "kpixmapmodifier_p.h"
#include <KLocale>
#include <KStringHandler>

#include <KDebug>
#include <KIcon>

#include <QPainter>
#include <QTextLine>
#include <QTimer>

#define KFILEITEMLISTVIEW_DEBUG

namespace {
    const int ShortInterval = 50;
    const int LongInterval = 300;
}

KFileItemListView::KFileItemListView(QGraphicsWidget* parent) :
    KItemListView(parent),
    m_itemLayout(IconsLayout),
    m_modelRolesUpdater(0),
    m_updateVisibleIndexRangeTimer(0),
    m_updateIconSizeTimer(0),
    m_minimumRolesWidths()
{
    setAcceptDrops(true);

    setScrollOrientation(Qt::Vertical);
    setWidgetCreator(new KItemListWidgetCreator<KFileItemListWidget>());
    setGroupHeaderCreator(new KItemListGroupHeaderCreator<KFileItemListGroupHeader>());

    m_updateVisibleIndexRangeTimer = new QTimer(this);
    m_updateVisibleIndexRangeTimer->setSingleShot(true);
    m_updateVisibleIndexRangeTimer->setInterval(ShortInterval);
    connect(m_updateVisibleIndexRangeTimer, SIGNAL(timeout()), this, SLOT(updateVisibleIndexRange()));

    m_updateIconSizeTimer = new QTimer(this);
    m_updateIconSizeTimer->setSingleShot(true);
    m_updateIconSizeTimer->setInterval(ShortInterval);
    connect(m_updateIconSizeTimer, SIGNAL(timeout()), this, SLOT(updateIconSize()));

    updateMinimumRolesWidths();
}

KFileItemListView::~KFileItemListView()
{
    // The group headers are children of the widgets created by
    // widgetCreator(). So it is mandatory to delete the group headers
    // first.
    delete groupHeaderCreator();
    delete widgetCreator();

    delete m_modelRolesUpdater;
    m_modelRolesUpdater = 0;
}

void KFileItemListView::setPreviewsShown(bool show)
{
    if (m_modelRolesUpdater) {
        m_modelRolesUpdater->setPreviewShown(show);
    }
}

bool KFileItemListView::previewsShown() const
{
    return m_modelRolesUpdater->isPreviewShown();
}

void KFileItemListView::setItemLayout(Layout layout)
{
    if (m_itemLayout != layout) {
        m_itemLayout = layout;
        updateLayoutOfVisibleItems();
    }
}

KFileItemListView::Layout KFileItemListView::itemLayout() const
{
    return m_itemLayout;
}

QSizeF KFileItemListView::itemSizeHint(int index) const
{
    const QHash<QByteArray, QVariant> values = model()->data(index);
    const KItemListStyleOption& option = styleOption();
    const int additionalRolesCount = qMax(visibleRoles().count() - 1, 0);

    switch (m_itemLayout) {
    case IconsLayout: {
        const QString text = KStringHandler::preProcessWrap(values["name"].toString());

        const qreal maxWidth = itemSize().width() - 2 * option.margin;
        int textLinesCount = 0;
        QTextLine line;

        // Calculate the number of lines required for wrapping the name
        QTextOption textOption(Qt::AlignHCenter);
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

        QTextLayout layout(text, option.font);
        layout.setTextOption(textOption);
        layout.beginLayout();
        while ((line = layout.createLine()).isValid()) {
            line.setLineWidth(maxWidth);
            line.naturalTextWidth();
            ++textLinesCount;
        }
        layout.endLayout();

        // Add one line for each additional information
        textLinesCount += additionalRolesCount;

        const qreal height = textLinesCount * option.fontMetrics.height() +
                             option.iconSize +
                             option.margin * 4;
        return QSizeF(itemSize().width(), height);
    }

    case CompactLayout: {
        // For each row exactly one role is shown. Calculate the maximum required width that is necessary
        // to show all roles without horizontal clipping.
        qreal maximumRequiredWidth = 0.0;

        foreach (const QByteArray& role, visibleRoles()) {
            const QString text = KFileItemListWidget::roleText(role, values);
            const qreal requiredWidth = option.fontMetrics.width(text);
            maximumRequiredWidth = qMax(maximumRequiredWidth, requiredWidth);
        }

        const qreal width = option.margin * 4 + option.iconSize + maximumRequiredWidth;
        const qreal height = option.margin * 2 + qMax(option.iconSize, (1 + additionalRolesCount) * option.fontMetrics.height());
        return QSizeF(width, height);
    }

    case DetailsLayout: {
        // The width will be determined dynamically by KFileItemListView::visibleRoleSizes()
        const qreal height = option.margin * 2 + qMax(option.iconSize, option.fontMetrics.height());
        return QSizeF(-1, height);
    }

    default:
        Q_ASSERT(false);
        break;
    }

    return QSize();
}

QHash<QByteArray, QSizeF> KFileItemListView::visibleRolesSizes(const KItemRangeList& itemRanges) const
{
    QElapsedTimer timer;
    timer.start();

    QHash<QByteArray, QSizeF> sizes;

    int calculatedItemCount = 0;
    bool maxTimeExceeded = false;
    foreach (const KItemRange& itemRange, itemRanges) {
        const int startIndex = itemRange.index;
        const int endIndex = startIndex + itemRange.count - 1;

        for (int i = startIndex; i <= endIndex; ++i) {
            foreach (const QByteArray& visibleRole, visibleRoles()) {
                QSizeF maxSize = sizes.value(visibleRole, QSizeF(0, 0));
                const QSizeF itemSize = visibleRoleSizeHint(i, visibleRole);
                maxSize = maxSize.expandedTo(itemSize);
                sizes.insert(visibleRole, maxSize);
            }

            if (calculatedItemCount > 100 && timer.elapsed() > 200) {
                // When having several thousands of items calculating the sizes can get
                // very expensive. We accept a possibly too small role-size in favour
                // of having no blocking user interface.
                #ifdef KFILEITEMLISTVIEW_DEBUG
                    kDebug() << "Timer exceeded, stopped after" << calculatedItemCount << "items";
                #endif
                maxTimeExceeded = true;
                break;
            }
            ++calculatedItemCount;
        }
        if (maxTimeExceeded) {
            break;
        }
    }

#ifdef KFILEITEMLISTVIEW_DEBUG
    int rangesItemCount = 0;
    foreach (const KItemRange& itemRange, itemRanges) {
        rangesItemCount += itemRange.count;
    }
    kDebug() << "[TIME] Calculated dynamic item size for " << rangesItemCount << "items:" << timer.elapsed();
#endif
    return sizes;
}

QPixmap KFileItemListView::createDragPixmap(const QSet<int>& indexes) const
{
    if (!model()) {
        return QPixmap();
    }

    const int itemCount = indexes.count();
    Q_ASSERT(itemCount > 0);

    // If more than one item is dragged, align the items inside a
    // rectangular grid. The maximum grid size is limited to 5 x 5 items.
    int xCount;
    int size;
    if (itemCount > 16) {
        xCount = 5;
        size = KIconLoader::SizeSmall;
    } else if (itemCount > 9) {
        xCount = 4;
        size = KIconLoader::SizeSmallMedium;
    } else {
        xCount = 3;
        size = KIconLoader::SizeMedium;
    }

    if (itemCount < xCount) {
        xCount = itemCount;
    }

    int yCount = itemCount / xCount;
    if (itemCount % xCount != 0) {
        ++yCount;
    }
    if (yCount > xCount) {
        yCount = xCount;
    }

    // Draw the selected items into the grid cells.
    QPixmap dragPixmap(xCount * size + xCount, yCount * size + yCount);
    dragPixmap.fill(Qt::transparent);

    QPainter painter(&dragPixmap);
    int x = 0;
    int y = 0;
    QSetIterator<int> it(indexes);
    while (it.hasNext()) {
        const int index = it.next();

        QPixmap pixmap = model()->data(index).value("iconPixmap").value<QPixmap>();
        if (pixmap.isNull()) {
            KIcon icon(model()->data(index).value("iconName").toString());
            pixmap = icon.pixmap(size, size);
        } else {
            KPixmapModifier::scale(pixmap, QSize(size, size));
        }

        painter.drawPixmap(x, y, pixmap);

        x += size + 1;
        if (x >= dragPixmap.width()) {
            x = 0;
            y += size + 1;
        }

        if (y >= dragPixmap.height()) {
            break;
        }
    }

    return dragPixmap;
}

void KFileItemListView::initializeItemListWidget(KItemListWidget* item)
{
    KFileItemListWidget* fileItemListWidget = static_cast<KFileItemListWidget*>(item);

    switch (m_itemLayout) {
    case IconsLayout:   fileItemListWidget->setLayout(KFileItemListWidget::IconsLayout); break;
    case CompactLayout: fileItemListWidget->setLayout(KFileItemListWidget::CompactLayout); break;
    case DetailsLayout: fileItemListWidget->setLayout(KFileItemListWidget::DetailsLayout); break;
    default:            Q_ASSERT(false); break;
    }

    fileItemListWidget->setAlternatingBackgroundColors(m_itemLayout == DetailsLayout);
}

bool KFileItemListView::itemSizeHintUpdateRequired(const QSet<QByteArray>& changedRoles) const
{
    // Even if the icons have a different size they are always aligned within
    // the area defined by KItemStyleOption.iconSize and hence result in no
    // change of the item-size.
    const bool containsIconName = changedRoles.contains("iconName");
    const bool containsIconPixmap = changedRoles.contains("iconPixmap");
    const int count = changedRoles.count();

    const bool iconChanged = (containsIconName && containsIconPixmap && count == 2) ||
                             (containsIconName && count == 1) ||
                             (containsIconPixmap && count == 1);
    return !iconChanged;
}

void KFileItemListView::onModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    Q_UNUSED(previous);
    Q_ASSERT(qobject_cast<KFileItemModel*>(current));

    if (m_modelRolesUpdater) {
        delete m_modelRolesUpdater;
    }

    m_modelRolesUpdater = new KFileItemModelRolesUpdater(static_cast<KFileItemModel*>(current), this);
    const int size = styleOption().iconSize;
    m_modelRolesUpdater->setIconSize(QSize(size, size));
}

void KFileItemListView::onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    updateLayoutOfVisibleItems();
}

void KFileItemListView::onItemSizeChanged(const QSizeF& current, const QSizeF& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onScrollOffsetChanged(qreal current, qreal previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onVisibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    applyRolesToModel();
}

void KFileItemListView::onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    triggerIconSizeUpdate();
}

void KFileItemListView::onTransactionBegin()
{
    m_modelRolesUpdater->setPaused(true);
}

void KFileItemListView::onTransactionEnd()
{
    // Only unpause the model-roles-updater if no timer is active. If one
    // timer is still active the model-roles-updater will be unpaused later as
    // soon as the timer has been exceeded.
    const bool timerActive = m_updateVisibleIndexRangeTimer->isActive() ||
                             m_updateIconSizeTimer->isActive();
    if (!timerActive) {
        m_modelRolesUpdater->setPaused(false);
    }
}

void KFileItemListView::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    KItemListView::resizeEvent(event);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::slotItemsRemoved(const KItemRangeList& itemRanges)
{
    KItemListView::slotItemsRemoved(itemRanges);
    updateTimersInterval();
}

void KFileItemListView::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    const QByteArray sortRole = model()->sortRole();
    if (!visibleRoles().contains(sortRole)) {
        applyRolesToModel();
    }

    KItemListView::slotSortRoleChanged(current, previous);
}

void KFileItemListView::triggerVisibleIndexRangeUpdate()
{
    m_modelRolesUpdater->setPaused(true);
    m_updateVisibleIndexRangeTimer->start();
}

void KFileItemListView::updateVisibleIndexRange()
{
    if (!m_modelRolesUpdater) {
        return;
    }

    const int index = firstVisibleIndex();
    const int count = lastVisibleIndex() - index + 1;
    m_modelRolesUpdater->setVisibleIndexRange(index, count);

    if (m_updateIconSizeTimer->isActive()) {
        // If the icon-size update is pending do an immediate update
        // of the icon-size before unpausing m_modelRolesUpdater. This prevents
        // an unnecessary expensive recreation of all previews afterwards.
        m_updateIconSizeTimer->stop();
        const KItemListStyleOption& option = styleOption();
        m_modelRolesUpdater->setIconSize(QSize(option.iconSize, option.iconSize));
    }

    m_modelRolesUpdater->setPaused(isTransactionActive());
    updateTimersInterval();
}

void KFileItemListView::triggerIconSizeUpdate()
{
    m_modelRolesUpdater->setPaused(true);
    m_updateIconSizeTimer->start();
}

void KFileItemListView::updateIconSize()
{
    if (!m_modelRolesUpdater) {
        return;
    }

    const KItemListStyleOption& option = styleOption();
    m_modelRolesUpdater->setIconSize(QSize(option.iconSize, option.iconSize));

    if (m_updateVisibleIndexRangeTimer->isActive()) {
        // If the visibility-index-range update is pending do an immediate update
        // of the range before unpausing m_modelRolesUpdater. This prevents
        // an unnecessary expensive recreation of all previews afterwards.
        m_updateVisibleIndexRangeTimer->stop();
        const int index = firstVisibleIndex();
        const int count = lastVisibleIndex() - index + 1;
        m_modelRolesUpdater->setVisibleIndexRange(index, count);
    }

    m_modelRolesUpdater->setPaused(isTransactionActive());
    updateTimersInterval();
}

QSizeF KFileItemListView::visibleRoleSizeHint(int index, const QByteArray& role) const
{
    const KItemListStyleOption& option = styleOption();

    qreal width = m_minimumRolesWidths.value(role, 0);
    const qreal height = option.margin * 2 + option.fontMetrics.height();

    const QHash<QByteArray, QVariant> values = model()->data(index);
    const QString text = KFileItemListWidget::roleText(role, values);
    if (!text.isEmpty()) {
        const qreal columnMargin = option.margin * 3;
        width = qMax(width, qreal(2 * columnMargin + option.fontMetrics.width(text)));
    }

    if (role == "name") {
        // Increase the width by the expansion-toggle and the current expansion level
        const int expansionLevel = values.value("expansionLevel", 0).toInt();
        width += option.margin + expansionLevel * itemSize().height() + KIconLoader::SizeSmall;

        // Increase the width by the required space for the icon
        width += option.margin * 2 + option.iconSize;
    }

    return QSizeF(width, height);
}

void KFileItemListView::updateLayoutOfVisibleItems()
{
    foreach (KItemListWidget* widget, visibleItemListWidgets()) {
        initializeItemListWidget(widget);
    }
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::updateTimersInterval()
{
    if (!model()) {
        return;
    }

    // The ShortInterval is used for cases like switching the directory: If the
    // model is empty and filled later the creation of the previews should be done
    // as soon as possible. The LongInterval is used when the model already contains
    // items and assures that operations like zooming don't result in too many temporary
    // recreations of the previews.

    const int interval = (model()->count() <= 0) ? ShortInterval : LongInterval;
    m_updateVisibleIndexRangeTimer->setInterval(interval);
    m_updateIconSizeTimer->setInterval(interval);
}

void KFileItemListView::updateMinimumRolesWidths()
{
    m_minimumRolesWidths.clear();

    const KItemListStyleOption& option = styleOption();
    const QString sizeText = QLatin1String("888888") + i18nc("@item:intable", "items");
    m_minimumRolesWidths.insert("size", option.fontMetrics.width(sizeText));
}

void KFileItemListView::applyRolesToModel()
{
    Q_ASSERT(qobject_cast<KFileItemModel*>(model()));
    KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(model());

    // KFileItemModel does not distinct between "visible" and "invisible" roles.
    // Add all roles that are mandatory for having a working KFileItemListView:
    QSet<QByteArray> roles = visibleRoles().toSet();
    roles.insert("iconPixmap");
    roles.insert("iconName");
    roles.insert("name");
    roles.insert("isDir");
    if (m_itemLayout == DetailsLayout) {
        roles.insert("isExpanded");
        roles.insert("expansionLevel");
    }

    // Assure that the role that is used for sorting will be determined
    roles.insert(fileItemModel->sortRole());

    fileItemModel->setRoles(roles);
    m_modelRolesUpdater->setRoles(roles);
}

#include "kfileitemlistview.moc"
