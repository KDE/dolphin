/* This file is part of the KDE project
   Copyright (C) 2001 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <string.h>
#include <time.h>

#include <qbuffer.h>
#include <qfile.h>
#include <qcache.h>
#include <qimage.h>
#include <qtimer.h>
#include <QImageReader>

#include <kdatastream.h> // DO NOT REMOVE, otherwise bool marshalling breaks
#include <kicontheme.h>
#include <kimageio.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kio/job.h>

#include "favicons.moc"

struct FaviconsModulePrivate
{
    virtual ~FaviconsModulePrivate() { delete config; }

    struct DownloadInfo
    {
        QString hostOrURL;
        bool isHost;
        QByteArray iconData;
    };
    QMap<KIO::Job *, DownloadInfo> downloads;
    QStringList failedDownloads;
    KSimpleConfig *config;
    QList<KIO::Job*> killJobs;
    KIO::MetaData metaData;
    QString faviconsDir;
    QCache<QString,QString> faviconsCache;
};

FaviconsModule::FaviconsModule(const DCOPCString &obj)
    : KDEDModule(obj)
{
    // create our favicons folder so that KIconLoader knows about it
    d = new FaviconsModulePrivate;
    d->faviconsDir = KGlobal::dirs()->saveLocation( "cache", "favicons/" );
    d->faviconsDir.truncate(d->faviconsDir.length()-9); // Strip off "favicons/"
    d->metaData.insert("ssl_no_client_cert", "TRUE");
    d->metaData.insert("ssl_militant", "TRUE");
    d->metaData.insert("UseCache", "false");
    d->metaData.insert("cookies", "none");
    d->metaData.insert("no-auth", "true");
    d->config = new KSimpleConfig(locateLocal("data", "konqueror/faviconrc"));
}

FaviconsModule::~FaviconsModule()
{
    delete d;
}

QString removeSlash(QString result) 
{
    for (unsigned int i = result.length() - 1; i > 0; --i)
        if (result[i] != '/')
        {
            result.truncate(i + 1);
            break;
        }

    return result;
}


QString FaviconsModule::iconForURL(const KURL &url)
{
    if (url.host().isEmpty())
        return QString();

    QString icon;
    QString simplifiedURL = simplifyURL(url);

    QString *iconURL = d->faviconsCache[ removeSlash(simplifiedURL) ];
    if (iconURL)
        icon = *iconURL;
    else
        icon = d->config->readEntry( removeSlash(simplifiedURL) );

    if (!icon.isEmpty())
        icon = iconNameFromURL(KURL( icon ));
    else 
        icon = url.host();
        
    icon = "favicons/" + icon;

    if (QFile::exists(d->faviconsDir+icon+".png"))
        return icon;

    return QString();
}

QString FaviconsModule::simplifyURL(const KURL &url)
{
    // splat any = in the URL so it can be safely used as a config key
    QString result = url.host() + url.path();
    for (int i = 0; i < result.length(); ++i)
        if (result[i] == '=')
            result[i] = '_';
    return result;
}

QString FaviconsModule::iconNameFromURL(const KURL &iconURL)
{
    if (iconURL.path() == "/favicon.ico")
       return iconURL.host();

    QString result = simplifyURL(iconURL);
    // splat / so it can be safely used as a file name
    for (int i = 0; i < result.length(); ++i)
        if (result[i] == '/')
            result[i] = '_';

    QString ext = result.right(4);
    if (ext == ".ico" || ext == ".png" || ext == ".xpm")
        result.remove(result.length() - 4, 4);

    return result;
}

bool FaviconsModule::isIconOld(const QString &icon)
{
    struct stat st;
    if (stat(QFile::encodeName(icon), &st) != 0)
        return true; // Trigger a new download on error

    return (time(0) - st.st_mtime) > 604800; // arbitrary value (one week)
}

void FaviconsModule::setIconForURL(const KURL &url, const KURL &iconURL)
{
    QString simplifiedURL = simplifyURL(url);

    d->faviconsCache.insert(removeSlash(simplifiedURL), new QString(iconURL.url()) );

    QString iconName = "favicons/" + iconNameFromURL(iconURL);
    QString iconFile = d->faviconsDir + iconName + ".png";

    if (!isIconOld(iconFile)) {
        emit iconChanged(false, simplifiedURL, iconName);
        return;
    }

    startDownload(simplifiedURL, false, iconURL);
}

void FaviconsModule::downloadHostIcon(const KURL &url)
{
    QString iconFile = d->faviconsDir + "favicons/" + url.host() + ".png";
    if (!isIconOld(iconFile))
        return;

    startDownload(url.host(), true, KURL(url, "/favicon.ico"));
}

void FaviconsModule::startDownload(const QString &hostOrURL, bool isHost, const KURL &iconURL)
{
    if (d->failedDownloads.contains(iconURL.url()))
        return;

    KIO::Job *job = KIO::get(iconURL, false, false);
    job->addMetaData(d->metaData);
    connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)), SLOT(slotData(KIO::Job *, const QByteArray &)));
    connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
    connect(job, SIGNAL(infoMessage(KIO::Job *, const QString &)), SLOT(slotInfoMessage(KIO::Job *, const QString &)));
    FaviconsModulePrivate::DownloadInfo download;
    download.hostOrURL = hostOrURL;
    download.isHost = isHost;
    d->downloads.insert(job, download);
}

void FaviconsModule::slotData(KIO::Job *job, const QByteArray &data)
{
    FaviconsModulePrivate::DownloadInfo &download = d->downloads[job];
    unsigned int oldSize = download.iconData.size();
    if (oldSize > 0x10000)
    {
        d->killJobs.append(job);
        QTimer::singleShot(0, this, SLOT(slotKill()));
    }
    download.iconData.resize(oldSize + data.size());
    memcpy(download.iconData.data() + oldSize, data.data(), data.size());
}

void FaviconsModule::slotResult(KIO::Job *job)
{
    FaviconsModulePrivate::DownloadInfo download = d->downloads[job];
    d->downloads.remove(job);
    KURL iconURL = static_cast<KIO::TransferJob *>(job)->url();
    QString iconName;
    if (!job->error())
    {
        QBuffer buffer(&download.iconData);
        buffer.open(QIODevice::ReadOnly);
        QImageReader ir( &buffer );
        QSize desired( 16,16 );
        if( ir.canRead() ) {

            while( ir.imageCount() > 1 
              && ir.currentImageRect() != QRect( 0, 0, desired.width(), desired.height() )
              && ir.imageCount() >= ir.currentImageNumber() )
                ir.jumpToNextImage();
            ir.setScaledSize( desired );
            QImage img = ir.read();
            if( !img.isNull() ) {
                if (download.isHost)
                    iconName = download.hostOrURL;
                else
                    iconName = iconNameFromURL(iconURL);

                iconName = "favicons/" + iconName;
                if( !img.save( d->faviconsDir + iconName + ".png", "PNG" ) )
                    iconName.clear();
                else if (!download.isHost)
                    d->config->writeEntry( removeSlash(download.hostOrURL), iconURL.url());
            }
        }
    }
    if (iconName.isEmpty())
        d->failedDownloads.append(iconURL.url());

    emit iconChanged(download.isHost, download.hostOrURL, iconName);
}

void FaviconsModule::slotInfoMessage(KIO::Job *job, const QString &msg)
{
    emit infoMessage(static_cast<KIO::TransferJob *>( job )->url(), msg);
}

void FaviconsModule::slotKill()
{
    qDeleteAll(d->killJobs);
    d->killJobs.clear();
}

extern "C" {
    KDE_EXPORT KDEDModule *create_favicons(const DCOPCString &obj)
    {
        KImageIO::registerFormats();
        return new FaviconsModule(obj);
    }
}

// vim: ts=4 sw=4 et
