/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINITEMLISTVIEW_H
#define DOLPHINITEMLISTVIEW_H

#include "dolphin_export.h"
#include "kitemviews/kfileitemlistview.h"

class KFileItemListView;

/**
 * @brief Dolphin specific view-implementation.
 *
 * Offers zoom-level support and takes care for translating
 * the view-properties into the corresponding KItemListView
 * properties.
 */
class DOLPHIN_EXPORT DolphinItemListView : public KFileItemListView
{
    Q_OBJECT

public:
    explicit DolphinItemListView(QGraphicsWidget *parent = nullptr);
    ~DolphinItemListView() override;

    void setZoomLevel(int level);
    int zoomLevel() const;

    enum SelectionTogglesEnabled { True, False, FollowSetting };
    /**
     * Sets whether the items in this view should show a small selection toggle area on mouse hover.
     * The default for this view is to follow the "showSelectionToggle" setting but this method can
     * be used to ignore that setting and force a different value.
     */
    void setEnabledSelectionToggles(SelectionTogglesEnabled selectionTogglesEnabled);

    void readSettings();
    void writeSettings();

protected:
    KItemListWidgetCreatorBase *defaultWidgetCreator() const override;
    /** Overwriting in the Dolphin-specific class because we want this to be user-configurable.
     * @see KStandardItemListView::itemLayoutHighlightEntireRow */
    bool itemLayoutHighlightEntireRow(ItemLayout layout) const override;
    bool itemLayoutSupportsItemExpanding(ItemLayout layout) const override;
    void onItemLayoutChanged(ItemLayout current, ItemLayout previous) override;
    void onPreviewsShownChanged(bool shown) override;
    void onVisibleRolesChanged(const QList<QByteArray> &current, const QList<QByteArray> &previous) override;

    void updateFont() override;

private:
    void updateGridSize();

    using KItemListView::setEnabledSelectionToggles; // Makes sure that the setEnabledSelectionToggles() declaration above doesn't hide
                                                     // the one from the base class so we can still use it privately.
    SelectionTogglesEnabled m_selectionTogglesEnabled = FollowSetting;

private:
    int m_zoomLevel;
};

#endif
