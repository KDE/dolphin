/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef POWERCOPYJOB_H
#define POWERCOPYJOB_H

#include "dolphin_export.h"

#include <KIO/Job>

#include <QDateTime>
#include <QList>
#include <QStringList>
#include <QUrl>

/**
 * @brief Copies many files with several operations in flight.
 *
 * KIO::CopyJob copies strictly one file at a time: the next file starts only
 * once the previous one has come back from the worker process. For a few large
 * files that costs nothing, but for thousands of small ones the device spends
 * most of its time idle between round trips.
 *
 * This job keeps a configurable number of KIO::FileCopyJobs in flight instead.
 * The per-file work is still done by KIO's own worker, so reflinks,
 * copy_file_range, extended attributes, ACLs, timestamps and the .part file
 * that makes an interrupted copy safe all behave exactly as before.
 *
 * The job hands the whole operation back to KIO::copy() before touching
 * anything when it cannot do better: a destination that already exists, too few
 * files to be worth it, or sources that are not local.
 */
class DOLPHIN_EXPORT PowerCopyJob : public KIO::Job
{
    Q_OBJECT

public:
    /*! Copies @p sources into the directory @p destination. */
    static PowerCopyJob *copy(const QList<QUrl> &sources, const QUrl &destination);

    /*! Moves @p sources into the directory @p destination. */
    static PowerCopyJob *move(const QList<QUrl> &sources, const QUrl &destination);

    /*!
     * Whether the settings and these URLs allow the concurrent path at all.
     * Call sites can use this to decide before creating a job.
     */
    static bool canAccelerate(const QList<QUrl> &sources, const QUrl &destination);

    ~PowerCopyJob() override;

Q_SIGNALS:
    /*! Same meaning as KIO::CopyJob::copying, so views can follow along. */
    void copying(KIO::Job *job, const QUrl &from, const QUrl &to);
    void moving(KIO::Job *job, const QUrl &from, const QUrl &to);
    void copyingDone(KIO::Job *job, const QUrl &from, const QUrl &to, const QDateTime &mtime, bool directory, bool renamed);

private:
    enum Operation {
        CopyOperation,
        MoveOperation,
    };

    struct Task {
        QUrl source;
        QUrl destination;
    };

    PowerCopyJob(Operation operation, const QList<QUrl> &sources, const QUrl &destination);

    /*! Walks the sources, builds the task list and decides whether to go ahead. */
    bool buildPlan();

    /*! Runs the whole thing through KIO::copy() instead, forwarding its signals. */
    void runThroughKio();

    void dispatch();
    void startNextTask();

    /*! After a move, the emptied source directories have to go as well. */
    void removeMovedSourceDirectories();
    void slotResult(KJob *job) override;

    const Operation m_operation;
    const QList<QUrl> m_sources;
    const QUrl m_destination;

    QList<Task> m_tasks;
    QList<QUrl> m_directories;
    /*! Source directories walked for a move, to remove once their files left. */
    QStringList m_sourceDirectories;
    int m_nextTask = 0;
    int m_running = 0;
    int m_inFlightLimit = 1;
    qint64 m_totalBytes = 0;
    qint64 m_processedBytes = 0;
    int m_completedTasks = 0;
    bool m_failed = false;
    bool m_delegatedToKio = false;
};

#endif
