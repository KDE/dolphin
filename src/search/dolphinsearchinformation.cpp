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

#include "dolphinsearchinformation.h"

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #include <KConfig>
    #include <KConfigGroup>
    #include <Nepomuk/ResourceManager>
#endif

#include <KGlobal>
#include <KUrl>

struct DolphinSearchInformationSingleton
{
    DolphinSearchInformation instance;
};
K_GLOBAL_STATIC(DolphinSearchInformationSingleton, s_dolphinSearchInformation)


DolphinSearchInformation& DolphinSearchInformation::instance()
{
    return s_dolphinSearchInformation->instance;
}

DolphinSearchInformation::~DolphinSearchInformation()
{
}

bool DolphinSearchInformation::isIndexingEnabled() const
{
    return m_indexingEnabled;
}

bool DolphinSearchInformation::isPathIndexed(const KUrl& url) const
{
#ifdef HAVE_NEPOMUK
    const KConfig strigiConfig("nepomukstrigirc");
    const QStringList indexedFolders = strigiConfig.group("General").readPathEntry("folders", QStringList());

    // Check whether the path is part of an indexed folder
    bool isIndexed = false;
    foreach (const QString& indexedFolder, indexedFolders) {
        const KUrl indexedPath(indexedFolder);
        if (indexedPath.isParentOf(url)) {
            isIndexed = true;
            break;
        }
    }

    if (isIndexed) {
        // The path is part of an indexed folder. Check whether no
        // excluded folder is part of the path.
        const QStringList excludedFolders = strigiConfig.group("General").readPathEntry("exclude folders", QStringList());
        foreach (const QString& excludedFolder, excludedFolders) {
            const KUrl excludedPath(excludedFolder);
            if (excludedPath.isParentOf(url)) {
                isIndexed = false;
                break;
            }
        }
    }

    return isIndexed;
#else
    Q_UNUSED(path);
    return false;
#endif
}

DolphinSearchInformation::DolphinSearchInformation() :
    m_indexingEnabled(false)
{
#ifdef HAVE_NEPOMUK
    if (Nepomuk::ResourceManager::instance()->init() == 0) {
        KConfig config("nepomukserverrc");
        m_indexingEnabled = config.group("Service-nepomukstrigiservice").readEntry("autostart", false);
    }
#endif
}

