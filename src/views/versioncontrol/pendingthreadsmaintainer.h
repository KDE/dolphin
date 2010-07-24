/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef PENDINGTHREADSMAINTAINER_H
#define PENDINGTHREADSMAINTAINER_H

#include <libdolphin_export.h>

#include <QObject>

class QTimer;

/**
 * If the creator of a thread gets deleted, although the thread is still
 * working, usually QThread::wait() is invoked. The drawback of this
 * approach is that the user interface gets blocked for an undefined amount
 * of time. If the thread does not contain references to the creator, the
 * deleting can be forwarded to the PendingThreadsMaintainer. In the following
 * example it is assumed, that m_thread will be 0, if it has been deleted by the
 * creator after receiving the signal QThread::finished():
 *
 * \code
 * ThreadCreator::~ThreadCreator()
 * {
 *     if (m_thread != 0) {
 *         PendingThreadsMaintainer::instance().append(m_thread);
 *         m_thread = 0;
 *     }
 * }
 * \endcode
 *
 * The thread will get automatically deleted after it (or has already) been finished.
 *
 * Implementation note: Connecting to the signal QThread::finished() is
 * not sufficient, as it is possible that the thread has already emitted
 * the signal, but the signal has not been received yet by the thread creator.
 * Because of this a polling is done each 5 seconds to check, whether the
 * thread has been finished.
 */
class LIBDOLPHINPRIVATE_EXPORT PendingThreadsMaintainer : public QObject
{
    Q_OBJECT

public:
    static PendingThreadsMaintainer& instance();
    virtual ~PendingThreadsMaintainer();

    /**
     * Appends the thread \p thread to the maintainer. The thread
     * will be deleted by the maintainer after it has been finished.
     */
    void append(QThread* thread);

protected:
    PendingThreadsMaintainer();

private slots:
    void cleanup();

private:
    QList<QThread*> m_threads;
    QTimer* m_timer;

    friend class PendingThreadsMaintainerSingleton;
};

#endif
