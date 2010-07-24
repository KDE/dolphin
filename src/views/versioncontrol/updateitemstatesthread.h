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

#ifndef UPDATEITEMSTATESTHREAD_H
#define UPDATEITEMSTATESTHREAD_H

#include <libdolphin_export.h>
#include <views/versioncontrol/versioncontrolobserver.h>

#include <QMutex>
#include <QThread>

class KVersionControlPlugin;

/**
 * The performance of updating the version state of items depends
 * on the used plugin. To prevent that Dolphin gets blocked by a
 * slow plugin, the updating is delegated to a thread.
 */
class LIBDOLPHINPRIVATE_EXPORT UpdateItemStatesThread : public QThread
{
    Q_OBJECT

public:
    UpdateItemStatesThread();
    virtual ~UpdateItemStatesThread();

    /**
     * @param plugin     Version control plugin that is used to update the
     *                   state of the items. Whenever the plugin is accessed
     *                   from the thread creator after starting the thread,
     *                   UpdateItemStatesThread::lockPlugin() and
     *                   UpdateItemStatesThread::unlockPlugin() must be used.
     * @param itemStates List of items, where the states get updated.
     */
    void setData(KVersionControlPlugin* plugin,
                 const QList<VersionControlObserver::ItemState>& itemStates);

    /**
     * Whenever the plugin is accessed by the thread creator, lockPlugin() must
     * be invoked. True is returned, if the plugin could be locked within 300
     * milliseconds.
     */
    bool lockPlugin();

    /**
     * Must be invoked if lockPlugin() returned true and plugin has been accessed
     * by the thread creator.
     */
    void unlockPlugin();

    QList<VersionControlObserver::ItemState> itemStates() const;

    bool retrievedItems() const;

protected:
    virtual void run();

private:
    QMutex* m_globalPluginMutex; // Protects the m_plugin globally
    KVersionControlPlugin* m_plugin;

    mutable QMutex m_itemMutex; // Protects m_retrievedItems and m_itemStates
    bool m_retrievedItems;
    QList<VersionControlObserver::ItemState> m_itemStates;
};

#endif // UPDATEITEMSTATESTHREAD_H
