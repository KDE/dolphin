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

#include <stdio.h> /* rename() */
#include <string.h> /* memcpy() */

#include <qfile.h>
#include <qimage.h>
#include <qbuffer.h>
#include <qdom.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <kdatastream.h>
#include <kapp.h>
#include <kiconloader.h>
#include <ksimpleconfig.h>
#include <dcopclient.h>
#include <kstaticdeleter.h>
#include <kdebug.h>

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include "konq_faviconmgr.h"

KSimpleConfig *KonqFavIconMgr::s_favicons = 0;
KStaticDeleter<KSimpleConfig> faviconsDeleter;

// Old format for converting
struct URLInfo
{
    QString iconURL;
    bool hostDefault;
};
                                
QDataStream &operator>> (QDataStream &str, URLInfo &info)
{
    str >> info.iconURL;
    str >> info.hostDefault;
    return str;
}

bool changeBookmarkIcon(KBookmarkGroup group,
    const QString &oldName, const QString &newName)
{
    bool changed = false;
    for (KBookmark bookmark = group.first();
         !bookmark.isNull();
         bookmark = group.next(bookmark))
    {
        if (bookmark.isSeparator())
            continue;
        if (bookmark.isGroup())
        {
            if (changeBookmarkIcon(bookmark.toGroup(), oldName, newName))
                changed = true;
        }
        else if (bookmark.icon() == oldName)
        {
            bookmark.internalElement().setAttribute("icon", newName);
            changed = true;
        }
    }
    return changed;
}

void convertFavIcons()
{
    QString path = locate("data", "konqueror/favicon_map");
    if ( path.isEmpty() ) return;
    QFile file(path);
    if (file.open(IO_ReadOnly))
    {
        QDataStream str(&file);
        QMap<QString, URLInfo> knownURLs;
        QMap<QString, QString> knownIcons;
        KSimpleConfig *favicons = KonqFavIconMgr::favicons();
        bool bookmarksChanged = false;
        str >> knownURLs;
        str >> knownIcons;
        for (QMap<QString, URLInfo>::ConstIterator it = knownURLs.begin();
             it != knownURLs.end();
             ++it)
        {
            if (it.data().hostDefault)
                continue;
            favicons->writeEntry(KonqFavIconMgr::simplifyURL(it.key()), it.data().iconURL);
            QString oldName = knownIcons[it.data().iconURL];
            if (oldName.isEmpty())
                continue;
            QString newName = KonqFavIconMgr::iconNameFromURL(it.data().iconURL);
            if (changeBookmarkIcon(KBookmarkManager::self()->root(), oldName, newName))
                bookmarksChanged = true;
            rename(QFile::encodeName(locateLocal("icon", oldName + ".png")),
                   QFile::encodeName(locateLocal("icon", newName + ".png")));
        }
        if (bookmarksChanged)
            if (KBookmarkManager::self()->save())
            {
                QByteArray data;
                kapp->dcopClient()->send("*", "KBookmarkManager", "notifyCompleteChange()", data);
            }
        file.remove();
    }
}

KonqFavIconMgr::KonqFavIconMgr(QObject *parent, const char *name)
    : QObject(parent, name),
      DCOPObject("KonqFavIconMgr")
{
    // If there is a file with the old format, read and convert it
    convertFavIcons();
}

QString KonqFavIconMgr::iconForURL(const QString &url)
{
    KURL _url(url);
    if (_url.host().isEmpty())
        return QString::null;

    KConfig *config = KGlobal::config();
    config->setGroup( "HTML Settings" );
    if (config->readBoolEntry( "EnableFavicon", true ))
    {
        QString iconURL = favicons()->readEntry(simplifyURL(url));
        if (!iconURL.isEmpty())
            return iconNameFromURL(iconURL);
    
        QString icon = "favicons/" + _url.host();
        if (!locate("icon", icon + ".png").isEmpty())
            return icon;
    }
    return QString::null;
}

QString KonqFavIconMgr::simplifyURL(const KURL &url)
{
    QString result = url.host() + url.path();
    for (unsigned int i = 0; i < result.length(); ++i)
        if (result[i] == '=')
            result[i] = '_';
    return result;
}

