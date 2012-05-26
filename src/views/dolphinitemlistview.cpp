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

#include "dolphinitemlistview.h"

#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_compactmodesettings.h"
#include "dolphinfileitemlistwidget.h"

#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kfileitemmodel.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kitemliststyleoption.h>

#include <KGlobalSettings>

#include <views/viewmodecontroller.h>

#include "zoomlevelinfo.h"


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

    ViewModeSettings settings(viewMode());
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
    ViewModeSettings settings(viewMode());
    settings.readConfig();

    beginTransaction();

    setEnabledSelectionToggles(GeneralSettings::showSelectionToggle());

    const bool expandableFolders = (itemLayout() && KFileItemListView::DetailsLayout) &&
                                   DetailsModeSettings::expandableFolders();
    setSupportsItemExpanding(expandableFolders);

    updateFont();
    updateGridSize();

    const KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    const QStringList plugins = globalConfig.readEntry("Plugins", QStringList()
                                                       << "directorythumbnail"
                                                       << "imagethumbnail"
                                                       << "jpegthumbnail");
    setEnabledPlugins(plugins);

    endTransaction();
}

void DolphinItemListView::writeSettings()
{
    IconsModeSettings::self()->writeConfig();
    CompactModeSettings::self()->writeConfig();
    DetailsModeSettings::self()->writeConfig();
}

KItemListWidgetCreatorBase* DolphinItemListView::defaultWidgetCreator() const
{
    return new KItemListWidgetCreator<DolphinFileItemListWidget>();
}

void DolphinItemListView::onItemLayoutChanged(ItemLayout current, ItemLayout previous)
{
    Q_UNUSED(previous);

    if (current == DetailsLayout) {
        setSupportsItemExpanding(DetailsModeSettings::expandableFolders());
        setHeaderVisible(true);
    } else {
        setHeaderVisible(false);
    }

    updateFont();
    updateGridSize();
}

void DolphinItemListView::onPreviewsShownChanged(bool shown)
{
    Q_UNUSED(shown);
    updateGridSize();
}

void DolphinItemListView::onVisibleRolesChanged(const QList<QByteArray>& current,
                                                const QList<QByteArray>& previous)
{
    KFileItemListView::onVisibleRolesChanged(current, previous);
    updateGridSize();
}

void DolphinItemListView::updateGridSize()
{
    const ViewModeSettings settings(viewMode());

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
    QSize maxTextSize;

    switch (itemLayout()) {
    case KFileItemListView::IconsLayout: {
        const int minItemWidth = 48;
        itemWidth = minItemWidth + IconsModeSettings::textWidthIndex() * 64;

        if (previewsShown()) {
            // Optimize the width for previews with a 3:2 aspect ratio instead
            // of a 1:1 ratio to avoid wasting too much vertical space when
            // showing photos.
            const int minWidth = iconSize * 3 / 2;
            itemWidth = qMax(itemWidth, minWidth);
        }

        if (itemWidth < iconSize + padding * 2) {
            itemWidth = iconSize + padding * 2;
        }

        itemHeight = padding * 3 + iconSize + option.fontMetrics.lineSpacing();
        if (IconsModeSettings::maximumTextLines() > 0) {
            // A restriction is given for the maximum number of textlines (0 means
            // having no restriction)
            const int additionalInfoCount = visibleRoles().count() - 1;
            const int maxAdditionalLines = additionalInfoCount + IconsModeSettings::maximumTextLines();
            maxTextSize.rheight() = option.fontMetrics.lineSpacing() * maxAdditionalLines;
        }

        horizontalMargin = 4;
        verticalMargin = 8;
        break;
    }
    case KFileItemListView::CompactLayout: {
        itemWidth = padding * 4 + iconSize + option.fontMetrics.height() * 5;
        const int textLinesCount = visibleRoles().count();
        itemHeight = padding * 2 + qMax(iconSize, textLinesCount * option.fontMetrics.lineSpacing());

        if (CompactModeSettings::maximumTextWidthIndex() > 0) {
            // A restriction is given for the maximum width of the text (0 means
            // having no restriction)
            maxTextSize.rwidth() = option.fontMetrics.height() * 10 *
                                   CompactModeSettings::maximumTextWidthIndex();
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
    option.maxTextSize = maxTextSize;
    beginTransaction();
    setStyleOption(option);
    setItemSize(QSizeF(itemWidth, itemHeight));
    endTransaction();
}

void DolphinItemListView::updateFont()
{
    KItemListStyleOption option = styleOption();

    const ViewModeSettings settings(viewMode());

    QFont font(settings.fontFamily(), qRound(settings.fontSize()));
    font.setItalic(settings.italicFont());
    font.setWeight(settings.fontWeight());
    font.setPointSizeF(settings.fontSize());

    option.font = font;
    option.fontMetrics = QFontMetrics(font);

    setStyleOption(option);
}

ViewModeSettings::ViewMode DolphinItemListView::viewMode() const
{
    ViewModeSettings::ViewMode mode;

    switch (itemLayout()) {
    case KFileItemListView::IconsLayout:   mode = ViewModeSettings::IconsMode; break;
    case KFileItemListView::CompactLayout: mode = ViewModeSettings::CompactMode; break;
    case KFileItemListView::DetailsLayout: mode = ViewModeSettings::DetailsMode; break;
    default:                               mode = ViewModeSettings::IconsMode;
                                           Q_ASSERT(false);
                                           break;
    }

    return mode;
}

#include "dolphinitemlistview.moc"
