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

#include "dolphinitemlistcontainer.h"

#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_compactmodesettings.h"

#include "dolphinfileitemlistwidget.h"

#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kfileitemmodel.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kitemliststyleoption.h>

#include <KDirLister>
#include <KGlobalSettings>

#include <views/viewmodecontroller.h>

#include "zoomlevelinfo.h"


DolphinItemListContainer::DolphinItemListContainer(KDirLister* dirLister,
                                                   QWidget* parent) :
    KItemListContainer(parent),
    m_zoomLevel(0),
    m_fileItemListView(0)
{
    controller()->setModel(new KFileItemModel(dirLister, this));

    m_fileItemListView = new KFileItemListView();
    m_fileItemListView->setWidgetCreator(new KItemListWidgetCreator<DolphinFileItemListWidget>());    
    m_fileItemListView->setEnabledSelectionToggles(GeneralSettings::showSelectionToggle());
    controller()->setView(m_fileItemListView);

    updateAutoActivationDelay();
    updateFont();
    updateGridSize();
}

DolphinItemListContainer::~DolphinItemListContainer()
{
    writeSettings();
    
    controller()->setView(0);
    delete m_fileItemListView;
    m_fileItemListView = 0;
}

void DolphinItemListContainer::setPreviewsShown(bool show)
{
    beginTransaction();
    m_fileItemListView->setPreviewsShown(show);
    updateGridSize();
    endTransaction();
}

bool DolphinItemListContainer::previewsShown() const
{
    return m_fileItemListView->previewsShown();
}

void DolphinItemListContainer::setVisibleRoles(const QList<QByteArray>& roles)
{
    m_fileItemListView->setVisibleRoles(roles);
    updateGridSize();
}

QList<QByteArray> DolphinItemListContainer::visibleRoles() const
{
    return m_fileItemListView->visibleRoles();
}

void DolphinItemListContainer::setZoomLevel(int level)
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

int DolphinItemListContainer::zoomLevel() const
{
    return m_zoomLevel;
}

void DolphinItemListContainer::setItemLayout(KFileItemListView::Layout layout)
{
    if (layout == itemLayout()) {
        return;
    }

    beginTransaction();
    m_fileItemListView->setItemLayout(layout);

    switch (layout) {
    case KFileItemListView::IconsLayout:
        m_fileItemListView->setScrollOrientation(Qt::Vertical);
        m_fileItemListView->setHeaderShown(false);
        break;
    case KFileItemListView::DetailsLayout:
        m_fileItemListView->setScrollOrientation(Qt::Vertical);
        m_fileItemListView->setHeaderShown(true);
        break;
    case KFileItemListView::CompactLayout:
        m_fileItemListView->setScrollOrientation(Qt::Horizontal);
        m_fileItemListView->setHeaderShown(false);
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    updateFont();
    updateGridSize();
    endTransaction();
}

KFileItemListView::Layout DolphinItemListContainer::itemLayout() const
{
    return m_fileItemListView->itemLayout();
}

void DolphinItemListContainer::beginTransaction()
{
    m_fileItemListView->beginTransaction();
}

void DolphinItemListContainer::endTransaction()
{
    m_fileItemListView->endTransaction();
}

void DolphinItemListContainer::readSettings()
{
    ViewModeSettings settings(viewMode());
    settings.readConfig();

    beginTransaction();

    m_fileItemListView->setEnabledSelectionToggles(GeneralSettings::showSelectionToggle());
    updateAutoActivationDelay();
    updateFont();
    updateGridSize();

    const KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    const QStringList plugins = globalConfig.readEntry("Plugins", QStringList()
                                                       << "directorythumbnail"
                                                       << "imagethumbnail"
                                                       << "jpegthumbnail");
    m_fileItemListView->setEnabledPlugins(plugins);

    endTransaction();
}

void DolphinItemListContainer::writeSettings()
{  
    IconsModeSettings::self()->writeConfig();
    CompactModeSettings::self()->writeConfig();
    DetailsModeSettings::self()->writeConfig();
}

void DolphinItemListContainer::updateGridSize()
{
    const ViewModeSettings settings(viewMode());

    // Calculate the size of the icon
    const int iconSize = previewsShown() ? settings.previewSize() : settings.iconSize();
    m_zoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(iconSize, iconSize));
    KItemListStyleOption styleOption = m_fileItemListView->styleOption();

    const int padding = 2;
    int horizontalMargin = 0;
    int verticalMargin = 0;

    // Calculate the item-width and item-height
    int itemWidth;
    int itemHeight;
    switch (itemLayout()) {
    case KFileItemListView::IconsLayout: {
        const int minItemWidth = 48;
        itemWidth = minItemWidth + IconsModeSettings::textWidthIndex() * 64;
        
        if (previewsShown()) {
            // Optimize the width for previews with a 3:2 aspect ratio instead
            // of a 1:1 ratio to avoid wasting too much vertical space when
            // photos.
            const int minWidth = iconSize * 3 / 2;
            itemWidth = qMax(itemWidth, minWidth);
        }
        
        if (itemWidth < iconSize + padding * 2) {
            itemWidth = iconSize + padding * 2;
        }
        itemHeight = padding * 3 + iconSize + styleOption.fontMetrics.height();
        
        horizontalMargin = 4;
        verticalMargin = 8;
        break;
    }
    case KFileItemListView::CompactLayout: {
        itemWidth = padding * 4 + iconSize + styleOption.fontMetrics.height() * 5;
        const int textLinesCount = m_fileItemListView->visibleRoles().count();
        itemHeight = padding * 2 + qMax(iconSize, textLinesCount * styleOption.fontMetrics.height());
        
        horizontalMargin = 8;
        break;
    }
    case KFileItemListView::DetailsLayout: {
        itemWidth = -1;
        itemHeight = padding * 2 + qMax(iconSize, styleOption.fontMetrics.height());
        break;
    }
    default:
        itemWidth = -1;
        itemHeight = -1;
        Q_ASSERT(false);
        break;
    }

    // Apply the calculated values
    styleOption.padding = padding;
    styleOption.horizontalMargin = horizontalMargin;
    styleOption.verticalMargin = verticalMargin;
    styleOption.iconSize = iconSize;
    m_fileItemListView->beginTransaction();
    m_fileItemListView->setStyleOption(styleOption);
    m_fileItemListView->setItemSize(QSizeF(itemWidth, itemHeight));
    m_fileItemListView->endTransaction();
}

void DolphinItemListContainer::updateFont()
{
    KItemListStyleOption styleOption = m_fileItemListView->styleOption();

    const ViewModeSettings settings(viewMode());

    QFont font(settings.fontFamily(), qRound(settings.fontSize()));
    font.setItalic(settings.italicFont());
    font.setWeight(settings.fontWeight());
    font.setPointSizeF(settings.fontSize());

    styleOption.font = font;
    styleOption.fontMetrics = QFontMetrics(font);

    m_fileItemListView->setStyleOption(styleOption);
}

void DolphinItemListContainer::updateAutoActivationDelay()
{
    const int delay = GeneralSettings::autoExpandFolders() ? 750 : -1;
    controller()->setAutoActivationDelay(delay);
}

ViewModeSettings::ViewMode DolphinItemListContainer::viewMode() const
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

#include "dolphinitemlistcontainer.moc"
