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

#include "kfileitemmodelrolesupdater.h"
#include "kfileitemlistwidget.h"
#include "kfileitemmodel.h"
#include "private/kpixmapmodifier.h"

#include <QIcon>
#include <KIconLoader>

#include <QPainter>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>

// #define KFILEITEMLISTVIEW_DEBUG

namespace {
    // If the visible index range changes, KFileItemModelRolesUpdater is not
    // informed immediatetly, but with a short delay. This ensures that scrolling
    // always feels smooth and is not interrupted by icon loading (which can be
    // quite expensive if a disk access is required to determine the final icon).
    const int ShortInterval = 50;

    // If the icon size changes, a longer delay is used. This prevents that
    // the expensive re-generation of all previews is triggered repeatedly when
    // changing the zoom level.
    const int LongInterval = 300;
}

KFileItemListView::KFileItemListView(QGraphicsWidget* parent) :
    KStandardItemListView(parent),
    m_modelRolesUpdater(0),
    m_updateVisibleIndexRangeTimer(0),
    m_updateIconSizeTimer(0)
{
    setAcceptDrops(true);

    setScrollOrientation(Qt::Vertical);

    m_updateVisibleIndexRangeTimer = new QTimer(this);
    m_updateVisibleIndexRangeTimer->setSingleShot(true);
    m_updateVisibleIndexRangeTimer->setInterval(ShortInterval);
    connect(m_updateVisibleIndexRangeTimer, &QTimer::timeout, this, &KFileItemListView::updateVisibleIndexRange);

    m_updateIconSizeTimer = new QTimer(this);
    m_updateIconSizeTimer->setSingleShot(true);
    m_updateIconSizeTimer->setInterval(LongInterval);
    connect(m_updateIconSizeTimer, &QTimer::timeout, this, &KFileItemListView::updateIconSize);

    setVisibleRoles({"text"});
}

KFileItemListView::~KFileItemListView()
{
}

void KFileItemListView::setPreviewsShown(bool show)
{
    if (!m_modelRolesUpdater) {
        return;
    }

    if (m_modelRolesUpdater->previewsShown() != show) {
        beginTransaction();
        m_modelRolesUpdater->setPreviewsShown(show);
        onPreviewsShownChanged(show);
        endTransaction();
    }
}

bool KFileItemListView::previewsShown() const
{
    return m_modelRolesUpdater ? m_modelRolesUpdater->previewsShown() : false;
}

void KFileItemListView::setEnlargeSmallPreviews(bool enlarge)
{
    if (m_modelRolesUpdater) {
        m_modelRolesUpdater->setEnlargeSmallPreviews(enlarge);
    }
}

bool KFileItemListView::enlargeSmallPreviews() const
{
    return m_modelRolesUpdater ? m_modelRolesUpdater->enlargeSmallPreviews() : false;
}

void KFileItemListView::setEnabledPlugins(const QStringList& list)
{
    if (m_modelRolesUpdater) {
        m_modelRolesUpdater->setEnabledPlugins(list);
    }
}

QStringList KFileItemListView::enabledPlugins() const
{
    return m_modelRolesUpdater ? m_modelRolesUpdater->enabledPlugins() : QStringList();
}

