/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kdirectorycontentscounterworker.h"

// Required includes for countDirectoryContents():
#ifdef Q_OS_WIN
#include <QDir>
#else
#include <QElapsedTimer>
#include <fts.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

KDirectoryContentsCounterWorker::KDirectoryContentsCounterWorker(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<KDirectoryContentsCounterWorker::Options>();
}

#ifndef Q_OS_WIN
void KDirectoryContentsCounterWorker::walkDir(const QString &dirPath, bool countHiddenFiles, uint allowedRecursiveLevel)
{
    QByteArray text = dirPath.toLocal8Bit();
    char *rootPath = new char[text.size() + 1];
    ::strncpy(rootPath, text.constData(), text.size() + 1);
    char *path[2]{rootPath, nullptr};

    // follow symlink only for root dir
    auto tree = ::fts_open(path, FTS_COMFOLLOW | FTS_PHYSICAL | FTS_XDEV, nullptr);
    if (!tree) {
        delete[] rootPath;
        return;
    }

    FTSENT *node;
    long long totalSize = -1;
    int totalCount = -1;
    QElapsedTimer timer;
    timer.start();

    while ((node = fts_read(tree)) && !m_stopping) {
        auto info = node->fts_info;

        if (info == FTS_DC) {
            // ignore directories clausing cycles
            continue;
        }
        if (info == FTS_DNR) {
            // ignore directories that can’t be read
            continue;
        }
        if (info == FTS_ERR) {
            // ignore directories causing errors
            fts_set(tree, node, FTS_SKIP);
            continue;
        }
        if (info == FTS_DP) {
            // ignore end traversal of dir
            continue;
        }

        if (!countHiddenFiles && node->fts_name[0] == '.' && strncmp(".git", node->fts_name, 4) != 0) {
            // skip hidden files, except .git dirs
            if (info == FTS_D) {
                fts_set(tree, node, FTS_SKIP);
            }
            continue;
        }

        if (info == FTS_F) {
            // only count files that are physical (aka skip /proc/kcore...)
            // naive size counting not taking into account effective disk space used (aka size/block_size * block_size)
            // skip directory size (usually a 4KB block)
            if (node->fts_statp->st_blocks > 0) {
                totalSize += node->fts_statp->st_size;
            }
        }

        if (info == FTS_D) {
            if (node->fts_level == 0) {
                // first read was sucessful, we can init counters
                totalSize = 0;
                totalCount = 0;
            }

            if (node->fts_level > (int)allowedRecursiveLevel) {
                // skip too deep nodes
                fts_set(tree, node, FTS_SKIP);
                continue;
            }
        }
        // count first level elements
        if (node->fts_level == 1) {
            ++totalCount;
        }

        // delay intermediate results
        if (timer.hasExpired(200) || node->fts_level == 0) {
            Q_EMIT intermediateResult(dirPath, totalCount, totalSize);
            timer.restart();
        }
    }

    delete[] rootPath;
    fts_close(tree);
    if (errno != 0) {
        return;
    }

    if (!m_stopping) {
        Q_EMIT result(dirPath, totalCount, totalSize);
    }
}
#endif

void KDirectoryContentsCounterWorker::stop()
{
    m_stopping = true;
}

bool KDirectoryContentsCounterWorker::stopping() const
{
    return m_stopping;
}

QString KDirectoryContentsCounterWorker::scannedPath() const
{
    return m_scannedPath;
}

void KDirectoryContentsCounterWorker::countDirectoryContents(const QString &path, Options options, int maxRecursiveLevel)
{
    const bool countHiddenFiles = options & CountHiddenFiles;

#ifdef Q_OS_WIN
    QDir dir(path);
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::System | QDir::AllEntries;
    if (countHiddenFiles) {
        filters |= QDir::Hidden;
    }

    Q_EMIT result(path, static_cast<int>(dir.entryList(filters).count()), 0);
#else

    m_scannedPath = path;
    walkDir(path, countHiddenFiles, maxRecursiveLevel);

#endif

    m_stopping = false;
    Q_EMIT finished();
}
