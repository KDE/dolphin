/*
 *   SPDX-FileCopyrightText: 2010-2011 Frank Reininghaus <frank78ac@googlemail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "testdir.h"

#ifdef Q_OS_UNIX
#include <utime.h>
#else
#include <sys/utime.h>
#endif

TestDir::TestDir(const QString& directoryPrefix) :
    QTemporaryDir(directoryPrefix)
{
}

TestDir::~TestDir()
{
}

QUrl TestDir::url() const
{
    return QUrl::fromLocalFile(path());
}

/** The following function is taken from kdelibs/kio/tests/kiotesthelper.h, copyright (C) 2006 by David Faure */
static void setTimeStamp(const QString& path, const QDateTime& mtime)
{
#ifdef Q_OS_UNIX
    struct utimbuf utbuf;
    utbuf.actime = mtime.toSecsSinceEpoch();
    utbuf.modtime = utbuf.actime;
    utime(QFile::encodeName(path), &utbuf);
#elif defined(Q_OS_WIN)
    struct _utimbuf utbuf;
    utbuf.actime = mtime.toSecsSinceEpoch();
    utbuf.modtime = utbuf.actime;
    _wutime(reinterpret_cast<const wchar_t *>(path.utf16()), &utbuf);
#endif
}

void TestDir::createFile(const QString& path, const QByteArray& data, const QDateTime& time)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);

    QFile f(absolutePath);
    f.open(QIODevice::WriteOnly);
    f.write(data);
    f.close();

    if (time.isValid()) {
        setTimeStamp(absolutePath, time);
    }

    Q_ASSERT(QFile::exists(absolutePath));
}

void TestDir::createFiles(const QStringList& files)
{
    for (const QString& path : files) {
        createFile(path);
    }
}

void TestDir::createDir(const QString& path, const QDateTime& time)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);
    QDir(TestDir::path()).mkdir(absolutePath);

    if (time.isValid()) {
        setTimeStamp(absolutePath, time);
    }

    Q_ASSERT(QFile::exists(absolutePath));
}

void TestDir::removeFiles(const QStringList& files)
{
    for (const QString& path : files) {
        removeFile(path);
    }
}

void TestDir::removeFile(const QString& path)
{
    QString absolutePath = path;
    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.isAbsolute()) {
        absolutePath = TestDir::path() + QLatin1Char('/') + path;
    }
    QFile::remove(absolutePath);
}

void TestDir::removeDir(const QString& path)
{
    QString absolutePath = path;
    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.isAbsolute()) {
        absolutePath = TestDir::path() + QLatin1Char('/') + path;
    }
    QDir dirToRemove = QDir(absolutePath);
    dirToRemove.removeRecursively();
}

void TestDir::makePathAbsoluteAndCreateParents(QString& path)
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