QPixmap KFileItemListView::createDragPixmap(const KItemSet& indexes) const
{
    if (!model()) {
        return QPixmap();
    }

    const int itemCount = indexes.count();
    Q_ASSERT(itemCount > 0);
    if (itemCount == 1) {
        return KItemListView::createDragPixmap(indexes);
    }

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

    const qreal dpr = scene()->views()[0]->devicePixelRatio();
    // Draw the selected items into the grid cells.
    QPixmap dragPixmap(QSize(xCount * size + xCount, yCount * size + yCount) * dpr);
    dragPixmap.setDevicePixelRatio(dpr);
    dragPixmap.fill(Qt::transparent);

    QPainter painter(&dragPixmap);
    int x = 0;
    int y = 0;

    foreach (int index, indexes) {
        QPixmap pixmap = model()->data(index).value("iconPixmap").value<QPixmap>();
        if (pixmap.isNull()) {
            QIcon icon = QIcon::fromTheme(model()->data(index).value("iconName").toString());
            pixmap = icon.pixmap(size, size);
        } else {
            KPixmapModifier::scale(pixmap, QSize(size, size) * dpr);
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

KItemListWidgetCreatorBase* KFileItemListView::defaultWidgetCreator() const
{
    return new KItemListWidgetCreator<KFileItemListWidget>();
}

void KFileItemListView::initializeItemListWidget(KItemListWidget* item)
{
    KStandardItemListView::initializeItemListWidget(item);

    // Make sure that the item has an icon.
    QHash<QByteArray, QVariant> data = item->data();
    if (!data.contains("iconName") && data["iconPixmap"].value<QPixmap>().isNull()) {
        Q_ASSERT(qobject_cast<KFileItemModel*>(model()));
        KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(model());

        const KFileItem fileItem = fileItemModel->fileItem(item->index());
        data.insert("iconName", fileItem.iconName());
        item->setData(data, {"iconName"});
    }
}

void KFileItemListView::onPreviewsShownChanged(bool shown)
{
    Q_UNUSED(shown);
}

void KFileItemListView::onItemLayoutChanged(ItemLayout current, ItemLayout previous)
{
    KStandardItemListView::onItemLayoutChanged(current, previous);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onModelChanged(KItemModelBase* current, KItemModelBase* previous)
{
    Q_ASSERT(qobject_cast<KFileItemModel*>(current));
    KStandardItemListView::onModelChanged(current, previous);

    delete m_modelRolesUpdater;
    m_modelRolesUpdater = 0;

    if (current) {
        m_modelRolesUpdater = new KFileItemModelRolesUpdater(static_cast<KFileItemModel*>(current), this);
        m_modelRolesUpdater->setIconSize(availableIconSize());

        applyRolesToModel();
    }
}

void KFileItemListView::onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    KStandardItemListView::onScrollOrientationChanged(current, previous);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onItemSizeChanged(const QSizeF& current, const QSizeF& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onScrollOffsetChanged(qreal current, qreal previous)
{
    KStandardItemListView::onScrollOffsetChanged(current, previous);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onVisibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous)
{
    KStandardItemListView::onVisibleRolesChanged(current, previous);
    applyRolesToModel();
}

void KFileItemListView::onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous)
{
    KStandardItemListView::onStyleOptionChanged(current, previous);
    triggerIconSizeUpdate();
}

void KFileItemListView::onSupportsItemExpandingChanged(bool supportsExpanding)
{
    applyRolesToModel();
    KStandardItemListView::onSupportsItemExpandingChanged(supportsExpanding);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::onTransactionBegin()
{
    if (m_modelRolesUpdater) {
        m_modelRolesUpdater->setPaused(true);
    }
}

void KFileItemListView::onTransactionEnd()
{
    if (!m_modelRolesUpdater) {
        return;
    }

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
    KStandardItemListView::resizeEvent(event);
    triggerVisibleIndexRangeUpdate();
}

void KFileItemListView::slotItemsRemoved(const KItemRangeList& itemRanges)
{
    KStandardItemListView::slotItemsRemoved(itemRanges);
}

void KFileItemListView::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    const QByteArray sortRole = model()->sortRole();
    if (!visibleRoles().contains(sortRole)) {
        applyRolesToModel();
    }

    KStandardItemListView::slotSortRoleChanged(current, previous);
}

void KFileItemListView::triggerVisibleIndexRangeUpdate()
{
    if (!model()) {
        return;
    }
    m_modelRolesUpdater->setPaused(true);

    // If the icon size has been changed recently, wait until
    // m_updateIconSizeTimer expires.
    if (!m_updateIconSizeTimer->isActive()) {
        m_updateVisibleIndexRangeTimer->start();
    }
}

void KFileItemListView::updateVisibleIndexRange()
{
    if (!m_modelRolesUpdater) {
        return;
    }

    const int index = firstVisibleIndex();
    const int count = lastVisibleIndex() - index + 1;
    m_modelRolesUpdater->setMaximumVisibleItems(maximumVisibleItems());
    m_modelRolesUpdater->setVisibleIndexRange(index, count);
    m_modelRolesUpdater->setPaused(isTransactionActive());
}

void KFileItemListView::triggerIconSizeUpdate()
{
    if (!model()) {
        return;
    }
    m_modelRolesUpdater->setPaused(true);
    m_updateIconSizeTimer->start();

    // The visible index range will be updated when m_updateIconSizeTimer expires.
    // Stop m_updateVisibleIndexRangeTimer to prevent an expensive re-generation
    // of all previews (note that the user might change the icon size again soon).
    m_updateVisibleIndexRangeTimer->stop();
}

void KFileItemListView::updateIconSize()
{
    if (!m_modelRolesUpdater) {
        return;
    }

    m_modelRolesUpdater->setIconSize(availableIconSize());

    // Update the visible index range (which has most likely changed after the
    // icon size change) before unpausing m_modelRolesUpdater.
    const int index = firstVisibleIndex();
    const int count = lastVisibleIndex() - index + 1;
    m_modelRolesUpdater->setVisibleIndexRange(index, count);

    m_modelRolesUpdater->setPaused(isTransactionActive());
}

void KFileItemListView::applyRolesToModel()
{
    if (!model()) {
        return;
    }

    Q_ASSERT(qobject_cast<KFileItemModel*>(model()));
    KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(model());

    // KFileItemModel does not distinct between "visible" and "invisible" roles.
    // Add all roles that are mandatory for having a working KFileItemListView:
    QSet<QByteArray> roles = visibleRoles().toSet();
    roles.insert("iconPixmap");
    roles.insert("iconName");
    roles.insert("text");
    roles.insert("isDir");
    roles.insert("isLink");
    if (supportsItemExpanding()) {
        roles.insert("isExpanded");
        roles.insert("isExpandable");
        roles.insert("expandedParentsCount");
    }

    // Assure that the role that is used for sorting will be determined
    roles.insert(fileItemModel->sortRole());

    fileItemModel->setRoles(roles);
    m_modelRolesUpdater->setRoles(roles);
}

QSize KFileItemListView::availableIconSize() const
{
    const KItemListStyleOption& option = styleOption();
    const int iconSize = option.iconSize;
    if (itemLayout() == IconsLayout) {
        const int maxIconWidth = itemSize().width() - 2 * option.padding;
        return QSize(maxIconWidth, iconSize);
    }

    return QSize(iconSize, iconSize);
}

