/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kdirectorycontentscounterworker.h"

// Required includes for subItemsCount():
#ifdef Q_OS_WIN
#include <QDir>
#else
#include <QFile>
#include <qplatformdefs.h>
#endif

#include "dolphin_detailsmodesettings.h"

KDirectoryContentsCounterWorker::KDirectoryContentsCounterWorker(QObject* parent) :
    QObject(parent)
{
    qRegisterMetaType<KDirectoryContentsCounterWorker::Options>();
}

#ifndef Q_OS_WIN
KDirectoryContentsCounterWorker::CountResult walkDir(const QString &dirPath,
                                                     const bool countHiddenFiles,
                                                     const bool countDirectoriesOnly,
                                                     QT_DIRENT *dirEntry,
                                                     const uint allowedRecursiveLevel)
{
    int count = -1;
    long size = -1;
    auto dir = QT_OPENDIR(QFile::encodeName(dirPath));
    if (dir) {
        count = 0;
        size = 0;
        QT_STATBUF buf;

        while ((dirEntry = QT_READDIR(dir))) {
            if (dirEntry->d_name[0] == '.') {
                if (dirEntry->d_name[1] == '\0' || !countHiddenFiles) {
                    // Skip "." or hidden files
                    continue;
                }
                if (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0') {
                    // Skip ".."
                    continue;
                }
            }

            // If only directories are counted, consider an unknown file type and links also
            // as directory instead of trying to do an expensive stat()
            // (see bugs 292642 and 299997).
            const bool countEntry = !countDirectoriesOnly ||
                    dirEntry->d_type == DT_DIR ||
                    dirEntry->d_type == DT_LNK ||
                    dirEntry->d_type == DT_UNKNOWN;
            if (countEntry) {
                ++count;
            }

            if (allowedRecursiveLevel > 0) {

                bool linkFound = false;
                QString nameBuf = QStringLiteral("%1/%2").arg(dirPath, dirEntry->d_name);

                if (dirEntry->d_type == DT_REG || dirEntry->d_type == DT_LNK) {
                    if (QT_STAT(nameBuf.toLocal8Bit(), &buf) == 0) {
                        if (S_ISDIR(buf.st_mode)) {
                            // was a dir link, recurse
                            linkFound = true;
                        }
                        size += buf.st_size;
                    }
                }
                if (dirEntry->d_type == DT_DIR || linkFound) {
                    // recursion for dirs and dir links
                    size += walkDir(nameBuf, countHiddenFiles, countDirectoriesOnly, dirEntry, allowedRecursiveLevel - 1).size;
                }
            }
        }
        QT_CLOSEDIR(dir);
    }
    return KDirectoryContentsCounterWorker::CountResult{count, size};
}
#endif

KDirectoryContentsCounterWorker::CountResult KDirectoryContentsCounterWorker::subItemsCount(const QString& path, Options options)
{
    const bool countHiddenFiles = options & CountHiddenFiles;
    const bool countDirectoriesOnly = options & CountDirectoriesOnly;

#ifdef Q_OS_WIN
    QDir dir(path);
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::System;
    if (countHiddenFiles) {
        filters |= QDir::Hidden;
    }
    if (countDirectoriesOnly) {
        filters |= QDir::Dirs;
    } else {
        filters |= QDir::AllEntries;
    }
    return {dir.entryList(filters).count(), 0};
#else

    const uint maxRecursiveLevel = DetailsModeSettings::directorySizeCount() ? 1 : DetailsModeSettings::recursiveDirectorySizeLimit();

    QT_DIRENT *dirEntry = nullptr;

    auto res = walkDir(path, countHiddenFiles, countDirectoriesOnly, dirEntry, maxRecursiveLevel);

    return res;
#endif
}

void KDirectoryContentsCounterWorker::countDirectoryContents(const QString& path, Options options)
{
    auto res = subItemsCount(path, options);
    Q_EMIT result(path, res.count, res.size);
}
