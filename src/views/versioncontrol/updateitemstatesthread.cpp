/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "updateitemstatesthread.h"

UpdateItemStatesThread::UpdateItemStatesThread(KVersionControlPlugin *plugin, const QMap<QString, QVector<VersionControlObserver::ItemState>> &itemStates)
    : QThread()
    , m_globalPluginMutex(nullptr)
    , m_plugin(plugin)
    , m_itemStates(itemStates)
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
    if (!m_plugin) {
        return;
    }

    QMutexLocker pluginLocker(m_globalPluginMutex);
    QMap<QString, QVector<VersionControlObserver::ItemState>>::iterator it = m_itemStates.begin();
    for (; it != m_itemStates.end() && m_plugin; ++it) {
        if (m_plugin->beginRetrieval(it.key())) {
            QVector<VersionControlObserver::ItemState> &items = it.value();
            const int count = items.count();
            for (int i = 0; i < count && m_plugin; ++i) {
                const KFileItem &item = items.at(i).first;
                const KVersionControlPlugin::ItemVersion version = m_plugin->itemVersion(item);
                items[i].second = version;
            }
        }

        if (m_plugin) {
            m_plugin->endRetrieval();
        }
    }
}

QMap<QString, QVector<VersionControlObserver::ItemState>> UpdateItemStatesThread::itemStates() const
{
    return m_itemStates;
}

#include "moc_updateitemstatesthread.cpp"
