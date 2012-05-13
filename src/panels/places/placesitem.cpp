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
#include <KDebug>
#include <KIcon>
#include <KLocale>
#include "placesitemstorageaccesslistener.h"
#include <QDateTime>
#include <Solid/Block>

PlacesItem::PlacesItem(const KBookmark& bookmark, PlacesItem* parent) :
    KStandardItem(parent),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_accessListener(0),
    m_bookmark()
{
    setBookmark(bookmark);
}

PlacesItem::PlacesItem(const PlacesItem& item) :
    KStandardItem(item),
    m_device(),
    m_access(),
    m_volume(),
    m_disc(),
    m_accessListener(0),
    m_bookmark()
{
}

PlacesItem::~PlacesItem()
{
    delete m_accessListener;
}

void PlacesItem::setUrl(const KUrl& url)
{
    setDataValue("url", url);
}

KUrl PlacesItem::url() const
{
    return dataValue("url").value<KUrl>();
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
    if (!m_bookmark.isNull()) {
        m_bookmark.setMetaDataItem("IsHidden", hidden ? "true" : "false");
    }
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
    m_bookmark = bookmark;

    delete m_access;
    delete m_volume;
    delete m_disc;
    delete m_accessListener;

    const QString udi = bookmark.metaDataItem("UDI");
    if (udi.isEmpty()) {
        setIcon(bookmark.icon());
        setText(bookmark.text());
        setUrl(bookmark.url());
        setDataValue("address", bookmark.address());
        setGroup(i18nc("@item", "Places"));
    } else {
        initializeDevice(udi);
    }
    setHidden(bookmark.metaDataItem("IsHidden") == QLatin1String("true"));
}

KBookmark PlacesItem::bookmark() const
{
    return m_bookmark;
}

KBookmark PlacesItem::createBookmark(KBookmarkManager* manager,
                                     const QString& text,
                                     const KUrl& url,
                                     const QString& iconName,
                                     PlacesItem* after)
{
    KBookmarkGroup root = manager->root();
    if (root.isNull()) {
        return KBookmark();
    }

    KBookmark bookmark = root.addBookmark(text, url, iconName);
    bookmark.setMetaDataItem("ID", generateNewId());

    if (after) {
        root.moveBookmark(bookmark, after->bookmark());
    }

    return bookmark;
}

KBookmark PlacesItem::createDeviceBookmark(KBookmarkManager* manager,
                                           const QString& udi)
{
    KBookmarkGroup root = manager->root();
    if (root.isNull()) {
        return KBookmark();
    }

    KBookmark bookmark = root.createNewSeparator();
    bookmark.setMetaDataItem("UDI", udi);
    bookmark.setMetaDataItem("isSystemItem", "true");
    return bookmark;
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

    setText(m_device.description());
    setIcon(m_device.icon());
    setIconOverlays(m_device.emblems());
    setUdi(udi);
    setGroup(i18nc("@item", "Devices"));

    if (m_access) {
        setUrl(m_access->filePath());
        if (!m_accessListener) {
            // The access listener takes care to call PlacesItem::onAccessibilityChanged()
            // in case if the accessibility of m_access has been changed.
            m_accessListener = new PlacesItemStorageAccessListener(this);
        }
    } else if (m_disc && (m_disc->availableContent() & Solid::OpticalDisc::Audio) != 0) {
        const QString device = m_device.as<Solid::Block>()->device();
        setUrl(QString("audiocd:/?device=%1").arg(device));
    }
}

void PlacesItem::onAccessibilityChanged()
{
    setIconOverlays(m_device.emblems());
}

void PlacesItem::updateBookmarkForRole(const QByteArray& role)
{
    Q_ASSERT(!m_bookmark.isNull());
    if (role == "iconName") {
        m_bookmark.setIcon(icon());
    } else if (role == "text") {
        m_bookmark.setDescription(text());
    } else if (role == "url") {
        m_bookmark.setUrl(url());
    } else if (role == "udi)") {
        m_bookmark.setMetaDataItem("UDI", udi());
    } else if (role == "isSystemItem") {
        m_bookmark.setMetaDataItem("isSystemItem", isSystemItem() ? "true" : "false");
    } else if (role == "isHidden") {
        m_bookmark.setMetaDataItem("IsHidden", isHidden() ? "true" : "false");
    }
}

QString PlacesItem::generateNewId()
{
    static int count = 0;
    return QString::number(QDateTime::currentDateTime().toTime_t()) + '/' + QString::number(count++);
}
