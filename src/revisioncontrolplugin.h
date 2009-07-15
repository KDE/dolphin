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

class KFileItem;

/**
 * @brief Base class for revision control plugins.
 *
 * Enables the file manager to show the revision state
 * of a revisioned file.
 */
class LIBDOLPHINPRIVATE_EXPORT RevisionControlPlugin
{
public:
    enum RevisionState
    {
        LocalRevision,
        LatestRevision,
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

private:
    struct RevisionInfo
    {
        // TODO...
        QDateTime timeStamp;
    };

    QString m_directory;
    QHash<QString, RevisionInfo> m_revisionInfoHash;
};
#endif // REVISIONCONTROLPLUGIN_H

