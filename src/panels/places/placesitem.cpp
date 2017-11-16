/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on KFilePlacesItem from kdelibs:                                *
 *   Copyright (C) 2007 Kevin Ottens <ervin@kde.org>                       *
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

#include "placesitem.h"

#include <KBookmarkManager>
#include "dolphindebug.h"
#include <KDirLister>
#include <KLocalizedString>
#include "placesitemsignalhandler.h"
#include <QDateTime>
#include <Solid/Block>

PlacesItem::PlacesItem(const KBookmark& bookmark, PlacesItem* parent) :
    KStandardItem(parent),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_mtp(),
    m_signalHandler(nullptr),
    m_trashDirLister(nullptr),
    m_bookmark()
{
    m_signalHandler = new PlacesItemSignalHandler(this);
    setBookmark(bookmark);
}

PlacesItem::~PlacesItem()
{
    delete m_signalHandler;
    delete m_trashDirLister;
}

void PlacesItem::setUrl(const QUrl &url)
{
    // The default check in KStandardItem::setDataValue()
    // for equal values does not work with a custom value
    // like QUrl. Hence do a manual check to prevent that
    // setting an equal URL results in an itemsChanged()
    // signal.
    if (dataValue("url").toUrl() != url) {
        delete m_trashDirLister;
        if (url.scheme() == QLatin1String("trash")) {
            // The trash icon must always be updated dependent on whether
            // the trash is empty or not. We use a KDirLister that automatically
            // watches for changes if the number of items has been changed.
            // The update of the icon is handled in onTrashDirListerCompleted().
            m_trashDirLister = new KDirLister();
            m_trashDirLister->setAutoErrorHandlingEnabled(false, nullptr);
            m_trashDirLister->setDelayedMimeTypes(true);
            QObject::connect(m_trashDirLister.data(), static_cast<void(KDirLister::*)()>(&KDirLister::completed),
                             m_signalHandler.data(), &PlacesItemSignalHandler::onTrashDirListerCompleted);
            m_trashDirLister->openUrl(url);
        }

        setDataValue("url", url);
    }
}

QUrl PlacesItem::url() const
{
    return dataValue("url").toUrl();
}

void PlacesItem::setUdi(const QString& udi)
{
    setDataValue("udi", udi);
}

QString PlacesItem::udi() const
{
    return dataValue("udi").toString();
}

void PlacesItem::setHidden(bool hidden)
{
    setDataValue("isHidden", hidden);
}

bool PlacesItem::isHidden() const
{
    return dataValue("isHidden").toBool();
}

void PlacesItem::setSystemItem(bool isSystemItem)
{
    setDataValue("isSystemItem", isSystemItem);
}

bool PlacesItem::isSystemItem() const
{
    return dataValue("isSystemItem").toBool();
}

Solid::Device PlacesItem::device() const
{
    return m_device;
}

void PlacesItem::setBookmark(const KBookmark& bookmark)
{
    if (bookmark == m_bookmark) {
        return;
    }

    m_bookmark = bookmark;

    delete m_access;
    delete m_volume;
    delete m_disc;
    delete m_mtp;

    const QString udi = bookmark.metaDataItem(QStringLiteral("UDI"));
    if (udi.isEmpty()) {
        setIcon(bookmark.icon());
        setText(i18nc("KFile System Bookmarks", bookmark.text().toUtf8().constData()));
        setUrl(bookmark.url());
        setSystemItem(bookmark.metaDataItem(QStringLiteral("isSystemItem")) == QLatin1String("true"));
    } else {
        initializeDevice(udi);
    }

    setHidden(bookmark.metaDataItem(QStringLiteral("IsHidden")) == QLatin1String("true"));
}

KBookmark PlacesItem::bookmark() const
{
    return m_bookmark;
}

bool PlacesItem::storageSetupNeeded() const
{
    return m_access ? !m_access->isAccessible() : false;
}

bool PlacesItem::isSearchOrTimelineUrl() const
{
    const QString urlScheme = url().scheme();
    return (urlScheme.contains("search") || urlScheme.contains("timeline"));
}

void PlacesItem::onDataValueChanged(const QByteArray& role,
                                    const QVariant& current,
                                    const QVariant& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);

    if (!m_bookmark.isNull()) {
        updateBookmarkForRole(role);
    }
}

