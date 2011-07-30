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

#ifndef KFILEITEMLISTVIEW_H
#define KFILEITEMLISTVIEW_H

#include <libdolphin_export.h>

#include <kitemviews/kitemlistview.h>

class KFileItemModelRolesUpdater;
class QTimer;

class LIBDOLPHINPRIVATE_EXPORT KFileItemListView : public KItemListView
{
    Q_OBJECT

public:
    enum Layout
    {
        IconsLayout,
        CompactLayout,
        DetailsLayout
    };

    KFileItemListView(QGraphicsWidget* parent = 0);
    virtual ~KFileItemListView();

    void setPreviewsShown(bool show);
    bool previewsShown() const;

    void setItemLayout(Layout layout);
    Layout itemLayout() const;

    virtual QSizeF itemSizeHint(int index) const;
    virtual QHash<QByteArray, QSizeF> visibleRoleSizes() const;

protected:
    virtual void initializeItemListWidget(KItemListWidget* item);
    virtual void onModelChanged(KItemModelBase* current, KItemModelBase* previous);
    virtual void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    virtual void onItemSizeChanged(const QSizeF& current, const QSizeF& previous);
    virtual void onOffsetChanged(qreal current, qreal previous);
    virtual void onVisibleRolesChanged(const QHash<QByteArray, int>& current, const QHash<QByteArray, int>& previous);
    virtual void onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);
    virtual void onTransactionBegin();
    virtual void onTransactionEnd();
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

protected slots:
    virtual void slotItemsRemoved(const KItemRangeList& itemRanges);

private slots:
    void triggerVisibleIndexRangeUpdate();
    void updateVisibleIndexRange();

    void triggerIconSizeUpdate();
    void updateIconSize();

private:
    QSizeF visibleRoleSizeHint(int index, const QByteArray& role) const;
    void updateLayoutOfVisibleItems();
    void updateTimersInterval();
    void updateMinimumRolesWidths();

private:
    Layout m_itemLayout;

    KFileItemModelRolesUpdater* m_modelRolesUpdater;
    QTimer* m_updateVisibleIndexRangeTimer;
    QTimer* m_updateIconSizeTimer;

    // Cache for calculating visibleRoleSizes() in a fast way
    QHash<QByteArray, int> m_minimumRolesWidths;

    friend class KFileItemListViewTest; // For unit testing
};

#endif


