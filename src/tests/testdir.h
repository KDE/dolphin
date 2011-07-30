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

#ifndef TESTDIR_H
#define TESTDIR_H

#include <KTempDir>
#include <KUrl>

#include <QDateTime>

/**
 * TestDir provides a temporary directory. In addition to KTempDir, it has
 * methods that create files and subdirectories inside the directory.
 */
class TestDir : public KTempDir
{

public:
    TestDir();
    virtual ~TestDir();

    KUrl url() const;

    /**
     * The following functions create either a file, a list of files, or a directory.
     * The paths may be absolute or relative to the test directory. Any missing parent
     * directories will be created automatically.
     */
    void createFile(const QString& path,
                    const QByteArray& data = QByteArray("test"),
                    const QDateTime& time = QDateTime());
    void createFiles(const QStringList& files);
    void createDir(const QString& path, const QDateTime& time = QDateTime());

private:
    void makePathAbsoluteAndCreateParents(QString& path);

};

#endif
