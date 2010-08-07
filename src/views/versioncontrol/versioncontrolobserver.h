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

#ifndef VERSIONCONTROLOBSERVER_H
#define VERSIONCONTROLOBSERVER_H

#include <libdolphin_export.h>

#include <kfileitem.h>
#include <kversioncontrolplugin.h>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QPersistentModelIndex>
#include <QString>

class DolphinModel;
class KDirLister;
class KFileItemList;
class QAbstractItemView;
class QAction;
class QTimer;
class UpdateItemStatesThread;

/**
 * @brief Observes all version control plugins.
 *
 * The item view gets updated automatically if the currently shown
 * directory is under version control.
 *
 * @see VersionControlPlugin
 */
class LIBDOLPHINPRIVATE_EXPORT VersionControlObserver : public QObject
{
    Q_OBJECT

public:
    VersionControlObserver(QAbstractItemView* view);
    virtual ~VersionControlObserver();

    QList<QAction*> contextMenuActions(const KFileItemList& items) const;
    QList<QAction*> contextMenuActions(const QString& directory) const;
    
signals:
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
    
private slots:
    /**
     * Invokes verifyDirectory() with a small delay. If delayedDirectoryVerification()
     * is invoked before the delay has been exceeded, the delay will be reset. This
     * assures that a lot of short requests for directory verification only result
     * in one (expensive) call.
     */
    void delayedDirectoryVerification();

    /**
     * Invokes verifyDirectory() with a small delay. In opposite to
     * delayedDirectoryVerification() it and assures that the verification of
     * the directory is done silently without information messages.
     */
    void silentDirectoryVerification();    

    void verifyDirectory();

    /**
     * Is invoked if the thread m_updateItemStatesThread has been finished
     * and applys the item states.
     */
    void slotThreadFinished();
    
private:
    struct ItemState
    {
        QPersistentModelIndex index;
        KFileItem item;
        KVersionControlPlugin::VersionState version;
    };

    void updateItemStates();

    /**
     * Adds recursively all items from the directory \p parentIndex into
     * the list \p itemStates.
     */
    void addDirectory(const QModelIndex& parentIndex, QList<ItemState>& itemStates);

    /**
     * Returns a matching plugin for the given directory.
     * 0 is returned, if no matching plugin has been found.
     */
    KVersionControlPlugin* searchPlugin(const KUrl& directory) const;

    /**
     * Returns true, if the directory contains a version control information.
     */
    bool isVersioned() const;

private:
    bool m_pendingItemStatesUpdate;
    bool m_versionedDirectory;
    bool m_silentUpdate; // if true, no messages will be send during the update
                         // of version states
    
    QAbstractItemView* m_view;
    KDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    
    QTimer* m_dirVerificationTimer;

    KVersionControlPlugin* m_plugin;
    UpdateItemStatesThread* m_updateItemStatesThread;

    friend class UpdateItemStatesThread;
};

#endif // REVISIONCONTROLOBSERVER_H

