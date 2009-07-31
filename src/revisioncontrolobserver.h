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

#ifndef REVISIONCONTROLOBSERVER_H
#define REVISIONCONTROLOBSERVER_H

#include <libdolphin_export.h>

#include <kfileitem.h>
#include <revisioncontrolplugin.h>
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
 * @brief Observes all revision control plugins.
 *
 * The item view gets updated automatically if the currently shown
 * directory is under revision control.
 *
 * @see RevisionControlPlugin
 */
class LIBDOLPHINPRIVATE_EXPORT RevisionControlObserver : public QObject
{
    Q_OBJECT

public:
    RevisionControlObserver(QAbstractItemView* view);
    virtual ~RevisionControlObserver();

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
    void applyUpdatedItemStates();
    
private:
    void updateItemStates();

private:
    struct ItemState
    {
        QPersistentModelIndex index;
        KFileItem item;
        RevisionControlPlugin::RevisionState revision;
    };
    
    bool m_pendingItemStatesUpdate;
    bool m_revisionedDirectory;
    bool m_silentUpdate; // if true, no messages will be send during the update
                         // of revision states
    
    QAbstractItemView* m_view;
    KDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    
    QTimer* m_dirVerificationTimer;
    
    mutable QMutex m_pluginMutex;
    RevisionControlPlugin* m_plugin;
    UpdateItemStatesThread* m_updateItemStatesThread;

    friend class UpdateItemStatesThread;
};

#endif // REVISIONCONTROLOBSERVER_H

