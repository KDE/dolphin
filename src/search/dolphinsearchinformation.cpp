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
#ifdef HAVE_BALOO
    #include <KConfig>
    #include <KConfigGroup>
#endif

#include <KGlobal>
#include <KUrl>
#include <QFileInfo>
#include <QDir>

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

namespace {
    /// recursively check if a folder is hidden
    bool isDirHidden( QDir& dir ) {
        if (QFileInfo(dir.path()).isHidden()) {
            return true;
        } else if (dir.cdUp()) {
            return isDirHidden(dir);
        } else {
            return false;
        }
    }

    bool isDirHidden(const QString& path) {
        QDir dir(path);
        return isDirHidden(dir);
    }
}

bool DolphinSearchInformation::isPathIndexed(const KUrl& url) const
{
#ifdef HAVE_BALOO
    const KConfig strigiConfig("baloofilerc");
    const QStringList indexedFolders = strigiConfig.group("General").readPathEntry("folders", QStringList());

    // Baloo does not index hidden folders
    if (isDirHidden(url.toLocalFile())) {
        return false;
    }

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
    Q_UNUSED(url);
    return false;
#endif
}

DolphinSearchInformation::DolphinSearchInformation() :
    m_indexingEnabled(false)
{
#ifdef HAVE_BALOO
    KConfig config("baloofilerc");
    KConfigGroup group = config.group("Basic Settings");
    if (group.readEntry("Enabled", true)) {
        m_indexingEnabled = config.group("Basic Settings").readEntry("Indexing-Enabled", true);
    }
#endif
}

