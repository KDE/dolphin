/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#ifndef KDIRECTORYCONTENTSCOUNTER_H
#define KDIRECTORYCONTENTSCOUNTER_H

#include "kdirectorycontentscounterworker.h"

#include <QSet>
#include <QQueue>

class KDirWatch;
class KFileItemModel;
class QString;

class KDirectoryContentsCounter : public QObject
{
    Q_OBJECT

public:
    explicit KDirectoryContentsCounter(KFileItemModel* model, QObject* parent = 0);
    ~KDirectoryContentsCounter();

    /**
     * Requests the number of items inside the directory \a path. The actual
     * counting is done asynchronously, and the result is announced via the
     * signal \a result.
     *
     * The directory \a path is watched for changes, and the signal is emitted
     * again if a change occurs.
     */
    void addDirectory(const QString& path);

    /**
     * In contrast to \a addDirectory, this function counts the items inside
     * the directory \a path synchronously and returns the result.
     *
     * The directory is watched for changes, and the signal \a result is
     * emitted if a change occurs.
     */
    int countDirectoryContentsSynchronously(const QString& path);

signals:
    /**
     * Signals that the directory \a path contains \a count items.
     */
    void result(const QString& path, int count);

    void requestDirectoryContentsCount(const QString& path, KDirectoryContentsCounterWorker::Options options);

private slots:
    void slotResult(const QString& path, int count);
    void slotDirWatchDirty(const QString& path);
    void slotItemsRemoved();

private:
    void startWorker(const QString& path);

private:
    KFileItemModel* m_model;

    QQueue<QString> m_queue;

    QThread* m_workerThread;
    KDirectoryContentsCounterWorker* m_worker;
    bool m_workerIsBusy;

    KDirWatch* m_dirWatcher;
    QSet<QString> m_watchedDirs;    // Required as sadly KDirWatch does not offer a getter method
                                    // to get all watched directories.
};

#endif