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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
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

#include "generalsettings.h"
#include "iconsmodesettings.h"
#include "detailsmodesettings.h"

#include <Q3IconView>

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
    m_detailsModeSettings->writeConfig();

    QString basePath = KGlobal::instance()->instanceName();
    basePath.append("/bookmarks.xml");
    const QString file = KStandardDirs::locateLocal( "data", basePath);

    KBookmarkManager* manager = KBookmarkManager::managerForFile(file, "dolphin", false);
    manager->save(false);
}

DolphinSettings::DolphinSettings()
{
    m_generalSettings = new GeneralSettings();
    m_iconsModeSettings = new IconsModeSettings();
    m_detailsModeSettings = new DetailsModeSettings();
}

DolphinSettings::~DolphinSettings()
{
    delete m_generalSettings;
    m_generalSettings = 0;

    delete m_iconsModeSettings;
    m_iconsModeSettings = 0;

    delete m_detailsModeSettings;
    m_detailsModeSettings = 0;
}
