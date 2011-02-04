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

#include "testbase.h"

#include <qtest_kde.h>

#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"

#include <KTempDir>

#include <QDir>
#include <QAbstractItemView>

#include <KDebug>

#ifdef Q_OS_UNIX
#include <utime.h>
#else
#include <sys/utime.h>
#endif

TestBase::TestBase()
{
    m_tempDir = new KTempDir;
    Q_ASSERT(m_tempDir->exists());
    m_path = m_tempDir->name();
    m_dir = new QDir(m_path);
    m_view = new DolphinView(KUrl(m_path), 0);
}

TestBase::~TestBase()
{
    delete m_view;
    delete m_dir;
    delete m_tempDir;
}

QAbstractItemView* TestBase::itemView() const
{
    return m_view->m_viewAccessor.itemView();
}

void TestBase::reloadViewAndWait()
{
    m_view->reload();
    QVERIFY(QTest::kWaitForSignal(m_view, SIGNAL(finishedPathLoading(const KUrl&)), 2000));
}

KUrl TestBase::testDirUrl() const
{
    return KUrl(m_path);
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

void TestBase::createFile(const QString& path, const QByteArray& data, const QDateTime& time)
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

void TestBase::createFiles(const QStringList& files)
{
    foreach(const QString& path, files) {
        createFile(path);
    }
}

void TestBase::createDir(const QString& path, const QDateTime& time)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);
    m_dir->mkdir(absolutePath);

    if (time.isValid()) {
        setTimeStamp(absolutePath, time);
    }

    Q_ASSERT(QFile::exists(absolutePath));
}

QStringList TestBase::viewItems() const
{
    QStringList itemList;
    const QAbstractItemModel* model = itemView()->model();

    for (int row = 0; row < model->rowCount(); row++) {
        itemList << model->data(model->index(row, 0), Qt::DisplayRole).toString();
    }

    return itemList;
}

void TestBase::makePathAbsoluteAndCreateParents(QString& path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isAbsolute()) {
        path = m_path + path;
        fileInfo.setFile(path);
    }

    const QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        createDir(dir.absolutePath());
    }

    Q_ASSERT(dir.exists());
}

void TestBase::cleanupTestDir()
{
    delete m_tempDir;
    m_tempDir = new KTempDir;
    Q_ASSERT(m_tempDir->exists());
    m_path = m_tempDir->name();
    m_dir->setPath(m_path);
    m_view->setUrl(m_path);
}
