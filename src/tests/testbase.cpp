/***************************************************************************
 *   Copyright (C) 2010 by Frank Reininghaus (frank78ac@googlemail.com)    *
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

#include "testbase.h"

#include <qtest_kde.h>

#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"

#include <KTempDir>

#include <QtCore/QDir>
#include <QtGui/QAbstractItemView>

#include <kdebug.h>

TestBase::TestBase()
{
    m_tempDir = new KTempDir;
    Q_ASSERT(m_tempDir->exists());
    m_path = m_tempDir->name();
    m_dir = new QDir(m_path);
    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dolphinModel = new DolphinModel();
    m_dolphinModel->setDirLister(m_dirLister);
    m_proxyModel = new DolphinSortFilterProxyModel(0);
    m_proxyModel->setSourceModel(m_dolphinModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_view = new DolphinView(0, KUrl(m_path), m_proxyModel);
}

TestBase::~TestBase()
{
    delete m_view;
    delete m_proxyModel;
    // m_dolphinModel owns m_dirLister -> do not delete it here!
    delete m_dolphinModel;
    delete m_dir;
    delete m_tempDir;
}

QAbstractItemView* TestBase::itemView () const
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

void TestBase::createFile(const QString& path, const QByteArray& data)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);

    QFile f(absolutePath);
    f.open(QIODevice::WriteOnly);
    f.write(data);
    f.close();

    Q_ASSERT(QFile::exists(absolutePath));
}

void TestBase::createFiles(const QStringList& files)
{
    foreach(const QString& path, files) {
        createFile(path);
    }
}

void TestBase::createDir(const QString& path)
{
    QString absolutePath = path;
    makePathAbsoluteAndCreateParents(absolutePath);
    m_dir->mkdir(absolutePath);

    Q_ASSERT(QFile::exists(absolutePath));
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
