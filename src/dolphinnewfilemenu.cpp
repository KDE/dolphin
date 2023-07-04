/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinnewfilemenu.h"

#include "views/dolphinnewfilemenuobserver.h"

#include <KActionCollection>
#include <KIO/Job>

DolphinNewFileMenu::DolphinNewFileMenu(KActionCollection *collection, QObject *parent)
    : KNewFileMenu(collection, QStringLiteral("new_menu"), parent)
{
    DolphinNewFileMenuObserver::instance().attach(this);
}

DolphinNewFileMenu::~DolphinNewFileMenu()
{
    DolphinNewFileMenuObserver::instance().detach(this);
}

void DolphinNewFileMenu::slotResult(KJob *job)
{
    if (job->error() && job->error() != KIO::ERR_USER_CANCELED) {
        Q_EMIT errorMessage(job->errorString());
    } else {
        KNewFileMenu::slotResult(job);
    }
}

#include "moc_dolphinnewfilemenu.cpp"
