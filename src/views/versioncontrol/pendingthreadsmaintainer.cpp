/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "pendingthreadsmaintainer.h"

#include <KGlobal>
#include <QThread>
#include <QTimer>

struct PendingThreadsMaintainerSingleton
{
    PendingThreadsMaintainer instance;
};
K_GLOBAL_STATIC(PendingThreadsMaintainerSingleton, s_pendingThreadsMaintainer)


PendingThreadsMaintainer& PendingThreadsMaintainer::instance()
{
    return s_pendingThreadsMaintainer->instance;
}

PendingThreadsMaintainer::~PendingThreadsMaintainer()
{
}

void PendingThreadsMaintainer::append(QThread* thread)
{
    Q_ASSERT(thread != 0);
    m_threads.append(thread);
    m_timer->start();
}

PendingThreadsMaintainer::PendingThreadsMaintainer() :
    QObject(),
    m_threads(),
    m_timer(0)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(5000); // 5 seconds
    connect(m_timer, SIGNAL(timeout()), this, SLOT(cleanup()));
}

void PendingThreadsMaintainer::cleanup()
{
    QList<QThread*>::iterator it = m_threads.begin();
    while (it != m_threads.end()) {
        if ((*it)->isFinished()) {
            (*it)->deleteLater();
            it = m_threads.erase(it);
        } else {
            ++it;
        }
    }

    if (!m_threads.isEmpty()) {
        m_timer->start();
    }
}

#include "pendingthreadsmaintainer.moc"
