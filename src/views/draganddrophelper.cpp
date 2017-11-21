/***************************************************************************
 *   Copyright (C) 2007-2011 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2007 by David Faure <faure@kde.org>                     *
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

#include "draganddrophelper.h"

#include <QUrl>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDropEvent>
#include <QMimeData>

#include <KIO/DropJob>
#include <KJobWidgets>


bool DragAndDropHelper::urlListMatchesUrl(const QList<QUrl>& urls, const QUrl& destUrl)
{
    return std::find_if(urls.constBegin(), urls.constEnd(), [destUrl](const QUrl& url) {
        return url.matches(destUrl, QUrl::StripTrailingSlash);
    }) != urls.constEnd();
}

KIO::DropJob* DragAndDropHelper::dropUrls(const QUrl& destUrl, QDropEvent* event, QWidget* window)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat(QStringLiteral("application/x-kde-ark-dndextract-service")) &&
        mimeData->hasFormat(QStringLiteral("application/x-kde-ark-dndextract-path"))) {
        const QString remoteDBusClient = mimeData->data(QStringLiteral("application/x-kde-ark-dndextract-service"));
        const QString remoteDBusPath = mimeData->data(QStringLiteral("application/x-kde-ark-dndextract-path"));

        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient, remoteDBusPath,
                                                              QStringLiteral("org.kde.ark.DndExtract"), QStringLiteral("extractSelectedFilesTo"));
        message.setArguments({destUrl.toDisplayString(QUrl::PreferLocalFile)});
        QDBusConnection::sessionBus().call(message);
    } else {
        if (urlListMatchesUrl(event->mimeData()->urls(), destUrl)) {
            return nullptr;
        }

        // Drop into a directory or a desktop-file
        KIO::DropJob *job = KIO::drop(event, destUrl);
        KJobWidgets::setWindow(job, window);
        return job;
    }

    return nullptr;
}

