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
#include <kurl.h>
 
#include <QCoreApplication>
#include <QEventLoop>
#include <QRegExp>

FileNameSearchProtocol::FileNameSearchProtocol( const QByteArray &pool, const QByteArray &app ) :
    SlaveBase("search", pool, app),
    m_checkContent(false),
    m_regExp(0)
{
}

FileNameSearchProtocol::~FileNameSearchProtocol()
{
    delete m_regExp;
    m_regExp = 0;
}

void FileNameSearchProtocol::listDir(const KUrl& url)
{
    delete m_regExp;
    m_regExp = 0;

    const QStringList searchValues = url.allQueryItemValues("search");
    if (!searchValues.isEmpty()) {
        m_regExp = new QRegExp(searchValues.first(), Qt::CaseInsensitive, QRegExp::Wildcard);
    }

    m_checkContent = false;
    const QStringList checkContentValues = url.allQueryItemValues("checkContent");
    if (!checkContentValues.isEmpty() && (checkContentValues.first() == QLatin1String("yes"))) {
        m_checkContent = true;
    }

    KUrl directory = url;
    directory.setProtocol("file");
    directory.setEncodedQuery(QByteArray());

    searchDirectory(directory);

    finished();
}

void FileNameSearchProtocol::searchDirectory(const KUrl& directory)
{
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
            addItem = containsPattern(item.url());
        }

        if (addItem) {
            KIO::UDSEntry entry = item.entry();
            entry.insert(KIO::UDSEntry::UDS_URL, item.url().url() );
            listEntry(entry,false);
        }

        if (item.isDir()) {
            pendingDirs.append(item.url());
        }
    }
    listEntry(KIO::UDSEntry(), true);

    // Recursively iterate all sub directories
    foreach (const KUrl& pendingDir, pendingDirs) {
        searchDirectory(pendingDir);
    }
}

bool FileNameSearchProtocol::containsPattern(const KUrl& fileName) const
{
    Q_ASSERT(m_regExp != 0);

    QFile file(fileName.path());
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
