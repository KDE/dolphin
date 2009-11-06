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

#include "updateitemstatesthread.h"

UpdateItemStatesThread::UpdateItemStatesThread() :
    QThread(),
    m_retrievedItems(false),  
    m_mutex(0),
    m_itemStates()
{
    static QMutex globalMutex;
    m_mutex = &globalMutex;
}

UpdateItemStatesThread::~UpdateItemStatesThread()
{
}

void UpdateItemStatesThread::setData(KVersionControlPlugin* plugin,
                                     const QList<VersionControlObserver::ItemState>& itemStates)
{
    m_plugin = plugin;
    m_itemStates = itemStates;
}

void UpdateItemStatesThread::run()
{
    Q_ASSERT(!m_itemStates.isEmpty());
    Q_ASSERT(m_plugin != 0);

    // The items from m_itemStates may be located in different directory levels. The version
    // plugin requires the root directory for KVersionControlPlugin::beginRetrieval(). Instead
    // of doing an expensive search, we utilize the knowledge of the implementation of
    // VersionControlObserver::addDirectory() to be sure that the last item contains the root.
    const QString directory = m_itemStates.last().item.url().directory(KUrl::AppendTrailingSlash);

    QMutexLocker locker(m_mutex);
    m_retrievedItems = false;
    if (m_plugin->beginRetrieval(directory)) {
        const int count = m_itemStates.count();
        for (int i = 0; i < count; ++i) {
            m_itemStates[i].version = m_plugin->versionState(m_itemStates[i].item);
        }
        m_plugin->endRetrieval();
        m_retrievedItems = true;
    }
}

bool UpdateItemStatesThread::beginReadItemStates()
{
    return m_mutex->tryLock(300);
}

void UpdateItemStatesThread::endReadItemStates()
{
    m_mutex->unlock();
}

QList<VersionControlObserver::ItemState> UpdateItemStatesThread::itemStates() const
{
    return m_itemStates;
}

bool UpdateItemStatesThread::retrievedItems() const
{
    return m_retrievedItems;
}

void UpdateItemStatesThread::deleteWhenFinished()
{
    connect(this, SIGNAL(finished()), this, SLOT(slotFinished()));
}

void UpdateItemStatesThread::slotFinished()
{
    deleteLater();
}

#include "updateitemstatesthread.moc"
