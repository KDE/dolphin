/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinnewfilemenu.h"

#include "views/dolphinnewfilemenuobserver.h"

#include <KIO/Global>

#include <QAction>

DolphinNewFileMenu::DolphinNewFileMenu(QAction *createDirAction, QAction *createFileAction, QObject *parent)
    : KNewFileMenu(parent)
{
    setNewFolderShortcutAction(createDirAction);
    setNewFileShortcutAction(createFileAction);
    attachObserver();
}

void DolphinNewFileMenu::detachObserver()
{
    DolphinNewFileMenuObserver::instance().detach(this);
}

void DolphinNewFileMenu::attachObserver()
{
    DolphinNewFileMenuObserver::instance().attach(this);
}

DolphinNewFileMenu::~DolphinNewFileMenu()
{
    detachObserver();
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
