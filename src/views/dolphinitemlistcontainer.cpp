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
    controller()->setView(m_fileItemListView);

    KItemListStyleOption option;

    // TODO:
    option.font = parent->font();
    option.fontMetrics = QFontMetrics(parent->font());

    updateGridSize();
/*
    connect(this, SIGNAL(clicked(QModelIndex)),
            dolphinViewController, SLOT(requestTab(QModelIndex)));*/
/*
    connect(this, SIGNAL(entered(QModelIndex)),
            dolphinViewController, SLOT(emitItemEntered(QModelIndex)));
    connect(this, SIGNAL(viewportEntered()),
            dolphinViewController, SLOT(emitViewportEntered()));*/

    // apply the icons mode settings to the widget
    //const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    //Q_ASSERT(settings);

    /*if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    } else {
        m_font = QFont(settings->fontFamily(),
                       qRound(settings->fontSize()),
                       settings->fontWeight(),
                       settings->italicFont());
        m_font.setPointSizeF(settings->fontSize());
    }

    setWordWrap(settings->numberOfTextlines() > 1);

    if (settings->arrangement() == QListView::TopToBottom) {
        setFlow(QListView::LeftToRight);
        m_decorationPosition = QStyleOptionViewItem::Top;
        m_displayAlignment = Qt::AlignHCenter;
    } else {
        setFlow(QListView::TopToBottom);
        m_decorationPosition = QStyleOptionViewItem::Left;
        m_displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }

    connect(m_categoryDrawer, SIGNAL(actionRequested(int,QModelIndex)), this, SLOT(categoryDrawerActionRequested(int,QModelIndex)));
    setCategoryDrawer(m_categoryDrawer);

    connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)),
            this, SLOT(slotGlobalSettingsChanged(int)));*/

    //updateGridSize(dolphinView->showPreview(), 0);
    /*m_extensionsFactory = new ViewExtensionsFactory(this, dolphinViewController, viewModeController);*/
}

DolphinItemListContainer::~DolphinItemListContainer()
{
    IconsModeSettings::self()->writeConfig();
    CompactModeSettings::self()->writeConfig();
    DetailsModeSettings::self()->writeConfig();

    KItemListView* view = controller()->view();
    controller()->setView(0);
    delete view;
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

    if (previewsShown()) {
        const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(level);

        switch (itemLayout()) {
        case KFileItemListView::IconsLayout:   IconsModeSettings::setPreviewSize(previewSize); break;
        case KFileItemListView::CompactLayout: CompactModeSettings::setPreviewSize(previewSize); break;
        case KFileItemListView::DetailsLayout: DetailsModeSettings::setPreviewSize(previewSize); break;
        default: Q_ASSERT(false); break;
        }
    } else {
        const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(level);
        switch (itemLayout()) {
        case KFileItemListView::IconsLayout:   IconsModeSettings::setIconSize(iconSize); break;
        case KFileItemListView::CompactLayout: CompactModeSettings::setIconSize(iconSize); break;
        case KFileItemListView::DetailsLayout: DetailsModeSettings::setIconSize(iconSize); break;
        default: Q_ASSERT(false); break;
        }
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

void DolphinItemListContainer::updateGridSize()
{
    // Calculate the size of the icon
    int iconSize;
    if (previewsShown()) {
        switch (itemLayout()) {
        case KFileItemListView::IconsLayout:   iconSize = IconsModeSettings::previewSize(); break;
        case KFileItemListView::CompactLayout: iconSize = CompactModeSettings::previewSize(); break;
        case KFileItemListView::DetailsLayout: iconSize = DetailsModeSettings::previewSize(); break;
        default: Q_ASSERT(false); break;
        }
    } else {
        switch (itemLayout()) {
        case KFileItemListView::IconsLayout:   iconSize = IconsModeSettings::iconSize(); break;
        case KFileItemListView::CompactLayout: iconSize = CompactModeSettings::iconSize(); break;
        case KFileItemListView::DetailsLayout: iconSize = DetailsModeSettings::iconSize(); break;
        default: Q_ASSERT(false); break;
        }
    }

    m_zoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(iconSize, iconSize));
    KItemListStyleOption styleOption = m_fileItemListView->styleOption();

    const int innerMargin = (iconSize >= KIconLoader::SizeSmallMedium) ? 4 : 2;

    // Calculate the item-width and item-height
    int itemWidth;
    int itemHeight;
    switch (itemLayout()) {
    case KFileItemListView::IconsLayout: {
        const int minItemWidth = 64;
        itemWidth = minItemWidth + IconsModeSettings::textWidthIndex() * 64; // TODO:
        if (itemWidth < iconSize + innerMargin * 2) {
            itemWidth = iconSize + innerMargin * 2;
        }
        itemHeight = innerMargin * 2 + iconSize + styleOption.fontMetrics.height();
        break;
    }
    case KFileItemListView::CompactLayout: {
        itemWidth = innerMargin * 2;
        const int textLinesCount = m_fileItemListView->visibleRoles().count();
        itemHeight = innerMargin * 2 + qMax(iconSize, textLinesCount * styleOption.fontMetrics.height());
        break;
    }
    case KFileItemListView::DetailsLayout: {
        itemWidth = -1;
        itemHeight = innerMargin * 2 + qMax(iconSize, styleOption.fontMetrics.height());
        break;
    }
    default: Q_ASSERT(false); break;
    }

    // Apply the calculated values
    styleOption.margin = innerMargin;
    styleOption.iconSize = iconSize;
    m_fileItemListView->setStyleOption(styleOption);
    m_fileItemListView->setItemSize(QSizeF(itemWidth, itemHeight));
}

#include "dolphinitemlistcontainer.moc"
