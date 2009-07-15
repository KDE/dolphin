/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "revisioncontrolplugin.h"

#include <kfileitem.h>
#include <QDir>
#include <QString>
#include <QTextStream>

RevisionControlPlugin::RevisionControlPlugin()
{
}

RevisionControlPlugin::~RevisionControlPlugin()
{
}

// ----------------------------------------------------------------------------

SubversionPlugin::SubversionPlugin() :
    m_directory(),
    m_revisionInfoHash()
{
}

SubversionPlugin::~SubversionPlugin()
{
}

QString SubversionPlugin::fileName() const
{
    return ".svn";
}

bool SubversionPlugin::beginRetrieval(const QString& directory)
{
    Q_ASSERT(directory.endsWith('/'));
    m_directory = directory;

    QFile file(directory + ".svn/entries");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    QString fileName;
    QString line;
    while (!in.atEnd()) {
        fileName = line;
        line = in.readLine();
        const bool isRevisioned = !line.isEmpty() &&
                                  ((line == QLatin1String("dir")) ||
                                   (line == QLatin1String("file")));
        if (isRevisioned) {
            RevisionInfo info; // TODO
            m_revisionInfoHash.insert(fileName, info);
        }
    }
    return true;
}

void SubversionPlugin::endRetrieval()
{
}

RevisionControlPlugin::RevisionState SubversionPlugin::revisionState(const KFileItem& item)
{
    const QString name = item.name();
    if (m_revisionInfoHash.contains(name)) {
        // TODO...
        return RevisionControlPlugin::LatestRevision;
    }

    return RevisionControlPlugin::LocalRevision;
}
