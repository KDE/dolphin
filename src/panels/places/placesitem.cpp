/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2018 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 * Based on KFilePlacesItem from kdelibs:
 * SPDX-FileCopyrightText: 2007 Kevin Ottens <ervin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesitem.h"
#include "trash/dolphintrash.h"

#include "dolphindebug.h"
#include "placesitemsignalhandler.h"

#include <KDirLister>
#include <KLocalizedString>
#include <Solid/Block>

PlacesItem::PlacesItem(const KBookmark& bookmark, PlacesItem* parent) :
    KStandardItem(parent),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_player(),
    m_signalHandler(nullptr),
    m_bookmark()
{
    m_signalHandler = new PlacesItemSignalHandler(this);
    setBookmark(bookmark);
}

PlacesItem::~PlacesItem()
{
    delete m_signalHandler;
}

void PlacesItem::setUrl(const QUrl &url)
{
    // The default check in KStandardItem::setDataValue()
    // for equal values does not work with a custom value
    // like QUrl. Hence do a manual check to prevent that
    // setting an equal URL results in an itemsChanged()
    // signal.
    if (dataValue("url").toUrl() != url) {
        if (url.scheme() == QLatin1String("trash")) {
            QObject::connect(&Trash::instance(), &Trash::emptinessChanged, m_signalHandler.data(), &PlacesItemSignalHandler::onTrashEmptinessChanged);
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

void PlacesItem::setApplicationName(const QString &applicationName)
{
    setDataValue("applicationName", applicationName);
}

QString PlacesItem::applicationName() const
{
    return dataValue("applicationName").toString();
}

void PlacesItem::setHidden(bool hidden)
{
    setDataValue("isHidden", hidden);
}

bool PlacesItem::isHidden() const
{
    return dataValue("isHidden").toBool();
}

bool PlacesItem::isGroupHidden() const
{
    return dataValue("isGroupHidden").toBool();
}

void PlacesItem::setGroupHidden(bool hidden)
{
    setDataValue("isGroupHidden", hidden);
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
    const bool bookmarkDataChanged = !(bookmark == m_bookmark);

    // bookmark object must be updated to keep in sync with source model
    m_bookmark = bookmark;

    if (!bookmarkDataChanged) {
        return;
    }

    delete m_access;
    delete m_volume;
    delete m_disc;
    delete m_player;

    const QString udi = bookmark.metaDataItem(QStringLiteral("UDI"));
    if (udi.isEmpty()) {
        setIcon(bookmark.icon());
        setText(i18ndc("kio5", "KFile System Bookmarks", bookmark.text().toUtf8().constData()));
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
    Q_UNUSED(current)
    Q_UNUSED(previous)

    if (!m_bookmark.isNull()) {
        updateBookmarkForRole(role);
    }
}

void PlacesItem::onDataChanged(const QHash<QByteArray, QVariant>& current,
                               const QHash<QByteArray, QVariant>& previous)
{
    Q_UNUSED(previous)

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
    m_player = m_device.as<Solid::PortableMediaPlayer>();

    setText(m_device.displayName());
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
    } else if (m_player) {
        const QStringList protocols = m_player->supportedProtocols();
        if (!protocols.isEmpty()) {
            setUrl(QUrl(QStringLiteral("%1:udi=%2").arg(protocols.first(), m_device.udi())));
        }
    }
}

void PlacesItem::onAccessibilityChanged()
{
    setIconOverlays(m_device.emblems());
    setUrl(QUrl::fromLocalFile(m_access->filePath()));
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
        if (text() != i18ndc("kio5", "KFile System Bookmarks", m_bookmark.text().toUtf8().data())) {
            m_bookmark.setFullText(text());
        }
    } else if (role == "url") {
        m_bookmark.setUrl(url());
    } else if (role == "udi") {
        m_bookmark.setMetaDataItem(QStringLiteral("UDI"), udi());
    } else if (role == "applicationName") {
        m_bookmark.setMetaDataItem(QStringLiteral("OnlyInApp"), applicationName());
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
    return QString::number(QDateTime::currentDateTimeUtc().toSecsSinceEpoch()) +
            '/' + QString::number(count++) + " (V2)";
}

PlacesItemSignalHandler *PlacesItem::signalHandler() const
{
    return m_signalHandler.data();
}
