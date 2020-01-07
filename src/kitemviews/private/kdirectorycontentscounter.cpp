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
#include <QThread>

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

    m_worker = new KDirectoryContentsCounterWorker();
    m_worker->moveToThread(m_workerThread);
    ++m_workersCount;

    connect(this,     &KDirectoryContentsCounter::requestDirectoryContentsCount,
            m_worker, &KDirectoryContentsCounterWorker::countDirectoryContents);
    connect(m_worker, &KDirectoryContentsCounterWorker::result,
            this,     &KDirectoryContentsCounter::slotResult);

    m_dirWatcher = new KDirWatch(this);
    connect(m_dirWatcher, &KDirWatch::dirty, this, &KDirectoryContentsCounter::slotDirWatchDirty);
}

KDirectoryContentsCounter::~KDirectoryContentsCounter()
{
    --m_workersCount;

    if (m_workersCount > 0) {
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

void KDirectoryContentsCounter::addDirectory(const QString& path)
{
    startWorker(path);
}

int KDirectoryContentsCounter::countDirectoryContentsSynchronously(const QString& path)
{
    const QString resolvedPath = QFileInfo(path).canonicalFilePath();

    if (!m_dirWatcher->contains(resolvedPath)) {
        m_dirWatcher->addDir(resolvedPath);
        m_watchedDirs.insert(resolvedPath);
    }

    KDirectoryContentsCounterWorker::Options options;

    if (m_model->showHiddenFiles()) {
        options |= KDirectoryContentsCounterWorker::CountHiddenFiles;
    }

    if (m_model->showDirectoriesOnly()) {
        options |= KDirectoryContentsCounterWorker::CountDirectoriesOnly;
    }

    return KDirectoryContentsCounterWorker::subItemsCount(path, options);
}

void KDirectoryContentsCounter::slotResult(const QString& path, int count)
{
    m_workerIsBusy = false;

    const QString resolvedPath = QFileInfo(path).canonicalFilePath();

    if (!m_dirWatcher->contains(resolvedPath)) {
        m_dirWatcher->addDir(resolvedPath);
        m_watchedDirs.insert(resolvedPath);
    }

    if (!m_queue.isEmpty()) {
        startWorker(m_queue.dequeue());
    }

    emit result(path, count);
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
            foreach (const QString& path, m_watchedDirs) {
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
    if (m_workerIsBusy) {
        m_queue.enqueue(path);
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
int KDirectoryContentsCounter::m_workersCount = 0;
