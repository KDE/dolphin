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

#ifndef TESTHELPER_H
#define TESTHELPER_H

#include <KUrl>

#include <QDateTime>

class KTempDir;
class QAbstractItemView;
class QDir;
class DolphinDirLister;
class DolphinModel;
class DolphinSortFilterProxyModel;
class DolphinView;

/**
 * The class TestBase aims to make writing Dolphin unit tests easier.
 * It provides functionality that almost every unit test needs: setup of the DolphinView and
 * easy creation of test files and subfolders in a temporary directory which is removed in
 * the TestBase destructor.
 *
 * TODO: TestBase should also backup the DolphinSettings and restore them later!
 */

class TestBase : public QObject
{
    Q_OBJECT

public:

    TestBase();
    ~TestBase();

    // Returns the item view (icons, details, or columns)
    QAbstractItemView* itemView() const;

    // Reloads the view and waits for the finishedPathLoading(const KUrl&) signal.
    void reloadViewAndWait();

    KUrl testDirUrl() const;

    /**
     * The following functions create either a file, a list of files, or a directory.
     * The paths may be absolute or relative to the test directory. Any missing parent
     * directories will be created automatically.
     */

    void createFile(const QString& path, const QByteArray& data = QByteArray("test"), const QDateTime& time = QDateTime());
    void createFiles(const QStringList& files);
    void createDir(const QString& path, const QDateTime& time = QDateTime());

    /**
     * Returns the items shown in the view. The order corresponds to the sort order of the view.
     */

    QStringList viewItems() const;

    /**
     * Remove the test directory and create an empty one.
     */

    void cleanupTestDir();

    //  Make members that are accessed frequently by the derived test classes public

    DolphinView* m_view;

    QString m_path;

private:

    KTempDir* m_tempDir;
    QDir* m_dir;

    void makePathAbsoluteAndCreateParents(QString& path);

};

#endif
