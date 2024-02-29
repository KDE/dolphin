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
#include <KIO/StatJob>

#include <QList>
#include <QString>
#include <QTimer>
#include <QUrl>

class QDropEvent;
class QMimeData;
class QWidget;
namespace KIO
{
class DropJob;
}

class DOLPHIN_EXPORT DragAndDropHelper : public QObject
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
     * @param destItem  The item destination.
     * @return          True if the destination is a directory and is writable, or it's a desktop file.
     *                  False otherwise.
     */
    static bool supportsDropping(const KFileItem &destItem);

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

    DragAndDropHelper(QObject *parent);

    /**
     * Updates the drop action according to whether the destination supports dropping.
     * If supportsDropping(destUrl), set dropAction = proposedAction. Otherwise, set
     * dropAction = Qt::IgnoreAction.
     *
     * @param event     Drop event.
     * @param destUrl   Destination URL.
     */
    void updateDropAction(QDropEvent *event, const QUrl &destUrl);

private:
    /**
     * Stores the results of the expensive checks made in urlListMatchesUrl.
     */
    static QHash<QUrl, bool> m_urlListMatchesUrlCache;

    /**
     * When updateDropAction() is called with a remote URL, we create a StatJob to
     * check if the destination is a directory or a desktop file. We cache the result
     * here to avoid doing the stat again on subsequent calls to updateDropAction().
     */
    KFileItem m_destItemCache;

    /**
     * Only keep the cache for 30 seconds, because the stat of the destUrl might change.
     */
    QTimer m_destItemCacheInvalidationTimer;

    /**
     * A StatJob on-fly to fill the cache for a remote URL. We shouldn't create more
     * than one StatJob at a time, so we keep a pointer to the current one.
     */
    KIO::StatJob *m_statJob = nullptr;

    /**
     * The URL for which the StatJob is running.
     * Note: We can't use m_statJob->url() because StatJob might resolve the URL to be
     *       different from what we passed into stat(). E.g. "mtp:<bus-name>" is resolved
     *       to "mtp:<phone name>"
     */
    QUrl m_statJobUrl;

    /**
     * The last event we received in updateDropAction(), but can't react to yet,
     * because a StatJob is on-fly.
     */
    QDropEvent *m_lastUndecidedEvent = nullptr;
};

#endif
