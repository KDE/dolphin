/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "powercopyjob.h"

#include "dolphin_powercopysettings.h"
#include "foldermerge.h"

#include <KIO/CopyJob>
#include <KIO/FileCopyJob>
#include <KJobWidgets>
#include <KLocalizedString>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTimer>

namespace
{
/*! Everything has to be local: a remote worker cannot be driven this way. */
bool allLocal(const QList<QUrl> &urls)
{
    return std::all_of(urls.cbegin(), urls.cend(), [](const QUrl &url) {
        return url.isLocalFile();
    });
}

/*!
 * Whether @p source can simply be written into the @p target already there.
 * Only a real folder over a real folder: a symlink is left to KIO, which
 * knows what it points at.
 */
bool isMergeableFolder(const QFileInfo &source, const QFileInfo &target)
{
    return source.isDir() && target.isDir() && !target.isSymLink();
}
}

PowerCopyJob::PowerCopyJob(Operation operation, const QList<QUrl> &sources, const QUrl &destination)
    : KIO::Job()
    , m_operation(operation)
    , m_sources(sources)
    , m_destination(destination)
{
    m_inFlightLimit = qBound(1, PowerCopySettings::filesInFlight(), 64);

    // KIO autostarts its jobs, so the work begins on the next event loop pass,
    // which gives the caller time to connect to the signals first.
    QTimer::singleShot(0, this, [this]() {
        if (buildPlan()) {
            dispatch();
        } else {
            runThroughKio();
        }
    });
}

PowerCopyJob::~PowerCopyJob() = default;

bool PowerCopyJob::canAccelerate(const QList<QUrl> &sources, const QUrl &destination)
{
    return PowerCopySettings::enabled() && destination.isLocalFile() && allLocal(sources) && !sources.isEmpty();
}

PowerCopyJob *PowerCopyJob::copy(const QList<QUrl> &sources, const QUrl &destination)
{
    return new PowerCopyJob(CopyOperation, sources, destination);
}

PowerCopyJob *PowerCopyJob::move(const QList<QUrl> &sources, const QUrl &destination)
{
    return new PowerCopyJob(MoveOperation, sources, destination);
}

bool PowerCopyJob::buildPlan()
{
    if (!canAccelerate(m_sources, m_destination)) {
        return false;
    }

    const QString destinationPath = m_destination.toLocalFile();
    const QDir destinationDir(destinationPath);

    for (const QUrl &source : m_sources) {
        const QString sourcePath = source.toLocalFile();
        const QFileInfo sourceInfo(sourcePath);
        const QString name = sourceInfo.fileName();
        if (name.isEmpty()) {
            return false;
        }

        const QString target = destinationDir.filePath(name);
        const QFileInfo targetInfo(target);

        if (sourceInfo.isSymLink() || (!sourceInfo.isDir() && !sourceInfo.isFile())) {
            // Symlinks and anything exotic keep KIO's careful handling.
            return false;
        }

        // A folder meeting a folder of the same name is merged: the one at the
        // destination stays and gains what the other one holds. Anything else
        // in the way - a file over a file, a folder over a file - goes to KIO,
        // which owns the overwrite, rename and skip dialogs. Nothing has been
        // touched at this point.
        if (targetInfo.exists() && !isMergeableFolder(sourceInfo, targetInfo)) {
            return false;
        }

        if (sourceInfo.isDir()) {
            if (!targetInfo.exists()) {
                m_directories.append(QUrl::fromLocalFile(target));
            }
            m_sourceDirectories.append(sourcePath);

            QDirIterator iterator(sourcePath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                const QString entry = iterator.next();
                const QFileInfo entryInfo = iterator.fileInfo();
                const QString relative = QDir(sourcePath).relativeFilePath(entry);
                const QString entryTarget = QDir(target).filePath(relative);
                const QFileInfo entryTargetInfo(entryTarget);

                if (entryInfo.isSymLink() || (!entryInfo.isDir() && !entryInfo.isFile())) {
                    return false;
                }
                // The same rule one level down: folders merge, everything else
                // in the way hands the operation over.
                if (entryTargetInfo.exists() && !isMergeableFolder(entryInfo, entryTargetInfo)) {
                    return false;
                }
                if (entryInfo.isDir()) {
                    if (!entryTargetInfo.exists()) {
                        m_directories.append(QUrl::fromLocalFile(entryTarget));
                    }
                    m_sourceDirectories.append(entry);
                } else {
                    m_tasks.append({QUrl::fromLocalFile(entry), QUrl::fromLocalFile(entryTarget)});
                    m_totalBytes += entryInfo.size();
                }
            }
        } else {
            m_tasks.append({source, QUrl::fromLocalFile(target)});
            m_totalBytes += sourceInfo.size();
        }
    }

    // With only a handful of files the batching costs more than it saves.
    if (m_tasks.count() < qMax(2, PowerCopySettings::minimumFiles())) {
        return false;
    }

    return true;
}

