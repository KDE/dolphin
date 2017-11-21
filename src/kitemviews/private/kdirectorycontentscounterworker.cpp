/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include "kdirectorycontentscounterworker.h"

// Required includes for subItemsCount():
#ifdef Q_OS_WIN
    #include <QDir>
#else
    #include <dirent.h>
    #include <QFile>
#endif

KDirectoryContentsCounterWorker::KDirectoryContentsCounterWorker(QObject* parent) :
    QObject(parent)
{
    qRegisterMetaType<KDirectoryContentsCounterWorker::Options>();
}

int KDirectoryContentsCounterWorker::subItemsCount(const QString& path, Options options)
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
    return dir.entryList(filters).count();
#else
    // Taken from kdelibs/kio/kio/kdirmodel.cpp
    // Copyright (C) 2006 David Faure <faure@kde.org>

    int count = -1;
    DIR* dir = ::opendir(QFile::encodeName(path));
    if (dir) {  // krazy:exclude=syscalls
        count = 0;
        struct dirent *dirEntry = nullptr;
        while ((dirEntry = ::readdir(dir))) {
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
        }
        ::closedir(dir);
    }
    return count;
#endif
}

void KDirectoryContentsCounterWorker::countDirectoryContents(const QString& path, Options options)
{
    emit result(path, subItemsCount(path, options));
}
