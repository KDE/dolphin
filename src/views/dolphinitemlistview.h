/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINITEMLISTVIEW_H
#define DOLPHINITEMLISTVIEW_H

#include "dolphin_export.h"
#include "kitemviews/kfileitemlistview.h"
#include "settings/viewmodes/viewmodesettings.h"

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
    explicit DolphinItemListView(QGraphicsWidget* parent = nullptr);
    ~DolphinItemListView() override;

    void setZoomLevel(int level);
    int zoomLevel() const;

    void readSettings();
    void writeSettings();

protected:
    KItemListWidgetCreatorBase* defaultWidgetCreator() const override;
    bool itemLayoutSupportsItemExpanding(ItemLayout layout) const override;
    void onItemLayoutChanged(ItemLayout current, ItemLayout previous) override;
    void onPreviewsShownChanged(bool shown) override;
    void onVisibleRolesChanged(const QList<QByteArray>& current,
                                       const QList<QByteArray>& previous) override;

    void updateFont() override;

private:
    void updateGridSize();

    ViewModeSettings::ViewMode viewMode() const;

private:
    int m_zoomLevel;
};

#endif
