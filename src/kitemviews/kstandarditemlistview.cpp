/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kstandarditemlistview.h"

#include "kstandarditemlistwidget.h"

#include <KIconLoader>

KStandardItemListView::KStandardItemListView(QGraphicsWidget* parent) :
    KItemListView(parent),
    m_itemLayout(DetailsLayout)
{
    setAcceptDrops(true);
    setScrollOrientation(Qt::Vertical);
    setVisibleRoles({"text"});
}

KStandardItemListView::~KStandardItemListView()
{
}

void KStandardItemListView::setItemLayout(ItemLayout layout)
{
    if (m_itemLayout == layout) {
        return;
    }

    beginTransaction();

    const ItemLayout previous = m_itemLayout;
    m_itemLayout = layout;

    setSupportsItemExpanding(itemLayoutSupportsItemExpanding(layout));
    setScrollOrientation(layout == CompactLayout ? Qt::Horizontal : Qt::Vertical);

    onItemLayoutChanged(layout, previous);

    endTransaction();
}

KStandardItemListView::ItemLayout KStandardItemListView::itemLayout() const
{
    return m_itemLayout;
}

KItemListWidgetCreatorBase* KStandardItemListView::defaultWidgetCreator() const
{
    return new KItemListWidgetCreator<KStandardItemListWidget>();
}

KItemListGroupHeaderCreatorBase* KStandardItemListView::defaultGroupHeaderCreator() const
{
    return new KItemListGroupHeaderCreator<KStandardItemListGroupHeader>();
}

void KStandardItemListView::initializeItemListWidget(KItemListWidget* item)
{
    KStandardItemListWidget* standardItemListWidget = qobject_cast<KStandardItemListWidget*>(item);
    Q_ASSERT(standardItemListWidget);

    switch (itemLayout()) {
    case IconsLayout:   standardItemListWidget->setLayout(KStandardItemListWidget::IconsLayout); break;
    case CompactLayout: standardItemListWidget->setLayout(KStandardItemListWidget::CompactLayout); break;
    case DetailsLayout: standardItemListWidget->setLayout(KStandardItemListWidget::DetailsLayout); break;
    default:            Q_ASSERT(false); break;
    }

    standardItemListWidget->setSupportsItemExpanding(supportsItemExpanding());
}


bool KStandardItemListView::itemSizeHintUpdateRequired(const QSet<QByteArray>& changedRoles) const
{
    // The only thing that can modify the item's size hint is the amount of space
    // needed to display the text for the visible roles.
    // Even if the icons have a different size they are always aligned within
    // the area defined by KItemStyleOption.iconSize and hence result in no
    // change of the item-size.
    foreach (const QByteArray& role, visibleRoles()) {
        if (changedRoles.contains(role)) {
            return true;
        }
    }
    return false;
}

bool KStandardItemListView::itemLayoutSupportsItemExpanding(ItemLayout layout) const
{
    return layout == DetailsLayout;
}

void KStandardItemListView::onItemLayoutChanged(ItemLayout current, ItemLayout previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    updateLayoutOfVisibleItems();
}

void KStandardItemListView::onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    updateLayoutOfVisibleItems();
}

void KStandardItemListView::onSupportsItemExpandingChanged(bool supportsExpanding)
{
    Q_UNUSED(supportsExpanding)
    updateLayoutOfVisibleItems();
}


void KStandardItemListView::polishEvent()
{
    switch (m_itemLayout) {
    case IconsLayout:   applyDefaultStyleOption(style()->pixelMetric(QStyle::PM_LargeIconSize), 2, 4, 8); break;
    case CompactLayout: applyDefaultStyleOption(style()->pixelMetric(QStyle::PM_SmallIconSize),  2, 8, 0); break;
    case DetailsLayout: applyDefaultStyleOption(style()->pixelMetric(QStyle::PM_SmallIconSize),  2, 0, 0); break;
    default:            Q_ASSERT(false); break;
    }

    QGraphicsWidget::polishEvent();
}

void KStandardItemListView::applyDefaultStyleOption(int iconSize,
                                                    int padding,
                                                    int horizontalMargin,
                                                    int verticalMargin)
{
    KItemListStyleOption option = styleOption();

    if (option.iconSize < 0) {
        option.iconSize = iconSize;
    }
    if (option.padding < 0) {
        option.padding = padding;
    }
    if (option.horizontalMargin < 0) {
        option.horizontalMargin = horizontalMargin;
    }
    if (option.verticalMargin < 0) {
        option.verticalMargin = verticalMargin;
    }

    setStyleOption(option);
}

void KStandardItemListView::updateLayoutOfVisibleItems()
{
    if (model()) {
        foreach (KItemListWidget* widget, visibleItemListWidgets()) {
            initializeItemListWidget(widget);
        }
    }
}

