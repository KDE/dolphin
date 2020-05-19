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

#include "kdirectorycontentscounter.h"
#include "kitemviews/kfileitemmodel.h"

#include <KDirWatch>

#include <QFileInfo>
#include <QDir>
#include <QThread>

namespace  {
    /// cache of directory counting result
    static QHash<QString, QPair<int, long>> *s_cache;
}

KDirectoryContentsCounter::KDirectoryContentsCounter(KFileItemModel* model, QObject* parent) :
    QObject(parent),
    m_model(model),
    m_queue(),
    m_worker(nullptr),
    m_workerIsBusy(false),
    m_dirWatcher(nullptr),
    m_watchedDirs()
{
    connect(m_model, &KFileItemModel::itemsRemoved,
            this,    &KDirectoryContentsCounter::slotItemsRemoved);

    if (!m_workerThread) {
        m_workerThread = new QThread();
        m_workerThread->start();
    }

    if (s_cache == nullptr) {
        s_cache = new QHash<QString, QPair<int, long>>();
    }

    m_worker = new KDirectoryContentsCounterWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this,     &KDirectoryContentsCounter::requestDirectoryContentsCount,
            m_worker, &KDirectoryContentsCounterWorker::countDirectoryContents);
    connect(m_worker, &KDirectoryContentsCounterWorker::result,
            this,     &KDirectoryContentsCounter::slotResult);

    m_dirWatcher = new KDirWatch(this);
    connect(m_dirWatcher, &KDirWatch::dirty, this, &KDirectoryContentsCounter::slotDirWatchDirty);
}

KDirectoryContentsCounter::~KDirectoryContentsCounter()
{
    if (m_workerThread->isRunning()) {
        // The worker thread will continue running. It could even be running
        // a method of m_worker at the moment, so we delete it using
        // deleteLater() to prevent a crash.
        m_worker->deleteLater();
    } else {
        // There are no remaining workers -> stop the worker thread.
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;

        // The worker thread has finished running now, so it's safe to delete
        // m_worker. deleteLater() would not work at all because the event loop
        // which would deliver the event to m_worker is not running any more.
        delete m_worker;
    }
}

void KDirectoryContentsCounter::scanDirectory(const QString& path)
{
    startWorker(path);
}

void KDirectoryContentsCounter::slotResult(const QString& path, int count, long size)
{
    m_workerIsBusy = false;

    const QFileInfo info = QFileInfo(path);
    const QString resolvedPath = info.canonicalFilePath();

    if (!m_dirWatcher->contains(resolvedPath)) {
        m_dirWatcher->addDir(resolvedPath);
        m_watchedDirs.insert(resolvedPath);
    }

    if (!m_priorityQueue.isEmpty()) {
        startWorker(m_priorityQueue.takeLast());
    } else if (!m_queue.isEmpty()) {
        startWorker(m_queue.takeFirst());
    }

    if (s_cache->contains(resolvedPath)) {
        const auto pair = s_cache->value(resolvedPath);
        if (pair.first == count && pair.second == size) {
            // no change no need to send another result event
            return;
        }
    }

    if (info.dir().path() == m_model->rootItem().url().path()) {
        // update cache or overwrite value
        // when path is a direct children of the current model root
        s_cache->insert(resolvedPath, QPair<int, long>(count, size));
    }

    // sends the results
    emit result(resolvedPath, count, size);
}

void KDirectoryContentsCounter::slotDirWatchDirty(const QString& path)
{
    const int index = m_model->index(QUrl::fromLocalFile(path));
    if (index >= 0) {
        if (!m_model->fileItem(index).isDir()) {
            // If INotify is used, KDirWatch issues the dirty() signal
            // also for changed files inside the directory, even if we
            // don't enable this behavior explicitly (see bug 309740).
            return;
        }

        startWorker(path);
    }
}

void KDirectoryContentsCounter::slotItemsRemoved()
{
    const bool allItemsRemoved = (m_model->count() == 0);

    if (!m_watchedDirs.isEmpty()) {
        // Don't let KDirWatch watch for removed items
        if (allItemsRemoved) {
            for (const QString& path : qAsConst(m_watchedDirs)) {
                m_dirWatcher->removeDir(path);
            }
            m_watchedDirs.clear();
            m_queue.clear();
        } else {
            QMutableSetIterator<QString> it(m_watchedDirs);
            while (it.hasNext()) {
                const QString& path = it.next();
                if (m_model->index(QUrl::fromLocalFile(path)) < 0) {
                    m_dirWatcher->removeDir(path);
                    it.remove();
                }
            }
        }
    }
}

void KDirectoryContentsCounter::startWorker(const QString& path)
{
    bool alreadyInCache = s_cache->contains(path);
    if (alreadyInCache) {
        // fast path when in cache
        // will be updated later if result has changed
        const auto pair = s_cache->value(path);
        emit result(path, pair.first, pair.second);
    }

    if (m_workerIsBusy) {
        if (!m_queue.contains(path) && !m_priorityQueue.contains(path)) {
            if (alreadyInCache) {
                m_queue.append(path);
            } else {
                // append to priority queue
                m_priorityQueue.append(path);
            }
        }
    } else {
        KDirectoryContentsCounterWorker::Options options;

        if (m_model->showHiddenFiles()) {
            options |= KDirectoryContentsCounterWorker::CountHiddenFiles;
        }

        if (m_model->showDirectoriesOnly()) {
            options |= KDirectoryContentsCounterWorker::CountDirectoriesOnly;
        }

        emit requestDirectoryContentsCount(path, options);
        m_workerIsBusy = true;
    }
}

QThread* KDirectoryContentsCounter::m_workerThread = nullptr;
