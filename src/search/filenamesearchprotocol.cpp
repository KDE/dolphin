/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "filenamesearchprotocol.h"

#include <kcomponentdata.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kurl.h>
#include <ktemporaryfile.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QRegExp>

FileNameSearchProtocol::FileNameSearchProtocol( const QByteArray &pool, const QByteArray &app ) :
    SlaveBase("search", pool, app),
    m_checkContent(false),
    m_regExp(0),
    m_iteratedDirs()
{
}

FileNameSearchProtocol::~FileNameSearchProtocol()
{
    cleanup();
}

void FileNameSearchProtocol::listDir(const KUrl& url)
{
    cleanup();

    const QString search = url.queryItem("search");
    if (!search.isEmpty()) {
        m_regExp = new QRegExp(search, Qt::CaseInsensitive, QRegExp::Wildcard);
    }

    m_checkContent = false;
    const QString checkContent = url.queryItem("checkContent");
    if (checkContent == QLatin1String("yes")) {
        m_checkContent = true;
    }

    const QString urlString = url.queryItem("url");
    searchDirectory(KUrl(urlString));

    cleanup();
    finished();
}

void FileNameSearchProtocol::searchDirectory(const KUrl& directory)
{
    if (directory.path() == QLatin1String("/proc")) {
        // Don't try to iterate the /proc directory of Linux
        return;
    }

    // Get all items of the directory
    KDirLister *dirLister = new KDirLister();
    dirLister->setDelayedMimeTypes(false);
    dirLister->setAutoErrorHandlingEnabled(false, 0);
    dirLister->openUrl(directory);

    QEventLoop eventLoop;
    QObject::connect(dirLister, SIGNAL(canceled()), &eventLoop, SLOT(quit()));
    QObject::connect(dirLister, SIGNAL(completed()), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    // Visualize all items that match the search pattern
    QList<KUrl> pendingDirs;
    const KFileItemList items = dirLister->items();
    foreach (const KFileItem& item, items) {
        bool addItem = false;
        if ((m_regExp == 0) || item.name().contains(*m_regExp)) {
            addItem = true;
        } else if (m_checkContent && item.mimetype().startsWith(QLatin1String("text/"))) {
            addItem = contentContainsPattern(item.url());
        }

        if (addItem) {
            KIO::UDSEntry entry = item.entry();
            entry.insert(KIO::UDSEntry::UDS_URL, item.url().url() );
            listEntry(entry,false);
        }

        if (item.isDir()) {
            if (item.isLink()) {
                // Assure that no endless searching is done in directories that
                // have already been iterated.
                const KUrl linkDest(item.url(), item.linkDest());
                if (!m_iteratedDirs.contains(linkDest.path())) {
                    pendingDirs.append(linkDest);
                }
            } else {
                pendingDirs.append(item.url());
            }
        }
    }
    listEntry(KIO::UDSEntry(), true);

    m_iteratedDirs.insert(directory.path());

    delete dirLister;
    dirLister = 0;

    // Recursively iterate all sub directories
    foreach (const KUrl& pendingDir, pendingDirs) {
        searchDirectory(pendingDir);
    }
}

bool FileNameSearchProtocol::contentContainsPattern(const KUrl& fileName) const
{
    Q_ASSERT(m_regExp != 0);

    QString path;
    KTemporaryFile tempFile;

    if (fileName.isLocalFile()) {
        path = fileName.path();
    } else if (tempFile.open()) {
        KIO::Job* getJob = KIO::file_copy(fileName,
                                          tempFile.fileName(),
                                          -1,
                                          KIO::Overwrite | KIO::HideProgressInfo);
        if (!KIO::NetAccess::synchronousRun(getJob, 0)) {
            // The non-local file could not be downloaded
            return false;
        }
        path = tempFile.fileName();
    } else {
        // No temporary file could be created for downloading non-local files
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         return false;
    }

     QTextStream in(&file);
     while (!in.atEnd()) {
         const QString line = in.readLine();
         if (line.contains(*m_regExp)) {
             return true;
         }
     }

     return false;
}

void FileNameSearchProtocol::cleanup()
{
    delete m_regExp;
    m_regExp = 0;
    m_iteratedDirs.clear();
}

extern "C" int KDE_EXPORT kdemain( int argc, char **argv )
{
    KComponentData instance("kio_search");
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_filenamesearch protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    FileNameSearchProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}
