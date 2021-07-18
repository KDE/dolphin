/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESITEM_H
#define PLACESITEM_H

#include "kitemviews/kstandarditem.h"

#include <KBookmark>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/PortableMediaPlayer>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

#include <QPointer>
#include <QUrl>


class KDirLister;
class PlacesItemSignalHandler;

/**
 * @brief Extends KStandardItem by places-specific properties.
 */
class PlacesItem : public KStandardItem
{

public:
    explicit PlacesItem(const KBookmark& bookmark, PlacesItem* parent = nullptr);
    ~PlacesItem() override;

    void setUrl(const QUrl& url);
    QUrl url() const;

    void setUdi(const QString& udi);
    QString udi() const;

    void setApplicationName(const QString& applicationName);
    QString applicationName() const;

    void setHidden(bool hidden);
    bool isHidden() const;

    void setGroupHidden(bool hidden);
    bool isGroupHidden() const;

    void setSystemItem(bool isSystemItem);
    bool isSystemItem() const;

    void setCapacityBarRecommended(bool recommended);
    bool isCapacityBarRecommended() const;

    Solid::Device device() const;

    void setBookmark(const KBookmark& bookmark);
    KBookmark bookmark() const;

    bool storageSetupNeeded() const;

    bool isSearchOrTimelineUrl() const;

    PlacesItemSignalHandler* signalHandler() const;

protected:
    void onDataValueChanged(const QByteArray& role,
                                    const QVariant& current,
                                    const QVariant& previous) override;

    void onDataChanged(const QHash<QByteArray, QVariant>& current,
                               const QHash<QByteArray, QVariant>& previous) override;

private:
    PlacesItem(const PlacesItem& item);

    void initializeDevice(const QString& udi);

    /**
     * Is invoked if the accessibility of the storage access
     * m_access has been changed and updates the emblem.
     */
    void onAccessibilityChanged();

    /**
     * Applies the data-value from the role to m_bookmark.
     */
    void updateBookmarkForRole(const QByteArray& role);

    static QString generateNewId();

private:
    Solid::Device m_device;
    QPointer<Solid::StorageAccess> m_access;
    QPointer<Solid::StorageVolume> m_volume;
    QPointer<Solid::OpticalDisc> m_disc;
    QPointer<Solid::PortableMediaPlayer> m_player;
    QPointer<PlacesItemSignalHandler> m_signalHandler;
    KBookmark m_bookmark;
    bool m_capacityBarRecommended = false;

    friend class PlacesItemSignalHandler; // Calls onAccessibilityChanged()
};

#endif