void PowerCopyJob::runThroughKio()
{
    m_delegatedToKio = true;

    KIO::CopyJob *job = m_operation == MoveOperation ? KIO::move(m_sources, m_destination, KIO::DefaultFlags) //
                                                     : KIO::copy(m_sources, m_destination, KIO::DefaultFlags);

    // The delegate is owned by its job, so it cannot be handed to a second one.
    // Give up our own progress entry instead and let KIO's job report, or the
    // operation would appear twice in the notification area.
    KJobWidgets::setWindow(job, KJobWidgets::window(this));
    setUiDelegate(nullptr);

    // A folder that meets one of the same name is written into rather than
    // asked about, the same as on the concurrent path above.
    FolderMerge::enableFor(job);

    // Pass everything through, so a caller cannot tell which path was taken.
    connect(job, &KIO::CopyJob::copying, this, [this](KIO::Job *, const QUrl &from, const QUrl &to) {
        Q_EMIT copying(this, from, to);
    });
    connect(job, &KIO::CopyJob::moving, this, [this](KIO::Job *, const QUrl &from, const QUrl &to) {
        Q_EMIT moving(this, from, to);
    });
    connect(job, &KIO::CopyJob::copyingDone, this, [this](KIO::Job *, const QUrl &from, const QUrl &to, const QDateTime &mtime, bool directory, bool renamed) {
        Q_EMIT copyingDone(this, from, to, mtime, directory, renamed);
    });

    addSubjob(job);
}

void PowerCopyJob::dispatch()
{
    // The directories first, parents before children, so the concurrent file
    // copies never race to create the same parent.
    std::sort(m_directories.begin(), m_directories.end(), [](const QUrl &left, const QUrl &right) {
        return left.toLocalFile().length() < right.toLocalFile().length();
    });
    for (const QUrl &directory : std::as_const(m_directories)) {
        if (!QDir().mkpath(directory.toLocalFile())) {
            setError(KIO::ERR_CANNOT_MKDIR);
            setErrorText(directory.toLocalFile());
            emitResult();
            return;
        }
    }

    setTotalAmount(KJob::Files, m_tasks.count());
    setTotalAmount(KJob::Bytes, m_totalBytes);
    Q_EMIT description(this,
                       m_operation == MoveOperation ? i18nc("@info:progress", "Moving") : i18nc("@info:progress", "Copying"),
                       {i18nc("@info:progress", "Source"), m_sources.count() == 1 ? m_sources.first().toLocalFile() : QString::number(m_sources.count())},
                       {i18nc("@info:progress", "Destination"), m_destination.toLocalFile()});

    for (int i = 0; i < m_inFlightLimit; ++i) {
        startNextTask();
    }
}

void PowerCopyJob::removeMovedSourceDirectories()
{
    // Children before parents, so each one is empty by the time it is removed.
    std::sort(m_sourceDirectories.begin(), m_sourceDirectories.end(), [](const QString &left, const QString &right) {
        return left.length() > right.length();
    });

    for (const QString &directory : std::as_const(m_sourceDirectories)) {
        // rmdir, not a recursive delete: if anything unexpected is still in
        // there it stays, rather than being thrown away silently.
        QDir().rmdir(directory);
    }
}

void PowerCopyJob::startNextTask()
{
    if (m_failed || m_nextTask >= m_tasks.count()) {
        if (m_running == 0 && !m_failed) {
            if (m_operation == MoveOperation) {
                removeMovedSourceDirectories();
            }
            emitResult();
        }
        return;
    }

    const Task task = m_tasks.at(m_nextTask++);
    ++m_running;

    // HideProgressInfo: this job reports the operation as a whole, the
    // individual files would each show up as their own entry otherwise.
    KIO::FileCopyJob *job = m_operation == MoveOperation ? KIO::file_move(task.source, task.destination, -1, KIO::HideProgressInfo)
                                                         : KIO::file_copy(task.source, task.destination, -1, KIO::HideProgressInfo);
    job->setProperty("powerCopySource", task.source);
    job->setProperty("powerCopyDestination", task.destination);

    if (m_operation == MoveOperation) {
        Q_EMIT moving(this, task.source, task.destination);
    } else {
        Q_EMIT copying(this, task.source, task.destination);
    }

    addSubjob(job);
}

void PowerCopyJob::slotResult(KJob *job)
{
    if (m_delegatedToKio) {
        // The fallback owns the outcome entirely.
        const int error = job->error();
        const QString errorText = job->errorText();
        removeSubjob(job);
        if (error) {
            setError(error);
            setErrorText(errorText);
        }
        emitResult();
        return;
    }

    const QUrl source = job->property("powerCopySource").toUrl();
    const QUrl destination = job->property("powerCopyDestination").toUrl();
    const int error = job->error();
    const QString errorText = job->errorText();

    removeSubjob(job);
    --m_running;

    if (error) {
        if (!m_failed) {
            m_failed = true;
            setError(error);
            setErrorText(errorText);
        }
        if (m_running == 0) {
            emitResult();
        }
        return;
    }

    ++m_completedTasks;
    m_processedBytes += QFileInfo(destination.toLocalFile()).size();
    setProcessedAmount(KJob::Files, m_completedTasks);
    setProcessedAmount(KJob::Bytes, m_processedBytes);

    Q_EMIT copyingDone(this, source, destination, QDateTime(), false, false);

    startNextTask();
}
