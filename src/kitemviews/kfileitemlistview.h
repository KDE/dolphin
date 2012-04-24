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

#include <kitemviews/kstandarditemlistview.h>

class KFileItemModelRolesUpdater;
class QTimer;

/**
 * @brief View that allows to show the content of file-items.
 *
 * The corresponding model set by the controller must be an instance
 * of KFileItemModel. Per default KFileItemListWidget is set as widget creator
 * value and KItemListGroupHeader as group-header creator value. Use
 * KItemListView::setWidgetCreator() and KItemListView::setGroupHeaderCreator()
 * to apply customized generators.
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemListView : public KStandardItemListView
{
    Q_OBJECT

public:
    KFileItemListView(QGraphicsWidget* parent = 0);
    virtual ~KFileItemListView();

    void setPreviewsShown(bool show);
    bool previewsShown() const;

    /**
     * If enabled a small preview gets upscaled to the icon size in case where
     * the icon size is larger than the preview. Per default enlarging is
     * enabled.
     */
    void setEnlargeSmallPreviews(bool enlarge);
    bool enlargeSmallPreviews() const;

    /**
     * Sets the list of enabled thumbnail plugins that are used for previews.
     * Per default all plugins enabled in the KConfigGroup "PreviewSettings"
     * are used.
     *
     * For a list of available plugins, call KServiceTypeTrader::self()->query("ThumbCreator").
     *
     * @see enabledPlugins
     */
    void setEnabledPlugins(const QStringList& list);

    /**
     * Returns the list of enabled thumbnail plugins.
     * @see setEnabledPlugins
     */
    QStringList enabledPlugins() const;

    /** @reimp */
    virtual QPixmap createDragPixmap(const QSet<int>& indexes) const;

protected:
    virtual KItemListWidgetCreatorBase* defaultWidgetCreator() const;
    virtual void onPreviewsShownChanged(bool shown);
    virtual void onItemLayoutChanged(ItemLayout current, ItemLayout previous);
    virtual void onModelChanged(KItemModelBase* current, KItemModelBase* previous);
    virtual void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    virtual void onItemSizeChanged(const QSizeF& current, const QSizeF& previous);
    virtual void onScrollOffsetChanged(qreal current, qreal previous);
    virtual void onVisibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous);
    virtual void onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);
    virtual void onSupportsItemExpandingChanged(bool supportsExpanding);
    virtual void onTransactionBegin();
    virtual void onTransactionEnd();
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

protected slots:
    virtual void slotItemsRemoved(const KItemRangeList& itemRanges);
    virtual void slotSortRoleChanged(const QByteArray& current, const QByteArray& previous);

private slots:
    void triggerVisibleIndexRangeUpdate();
    void updateVisibleIndexRange();

    void triggerIconSizeUpdate();
    void updateIconSize();

private:
    void updateTimersInterval();

    /**
     * Applies the roles defined by KItemListView::visibleRoles() to the
     * KFileItemModel and KFileItemModelRolesUpdater. As the model does not
     * distinct between visible and invisible roles also internal roles
     * are applied that are mandatory for having a working KFileItemModel.
     */
    void applyRolesToModel();

    /**
     * @return Size that is available for the icons. The size might be larger than specified by
     *         KItemListStyleOption::iconSize: With the IconsLayout also the empty left area left
     *         and right of an icon will be included.
     */
    QSize availableIconSize() const;

private:
    KFileItemModelRolesUpdater* m_modelRolesUpdater;
    QTimer* m_updateVisibleIndexRangeTimer;
    QTimer* m_updateIconSizeTimer;

    friend class KFileItemListViewTest; // For unit testing
};

#endif


