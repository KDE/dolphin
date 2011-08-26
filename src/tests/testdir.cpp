/*****************************************************************************
 *   Copyright (C) 2010-2011 by Frank Reininghaus (frank78ac@googlemail.com) *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA              *
 *****************************************************************************/

#include "testdir.h"

#include <QDir>

#ifdef Q_OS_UNIX
#include <utime.h>
#else
#include <sys/utime.h>
#endif

TestDir::TestDir()
{
}

TestDir::~TestDir()
{
}

KUrl TestDir::url() const
{
    return KUrl(name());
}

/** The following function is taken from kdelibs/kio/tests/kiotesthelper.h, copyright (C) 2006 by David Faure */
static void setTimeStamp(const QString& path, const QDateTime& mtime)
{
#ifdef Q_OS_UNIX
    struct utimbuf utbuf;
    utbuf.actime = mtime.toTime_t();
    utbuf.modtime = utbuf.actime;
    utime(QFile::encodeName(path), &utbuf);
#elif defined(Q_OS_WIN)
    struct _utimbuf utbuf;
    utbuf.actime = mtime.toTime_t();
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
    foreach (const QString& path, files) {
        createFile(path);
    }
}

void TestDir::createDir(const QString& path, const QDateTime& time)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);
    QDir(name()).mkdir(absolutePath);

    if (time.isValid()) {
        setTimeStamp(absolutePath, time);
    }

    Q_ASSERT(QFile::exists(absolutePath));
}

void TestDir::removeFile(const QString& path)
{
    QString absolutePath = path;
    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.isAbsolute()) {
        absolutePath = name() + path;
    }
    QFile::remove(absolutePath);
}

void TestDir::makePathAbsoluteAndCreateParents(QString& path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isAbsolute()) {
        path = name() + path;
        fileInfo.setFile(path);
    }

    const QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        createDir(dir.absolutePath());
    }

    Q_ASSERT(dir.exists());
}
