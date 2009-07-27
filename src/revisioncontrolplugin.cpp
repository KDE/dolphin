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

RevisionControlPlugin::RevisionControlPlugin()
{
}

RevisionControlPlugin::~RevisionControlPlugin()
{
}

#include "revisioncontrolplugin.moc"

// ----------------------------------------------------------------------------

#include <kaction.h>
#include <kdialog.h>
#include <kicon.h>
#include <klocale.h>
#include <krun.h>
#include <kshell.h>
#include <kfileitem.h>
#include <kvbox.h>
#include <QDir>
#include <QLabel>
#include <QString>
#include <QTextEdit>
#include <QTextStream>

SubversionPlugin::SubversionPlugin() :
    m_retrievalDir(),
    m_revisionInfoHash(),
    m_updateAction(0),
    m_showLocalChangesAction(0),
    m_commitAction(0),
    m_addAction(0),
    m_removeAction(0),
    m_contextDir(),
    m_contextItems()
{
    m_updateAction = new KAction(this);
    m_updateAction->setIcon(KIcon("view-refresh"));
    m_updateAction->setText(i18nc("@item:inmenu", "SVN Update"));
    connect(m_updateAction, SIGNAL(triggered()),
            this, SLOT(updateFiles()));

    m_showLocalChangesAction = new KAction(this);
    m_showLocalChangesAction->setIcon(KIcon("view-split-left-right"));
    m_showLocalChangesAction->setText(i18nc("@item:inmenu", "Show Local SVN Changes"));
    connect(m_showLocalChangesAction, SIGNAL(triggered()),
            this, SLOT(showLocalChanges()));

    m_commitAction = new KAction(this);
    m_commitAction->setText(i18nc("@item:inmenu", "SVN Commit..."));
    connect(m_commitAction, SIGNAL(triggered()),
            this, SLOT(commitFiles()));

    m_addAction = new KAction(this);
    m_addAction->setIcon(KIcon("list-add"));
    m_addAction->setText(i18nc("@item:inmenu", "SVN Add"));
    connect(m_addAction, SIGNAL(triggered()),
            this, SLOT(addFiles()));

    m_removeAction = new KAction(this);
    m_removeAction->setIcon(KIcon("list-remove"));
    m_removeAction->setText(i18nc("@item:inmenu", "SVN Delete"));
    connect(m_removeAction, SIGNAL(triggered()),
            this, SLOT(removeFiles()));
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
    m_retrievalDir = directory;
    const QString path = directory + ".svn/text-base/";

    QDir dir(path);
    const QFileInfoList fileInfoList = dir.entryInfoList();
    const int size = fileInfoList.size();
    QString fileName;
    for (int i = 0; i < size; ++i) {
        const QFileInfo fileInfo = fileInfoList.at(i);
        fileName = fileInfo.fileName();
        // Remove the ".svn-base" postfix to be able to compare the filenames
        // in a fast way in SubversionPlugin::revisionState().
        fileName.chop(sizeof(".svn-base") / sizeof(char) - 1);
        if (!fileName.isEmpty()) {
            RevisionInfo info;
            info.size = fileInfo.size();
            info.timeStamp = fileInfo.lastModified();
            m_revisionInfoHash.insert(directory + fileName, info);
        }
    }
    return size > 0;
}

void SubversionPlugin::endRetrieval()
{
}

RevisionControlPlugin::RevisionState SubversionPlugin::revisionState(const KFileItem& item)
{
    const QString itemUrl = item.localPath();
    if (item.isDir()) {
        QFile file(itemUrl + "/.svn");
        if (file.open(QIODevice::ReadOnly)) {
            file.close();
            return RevisionControlPlugin::NormalRevision;
        }
    } else if (m_revisionInfoHash.contains(itemUrl)) {
        const RevisionInfo info = m_revisionInfoHash.value(itemUrl);
        const QDateTime localTimeStamp = item.time(KFileItem::ModificationTime).dateTime();
        const QDateTime versionedTimeStamp = info.timeStamp;

        if (localTimeStamp > versionedTimeStamp) {
            if ((info.size != item.size()) || !equalRevisionContent(item.name())) {
                return RevisionControlPlugin::LocallyModifiedRevision;
            }
        } else if (localTimeStamp < versionedTimeStamp) {
            if ((info.size != item.size()) || !equalRevisionContent(item.name())) {
                return RevisionControlPlugin::UpdateRequiredRevision;
            }
        }
        return  RevisionControlPlugin::NormalRevision;
    }

    return RevisionControlPlugin::UnversionedRevision;
}

