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

QString DragAndDropHelper::dropUrls(const KFileItem& destItem,
                                    const KUrl& destPath,
                                    QDropEvent* event)
{
    const bool dropToItem = !destItem.isNull() && (destItem.isDir() || destItem.isDesktopFile());
    const KUrl destination = dropToItem ? destItem.url() : destPath;

    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-kde-dndextract")) {
        const QString remoteDBusClient = mimeData->data("application/x-kde-dndextract");
        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient, "/DndExtract",
                                                              "org.kde.DndExtract", "extractSelectedFilesTo");
        message.setArguments(QVariantList() << destination.pathOrUrl());
        QDBusConnection::sessionBus().call(message);
    } else {
        const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
        foreach (const KUrl& url, urls) {
            if (url == destination) {
                return i18nc("@info:status", "A folder cannot be dropped into itself");
            }
        }

        if (dropToItem) {
            KonqOperations::doDrop(destItem, destination, event, QApplication::activeWindow());
        } else {
            KonqOperations::doDrop(KFileItem(), destination, event, QApplication::activeWindow());
        }
    }

    return QString();
}

