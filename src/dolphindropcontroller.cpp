/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "dolphindropcontroller.h"
#include <kfileitem.h>
#include <klocale.h>
#include <kicon.h>
#include <QApplication>
#include <kdebug.h>
#include <kmenu.h>
#include <konq_operations.h>

DolphinDropController::DolphinDropController(QWidget* parentWidget)
    : QObject(parentWidget), m_parentWidget(parentWidget)
{
}

DolphinDropController::~DolphinDropController()
{
}

void DolphinDropController::dropUrls(const KFileItem& destItem,
                                     const KUrl& destPath,
                                     QDropEvent* event)
{
    const bool dropToItem = !destItem.isNull() && (destItem.isDir() || destItem.isDesktopFile());
    const KUrl destination = dropToItem ? destItem.url() : destPath;
                             
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    const KUrl sourceDir = KUrl(urls.first().directory());
    if (sourceDir != destination) {
        if (dropToItem) {
            KonqOperations::doDrop(destItem, destination, event, m_parentWidget);
        } else {
            KonqOperations::doDrop(KFileItem(), destination, event, m_parentWidget);
        }
    }
    // TODO: emit doingOperation, so that the main window gets informed about
    // about the finished operations
}

#include "dolphindropcontroller.moc"
