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

#include <qmap.h>

#include <kurl.h>
#include <dcopobject.h>
#include <kio/job.h>

class KSimpleConfig;

/**
 * Maintains a list of custom icons per URL.
 */
class KonqFavIconMgr : public QObject, public DCOPObject
{
    Q_OBJECT
    K_DCOP
public:
    /**
     * Constructor.
     */
    KonqFavIconMgr(QObject *parent = 0, const char *name = 0);

    /**
     * Downloads an icon from @p iconURL and associates @p url with it.
     */
    void setIconForURL(const KURL &url, const KURL &iconURL);

    /**
     * Downloads /favicon.ico from the host of @p url and associates all
     * URLs on that host with it
     * (unless a more specific entry for a URL exists)
     */
    void downloadHostIcon(const KURL &url);

    /**
     * Looks up an icon for @p url and returns its name if found
     * or QString::null otherwise
     */
    static QString iconForURL(const QString &url);

k_dcop:
    /**
     * an icon changed, used to propagate changes between applications
     * and instances
     */
    virtual ASYNC notifyChange( bool isHost, QString hostOrURL, QString iconURL );

signals:
    /**
     * An icon has changed, emitted upon reception of @ref notifyChanged()
     */
    void changed();

private slots:
    void slotData(KIO::Job *, const QByteArray &);
    void slotResult(KIO::Job *);

private:
    // Can be removed after
    // "everybody upgraded from KDE 2.1b1 to a later version"
    friend void convertFavIcons();

    void startDownload(const QString &, bool, const KURL &);
    static QString simplifyURL(const KURL &);
    static QString iconNameFromURL(const KURL &);
    static KSimpleConfig *favicons();

private:
    QStringList m_failedIcons;
    struct Download
    {
        QString hostOrURL;
        bool isHost;
        QByteArray iconData;
    };
    typedef QMap<KIO::Job *, Download> DownloadMap; // make dcopidl happy
    DownloadMap m_downloads;

    static KSimpleConfig *s_favicons;
};

#endif

