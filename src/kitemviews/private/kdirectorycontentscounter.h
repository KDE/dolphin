/*
 *   SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *   SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KDIRECTORYCONTENTSCOUNTER_H
#define KDIRECTORYCONTENTSCOUNTER_H

#include "kdirectorycontentscounterworker.h"

#include <QSet>

class KDirWatch;
class KFileItemModel;
class QString;

class KDirectoryContentsCounter : public QObject
{
    Q_OBJECT

public:
    enum PathCountPriority { Normal, High };

    explicit KDirectoryContentsCounter(KFileItemModel *model, QObject *parent = nullptr);
    ~KDirectoryContentsCounter() override;

    /**
     * Requests the number of items inside the directory \a path. The actual
     * counting is done asynchronously, and the result is announced via the
     * signal \a result.
     *
     * The directory \a path is watched for changes, and the signal is emitted
     * again if a change occurs.
     *
     * Uses a cache internally to speed up first result,
     * but emit again result when the cache was updated
     */
    void scanDirectory(const QString &path, PathCountPriority priority);

    /**
     * Stops the work until new input is passed
     */
    void stopWorker();

Q_SIGNALS:
    /**
     * Signals that the directory \a path contains \a count items of size \a
     * Size calculation depends on parameter DetailsModeSettings::recursiveDirectorySizeLimit
     */
    void result(const QString &path, int count, long size);

    void requestDirectoryContentsCount(const QString &path, KDirectoryContentsCounterWorker::Options options);

    void stop();

private Q_SLOTS:
    void slotResult(const QString &path, int count, long size);
    void slotDirWatchDirty(const QString &path);
    void slotItemsRemoved();

private:
    KFileItemModel *m_model;

    // Used as FIFO queues.
    std::list<QString> m_priorityQueue;
    std::list<QString> m_queue;

    static QThread *m_workerThread;

    KDirectoryContentsCounterWorker *m_worker;
    bool m_workerIsBusy;

    KDirWatch *m_dirWatcher;
    QSet<QString> m_watchedDirs; // Required as sadly KDirWatch does not offer a getter method
                                 // to get all watched directories.
};

#endif
