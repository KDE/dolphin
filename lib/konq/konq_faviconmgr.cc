/* This file is part of the KDE project
   Copyright (C) 2000 Malte Starostik <malte.starostik@t-online.de>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qfile.h>
#include <qimage.h>

#include <kstaticdeleter.h>
#include <ktempfile.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kdatastream.h>
#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>

#include "konq_faviconmgr.h"

QDataStream &operator<< (QDataStream &str, const KonqFavIconMgr::URLInfo &info)
{
    str << info.iconURL;
    str << info.hostDefault;
    return str;
}

QDataStream &operator>> (QDataStream &str, KonqFavIconMgr::URLInfo &info)
{
    str >> info.iconURL;
    str >> info.hostDefault;
    return str;
}

KonqFavIconMgr *KonqFavIconMgr::s_self = 0;
KStaticDeleter<KonqFavIconMgr> deleter;

KonqFavIconMgr *KonqFavIconMgr::self()
{
    if (!s_self)
        s_self = deleter.setObject(new KonqFavIconMgr);
    return s_self;
}

KonqFavIconMgr::KonqFavIconMgr()
    : QObject(),
      DCOPObject("FavIconIFace") // Not used yet
{
    readURLs();
}

void KonqFavIconMgr::readURLs()
{
    QFile file(locate("data", "konqueror/favicon_map"));
    if (file.open(IO_ReadOnly))
    {
        QDataStream str(&file);
        str >> m_knownURLs;
        str >> m_knownIcons;
    }
}

void KonqFavIconMgr::changed(const QString &url)
{
    emit iconChanged(url);
}

QString KonqFavIconMgr::iconForURL(const QString &url)
{
    QMapConstIterator<QString, URLInfo> it(self()->m_knownURLs.find(url));

    KConfig *config = KGlobal::config();
    config->setGroup( "HTML Settings" );


    // Don't pass an icon if favicon support is disabled
    if ( config->readBoolEntry( "EnableFavicon" ) == false )
    {
        return QString::null;
    }

    if (it == self()->m_knownURLs.end())
    {
        KURL _url(url);
        _url.setEncodedPathAndQuery("");
        it = self()->m_knownURLs.find(_url.url());
    }
    if (it != self()->m_knownURLs.end())
        return self()->m_knownIcons[(*it).iconURL];

    return QString::null;
}

void KonqFavIconMgr::setIconForURL(const QString &url, const KURL &iconURL, bool hostDefault)
{
    if (url.isEmpty() || iconURL.isEmpty() || m_failedIcons.contains(iconURL.url()))
        return;

    DownloadInfo info;
    KURL _url(url);
    if (hostDefault)
        _url.setEncodedPathAndQuery("");
    info.url = _url.url();
    info.iconURL = iconURL.url();
    info.hostDefault = hostDefault;
    
    if (m_knownIcons.contains(iconURL.url()))
    {
        m_knownURLs[info.url] = info;
        changed(info.url);
    }
    else
    {
        KTempFile temp;
        KIO::Job *job = KIO::file_copy(iconURL, temp.name(), -1, true, false, false);
        connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
        m_downloads.insert(job, info);
    }
}

void KonqFavIconMgr::slotResult(KIO::Job *job)
{
    DownloadInfo info = m_downloads[job];
    m_downloads.remove(job);
    KIO::FileCopyJob *j = static_cast<KIO::FileCopyJob *>(job);
    if (job->error())
    {
        m_failedIcons.append(info.iconURL);
        QFile::remove(j->destURL().path());
        return;
    }

    QImageIO io;
    io.setFileName(j->destURL().path());
    io.setParameters("16");
    if (!io.read())
    {
        // Here too, the job might have had no error, but the downloaded
        // file contains just a 404 message... (malte)
        m_failedIcons.append(info.iconURL);
        QFile::remove(j->destURL().path());
        return;
    }
    QFile::remove(j->destURL().path());
    // Some sites have nasty 32x32 icons, according to the MS docs
    // IE ignores them, well, we scale them, otherwise the location
    // combo / menu will look quite ugly
    if (io.image().width() != KIcon::SizeSmall || io.image().height() != KIcon::SizeSmall)
        io.setImage(io.image().smoothScale(KIcon::SizeSmall, KIcon::SizeSmall));

    QString path = kapp->dirs()->saveLocation("icon", "favicons/");
    QString iconName = KURL(info.url).host();
    if (!info.hostDefault)
    {
        for (unsigned int serial = 0; ; ++serial)
        {
            QString suffix = QString("-%1").arg(serial);
            if (!QFile::exists(path + iconName /* host */ + suffix + ".png"))
            {
                iconName += suffix;
                break;
            }
        }
    }
        
    io.setFileName(path + iconName + ".png");
    io.setFormat("PNG");
    if (!io.write())
        return;

    m_knownURLs[info.url] = info;
    m_knownIcons[info.iconURL] = "favicons/" + iconName;
    
    QFile file(kapp->dirs()->saveLocation("data", "konqueror/") + "favicon_map");
    if (file.open(IO_WriteOnly))
    {
        QDataStream str(&file);
        str << m_knownURLs;
        str << m_knownIcons;
    }
    
    changed(info.url);
}

#include "konq_faviconmgr.moc"