QList<QAction*> SubversionPlugin::contextMenuActions(const KFileItemList& items)
{
    Q_ASSERT(!items.isEmpty());

    m_contextItems = items;
    m_contextDir.clear();

    // iterate all items and check the revision state to know which
    // actions can be enabled
    const int itemsCount = items.count();
    int revisionedCount = 0;
    int editingCount = 0;
    foreach (const KFileItem& item, items) {
        const RevisionState state = revisionState(item);
        if (state != UnversionedRevision) {
            ++revisionedCount;
        }

        switch (state) {
        case LocallyModifiedRevision:
        case ConflictingRevision:
            ++editingCount;
            break;
        default:
            break;
        }
    }
    m_commitAction->setEnabled(editingCount > 0);
    m_addAction->setEnabled(revisionedCount == 0);
    m_removeAction->setEnabled(revisionedCount == itemsCount);

    QList<QAction*> actions;
    actions.append(m_updateAction);
    actions.append(m_commitAction);
    actions.append(m_addAction);
    actions.append(m_removeAction);
    return actions;
}

QList<QAction*> SubversionPlugin::contextMenuActions(const QString& directory)
{
    m_contextDir = directory;
    m_contextItems.clear();

    QList<QAction*> actions;
    actions.append(m_updateAction);
    actions.append(m_showLocalChangesAction);
    actions.append(m_commitAction);
    return actions;
}

void SubversionPlugin::updateFiles()
{
    execSvnCommand("update");
}

void SubversionPlugin::showLocalChanges()
{
    Q_ASSERT(!m_contextDir.isEmpty());
    Q_ASSERT(m_contextItems.isEmpty());

    const QString command = "mkfifo /tmp/fifo; svn diff " +
                            KShell::quoteArg(m_contextDir) +
                            " > /tmp/fifo & kompare /tmp/fifo; rm /tmp/fifo";
    KRun::runCommand(command, 0);
}

void SubversionPlugin::commitFiles()
{
    KDialog dialog(0, Qt::Dialog);

    KVBox* box = new KVBox(&dialog);
    new QLabel(i18nc("@label", "Description:"), box);
    QTextEdit* editor = new QTextEdit(box);

    dialog.setMainWidget(box);
    dialog.setCaption(i18nc("@title:window", "SVN Commit"));
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);
    dialog.setButtonText(KDialog::Ok, i18nc("@action:button", "Commit"));

    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "SvnCommitDialog");
    dialog.restoreDialogSize(dialogConfig);

    if (dialog.exec() == QDialog::Accepted) {
        const QString description = editor->toPlainText();
        execSvnCommand("commit -m " + KShell::quoteArg(description));
    }

    dialog.saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void SubversionPlugin::addFiles()
{
    execSvnCommand("add");
}

void SubversionPlugin::removeFiles()
{
    execSvnCommand("remove");
}

void SubversionPlugin::execSvnCommand(const QString& svnCommand)
{
    const QString command = "svn " + svnCommand + ' ';
    if (!m_contextDir.isEmpty()) {
        KRun::runCommand(command + KShell::quoteArg(m_contextDir), 0);
    } else {
        foreach (const KFileItem& item, m_contextItems) {
            KRun::runCommand(command + KShell::quoteArg(item.localPath()), 0);
        }
    }
}

bool SubversionPlugin::equalRevisionContent(const QString& name) const
{
    QFile localFile(m_retrievalDir + '/' + name);
    if (!localFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QFile revisionedFile(m_retrievalDir + "/.svn/text-base/" + name + ".svn-base");
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
