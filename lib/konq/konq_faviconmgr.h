/* This file is part of the KDE project
   Copyright (C) 1999 Malte Starostik <malte.starostik@t-online.de>

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

#ifndef __konq_faviconmgr_h__
#define __konq_faviconmgr_h__ "$Id$"

#include <qdatastream.h>
#include <qmap.h>

#include <dcopobject.h>
#include <kurl.h>
#include <kio/job.h>

// The use of KURL and QString for URLs might be a bit fuzzy.
// I used QString where the URL only serves as a key in the QMaps
// or in iconForURL / setIconForURL, where it correlates to a displayed
// string and is not used to access the document at that URL. (malte)

/**
 * Maintains a list of custom icons per URL.
 */
class KonqFavIconMgr : public QObject, public DCOPObject
{
    Q_OBJECT
public:
    /**
     * Constructor. Do not instantiate yourself, use @rep self() instead.
     */
    KonqFavIconMgr();
    
    /**
     * Downloads an icon from @p iconURL and associates @p url with it.
     * if @p hostDefault is true, all URLs on @p url's host default to
     * that icon.
     */
    void setIconForURL(const QString &url, const KURL &iconURL, bool hostDefault = false);

    /**
     * Looks up an icon for @p url and returns its name if found
     * or QString::null otherwise
     */
    static QString iconForURL(const QString &url);
    
    static KonqFavIconMgr *self();

signals:
    /**
     * This is emitted when the icon for a @p url has changed.
     */
    void iconChanged(const QString &url);

private slots:
    void slotResult(KIO::Job *);

private:
    void readURLs();
    void changed(const QString &);

private:
    struct URLInfo
    {
        QString iconURL;
        bool hostDefault;
    };
    friend QDataStream &operator<< (QDataStream &, const URLInfo &);
    friend QDataStream &operator>> (QDataStream &, URLInfo &);
    QMap<QString, URLInfo> m_knownURLs;
    QMap<QString, QString> m_knownIcons;
    QStringList m_failedIcons;
    
    struct DownloadInfo : public URLInfo
    {
         QString url;
    };
    QMap<KIO::Job *, DownloadInfo> m_downloads;

    static KonqFavIconMgr *s_self;
};

#endif

