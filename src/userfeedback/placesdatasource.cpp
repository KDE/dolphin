/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesdatasource.h"

#include <KLocalizedString>
#include <KMountPoint>
#include <KUserFeedback/Provider>
#include <Solid/Device>
#include <Solid/NetworkShare>
#include <Solid/StorageAccess>

#include <QVariant>

PlacesDataSource::PlacesDataSource()
    : KUserFeedback::AbstractDataSource(QStringLiteral("places"), KUserFeedback::Provider::DetailedSystemInformation)
{}

QString PlacesDataSource::name() const
{
    return i18nc("name of kuserfeedback data source provided by dolphin", "Places");
}

QString PlacesDataSource::description() const
{
    // TODO: add count of remote places grouped by protocol type.
    return i18nc("description of kuserfeedback data source provided by dolphin", "Count of available Network Shares");
}

QVariant PlacesDataSource::data()
{
    QVariantMap map;

    bool hasSSHFS = false;
    bool hasSambaShare = false;
    bool hasNfsShare = false;

    const auto devices = Solid::Device::listFromType(Solid::DeviceInterface::NetworkShare);
    for (const auto &device : devices) {
        if (!hasSSHFS && device.vendor() == QLatin1String("fuse.sshfs")) {
            // Ignore kdeconnect SSHFS mounts, we already know that people have those.
            auto storageAccess = device.as<Solid::StorageAccess>();
            if (storageAccess) {
                auto mountPoint = KMountPoint::currentMountPoints().findByPath(storageAccess->filePath());
                if (mountPoint && !mountPoint->mountedFrom().startsWith(QLatin1String("kdeconnect@"))) {
                    hasSSHFS = true;
                    continue;
                }
            }
        }

        if (!device.is<Solid::NetworkShare>()) {
            continue;
        }

        switch (device.as<Solid::NetworkShare>()->type()) {
        case Solid::NetworkShare::Cifs:
            hasSambaShare = true;
            continue;
        case Solid::NetworkShare::Nfs:
            hasNfsShare = true;
            continue;
        default:
            continue;
        }
    }

    map.insert(QStringLiteral("hasSSHFSMount"), hasSSHFS);
    map.insert(QStringLiteral("hasSambaShare"), hasSambaShare);
    map.insert(QStringLiteral("hasNfsShare"), hasNfsShare);

    return map;
}
