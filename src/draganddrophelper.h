/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef DRAGANDDROPHELPER_H
#define DRAGANDDROPHELPER_H

#include "libdolphin_export.h"
#include <QObject>

class DolphinViewController;
class KFileItem;
class KUrl;
class QDropEvent;
class QMimeData;
class QAbstractItemView;
class QWidget;

/**
 * @brief Helper class for having a common drag and drop behavior.
 *
 * The class is used by DolphinIconsView, DolphinDetailsView,
 * DolphinColumnView and PanelTreeView to have a consistent
 * drag and drop behavior between all views.
 */
class LIBDOLPHINPRIVATE_EXPORT DragAndDropHelper : public QObject
{
    Q_OBJECT

public:
    static DragAndDropHelper& instance();

    /**
     * Returns true, if Dolphin supports the dragging of
     * the given mime data.
     */
    bool isMimeDataSupported(const QMimeData* mimeData) const;

    /**
     * Creates a drag object for the view \a itemView for all selected items.
     */
    void startDrag(QAbstractItemView* itemView,
                   Qt::DropActions supportedActions,
                   DolphinViewController* dolphinViewController = 0);

    /**
     * Returns true if and only if the view \a itemView was the last view to 
     * be passed to startDrag(...), and that drag is still in progress.
     */
    bool isDragSource(QAbstractItemView* itemView);

    /**
     * Handles the dropping of URLs to the given
     * destination. A context menu with the options
     * 'Move Here', 'Copy Here', 'Link Here' and
     * 'Cancel' is offered to the user.
     * @param destItem  Item of the destination (can be null, see KFileItem::isNull()).
     * @param destPath  Path of the destination.
     * @param event     Drop event.
     * @param widget    Source widget where the dragging has been started.
     */
    void dropUrls(const KFileItem& destItem,
                  const KUrl& destPath,
                  QDropEvent* event,
                  QWidget* widget);
signals:
    void errorMessage(const QString& msg);

private:
    DragAndDropHelper();
    // The last view passed in startDrag(...), or 0 if
    // no startDrag(...) initiated drag is in progress.
    QAbstractItemView *m_dragSource;

    friend class DragAndDropHelperSingleton;
};

#endif
