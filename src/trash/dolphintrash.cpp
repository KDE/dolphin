/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2018 Roman Inflianskas <infroma@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintrash.h"

#include <KConfig>
#include <KConfigGroup>
#include <KIO/DeleteOrTrashJob>
#include <KLocalizedString>
#include <KNotification>
#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/StorageAccess>

#include <QList>

Trash::Trash()
    : m_trashDirLister(new KDirLister())
{
    // The trash icon must always be updated dependent on whether
    // the trash is empty or not. We use a KDirLister that automatically
    // watches for changes if the number of items has been changed.
    m_trashDirLister->setAutoErrorHandlingEnabled(false);
    m_trashDirLister->setDelayedMimeTypes(true);
    auto trashDirContentChanged = [this]() {
        bool isTrashEmpty = m_trashDirLister->items().isEmpty();
        Q_EMIT emptinessChanged(isTrashEmpty);
    };
    connect(m_trashDirLister, &KCoreDirLister::completed, this, trashDirContentChanged);
    connect(m_trashDirLister, &KDirLister::itemsDeleted, this, trashDirContentChanged);

    // Update trash directory when removable storage devices are changed to keep it in sync with the
    // storage device .Trash-1000 folders
    Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();
    connect(notifier, &Solid::DeviceNotifier::deviceAdded, this, [this](const QString &device) {
        const Solid::Device dev(device);
        if (dev.isValid() && dev.is<Solid::StorageAccess>()) {
            const Solid::StorageAccess *access = dev.as<Solid::StorageAccess>();
            connect(access, &Solid::StorageAccess::accessibilityChanged, this, [this]() {
                m_trashDirLister->updateDirectory(QUrl(QStringLiteral("trash:/")));
            });
        }
    });
    connect(notifier, &Solid::DeviceNotifier::deviceRemoved, this, [this](const QString &device) {
        const Solid::Device dev(device);
        if (dev.isValid() && dev.is<Solid::StorageAccess>()) {
            m_trashDirLister->updateDirectory(QUrl(QStringLiteral("trash:/")));
        }
    });

    m_trashDirLister->openUrl(QUrl(QStringLiteral("trash:/")));
}

Trash::~Trash()
{
    delete m_trashDirLister;
}

Trash &Trash::instance()
{
    static Trash result;
    return result;
}

static void notifyEmptied()
{
    // As long as KIO doesn't do this, do it ourselves
    KNotification::event(QStringLiteral("Trash: emptied"),
                         i18n("Trash Emptied"),
                         i18n("The Trash was emptied."),
                         QStringLiteral("user-trash"),
                         KNotification::DefaultEvent);
}

void Trash::empty(QWidget *window)
{
    using Iface = KIO::AskUserActionInterface;
    auto *emptyJob = new KIO::DeleteOrTrashJob(QList<QUrl>{}, Iface::EmptyTrash, Iface::DefaultConfirmation, window);
    QObject::connect(emptyJob, &KIO::Job::result, emptyJob, [emptyJob] {
        if (!emptyJob->error()) {
            notifyEmptied();
        }
    });
    emptyJob->start();
}

bool Trash::isEmpty()
{
    KConfig trashConfig(QStringLiteral("trashrc"), KConfig::SimpleConfig);
    return (trashConfig.group(QStringLiteral("Status")).readEntry("Empty", true));
}

#include "moc_dolphintrash.cpp"
