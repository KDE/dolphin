/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at),                 *
 *   Cvetoslav Ludmiloff and Patrice Tremblay                              *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dolphinsettings.h"

#include <assert.h>
#include <qdir.h>

#include <kapplication.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kicontheme.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "dolphin.h"
#include "generalsettings.h"
#include "iconsmodesettings.h"
#include "previewsmodesettings.h"
#include "detailsmodesettings.h"
#include "sidebarsettings.h"

DolphinSettings& DolphinSettings::instance()
{
    static DolphinSettings* instance = 0;
    if (instance == 0) {
        instance = new DolphinSettings();
    }
    return *instance;
}

KBookmark DolphinSettings::bookmark(int index) const
{
    int i = 0;
    KBookmarkGroup root = bookmarkManager()->root();
    KBookmark bookmark = root.first();
    while (!bookmark.isNull()) {
        if (i == index) {
            return bookmark;
        }
        ++i;
        bookmark = root.next(bookmark);
    }

    return KBookmark();
}

KBookmarkManager* DolphinSettings::bookmarkManager() const
{
    QString basePath = KGlobal::instance()->instanceName();
    basePath.append("/bookmarks.xml");
    const QString file = KStandardDirs::locateLocal("data", basePath);

    return KBookmarkManager::managerForFile(file, "dolphin", false);
}

void DolphinSettings::save()
{
    m_generalSettings->writeConfig();
    m_iconsModeSettings->writeConfig();
    m_previewsModeSettings->writeConfig();
    m_detailsModeSettings->writeConfig();
    m_sidebarSettings->writeConfig();

    QString basePath = KGlobal::instance()->instanceName();
    basePath.append("/bookmarks.xml");
    const QString file = KStandardDirs::locateLocal( "data", basePath);

    KBookmarkManager* manager = KBookmarkManager::managerForFile(file, "dolphin", false);
    manager->save(false);
}

void DolphinSettings::calculateGridSize(int hint)
{
    // TODO: remove in KDE4
    const int previewSize = m_iconsModeSettings->previewSize();
    const int iconSize = m_iconsModeSettings->iconSize();
    const int maxSize = (previewSize > iconSize) ? previewSize : iconSize;
    const Q3IconView::Arrangement arrangement = (m_iconsModeSettings->arrangement() == "LeftToRight") ?
                                               Q3IconView::LeftToRight : Q3IconView::TopToBottom;

    int gridWidth = 0;
    int gridHeight = 0;
    if (arrangement == Q3IconView::LeftToRight) {
        int widthUnit = maxSize + (maxSize / 2);
        if (widthUnit < K3Icon::SizeLarge) {
            widthUnit = K3Icon::SizeLarge;
        }

        gridWidth = widthUnit + hint * K3Icon::SizeLarge;

        gridHeight = iconSize;
        if (gridHeight <= K3Icon::SizeMedium) {
            gridHeight = gridHeight * 2;
        }
        else {
            gridHeight += maxSize / 2;
        }
    }
    else {
        assert(arrangement == Q3IconView::TopToBottom);
        gridWidth = maxSize + (hint + 1) * (8 * m_iconsModeSettings->fontSize());

        // The height-setting is ignored yet by KFileIconView if the TopToBottom
        // arrangement is active. Anyway write the setting to have a defined value.
        gridHeight = maxSize;
    }

    m_iconsModeSettings->setGridWidth(gridWidth);
    m_iconsModeSettings->setGridHeight(gridHeight);
}

int DolphinSettings::textWidthHint() const
{
    // TODO: remove in KDE4
    const int previewSize = m_iconsModeSettings->previewSize();
    const int iconSize = m_iconsModeSettings->iconSize();
    const Q3IconView::Arrangement arrangement = (m_iconsModeSettings->arrangement() == "LeftToRight") ?
                                               Q3IconView::LeftToRight : Q3IconView::TopToBottom;

    const int gridWidth = m_iconsModeSettings->gridWidth();

    const int maxSize = (previewSize > iconSize) ? previewSize : iconSize;
    int hint = 0;
    if (arrangement == Q3IconView::LeftToRight) {
        int widthUnit = maxSize + (maxSize / 2);
        if (widthUnit < K3Icon::SizeLarge) {
            widthUnit = K3Icon::SizeLarge;
        }
        hint = (gridWidth - widthUnit) / K3Icon::SizeLarge;
    }
    else {
        assert(arrangement == Q3IconView::TopToBottom);
        hint = (gridWidth - maxSize) / (8 * m_iconsModeSettings->fontSize()) - 1;
        if (hint > 2) {
            hint = 2;
        }
    }
    return hint;
}

DolphinSettings::DolphinSettings()
{
    m_generalSettings = new GeneralSettings();
    m_iconsModeSettings = new IconsModeSettings();
    m_previewsModeSettings = new PreviewsModeSettings();
    m_detailsModeSettings = new DetailsModeSettings();
    m_sidebarSettings = new SidebarSettings();
}

DolphinSettings::~DolphinSettings()
{
    delete m_generalSettings;
    m_generalSettings = 0;

    delete m_iconsModeSettings;
    m_iconsModeSettings = 0;

    delete m_previewsModeSettings;
    m_previewsModeSettings = 0;

    delete m_detailsModeSettings;
    m_detailsModeSettings = 0;

    delete m_sidebarSettings;
    m_sidebarSettings = 0;
}
