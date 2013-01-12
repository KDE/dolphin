/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "updateitemstatesthread.h"

#include <kversioncontrolplugin2.h>

#include <QMutexLocker>

UpdateItemStatesThread::UpdateItemStatesThread(KVersionControlPlugin* plugin,
                                     const QList<VersionControlObserver::ItemState>& itemStates) :
    QThread(),
    m_globalPluginMutex(0),
    m_plugin(plugin),
    m_retrievedItems(false),
    m_itemStates(itemStates)
{
    // Several threads may share one instance of a plugin. A global
    // mutex is required to serialize the retrieval of version control
    // states inside run().
    static QMutex globalMutex;
    m_globalPluginMutex = &globalMutex;
}

UpdateItemStatesThread::~UpdateItemStatesThread()
{
}

void UpdateItemStatesThread::run()
{
    Q_ASSERT(!m_itemStates.isEmpty());
    Q_ASSERT(m_plugin);

    const QString directory = m_itemStates.first().item.url().directory(KUrl::AppendTrailingSlash);
    m_retrievedItems = false;

    QMutexLocker pluginLocker(m_globalPluginMutex);
    if (m_plugin->beginRetrieval(directory)) {
        const int count = m_itemStates.count();

        KVersionControlPlugin2* pluginV2 = qobject_cast<KVersionControlPlugin2*>(m_plugin);
        if (pluginV2) {
            for (int i = 0; i < count; ++i) {
                m_itemStates[i].version = pluginV2->itemVersion(m_itemStates[i].item);
            }
        } else {
            for (int i = 0; i < count; ++i) {
                const KVersionControlPlugin::VersionState state = m_plugin->versionState(m_itemStates[i].item);
                m_itemStates[i].version = static_cast<KVersionControlPlugin2::ItemVersion>(state);
            }
        }

        m_plugin->endRetrieval();
        m_retrievedItems = true;
    }
}

bool UpdateItemStatesThread::lockPlugin()
{
    return m_globalPluginMutex->tryLock(300);
}

void UpdateItemStatesThread::unlockPlugin()
{
    m_globalPluginMutex->unlock();
}

QList<VersionControlObserver::ItemState> UpdateItemStatesThread::itemStates() const
{
    return m_itemStates;
}

bool UpdateItemStatesThread::retrievedItems() const
{
    return m_retrievedItems;
}

#include "updateitemstatesthread.moc"