void PlacesItem::onDataChanged(const QHash<QByteArray, QVariant>& current,
                               const QHash<QByteArray, QVariant>& previous)
{
    Q_UNUSED(previous);

    if (!m_bookmark.isNull()) {
        QHashIterator<QByteArray, QVariant> it(current);
        while (it.hasNext()) {
            it.next();
            updateBookmarkForRole(it.key());
        }
    }
}

void PlacesItem::initializeDevice(const QString& udi)
{
    m_device = Solid::Device(udi);
    if (!m_device.isValid()) {
        return;
    }

    m_access = m_device.as<Solid::StorageAccess>();
    m_volume = m_device.as<Solid::StorageVolume>();
    m_disc = m_device.as<Solid::OpticalDisc>();
    m_mtp = m_device.as<Solid::PortableMediaPlayer>();

    setText(m_device.description());
    setIcon(m_device.icon());
    setIconOverlays(m_device.emblems());
    setUdi(udi);

    if (m_access) {
        setUrl(QUrl::fromLocalFile(m_access->filePath()));
        QObject::connect(m_access.data(), &Solid::StorageAccess::accessibilityChanged,
                         m_signalHandler.data(), &PlacesItemSignalHandler::onAccessibilityChanged);
        QObject::connect(m_access.data(), &Solid::StorageAccess::teardownRequested,
                         m_signalHandler.data(), &PlacesItemSignalHandler::onTearDownRequested);
    } else if (m_disc && (m_disc->availableContent() & Solid::OpticalDisc::Audio) != 0) {
        Solid::Block *block = m_device.as<Solid::Block>();
        if (block) {
            const QString device = block->device();
            setUrl(QUrl(QStringLiteral("audiocd:/?device=%1").arg(device)));
        } else {
            setUrl(QUrl(QStringLiteral("audiocd:/")));
        }
    } else if (m_mtp) {
        setUrl(QUrl(QStringLiteral("mtp:udi=%1").arg(m_device.udi())));
    }
}

void PlacesItem::onAccessibilityChanged()
{
    setIconOverlays(m_device.emblems());
    setUrl(QUrl::fromLocalFile(m_access->filePath()));
}

void PlacesItem::onTrashDirListerCompleted()
{
    Q_ASSERT(url().scheme() == QLatin1String("trash"));

    const bool isTrashEmpty = m_trashDirLister->items().isEmpty();
    setIcon(isTrashEmpty ? QStringLiteral("user-trash") : QStringLiteral("user-trash-full"));
}

void PlacesItem::updateBookmarkForRole(const QByteArray& role)
{
    Q_ASSERT(!m_bookmark.isNull());
    if (role == "iconName") {
        m_bookmark.setIcon(icon());
    } else if (role == "text") {
        // Only store the text in the KBookmark if it is not the translation of
        // the current text. This makes sure that the text is re-translated if
        // the user chooses another language, or the translation itself changes.
        //
        // NOTE: It is important to use "KFile System Bookmarks" as context
        // (see PlacesItemModel::createSystemBookmarks()).
        if (text() != i18nc("KFile System Bookmarks", m_bookmark.text().toUtf8().data())) {
            m_bookmark.setFullText(text());
        }
    } else if (role == "url") {
        m_bookmark.setUrl(url());
    } else if (role == "udi") {
        m_bookmark.setMetaDataItem(QStringLiteral("UDI"), udi());
    } else if (role == "isSystemItem") {
        m_bookmark.setMetaDataItem(QStringLiteral("isSystemItem"), isSystemItem() ? QStringLiteral("true") : QStringLiteral("false"));
    } else if (role == "isHidden") {
        m_bookmark.setMetaDataItem(QStringLiteral("IsHidden"), isHidden() ? QStringLiteral("true") : QStringLiteral("false"));
    }
}

QString PlacesItem::generateNewId()
{
    // The ID-generation must be different as done in KFilePlacesItem from kdelibs
    // to prevent identical IDs, because 'count' is of course not shared. We append a
    // " (V2)" to indicate that the ID has been generated by
    // a new version of the places view.
    static int count = 0;
    return QString::number(QDateTime::currentDateTimeUtc().toTime_t()) +
            '/' + QString::number(count++) + " (V2)";
}

PlacesItemSignalHandler *PlacesItem::signalHandler() const
{
    return m_signalHandler.data();
}
