/*
 * SPDX-FileCopyrightText: 2007-2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "draganddrophelper.h"

#include <KIO/DropJob>
#include <KJobWidgets>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDropEvent>
#include <QMimeData>

QHash<QUrl, bool> DragAndDropHelper::m_urlListMatchesUrlCache;

bool DragAndDropHelper::urlListMatchesUrl(const QList<QUrl> &urls, const QUrl &destUrl)
{
    auto iteratorResult = m_urlListMatchesUrlCache.constFind(destUrl);
    if (iteratorResult != m_urlListMatchesUrlCache.constEnd()) {
        return *iteratorResult;
    }

    const bool destUrlMatches = std::find_if(urls.constBegin(),
                                             urls.constEnd(),
                                             [destUrl](const QUrl &url) {
                                                 return url.matches(destUrl, QUrl::StripTrailingSlash);
                                             })
        != urls.constEnd();

    return *m_urlListMatchesUrlCache.insert(destUrl, destUrlMatches);
}

KIO::DropJob *DragAndDropHelper::dropUrls(const QUrl &destUrl, QDropEvent *event, QWidget *window)
{
    const QMimeData *mimeData = event->mimeData();
    if (isArkDndMimeType(mimeData)) {
        const QString remoteDBusClient = QString::fromUtf8(mimeData->data(arkDndServiceMimeType()));
        const QString remoteDBusPath = QString::fromUtf8(mimeData->data(arkDndPathMimeType()));

        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient,
                                                              remoteDBusPath,
                                                              QStringLiteral("org.kde.ark.DndExtract"),
                                                              QStringLiteral("extractSelectedFilesTo"));
        message.setArguments({destUrl.toDisplayString(QUrl::PreferLocalFile)});
        QDBusConnection::sessionBus().call(message);
    } else {
        if (urlListMatchesUrl(event->mimeData()->urls(), destUrl)) {
            return nullptr;
        }

        // TODO: remove this check once Qt is fixed so that it doesn't emit a QDropEvent on Wayland
        // when we called QDragMoveEvent::ignore()
        // https://codereview.qt-project.org/c/qt/qtwayland/+/541750
        KFileItem item(destUrl);
        // KFileItem(QUrl) only stat local URLs, so we always allow dropping on non-local URLs
        if (!item.isLocalFile() || supportsDropping(item)) {
            // Drop into a directory or a desktop-file
            KIO::DropJob *job = KIO::drop(event, destUrl);
            KJobWidgets::setWindow(job, window);
            return job;
        }
    }

    return nullptr;
}

bool DragAndDropHelper::supportsDropping(const KFileItem &destItem)
{
    return (destItem.isDir() && destItem.isWritable()) || destItem.isDesktopFile();
}

DragAndDropHelper::DragAndDropHelper(QObject *parent)
    : QObject(parent)
{
    m_destItemCacheInvalidationTimer.setSingleShot(true);
    m_destItemCacheInvalidationTimer.setInterval(30000);
    connect(&m_destItemCacheInvalidationTimer, &QTimer::timeout, this, [this]() {
        m_destItemCache = KFileItem();
    });
}

void DragAndDropHelper::updateDropAction(QDropEvent *event, const QUrl &destUrl)
{
    auto processEvent = [this](QDropEvent *event) {
        if (supportsDropping(m_destItemCache)) {
            event->setDropAction(event->proposedAction());
            event->accept();
        } else {
            event->setDropAction(Qt::IgnoreAction);
            event->ignore();
        }
    };

    m_lastUndecidedEvent = nullptr;

    if (urlListMatchesUrl(event->mimeData()->urls(), destUrl)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    if (destUrl == m_destItemCache.url()) {
        // We already received events for this URL, and already have the
        // stat result cached because:
        // 1. it's a local file, and we already called KFileItem(destUrl)
        // 2. it's a remote file, and StatJob finished
        processEvent(event);
        return;
    }

    if (m_statJob) {
        if (destUrl == m_statJobUrl) {
            // We already received events for this URL. Still waiting for
            // the stat result. StatJob will process the event when it finishes.
            m_lastUndecidedEvent = event;
            return;
        }

        // We are waiting for the stat result of a different URL. Cancel.
        m_statJob->kill();
        m_statJob = nullptr;
        m_statJobUrl.clear();
    }

    if (destUrl.isLocalFile()) {
        // New local URL. KFileItem will stat on demand.
        m_destItemCache = KFileItem(destUrl);
        m_destItemCacheInvalidationTimer.start();
        processEvent(event);
        return;
    }

    // New remote URL. Start a StatJob and process the event when it finishes.
    m_lastUndecidedEvent = event;
    m_statJob = KIO::stat(destUrl, KIO::StatJob::SourceSide, KIO::StatDetail::StatBasic, KIO::JobFlag::HideProgressInfo);
    m_statJobUrl = destUrl;
    connect(m_statJob, &KIO::StatJob::result, this, [this, processEvent](KJob *job) {
        KIO::StatJob *statJob = static_cast<KIO::StatJob *>(job);

        m_destItemCache = KFileItem(statJob->statResult(), m_statJobUrl);
        m_destItemCacheInvalidationTimer.start();

        if (m_lastUndecidedEvent) {
            processEvent(m_lastUndecidedEvent);
            m_lastUndecidedEvent = nullptr;
        }

        m_statJob = nullptr;
        m_statJobUrl.clear();
    });
}

void DragAndDropHelper::clearUrlListMatchesUrlCache()
{
    DragAndDropHelper::m_urlListMatchesUrlCache.clear();
}

bool DragAndDropHelper::isArkDndMimeType(const QMimeData *mimeData)
{
    return mimeData->hasFormat(arkDndServiceMimeType()) && mimeData->hasFormat(arkDndPathMimeType());
}
