/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMLISTVIEW_H
#define KFILEITEMLISTVIEW_H

#include "dolphin_export.h"
#include "kitemviews/kstandarditemlistview.h"

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
class DOLPHIN_EXPORT KFileItemListView : public KStandardItemListView
{
    Q_OBJECT

public:
    explicit KFileItemListView(QGraphicsWidget* parent = nullptr);
    ~KFileItemListView() override;

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

    /**
     * Sets the maximum file size of local files for which
     * previews will be generated (if enabled). A value of 0
     * indicates no file size limit.
     * Per default the value from KConfigGroup "PreviewSettings"
     * MaximumSize is used, 0 otherwise.
     * @param size
     */
    void setLocalFileSizePreviewLimit(qlonglong size);
    qlonglong localFileSizePreviewLimit() const;

    /**
     * If set to true, directories contents are scanned to determine their size
     * Default true
     */
    void setScanDirectories(bool enabled);
    bool scanDirectories();

    QPixmap createDragPixmap(const KItemSet& indexes) const override;

protected:
    KItemListWidgetCreatorBase* defaultWidgetCreator() const override;
    void initializeItemListWidget(KItemListWidget* item) override;
    virtual void onPreviewsShownChanged(bool shown);
    void onItemLayoutChanged(ItemLayout current, ItemLayout previous) override;
    void onModelChanged(KItemModelBase* current, KItemModelBase* previous) override;
    void onScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous) override;
    void onItemSizeChanged(const QSizeF& current, const QSizeF& previous) override;
    void onScrollOffsetChanged(qreal current, qreal previous) override;
    void onVisibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous) override;
    void onStyleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous) override;
    void onSupportsItemExpandingChanged(bool supportsExpanding) override;
    void onTransactionBegin() override;
    void onTransactionEnd() override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;

protected Q_SLOTS:
    void slotItemsRemoved(const KItemRangeList& itemRanges) override;
    void slotSortRoleChanged(const QByteArray& current, const QByteArray& previous) override;

private Q_SLOTS:
    void triggerVisibleIndexRangeUpdate();
    void updateVisibleIndexRange();

    void triggerIconSizeUpdate();
    void updateIconSize();

private:
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
    bool m_scanDirectories;

    friend class KFileItemListViewTest; // For unit testing
};

#endif


