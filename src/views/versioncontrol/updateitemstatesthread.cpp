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
                                               const QMap<QString, QVector<VersionControlObserver::ItemState> >& itemStates) :
    QThread(),
    m_globalPluginMutex(0),
    m_plugin(plugin),
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

    QMutexLocker pluginLocker(m_globalPluginMutex);
    QMap<QString, QVector<VersionControlObserver::ItemState> >::iterator it = m_itemStates.begin();
    for (; it != m_itemStates.end(); ++it) {
        if (m_plugin->beginRetrieval(it.key())) {
            QVector<VersionControlObserver::ItemState>& items = it.value();
            const int count = items.count();

            KVersionControlPlugin2* pluginV2 = qobject_cast<KVersionControlPlugin2*>(m_plugin);
            if (pluginV2) {
                for (int i = 0; i < count; ++i) {
                    items[i].version = pluginV2->itemVersion(items[i].item);
                }
            } else {
                for (int i = 0; i < count; ++i) {
                    const KVersionControlPlugin::VersionState state = m_plugin->versionState(items[i].item);
                    items[i].version = static_cast<KVersionControlPlugin2::ItemVersion>(state);
                }
            }

            m_plugin->endRetrieval();
        }
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

QMap<QString, QVector<VersionControlObserver::ItemState> > UpdateItemStatesThread::itemStates() const
{
    return m_itemStates;
}

#include "updateitemstatesthread.moc"
