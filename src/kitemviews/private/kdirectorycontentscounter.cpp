/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kdirectorycontentscounter.h"
#include "dolphin_contentdisplaysettings.h"
#include "kitemviews/kfileitemmodel.h"

#include <KDirWatch>

#include <QDir>
#include <QFileInfo>
#include <QThread>

namespace
{

class LocalCache
{
public:
    struct cacheData {
        int count = 0;
        long long size = 0;
        ushort refCount = 0;
        qint64 timestamp = 0;

        inline operator bool() const
        {
            return timestamp != 0 && count != -1;
        }
    };

    LocalCache()
        : m_cache()
    {
    }

    cacheData insert(const QString &key, cacheData data, bool inserted)
    {
        data.timestamp = QDateTime::currentMSecsSinceEpoch();
        if (inserted) {
            data.refCount += 1;
        }
        m_cache.insert(key, data);
        return data;
    }

    cacheData value(const QString &key) const
    {
        return m_cache.value(key);
    }

    void unRefAll(const QSet<QString> &keys)
    {
        for (const auto &key : keys) {
            auto entry = m_cache[key];
            entry.refCount -= 1;
            if (entry.refCount == 0) {
                m_cache.remove(key);
            }
        }
    }

    void removeAll(const QSet<QString> &keys)
    {
        for (const auto &key : keys) {
            m_cache.remove(key);
        }
    }

private:
    QHash<QString, cacheData> m_cache;
};

/// cache of directory counting result
static LocalCache *s_cache;
static QThread *s_workerThread;
static KDirectoryContentsCounterWorker *s_worker;
}

