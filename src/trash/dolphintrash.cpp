/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2018 Roman Inflianskas <infroma@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintrash.h"

#include <KIO/JobUiDelegate>
#include <KJobWidgets>
#include <QList>
#include <KNotification>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

Trash::Trash()
    : m_trashDirLister(new KDirLister())
{
    // The trash icon must always be updated dependent on whether
    // the trash is empty or not. We use a KDirLister that automatically
    // watches for changes if the number of items has been changed.
    m_trashDirLister->setAutoErrorHandlingEnabled(false, nullptr);
    m_trashDirLister->setDelayedMimeTypes(true);
    auto trashDirContentChanged = [this]() {
        bool isTrashEmpty = m_trashDirLister->items().isEmpty();
        Q_EMIT emptinessChanged(isTrashEmpty);
    };
    connect(m_trashDirLister, QOverload<>::of(&KCoreDirLister::completed), this, trashDirContentChanged);
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

KIO::Job *Trash::empty(QWidget *window)
{
    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(window);
    bool confirmed = uiDelegate.askDeleteConfirmation(QList<QUrl>(), KIO::JobUiDelegate::EmptyTrash, KIO::JobUiDelegate::DefaultConfirmation);
    if (confirmed) {
        KIO::Job* job = KIO::emptyTrash();
        KJobWidgets::setWindow(job, window);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
         // as long as KIO doesn't do this, do it ourselves
        connect(job, &KIO::Job::result, []() {
            KNotification::event(QStringLiteral("Trash: emptied"), i18n("Trash Emptied"),
                                 i18n("The Trash was emptied."), QStringLiteral("user-trash"),
                                 nullptr, KNotification::DefaultEvent);
        });
        return job;
    }
    return nullptr;
}

bool Trash::isEmpty()
{
    KConfig trashConfig(QStringLiteral("trashrc"), KConfig::SimpleConfig);
    return (trashConfig.group("Status").readEntry("Empty", true));
}

