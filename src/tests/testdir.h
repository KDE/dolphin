/*
 * SPDX-FileCopyrightText: 2010-2011 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TESTDIR_H
#define TESTDIR_H

#include <QUrl>
#include <QTemporaryDir>
#include <QDateTime>

/**
 * TestDir provides a temporary directory. In addition to QTemporaryDir, it has
 * methods that create files and subdirectories inside the directory.
 */
class TestDir : public QTemporaryDir
{

public:
    TestDir(const QString& directoryPrefix = QString());
    virtual ~TestDir();

    QUrl url() const;

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

    void removeFile(const QString& path);
    void removeFiles(const QStringList& files);

private:
    void makePathAbsoluteAndCreateParents(QString& path);

};

#endif
