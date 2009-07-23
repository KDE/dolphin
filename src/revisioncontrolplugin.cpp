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

#include <kaction.h>
#include <kicon.h>
#include <klocale.h>
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

#include "revisioncontrolplugin.moc"

// ----------------------------------------------------------------------------

SubversionPlugin::SubversionPlugin() :
    m_directory(),
    m_revisionInfoHash(),
    m_updateAction(0),
    m_commitAction(0),
    m_addAction(0),
    m_removeAction(0)
{
    m_updateAction = new KAction(this);
    m_updateAction->setIcon(KIcon("view-refresh"));
    m_updateAction->setText(i18nc("@item:inmenu", "SVN Update"));

    m_commitAction = new KAction(this);
    m_commitAction->setText(i18nc("@item:inmenu", "SVN Commit..."));

    m_addAction = new KAction(this);
    m_addAction->setIcon(KIcon("list-add"));
    m_addAction->setText(i18nc("@item:inmenu", "SVN Add"));

    m_removeAction = new KAction(this);
    m_removeAction->setIcon(KIcon("list-remove"));
    m_removeAction->setText(i18nc("@item:inmenu", "SVN Delete"));
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
            if ((info.size != item.size()) || !equalRevisionContent(item.name())) {
                return RevisionControlPlugin::EditingRevision;
            }
        } else if (localTimeStamp < versionedTimeStamp) {
            if ((info.size != item.size()) || !equalRevisionContent(item.name())) {
                return RevisionControlPlugin::UpdateRequiredRevision;
            }
        }
        return  RevisionControlPlugin::LatestRevision;
    }

    return RevisionControlPlugin::LocalRevision;
}

QList<QAction*> SubversionPlugin::contextMenuActions(const KFileItemList& items) const
{
    Q_UNUSED(items);

    QList<QAction*> actions;
    actions.append(m_updateAction);
    actions.append(m_commitAction);
    actions.append(m_addAction);
    actions.append(m_removeAction);
    return actions;
}

QList<QAction*> SubversionPlugin::contextMenuActions(const QString& directory) const
{
    Q_UNUSED(directory);

    QList<QAction*> actions;
    actions.append(m_updateAction);
    actions.append(m_commitAction);
    return actions;
}

bool SubversionPlugin::equalRevisionContent(const QString& name) const
{
    QFile localFile(m_directory + '/' + name);
    if (!localFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QFile revisionedFile(m_directory + "/.svn/text-base/" + name + ".svn-base");
    if (!revisionedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

     QTextStream localText(&localFile);
     QTextStream revisionedText(&revisionedFile);
     while (!localText.atEnd() && !revisionedText.atEnd()) {
         if (localText.readLine() != revisionedText.readLine()) {
             return false;
         }
     }

     return localText.atEnd() && revisionedText.atEnd();
}

