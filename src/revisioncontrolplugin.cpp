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
#include <kfileitem.h>
#include <kicon.h>
#include <klocale.h>
#include <krun.h>
#include <kshell.h>
#include <kvbox.h>
#include <QDir>
#include <QLabel>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QTextStream>

SubversionPlugin::SubversionPlugin() :
    m_revisionInfoHash(),
    m_revisionInfoKeys(),
    m_updateAction(0),
    m_showLocalChangesAction(0),
    m_commitAction(0),
    m_addAction(0),
    m_removeAction(0),
    m_command(),
    m_errorMsg(),
    m_operationCompletedMsg(),
    m_contextDir(),
    m_contextItems(),
    m_tempFile()
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

    QStringList arguments;
    arguments << "status" << "--show-updates" << directory;

    QProcess process;
    process.start("svn", arguments);
    while (process.waitForReadyRead()) {
        char buffer[1024];
        while (process.readLine(buffer, sizeof(buffer)) > 0)  {
            RevisionState state = NormalRevision;
            QString filePath(buffer);

            switch (buffer[0]) {
            case '?': state = UnversionedRevision; break;
            case 'M': state = LocallyModifiedRevision; break;
            case 'A': state = AddedRevision; break;
            case 'D': state = RemovedRevision; break;
            case 'C': state = ConflictingRevision; break;
            default:
                if (filePath.contains('*')) {
                    state = UpdateRequiredRevision;
                }
                break;
            }

            int pos = filePath.indexOf('/');
            const int length = filePath.length() - pos - 1;
            filePath = filePath.mid(pos, length);
            if (!filePath.isEmpty()) {
                m_revisionInfoHash.insert(filePath, state);
            }
        }
    }

    m_revisionInfoKeys = m_revisionInfoHash.keys();
    return true;
}

void SubversionPlugin::endRetrieval()
{
}

RevisionControlPlugin::RevisionState SubversionPlugin::revisionState(const KFileItem& item)
{
    const QString itemUrl = item.localPath();
    if (m_revisionInfoHash.contains(itemUrl)) {
        return m_revisionInfoHash.value(itemUrl);
    }

    if (!item.isDir()) {
        // files that have not been listed by 'svn status' (= m_revisionInfoHash)
        // are under revision control per definition
        return NormalRevision;
    }

    // The item is a directory. Check whether an item listed by 'svn status' (= m_revisionInfoHash)
    // is part of this directory. In this case a local modification should be indicated in the
    // directory already.
    foreach (const QString& key, m_revisionInfoKeys) {
        if (key.startsWith(itemUrl)) {
            const RevisionState state = m_revisionInfoHash.value(key);
            if (state == LocallyModifiedRevision) {
                return LocallyModifiedRevision;
            }
        }
    }

    return NormalRevision;
}

QList<QAction*> SubversionPlugin::contextMenuActions(const KFileItemList& items)
{
    Q_ASSERT(!items.isEmpty());
    foreach (const KFileItem& item, items) {
        m_contextItems.append(item);
    }
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
    const bool enabled = m_contextItems.isEmpty();
    if (enabled) {
        m_contextDir = directory;
    }

    // Only enable the SVN actions if no SVN commands are
    // executed currently (see slotOperationCompleted() and
    // startSvnCommandProcess()).
    m_updateAction->setEnabled(enabled);
    m_showLocalChangesAction->setEnabled(enabled);
    m_commitAction->setEnabled(enabled);

    QList<QAction*> actions;
    actions.append(m_updateAction);
    actions.append(m_showLocalChangesAction);
    actions.append(m_commitAction);
    return actions;
}

void SubversionPlugin::updateFiles()
{
    execSvnCommand("update",
                   i18nc("@info:status", "Updating SVN repository..."),
                   i18nc("@info:status", "Update of SVN repository failed."),
                   i18nc("@info:status", "Updated SVN repository."));
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
        // Write the commit description into a temporary file, so
        // that it can be read by the command "svn commit -F". The temporary
        // file must stay alive until slotOperationCompleted() is invoked and will
        // be destroyed when the revision plugin is destructed.
        if (!m_tempFile.open())  {
            emit errorMessage(i18nc("@info:status", "Commit of SVN changes failed."));
            return;
        }

        QTextStream out(&m_tempFile);
        const QString fileName = m_tempFile.fileName();
        out << editor->toPlainText();
        m_tempFile.close();

        execSvnCommand("commit -F " + KShell::quoteArg(fileName),
                       i18nc("@info:status", "Committing SVN changes..."),
                       i18nc("@info:status", "Commit of SVN changes failed."),
                       i18nc("@info:status", "Committed SVN changes."));
    }

    dialog.saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void SubversionPlugin::addFiles()
{
    execSvnCommand("add",
                   i18nc("@info:status", "Adding files to SVN repository..."),
                   i18nc("@info:status", "Adding of files to SVN repository failed."),
                   i18nc("@info:status", "Added files to SVN repository."));
}

void SubversionPlugin::removeFiles()
{
    execSvnCommand("remove",
                   i18nc("@info:status", "Removing files from SVN repository..."),
                   i18nc("@info:status", "Removing of files from SVN repository failed."),
                   i18nc("@info:status", "Removed files from SVN repository."));
}

void SubversionPlugin::slotOperationCompleted()
{
    if (m_contextItems.isEmpty()) {
        emit operationCompletedMessage(m_operationCompletedMsg);
        emit revisionStatesChanged();
    } else {
        startSvnCommandProcess();
    }
}

void SubversionPlugin::slotOperationError()
{
    emit errorMessage(m_errorMsg);

    // don't do any operation on other items anymore
    m_contextItems.clear();
}

void SubversionPlugin::execSvnCommand(const QString& svnCommand,
                                      const QString& infoMsg,
                                      const QString& errorMsg,
                                      const QString& operationCompletedMsg)
{
    emit infoMessage(infoMsg);

    m_command = svnCommand;
    m_errorMsg = errorMsg;
    m_operationCompletedMsg = operationCompletedMsg;

    startSvnCommandProcess();
}

void SubversionPlugin::startSvnCommandProcess()
{
    QProcess* process = new QProcess(this);
    connect(process, SIGNAL(finished(int)),
            this, SLOT(slotOperationCompleted()));
    connect(process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(slotOperationError()));

    const QString program = "svn " + m_command + ' ';
    if (!m_contextDir.isEmpty()) {
        process->start(program + KShell::quoteArg(m_contextDir));
        m_contextDir.clear();
    } else {
        const KFileItem item = m_contextItems.takeLast();
        process->start(program + KShell::quoteArg(item.localPath()));
        // the remaining items of m_contextItems will be executed
        // after the process has finished (see slotOperationFinished())
    }
}
