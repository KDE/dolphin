/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kdirectorycontentscounterworker.h"

// Required includes for countDirectoryContents():
#if defined(Q_OS_WIN) || defined(Q_OS_HAIKU)
#include <QDir>
#else
#include <QElapsedTimer>
#include <QFile>
#include <QVarLengthArray>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#endif

KDirectoryContentsCounterWorker::KDirectoryContentsCounterWorker(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<KDirectoryContentsCounterWorker::Options>();
}

#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)
namespace
{
// Walks a folder with openat() and fstatat() relative to the parent folder, so the kernel resolves
// one name per entry and the process working directory stays the same. When the file system fills
// d_type, only regular files get a stat call, and hidden entries are skipped by name first.
class SizeWalker
{
public:
    SizeWalker(KDirectoryContentsCounterWorker *worker, const QString &rootPath, const std::atomic<bool> &stopping, bool countHiddenFiles, uint maxLevel)
        : m_worker(worker)
        , m_rootPath(rootPath)
        , m_stopping(stopping)
        , m_countHiddenFiles(countHiddenFiles)
        , m_maxLevel(maxLevel)
    {
        m_timer.start();
    }

    // Returns false if the folder is on another file system or is one of its own ancestors (a bind
    // mount loop). dirFd stays open either way.
    bool enter(int dirFd)
    {
        struct stat st;
        if (::fstat(dirFd, &st) != 0) {
            return false;
        }
        if (m_ancestors.isEmpty()) {
            m_rootDevice = st.st_dev;
        } else if (st.st_dev != m_rootDevice) {
            return false;
        }
        if (m_ancestors.contains(st.st_ino)) {
            return false;
        }
        m_ancestors.append(st.st_ino);
        return true;
    }

    // dirFd is the folder at depth level, the root being 0. Closes dirFd.
    void walk(int dirFd, uint level)
    {
        DIR *dir = ::fdopendir(dirFd);
        if (!dir) {
            ::close(dirFd);
            incomplete = true;
            return;
        }

        while (!m_stopping) {
            // readdir() returns nullptr at the end of the folder and on an error, and only an
            // error sets errno.
            errno = 0;
            const dirent *entry = ::readdir(dir);
            if (!entry) {
                if (errno != 0) {
                    incomplete = true;
                }
                break;
            }
            const char *name = entry->d_name;
            if (name[0] == '.') {
                if (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')) {
                    continue;
                }
                // skip hidden files, except .git dirs
                if (!m_countHiddenFiles && ::strncmp(".git", name, 4) != 0) {
                    continue;
                }
            }

            // count first level elements
            if (level == 0) {
                ++totalCount;
            }

#ifdef DT_UNKNOWN
            bool isDir = entry->d_type == DT_DIR;
            const bool needsStat = entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN;
#else
            bool isDir = false;
            const bool needsStat = true;
#endif
            if (needsStat) {
                struct stat st;
                if (::fstatat(::dirfd(dir), name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
                    continue;
                }
                if (S_ISREG(st.st_mode)) {
                    // only count files that are physical (aka skip /proc/kcore...)
                    // naive size counting not taking into account effective disk space used (aka size/block_size * block_size)
                    if (st.st_blocks > 0) {
                        totalSize += st.st_size;
                    }
                } else {
                    isDir = S_ISDIR(st.st_mode);
                }
            }

            // skip too deep nodes, and directory size (usually a 4KB block)
            if (isDir && level < m_maxLevel) {
                const int childFd = ::openat(::dirfd(dir), name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
                if (childFd >= 0) {
                    if (enter(childFd)) {
                        walk(childFd, level + 1);
                        m_ancestors.removeLast();
                    } else {
                        ::close(childFd);
                    }
                } else if (errno == EMFILE || errno == ENFILE || errno == ENOMEM) {
                    // An unreadable subfolder counts as empty, as the root does. Running out of
                    // file descriptors or memory leaves the totals incomplete.
                    incomplete = true;
                }
            }

            // delay intermediate results
            if (m_timer.hasExpired(200)) {
                Q_EMIT m_worker->intermediateResult(m_rootPath, totalCount, totalSize);
                m_timer.restart();
            }
        }

        ::closedir(dir);
    }

    long long totalSize = 0;
    int totalCount = 0;
    // true when a read failed, so the totals miss part of the tree
    bool incomplete = false;

private:
    KDirectoryContentsCounterWorker *m_worker;
    const QString &m_rootPath;
    const std::atomic<bool> &m_stopping;
    const bool m_countHiddenFiles;
    const uint m_maxLevel;
    dev_t m_rootDevice = 0;
    QVarLengthArray<ino_t, 16> m_ancestors;
    QElapsedTimer m_timer;
};
}

void KDirectoryContentsCounterWorker::walkDir(const QString &dirPath, bool countHiddenFiles, uint allowedRecursiveLevel)
{
    // follow symlink only for root dir
    const QByteArray encodedPath = QFile::encodeName(dirPath);
    const int rootFd = ::open(encodedPath.constData(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (rootFd < 0) {
        const int openError = errno;
        // a folder that cannot be listed counts as empty, a missing one as unknown
        if (openError == EACCES) {
            Q_EMIT result(dirPath, 0, 0);
        } else {
            Q_EMIT result(dirPath, -1, -1);
        }
        return;
    }

    SizeWalker walker(this, dirPath, m_stopping, countHiddenFiles, allowedRecursiveLevel);
    if (!walker.enter(rootFd)) {
        ::close(rootFd);
        Q_EMIT result(dirPath, -1, -1);
        return;
    }
    Q_EMIT intermediateResult(dirPath, 0, 0);
    walker.walk(rootFd, 0);

    if (!m_stopping && !walker.incomplete) {
        Q_EMIT result(dirPath, walker.totalCount, walker.totalSize);
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

#if defined(Q_OS_WIN) || defined(Q_OS_HAIKU)
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

#include "moc_kdirectorycontentscounterworker.cpp"
