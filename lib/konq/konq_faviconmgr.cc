/* This file is part of the KDE project
   Copyright (C) 2000 Malte Starostik <malte@kde.org>

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

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kmimetype.h>

#include "konq_faviconmgr.moc"

KonqFavIconMgr::KonqFavIconMgr(QObject *parent, const char *name)
    : QObject(parent),
      DCOPObject("KonqFavIconMgr")
{
    setObjectName(name);

    connectDCOPSignal("kded", "favicons",
        "iconChanged(bool, QString, QString)",
        "notifyChange(bool, QString, QString)", false);
}

QString KonqFavIconMgr::iconForURL(const KUrl &url)
{
    return KMimeType::favIconForURL( url );
}

void KonqFavIconMgr::setIconForURL(const KUrl &url, const KUrl &iconURL)
{
    QByteArray data;
    QDataStream str(&data, QIODevice::WriteOnly);
    str << url << iconURL;
	QDBusInterfacePtr favicon("org.kde.kded", "/modules/favicons", "org.kde.Favicons");
	favicon->send("setIconForURL", url, iconURL);
}

void KonqFavIconMgr::downloadHostIcon(const KUrl &url)
{
    QByteArray data;
    QDataStream str(&data, QIODevice::WriteOnly);
    str << url;
	QDBusInterfacePtr favicon("org.kde.kded", "/modules/favicons", "org.kde.Favicons");
	favicon->send("downloadHostIcon", url);
}

