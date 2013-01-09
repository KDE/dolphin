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

#include <KFileItem>
#include <KLocale>
#include <konq_operations.h>
#include <KUrl>
#include <QApplication>
#include <QtDBus>
#include <QDropEvent>

KonqOperations* DragAndDropHelper::dropUrls(const KFileItem& destItem, const KUrl& destUrl, QDropEvent* event, QString& error)
{
    error.clear();

    if (!destItem.isNull() && !destItem.isWritable()) {
        error = i18nc("@info:status", "Access denied. Could not write to <filename>%1</filename>", destUrl.pathOrUrl());
        return 0;
    }

    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-kde-ark-dndextract-service") &&
        mimeData->hasFormat("application/x-kde-ark-dndextract-path")) {
        const QString remoteDBusClient = mimeData->data("application/x-kde-ark-dndextract-service");
        const QString remoteDBusPath = mimeData->data("application/x-kde-ark-dndextract-path");

        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient, remoteDBusPath,
                                                              "org.kde.ark.DndExtract", "extractSelectedFilesTo");
        message.setArguments(QVariantList() << destUrl.pathOrUrl());
        QDBusConnection::sessionBus().call(message);
    } else if (!destItem.isNull() && (destItem.isDir() || destItem.isDesktopFile())) {
        // Drop into a directory or a desktop-file
        const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
        foreach (const KUrl& url, urls) {
            if (url == destUrl) {
                error = i18nc("@info:status", "A folder cannot be dropped into itself");
                return 0;
            }
        }

        return KonqOperations::doDrop(destItem, destUrl, event, QApplication::activeWindow(), QList<QAction*>());
    } else {
        return KonqOperations::doDrop(KFileItem(), destUrl, event, QApplication::activeWindow(), QList<QAction*>());
    }

    return 0;
}

