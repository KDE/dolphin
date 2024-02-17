/*
 * SPDX-FileCopyrightText: 2007-2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DRAGANDDROPHELPER_H
#define DRAGANDDROPHELPER_H

#include "dolphin_export.h"

#include <KFileItem>

#include <QList>
#include <QString>
#include <QUrl>

class QDropEvent;
class QMimeData;
class QWidget;
namespace KIO
{
class DropJob;
}

class DOLPHIN_EXPORT DragAndDropHelper
{
public:
    /**
     * Handles the dropping of URLs to the given destination. A context menu
     * with the options 'Move Here', 'Copy Here', 'Link Here' and 'Cancel' is
     * offered to the user. The drag destination must represent a directory or
     * a desktop-file, otherwise the dropping gets ignored.
     *
     * @param destUrl   URL of the item destination. Is used only if destItem::isNull()
     *                  is true.
     * @param event     Drop event.
     * @param window    Widget where the drop happened, will be used as parent of the drop menu.
     * @return          KIO::DropJob pointer or null in case the destUrl is contained
     *                  in the mimeData url list.
     */
    static KIO::DropJob *dropUrls(const QUrl &destUrl, QDropEvent *event, QWidget *window);

    /**
     * Checks if the destination supports dropping.
     *
     * @param destUrl   URL of the item destination.
     * @return          True if the destination is a directory and is writable, or it's a desktop file.
     *                  False otherwise.
     */
    static bool supportsDropping(const QUrl &destUrl);

    /**
     * Checks if the destination supports dropping.
     *
     * @param destItem  The item destination.
     * @return          True if the destination is a directory and is writable, or it's a desktop file.
     *                  False otherwise.
     */
    static bool supportsDropping(const KFileItem &destItem);

    /**
     * Updates the drop action according to whether the destination supports dropping.
     * If supportsDropping(destUrl), set dropAction = proposedAction. Otherwise, set
     * dropAction = Qt::IgnoreAction.
     *
     * @param event     Drop event.
     * @param destUrl   Destination URL.
     */
    static void updateDropAction(QDropEvent *event, const QUrl &destUrl);

    /**
     * @return True if destUrl is contained in the urls parameter.
     */
    static bool urlListMatchesUrl(const QList<QUrl> &urls, const QUrl &destUrl);

    /**
     * @return True if mimeData contains Ark's drag and drop mime types.
     */
    static bool isArkDndMimeType(const QMimeData *mimeData);
    static QString arkDndServiceMimeType()
    {
        return QStringLiteral("application/x-kde-ark-dndextract-service");
    }
    static QString arkDndPathMimeType()
    {
        return QStringLiteral("application/x-kde-ark-dndextract-path");
    }

    /**
     * clear the internal cache.
     */
    static void clearUrlListMatchesUrlCache();

private:
    /**
     * Stores the results of the expensive checks made in urlListMatchesUrl.
     */
    static QHash<QUrl, bool> m_urlListMatchesUrlCache;
};

#endif