KDirectoryContentsCounter::KDirectoryContentsCounter(KFileItemModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_priorityQueue()
    , m_queue()
    , m_workerIsBusy(false)
    , m_dirWatcher(nullptr)
    , m_watchedDirs()
{
    if (s_cache == nullptr) {
        s_cache = new LocalCache();
    }

    if (!s_workerThread) {
        s_workerThread = new QThread();
        s_workerThread->setObjectName(QStringLiteral("KDirectoryContentsCounterThread"));
        s_workerThread->start();
    }

    if (!s_worker) {
        s_worker = new KDirectoryContentsCounterWorker();
        s_worker->moveToThread(s_workerThread);
    }

    connect(m_model, &KFileItemModel::itemsRemoved, this, &KDirectoryContentsCounter::slotItemsRemoved);
    connect(m_model, &KFileItemModel::directoryRefreshing, this, &KDirectoryContentsCounter::slotDirectoryRefreshing);

    connect(this, &KDirectoryContentsCounter::requestDirectoryContentsCount, s_worker, &KDirectoryContentsCounterWorker::countDirectoryContents);

    connect(s_worker, &KDirectoryContentsCounterWorker::result, this, &KDirectoryContentsCounter::slotResult);
    connect(s_worker, &KDirectoryContentsCounterWorker::intermediateResult, this, &KDirectoryContentsCounter::result);
    connect(s_worker, &KDirectoryContentsCounterWorker::finished, this, &KDirectoryContentsCounter::scheduleNext);

    m_dirWatcher = new KDirWatch(this);
    connect(m_dirWatcher, &KDirWatch::dirty, this, &KDirectoryContentsCounter::slotDirWatchDirty);
}

KDirectoryContentsCounter::~KDirectoryContentsCounter()
{
    s_cache->unRefAll(m_watchedDirs);
}

void KDirectoryContentsCounter::slotResult(const QString &path, int count, long long size)
{
    const auto fileInfo = QFileInfo(path);
    const QString resolvedPath = fileInfo.canonicalFilePath();
    if (fileInfo.isReadable() && !m_watchedDirs.contains(resolvedPath)) {
        m_dirWatcher->addDir(resolvedPath);
    }
    bool inserted = m_watchedDirs.insert(resolvedPath) == m_watchedDirs.end();

    // update cache or overwrite value
    s_cache->insert(resolvedPath, {count, size, true}, inserted);

    // sends the results
    Q_EMIT result(path, count, size);
}

void KDirectoryContentsCounter::slotDirWatchDirty(const QString &path)
{
    const int index = m_model->index(QUrl::fromLocalFile(path));
    if (index >= 0) {
        if (!m_model->fileItem(index).isDir()) {
            // If INotify is used, KDirWatch issues the dirty() signal
            // also for changed files inside the directory, even if we
            // don't enable this behavior explicitly (see bug 309740).
            return;
        }

        scanDirectory(path, PathCountPriority::High);
    }
}

void KDirectoryContentsCounter::slotItemsRemoved()
{
    const bool allItemsRemoved = (m_model->count() == 0);

    if (allItemsRemoved) {
        s_cache->removeAll(m_watchedDirs);
        stopWorker();
    }

    if (!m_watchedDirs.isEmpty()) {
        // Don't let KDirWatch watch for removed items
        if (allItemsRemoved) {
            for (const QString &path : qAsConst(m_watchedDirs)) {
                m_dirWatcher->removeDir(path);
            }
            m_watchedDirs.clear();
        } else {
            QMutableSetIterator<QString> it(m_watchedDirs);
            while (it.hasNext()) {
                const QString &path = it.next();
                if (m_model->index(QUrl::fromLocalFile(path)) < 0) {
                    m_dirWatcher->removeDir(path);
                    it.remove();
                }
            }
        }
    }
}

void KDirectoryContentsCounter::slotDirectoryRefreshing()
{
    s_cache->removeAll(m_watchedDirs);
}

void KDirectoryContentsCounter::scheduleNext()
{
    if (!m_priorityQueue.empty()) {
        m_currentPath = m_priorityQueue.front();
        m_priorityQueue.pop_front();
    } else if (!m_queue.empty()) {
        m_currentPath = m_queue.front();
        m_queue.pop_front();
    } else {
        m_currentPath.clear();
        m_workerIsBusy = false;
        return;
    }

    const auto fileInfo = QFileInfo(m_currentPath);
    const QString resolvedPath = fileInfo.canonicalFilePath();
    const auto pair = s_cache->value(resolvedPath);
    if (pair) {
        // fast path when in cache
        // will be updated later if result has changed
        Q_EMIT result(m_currentPath, pair.count, pair.size);
    }

    // if scanned fully recently, skip rescan
    if (pair && pair.timestamp >= fileInfo.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch()) {
        scheduleNext();
        return;
    }

    KDirectoryContentsCounterWorker::Options options;

    if (m_model->showHiddenFiles()) {
        options |= KDirectoryContentsCounterWorker::CountHiddenFiles;
    }

    m_workerIsBusy = true;
    Q_EMIT requestDirectoryContentsCount(m_currentPath, options, ContentDisplaySettings::recursiveDirectorySizeLimit());
}

void KDirectoryContentsCounter::enqueuePathScanning(const QString &path, bool alreadyInCache, PathCountPriority priority)
{
    // ensure to update the entry in the queue
    auto it = std::find(m_queue.begin(), m_queue.end(), path);
    if (it != m_queue.end()) {
        m_queue.erase(it);
    } else {
        it = std::find(m_priorityQueue.begin(), m_priorityQueue.end(), path);
        if (it != m_priorityQueue.end()) {
            m_priorityQueue.erase(it);
        }
    }

    if (priority == PathCountPriority::Normal) {
        if (alreadyInCache) {
            // we already knew the dir size
            // otherwise it gets lower priority
            m_queue.push_back(path);
        } else {
            m_queue.push_front(path);
        }
    } else {
        // append to priority queue
        m_priorityQueue.push_front(path);
    }
}

void KDirectoryContentsCounter::scanDirectory(const QString &path, PathCountPriority priority)
{
    if (m_workerIsBusy && m_currentPath == path) {
        // already listing
        return;
    }

    const auto fileInfo = QFileInfo(path);
    const QString resolvedPath = fileInfo.canonicalFilePath();
    const auto pair = s_cache->value(resolvedPath);
    if (pair) {
        // fast path when in cache
        // will be updated later if result has changed
        Q_EMIT result(path, pair.count, pair.size);
    }

    // if scanned fully recently, skip rescan
    if (pair && pair.timestamp >= fileInfo.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch()) {
        return;
    }

    enqueuePathScanning(path, pair, priority);

    if (!m_workerIsBusy && !s_worker->stopping()) {
        scheduleNext();
    }
}

void KDirectoryContentsCounter::stopWorker()
{
    m_queue.clear();
    m_priorityQueue.clear();

    if (m_workerIsBusy && m_currentPath == s_worker->scannedPath()) {
        s_worker->stop();
    }
    m_currentPath.clear();
}
