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

#ifndef REVISIONCONTROLPLUGIN_H
#define REVISIONCONTROLPLUGIN_H

#include <libdolphin_export.h>

#include <QDateTime>
#include <QString>
#include <QObject>

class KFileItem;
class KFileItemList;
class QAction;

/**
 * @brief Base class for version control plugins.
 *
 * Enables the file manager to show the version state
 * of a versioned file.
 */
class LIBDOLPHINPRIVATE_EXPORT KVersionControlPlugin : public QObject
{
    Q_OBJECT

public:
    enum VersionState
    {
        /** The file is not under version control. */
        UnversionedVersion,
        /**
         * The file is under version control and represents
         * the latest version.
         */
        NormalVersion,
        /**
         * The file is under version control and a newer
         * version exists on the main branch.
         */
        UpdateRequiredVersion,
        /**
         * The file is under version control and has been
         * modified locally.
         */
        LocallyModifiedVersion,
        /**
         * The file has not been under version control but
         * has been marked to get added with the next commit.
         */
        AddedVersion,
        /**
         * The file is under version control but has been marked
         * for getting removed with the next commit.
         */
        RemovedVersion,
        /**
         * The file is under version control and has been locally
         * modified. A modification has also been done on the main
         * branch.
         */
        ConflictingVersion
    };

    KVersionControlPlugin();
    virtual ~KVersionControlPlugin();

    /**
     * Returns the name of the file which stores
     * the version control informations.
     * (e. g. .svn, .cvs, .git).
     */
    virtual QString fileName() const = 0;

    /**
     * Is invoked whenever the version control
     * information will get retrieved for the directory
     * \p directory. It is assured that the directory
     * contains a trailing slash.
     */
    virtual bool beginRetrieval(const QString& directory) = 0;

    /**
     * Is invoked after the version control information has been
     * received. It is assured that
     * KVersionControlPlugin::beginInfoRetrieval() has been
     * invoked before.
     */
    virtual void endRetrieval() = 0;

    /**
     * Returns the version state for the file \p item.
     * It is assured that KVersionControlPlugin::beginInfoRetrieval() has been
     * invoked before and that the file is part of the directory specified
     * in beginInfoRetrieval().
     */
    virtual VersionState versionState(const KFileItem& item) = 0;
    
    /**
     * Returns the list of actions that should be shown in the context menu
     * for the files \p items. It is assured that the passed list is not empty.
     * If an action triggers a change of the versions, the signal
     * KVersionControlPlugin::versionStatesChanged() must be emitted.
     */
    virtual QList<QAction*> contextMenuActions(const KFileItemList& items) = 0;

    /**
     * Returns the list of actions that should be shown in the context menu
     * for the directory \p directory. If an action triggers a change of the versions,
     * the signal KVersionControlPlugin::versionStatesChanged() must be emitted.
     */
    virtual QList<QAction*> contextMenuActions(const QString& directory) = 0;

signals:
    /**
     * Should be emitted when the version state of files might have been changed
     * after the last retrieval (e. g. by executing a context menu action
     * of the version control plugin). The file manager will be triggered to
     * update the version states of the directory \p directory by invoking
     * KVersionControlPlugin::beginRetrieval(),
     * KVersionControlPlugin::versionState() and
     * KVersionControlPlugin::endRetrieval().
     */
    void versionStatesChanged();

    /**
     * Is emitted if an information message with the content \a msg
     * should be shown.
     */
    void infoMessage(const QString& msg);

    /**
     * Is emitted if an error message with the content \a msg
     * should be shown.
     */
    void errorMessage(const QString& msg);

    /**
     * Is emitted if an "operation completed" message with the content \a msg
     * should be shown.
     */
    void operationCompletedMessage(const QString& msg);
};




// TODO: This is just a temporary test class. It will be made available as
// plugin outside Dolphin later.

#include <kfileitem.h>
#include <QHash>
#include <QTemporaryFile>

class LIBDOLPHINPRIVATE_EXPORT SubversionPlugin : public KVersionControlPlugin
{
    Q_OBJECT

public:
    SubversionPlugin();
    virtual ~SubversionPlugin();
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

    void slotOperationCompleted();
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
#endif // REVISIONCONTROLPLUGIN_H

