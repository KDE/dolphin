/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KSTANDARDITEMLISTVIEW_H
#define KSTANDARDITEMLISTVIEW_H

#include <libdolphin_export.h>

#include <kitemviews/kitemlistview.h>

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
class LIBDOLPHINPRIVATE_EXPORT KStandardItemListView : public KItemListView
{
    Q_OBJECT

public:
    enum ItemLayout
    {
        IconsLayout,
        CompactLayout,
        DetailsLayout
    };

    KStandardItemListView(QGraphicsWidget* parent = 0);
    virtual ~KStandardItemListView();

    void setItemLayout(ItemLayout layout);
    ItemLayout itemLayout() const;

protected:
    virtual KItemListWidgetCreatorBase* defaultWidgetCreator() const;
    virtual KItemListGroupHeaderCreatorBase* defaultGroupHeaderCreator() const;
    virtual void initializeItemListWidget(KItemListWidget* item);
    virtual bool itemSizeHintUpdateRequired(const QSet<QByteArray>& changedRoles) const;
    virtual void onItemLayoutChanged(ItemLayout current, ItemLayout previous);
    virtual void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    virtual void onSupportsItemExpandingChanged(bool supportsExpanding);
    virtual void polishEvent();

private:
    void applyDefaultStyleOption(int iconSize, int padding, int horizontalMargin, int verticalMargin);
    void updateLayoutOfVisibleItems();

private:
    ItemLayout m_itemLayout;
};

#endif


