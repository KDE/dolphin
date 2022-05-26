/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinitemlistview.h"

#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphinfileitemlistwidget.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontroller.h"
#include "settings/viewmodes/viewmodesettings.h"
#include "views/viewmodecontroller.h"
#include "zoomlevelinfo.h"

#include <KIO/PreviewJob>
#include <QtMath>


DolphinItemListView::DolphinItemListView(QGraphicsWidget* parent) :
    KFileItemListView(parent),
    m_zoomLevel(0)
{
    updateFont();
    updateGridSize();
}

DolphinItemListView::~DolphinItemListView()
{
    writeSettings();
}

void DolphinItemListView::setZoomLevel(int level)
{
    if (level < ZoomLevelInfo::minimumLevel()) {
        level = ZoomLevelInfo::minimumLevel();
    } else if (level > ZoomLevelInfo::maximumLevel()) {
        level = ZoomLevelInfo::maximumLevel();
    }

    if (level == m_zoomLevel) {
        return;
    }

    m_zoomLevel = level;

    ViewModeSettings settings(itemLayout());
    if (previewsShown()) {
        const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(level);
        settings.setPreviewSize(previewSize);
    } else {
        const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(level);
        settings.setIconSize(iconSize);
    }

    updateGridSize();
}

int DolphinItemListView::zoomLevel() const
{
    return m_zoomLevel;
}

void DolphinItemListView::readSettings()
{
    ViewModeSettings settings(itemLayout());
    settings.readConfig();

    beginTransaction();

    setEnabledSelectionToggles(GeneralSettings::showSelectionToggle());
    setHighlightEntireRow(DetailsModeSettings::sidePadding());
    setSupportsItemExpanding(itemLayoutSupportsItemExpanding(itemLayout()));

    updateFont();
    updateGridSize();

    const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
    setEnabledPlugins(globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins()));
    setLocalFileSizePreviewLimit(globalConfig.readEntry("MaximumSize", 0));
    endTransaction();
}

void DolphinItemListView::writeSettings()
{
    IconsModeSettings::self()->save();
    CompactModeSettings::self()->save();
    DetailsModeSettings::self()->save();
}

KItemListWidgetCreatorBase* DolphinItemListView::defaultWidgetCreator() const
{
    return new KItemListWidgetCreator<DolphinFileItemListWidget>();
}

bool DolphinItemListView::itemLayoutSupportsItemExpanding(ItemLayout layout) const
{
    return layout == DetailsLayout && DetailsModeSettings::expandableFolders();
}

void DolphinItemListView::onItemLayoutChanged(ItemLayout current, ItemLayout previous)
{
    setHeaderVisible(current == DetailsLayout);

    updateFont();
    updateGridSize();

    KFileItemListView::onItemLayoutChanged(current, previous);
}

void DolphinItemListView::onPreviewsShownChanged(bool shown)
{
    Q_UNUSED(shown)
    updateGridSize();
}

void DolphinItemListView::onVisibleRolesChanged(const QList<QByteArray>& current,
                                                const QList<QByteArray>& previous)
{
    KFileItemListView::onVisibleRolesChanged(current, previous);
    updateGridSize();
}

void DolphinItemListView::updateFont()
{
    const ViewModeSettings settings(itemLayout());

    if (settings.useSystemFont()) {
        KItemListView::updateFont();
    } else {
        QFont font(settings.fontFamily(), qRound(settings.fontSize()));
        font.setItalic(settings.italicFont());
        font.setWeight(settings.fontWeight());
        font.setPointSizeF(settings.fontSize());

        KItemListStyleOption option = styleOption();
        option.font = font;
        option.fontMetrics = QFontMetrics(font);

        setStyleOption(option);
    }
}

void DolphinItemListView::updateGridSize()
{
    const ViewModeSettings settings(itemLayout());

    // Calculate the size of the icon
    const int iconSize = previewsShown() ? settings.previewSize() : settings.iconSize();
    m_zoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(iconSize, iconSize));
    KItemListStyleOption option = styleOption();

    const int padding = 2;
    int horizontalMargin = 0;
    int verticalMargin = 0;

    // Calculate the item-width and item-height
    int itemWidth;
    int itemHeight;
    int maxTextLines = 0;
    int maxTextWidth = 0;

    switch (itemLayout()) {
    case KFileItemListView::IconsLayout: {

        // an exponential factor based on zoom, 0 -> 1, 4 -> 1.36 8 -> ~1.85, 16 -> 3.4
        auto zoomFactor = qExp(m_zoomLevel / 13.0);
        // 9 is the average char width for 10pt Noto Sans, making fontFactor =1
        // by each pixel the font gets larger the factor increases by 1/9
        auto fontFactor = option.fontMetrics.averageCharWidth() / 9.0;
        itemWidth = 48 + IconsModeSettings::textWidthIndex() * 64 * fontFactor * zoomFactor;

        if (itemWidth < iconSize + padding * 2 * zoomFactor) {
            itemWidth = iconSize + padding * 2 * zoomFactor;
        }

        itemHeight = padding * 3 + iconSize + option.fontMetrics.lineSpacing();

        horizontalMargin = 4;
        verticalMargin = 8;
        maxTextLines = IconsModeSettings::maximumTextLines();
        break;
    }
    case KFileItemListView::CompactLayout: {
        itemWidth = padding * 4 + iconSize + option.fontMetrics.height() * 5;
        const int textLinesCount = visibleRoles().count();
        itemHeight = padding * 2 + qMax(iconSize, textLinesCount * option.fontMetrics.lineSpacing());

        if (CompactModeSettings::maximumTextWidthIndex() > 0) {
            // A restriction is given for the maximum width of the text (0 means
            // having no restriction)
            maxTextWidth = option.fontMetrics.height() * 10 * CompactModeSettings::maximumTextWidthIndex();
        }

        horizontalMargin = 8;
        break;
    }
    case KFileItemListView::DetailsLayout: {
        itemWidth = -1;
        itemHeight = padding * 2 + qMax(iconSize, option.fontMetrics.lineSpacing());
        break;
    }
    default:
        itemWidth = -1;
        itemHeight = -1;
        Q_ASSERT(false);
        break;
    }

    // Apply the calculated values
    option.padding = padding;
    option.horizontalMargin = horizontalMargin;
    option.verticalMargin = verticalMargin;
    option.iconSize = iconSize;
    option.maxTextLines = maxTextLines;
    option.maxTextWidth = maxTextWidth;
    beginTransaction();
    setStyleOption(option);
    setItemSize(QSizeF(itemWidth, itemHeight));
    endTransaction();
}
