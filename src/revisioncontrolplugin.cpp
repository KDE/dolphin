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
    const QString path = directory + ".svn/text-base/";

    QDir dir(path);
    const QFileInfoList fileInfoList = dir.entryInfoList();
    const int size = fileInfoList.size();
    QString fileName;
    for (int i = 0; i < size; ++i) {
        fileName = fileInfoList.at(i).fileName();
        // Remove the ".svn-base" postfix to be able to compare the filenames
        // in a fast way in SubversionPlugin::revisionState().
        fileName.chop(sizeof(".svn-base") / sizeof(char) - 1);
        if (!fileName.isEmpty()) {
            RevisionInfo info;
            info.size = fileInfoList.at(i).size();
            info.timeStamp = fileInfoList.at(i).lastModified();            
            m_revisionInfoHash.insert(fileName, info);
        }
    }
    return size > 0;
}

void SubversionPlugin::endRetrieval()
{
}

RevisionControlPlugin::RevisionState SubversionPlugin::revisionState(const KFileItem& item)
{
    const QString name = item.name();
    if (item.isDir()) {
        QFile file(m_directory + name + "/.svn");
        if (file.open(QIODevice::ReadOnly)) {
            file.close();
            // TODO...
            return RevisionControlPlugin::LatestRevision;
        }
    } else if (m_revisionInfoHash.contains(name)) {
        const RevisionInfo info = m_revisionInfoHash.value(item.name());
        const QDateTime localTimeStamp = item.time(KFileItem::ModificationTime).dateTime();
        const QDateTime versionedTimeStamp = info.timeStamp;

        if (localTimeStamp > versionedTimeStamp) {
            if (info.size != item.size()) {
                return RevisionControlPlugin::EditingRevision;
            }
            // TODO: a comparison of the content is required
        } else if (localTimeStamp < versionedTimeStamp) {
            if (info.size != item.size()) {
                return RevisionControlPlugin::UpdateRequiredRevision;
            }
            // TODO: a comparison of the content is required
        }
        return  RevisionControlPlugin::LatestRevision;
    }

    return RevisionControlPlugin::LocalRevision;
}
