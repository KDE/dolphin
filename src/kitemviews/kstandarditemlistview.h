/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KSTANDARDITEMLISTVIEW_H
#define KSTANDARDITEMLISTVIEW_H

#include "dolphin_export.h"
#include "kitemviews/kitemlistview.h"

/**
 * @brief Provides layouts for icons-, compact- and details-view.
 *
 * Together with the KStandardItemModel lists for standard usecases
 * can be created in a straight forward way.
 *
 * Example code:
 * <code>
 * KStandardItemListView* view = new KStandardItemListView();
 * KStandardItemModel* model = new KStandardItemModel();
 * model->appendItem(new KStandardItem("Item 1"));
 * model->appendItem(new KStandardItem("Item 2"));
 * KItemListController* controller = new KItemListController(model, view);
 * KItemListContainer* container = new KItemListContainer(controller, parentWidget);
 * </code>
 */
class DOLPHIN_EXPORT KStandardItemListView : public KItemListView
{
    Q_OBJECT

public:
    enum ItemLayout
    {
        IconsLayout,
        CompactLayout,
        DetailsLayout
    };

    explicit KStandardItemListView(QGraphicsWidget* parent = nullptr);
    ~KStandardItemListView() override;

    void setItemLayout(ItemLayout layout);
    ItemLayout itemLayout() const;

protected:
    KItemListWidgetCreatorBase* defaultWidgetCreator() const override;
    KItemListGroupHeaderCreatorBase* defaultGroupHeaderCreator() const override;
    void initializeItemListWidget(KItemListWidget* item) override;
    bool itemSizeHintUpdateRequired(const QSet<QByteArray>& changedRoles) const override;
    virtual bool itemLayoutSupportsItemExpanding(ItemLayout layout) const;
    virtual void onItemLayoutChanged(ItemLayout current, ItemLayout previous);
    void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous) override;
    void onSupportsItemExpandingChanged(bool supportsExpanding) override;
    void polishEvent() override;

private:
    void applyDefaultStyleOption(int iconSize, int padding, int horizontalMargin, int verticalMargin);
    void updateLayoutOfVisibleItems();

private:
    ItemLayout m_itemLayout;
};

#endif


