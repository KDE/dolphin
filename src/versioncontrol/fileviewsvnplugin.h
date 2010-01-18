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

#ifndef FILEVIEWSVNPLUGIN_H
#define FILEVIEWSVNPLUGIN_H

#include <kfileitem.h>
#include <kversioncontrolplugin.h>
#include <QHash>
#include <QProcess>
#include <QTemporaryFile>

// TODO: This class will be moved to kdevplatform as soon as kdevplatform will
// be released. Moving it to kdevplatform allows to reuse code for the context
// menu actions like commit, add, update, ...
class FileViewSvnPlugin : public KVersionControlPlugin
{
    Q_OBJECT

public:
    FileViewSvnPlugin(QObject* parent, const QList<QVariant>& args);
    virtual ~FileViewSvnPlugin();
    virtual QString fileName() const;
    virtual bool beginRetrieval(const QString& directory);
    virtual void endRetrieval();
    virtual KVersionControlPlugin::VersionState versionState(const KFileItem& item);
    virtual QList<QAction*> contextMenuActions(const KFileItemList& items);
    virtual QList<QAction*> contextMenuActions(const QString& directory);

private slots:
    void updateFiles();
    void showLocalChanges();
    void commitFiles();
    void addFiles();
    void removeFiles();

    void slotOperationCompleted(int exitCode, QProcess::ExitStatus exitStatus);
    void slotOperationError();

private:
    /**
     * Executes the command "svn {svnCommand}" for the files that have been
     * set by getting the context menu actions (see contextMenuActions()).
     * @param infoMsg     Message that should be shown before the command is executed.
     * @param errorMsg    Message that should be shown if the execution of the command
     *                    has been failed.
     * @param operationCompletedMsg
     *                    Message that should be shown if the execution of the command
     *                    has been completed successfully.
     */
    void execSvnCommand(const QString& svnCommand,
                        const QString& infoMsg,
                        const QString& errorMsg,
                        const QString& operationCompletedMsg);

    void startSvnCommandProcess();

private:
    bool m_pendingOperation;
    QHash<QString, VersionState> m_versionInfoHash;
    QList<QString> m_versionInfoKeys; // cache for accessing the keys of the hash

    QAction* m_updateAction;
    QAction* m_showLocalChangesAction;
    QAction* m_commitAction;
    QAction* m_addAction;
    QAction* m_removeAction;

    QString m_command;
    QString m_errorMsg;
    QString m_operationCompletedMsg;

    QString m_contextDir;
    KFileItemList m_contextItems;

    QTemporaryFile m_tempFile;
};
#endif // FILEVIEWSVNPLUGIN_H

