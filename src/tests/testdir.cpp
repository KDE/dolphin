/*
 *   SPDX-FileCopyrightText: 2010-2011 Frank Reininghaus <frank78ac@googlemail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "testdir.h"

#ifdef Q_OS_UNIX
#include <utime.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

TestDir::TestDir(const QString &directoryPrefix)
    : QTemporaryDir(directoryPrefix)
{
}

TestDir::~TestDir() = default;

QUrl TestDir::url() const
{
    return QUrl::fromLocalFile(path());
}

/** The following function is taken from kdelibs/kio/tests/kiotesthelper.h, copyright (C) 2006 by David Faure */
static void setTimeStamp(const QString &path, const QDateTime &mtime)
{
#ifdef Q_OS_UNIX
    struct utimbuf utbuf;
    utbuf.actime = mtime.toSecsSinceEpoch();
    utbuf.modtime = utbuf.actime;
    utime(QFile::encodeName(path), &utbuf);
#elif defined(Q_OS_WIN)
    HANDLE h = CreateFileW(reinterpret_cast<const wchar_t *>(path.utf16()),
                           FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, // required to open directories
                           nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        // Convert a Unix-epoch time in milliseconds to a Windows FILETIME, which
        // counts 100-nanosecond intervals since 1601-01-01.
        // 11644473600000: milliseconds between 1601-01-01 and the Unix epoch (1970-01-01).
        // 10000: 100-nanosecond intervals per millisecond.
        qint64 fileTime = (mtime.toMSecsSinceEpoch() + 11644473600000LL) * 10000LL;
        FILETIME ft;
        ft.dwLowDateTime = static_cast<DWORD>(fileTime & 0xFFFFFFFF);
        ft.dwHighDateTime = static_cast<DWORD>((fileTime >> 32) & 0xFFFFFFFF);
        SetFileTime(h, nullptr, nullptr, &ft);
        CloseHandle(h);
    }
#endif
}

void TestDir::createFile(const QString &path, const QByteArray &data, const QDateTime &time)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);

    QFile f(absolutePath);
    if (!f.open(QIODevice::WriteOnly)) {
        qFatal() << "could not open" << absolutePath;
    }
    f.write(data);
    f.close();

    if (time.isValid()) {
        setTimeStamp(absolutePath, time);
    }

    Q_ASSERT(QFile::exists(absolutePath));
}

void TestDir::createFiles(const QStringList &files)
{
    for (const QString &path : files) {
        createFile(path);
    }
}

void TestDir::createDir(const QString &path, const QDateTime &time)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);
    QDir(TestDir::path()).mkdir(absolutePath);

    if (time.isValid()) {
        setTimeStamp(absolutePath, time);
    }

    Q_ASSERT(QFile::exists(absolutePath));
}

void TestDir::removeFiles(const QStringList &files)
{
    for (const QString &path : files) {
        removeFile(path);
    }
}

void TestDir::removeFile(const QString &path)
{
    QString absolutePath = path;
    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.isAbsolute()) {
        absolutePath = TestDir::path() + QLatin1Char('/') + path;
    }
    QFile::remove(absolutePath);
}

void TestDir::removeDir(const QString &path)
{
    QString absolutePath = path;
    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.isAbsolute()) {
        absolutePath = TestDir::path() + QLatin1Char('/') + path;
    }
    QDir dirToRemove = QDir(absolutePath);
    dirToRemove.removeRecursively();
}

void TestDir::makePathAbsoluteAndCreateParents(QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isAbsolute()) {
        path = TestDir::path() + QLatin1Char('/') + path;
        fileInfo.setFile(path);
    }

    const QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        createDir(dir.absolutePath());
    }

    Q_ASSERT(dir.exists());
}
