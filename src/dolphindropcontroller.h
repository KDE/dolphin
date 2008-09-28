/***************************************************************************
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

#ifndef DOLPHINDROPCONTROLLER_H
#define DOLPHINDROPCONTROLLER_H

#include <QObject>
#include <kio/fileundomanager.h>

#include "libdolphin_export.h"

class QDropEvent;
class KUrl;
class KFileItem;

/**
 * @brief Handler for drop events, shared between DolphinView and TreeViewSidebarPage
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinDropController : public QObject
{
    Q_OBJECT
public:
    explicit DolphinDropController(QWidget* parentWidget);
    virtual ~DolphinDropController();

    /**
     * Handles the dropping of URLs to the given
     * destination. A context menu with the options
     * 'Move Here', 'Copy Here', 'Link Here' and
     * 'Cancel' is offered to the user.
     * @param destItem  Item of the destination (can be null, see KFileItem::isNull()).
     * @param destPath  Path of the destination.
     * @param event     Drop event
     */
    void dropUrls(const KFileItem& destItem,
                  const KUrl& destPath,
                  QDropEvent* event);

signals:
    /**
     * Is emitted when renaming, copying, moving, linking etc.
     * Used for feedback in the mainwindow.
     */
    void doingOperation(KIO::FileUndoManager::CommandType type);

private:
    QWidget* m_parentWidget;
};

#endif // DOLPHINDROPCONTROLLER_H
