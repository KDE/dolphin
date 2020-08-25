/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UPDATEITEMSTATESTHREAD_H
#define UPDATEITEMSTATESTHREAD_H

#include "dolphin_export.h"
#include "views/versioncontrol/versioncontrolobserver.h"

#include <QMutex>
#include <QThread>

/**
 * The performance of updating the version state of items depends
 * on the used plugin. To prevent that Dolphin gets blocked by a
 * slow plugin, the updating is delegated to a thread.
 */
class DOLPHIN_EXPORT UpdateItemStatesThread : public QThread
{
    Q_OBJECT

public:
    /**
     * @param plugin     Version control plugin that is used to update the
     *                   state of the items. Whenever the plugin is accessed
     *                   from the thread creator after starting the thread,
     *                   UpdateItemStatesThread::lockPlugin() and
     *                   UpdateItemStatesThread::unlockPlugin() must be used.
     * @param itemStates List of items, where the states get updated.
     */
    UpdateItemStatesThread(KVersionControlPlugin* plugin,
                           const QMap<QString, QVector<VersionControlObserver::ItemState> >& itemStates);
    ~UpdateItemStatesThread() override;

    QMap<QString, QVector<VersionControlObserver::ItemState> > itemStates() const;

protected:
    void run() override;

private:
    QMutex* m_globalPluginMutex; // Protects the m_plugin globally
    KVersionControlPlugin* m_plugin;

    QMap<QString, QVector<VersionControlObserver::ItemState> > m_itemStates;
};

#endif // UPDATEITEMSTATESTHREAD_H