QString KonqFavIconMgr::iconNameFromURL(const KURL &iconURL)
{
    QString result = iconURL.host() + iconURL.path();
    for (unsigned int i = 0; i < result.length(); ++i)
        if (result[i] == '=' || result[i] == '/')
            result[i] = '_';
    QString ext = result.right(4);
    if (ext == ".ico" || ext == ".png" || ext == ".xpm")
        result.remove(result.length() - 4, 4);
    return "favicons/" + result;
}

KSimpleConfig *KonqFavIconMgr::favicons()
{
    if (!s_favicons)
        faviconsDeleter.setObject(s_favicons = new KSimpleConfig(locateLocal("data", "konqueror/faviconrc")));
    return s_favicons;
}

void KonqFavIconMgr::setIconForURL(const KURL &url, const KURL &iconURL)
{
    // Ignore pages that explicitly specify /favicon.ico, like kmail.kde.org
    if (iconURL.host() == url.host() && iconURL.path() == "/favicon.ico")
        return;
    if (!locate("icon", iconNameFromURL(iconURL)).isEmpty())
        return;
    startDownload(simplifyURL(url), false, iconURL);
}

void KonqFavIconMgr::downloadHostIcon(const KURL &url)
{
    if (!locate("icon", "favicons/" + url.host() + ".png").isEmpty())
        return;
    KURL iconURL(url);
    iconURL.setEncodedPathAndQuery("/favicon.ico");
    startDownload(url.host(), true, iconURL);
}

void KonqFavIconMgr::startDownload(const QString &hostOrURL, bool isHost, const KURL &iconURL)
{
    if (m_failedIcons.contains(iconURL.url()))
        return;
    KIO::Job *job = KIO::get(iconURL, false, false);
    connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)), SLOT(slotData(KIO::Job *, const QByteArray &)));
    connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
    Download download;
    download.hostOrURL = hostOrURL;
    download.isHost = isHost;
    m_downloads.insert(job, download);
}

void KonqFavIconMgr::notifyChange()
{
    favicons()->sync();
    emit changed();
}

void KonqFavIconMgr::slotData(KIO::Job *job, const QByteArray &data)
{
    Download &download = m_downloads[job];
    unsigned int oldSize = download.iconData.size();
    download.iconData.resize(oldSize + data.size());
    memcpy(download.iconData.data() + oldSize, data.data(), data.size());
}

void KonqFavIconMgr::slotResult(KIO::Job *job)
{
    Download download = m_downloads[job];
    m_downloads.remove(job);
    QString iconURL = static_cast<KIO::TransferJob *>(job)->url().url();
    if (job->error())
    {
        m_failedIcons.append(iconURL);
        return;
    }

    QBuffer buffer(download.iconData);
    buffer.open(IO_ReadOnly);
    QImageIO io;
    io.setIODevice(&buffer);
    io.setParameters("16");
    if (!io.read())
    {
        // Here too, the job might have had no error, but the downloaded
        // file contains just a 404 message sent with a 200 status.
        // microsoft.com does that... (malte)
        m_failedIcons.append(iconURL);
        return;
    }
    // Some sites have nasty 32x32 icons, according to the MS docs
    // IE ignores them, well, we scale them, otherwise the location
    // combo / menu will look quite ugly
    if (io.image().width() != KIcon::SizeSmall || io.image().height() != KIcon::SizeSmall)
        io.setImage(io.image().smoothScale(KIcon::SizeSmall, KIcon::SizeSmall));

    QString iconName;
    if (download.isHost)
        iconName = "favicons/" + download.hostOrURL;
    else
        iconName = iconNameFromURL(iconURL);
    
    io.setIODevice(0);                                   
    io.setFileName(locateLocal("icon", iconName + ".png"));
    io.setFormat("PNG");
    if (!io.write())
        return;

    if (!download.isHost)
        favicons()->writeEntry(download.hostOrURL, iconURL);
    QByteArray data;
    kapp->dcopClient()->send("*", "KonqFavIconMgr", "notifyChange()", data);
}

#include "konq_faviconmgr.moc"

