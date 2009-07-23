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
 * @brief Base class for revision control plugins.
 *
 * Enables the file manager to show the revision state
 * of a revisioned file. The methods
 * RevisionControlPlugin::beginRetrieval(),
 * RevisionControlPlugin::endRetrieval() and
 * RevisionControlPlugin::revisionState() are invoked
 * from a separate thread to assure that the GUI thread
 * won't be blocked. All other methods are invoked in the
 * scope of the GUI thread.
 */
class LIBDOLPHINPRIVATE_EXPORT RevisionControlPlugin : public QObject
{
    Q_OBJECT

public:
    enum RevisionState
    {
        LocalRevision,
        LatestRevision,
        UpdateRequiredRevision,
        EditingRevision,
        ConflictingRevision
        // TODO...
    };

    RevisionControlPlugin();
    virtual ~RevisionControlPlugin();

    /**
     * Returns the name of the file which stores
     * the revision control informations.
     * (e. g. .svn, .cvs, .git).
     */
    virtual QString fileName() const = 0;

    /**
     * Is invoked whenever the revision control
     * information will get retrieved for the directory
     * \p directory. It is assured that the directory
     * contains a trailing slash.
     */
    virtual bool beginRetrieval(const QString& directory) = 0;

    /**
     * Is invoked after the revision control information has been
     * received. It is assured that
     * RevisionControlPlugin::beginInfoRetrieval() has been
     * invoked before.
     */
    virtual void endRetrieval() = 0;

    /**
     * Returns the revision state for the file \p item.
     * It is assured that RevisionControlPlugin::beginInfoRetrieval() has been
     * invoked before and that the file is part of the directory specified
     * in beginInfoRetrieval().
     */
    virtual RevisionState revisionState(const KFileItem& item) = 0;
    
    /**
     * Returns the list of actions that should be shown in the context menu
     * for the files \p items. If no files are provided by \p items, the context
     * menu is valid for the current directory (see RevisionControlPlugin::beginRetrieval()).
     * If an action triggers a change of the revisions, the signal
     * RevisionControlPlugin::revisionStatesChanged() must be emitted.
     */
    virtual QList<QAction*> contextMenuActions(const KFileItemList& items) const = 0;

signals:
    /**
     * Should be emitted when the revision state of files has been changed
     * after the last retrieval. The file manager will be triggered to
     * update the revision states of the directory \p directory by invoking
     * RevisionControlPlugin::beginRetrieval(),
     * RevisionControlPlugin::revisionState() and
     * RevisionControlPlugin::endRetrieval().
     */
    void revisionStatesChanged(const QString& directory);
};




// TODO: This is just a temporary test class. It will be made available as
// plugin outside Dolphin later.

#include <QFileInfoList>
#include <QHash>

class LIBDOLPHINPRIVATE_EXPORT SubversionPlugin : public RevisionControlPlugin
{
public:
    SubversionPlugin();
    virtual ~SubversionPlugin();
    virtual QString fileName() const;
    virtual bool beginRetrieval(const QString& directory);
    virtual void endRetrieval();
    virtual RevisionControlPlugin::RevisionState revisionState(const KFileItem& item);
    virtual QList<QAction*> contextMenuActions(const KFileItemList& items) const;

private:
    /**
     * Returns true, if the content of the local file \p name is equal to the
     * content of the revisioned file.
     */
    bool equalRevisionContent(const QString& name) const;

private:
    struct RevisionInfo
    {
        quint64 size;
        QDateTime timeStamp;
    };

    QString m_directory;
    QHash<QString, RevisionInfo> m_revisionInfoHash;

    QAction* m_updateAction;
    QAction* m_commitAction;
    QAction* m_addAction;
    QAction* m_removeAction;
};
#endif // REVISIONCONTROLPLUGIN_H

