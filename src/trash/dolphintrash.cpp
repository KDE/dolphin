/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2018 by Roman Inflianskas <infroma@gmail.com>           *
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

#include "dolphintrash.h"

#include <KIO/JobUiDelegate>
#include <KJobWidgets>
#include <QList>
#include <KNotification>
#include <KConfig>
#include <KConfigGroup>


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
        emit emptinessChanged(isTrashEmpty);
    };
    connect(m_trashDirLister, static_cast<void(KDirLister::*)()>(&KDirLister::completed), this, trashDirContentChanged);
    connect(m_trashDirLister, &KDirLister::itemsDeleted, this, trashDirContentChanged);
    m_trashDirLister->openUrl(QStringLiteral("trash:/"));
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
        connect(job, &KIO::Job::result, [](){
            KNotification::event(QStringLiteral("Trash: emptied"), QString(), QPixmap(), nullptr, KNotification::DefaultEvent);
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

