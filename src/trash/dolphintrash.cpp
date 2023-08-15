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
    QObject::connect(emptyJob, &KIO::Job::result, notifyEmptied);
    emptyJob->start();
}

bool Trash::isEmpty()
{
    KConfig trashConfig(QStringLiteral("trashrc"), KConfig::SimpleConfig);
    return (trashConfig.group("Status").readEntry("Empty", true));
}

#include "moc_dolphintrash.cpp"
