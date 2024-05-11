/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "workerintegration.h"

#include "dolphinmainwindow.h"
#include "dolphinviewcontainer.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageDialog>
#include <KProtocolInfo>

#include <QAction>

using namespace Admin;

QString Admin::warningMessage()
{
    return xi18nc(
        "@info",
        "<para>You are about to use administrator privileges. While acting as an administrator you can change or replace any file or folder on this system. "
        "This includes items which are critical for this system to function.</para><para>You are able to <emphasis>delete every users' data</emphasis> on this "
        "computer and to <emphasis>break this installation beyond repair.</emphasis> Adding just one letter in a folder or file name or its contents can "
        "render a system <emphasis>unbootable.</emphasis></para><para>There is probably not going to be another warning even if you are about to break this "
        "system.</para><para>You might want to <emphasis>backup files and folders</emphasis> before proceeding.</para>");
}

namespace
{
/** The only WorkerIntegration object that is ever constructed. It is only ever accessed directly from within this file. */
WorkerIntegration *instance = nullptr;
}

WorkerIntegration::WorkerIntegration(DolphinMainWindow *parent, QAction *actAsAdminAction)
    : QObject{parent}
    , m_actAsAdminAction{actAsAdminAction}
{
    Q_CHECK_PTR(parent);
    Q_CHECK_PTR(actAsAdminAction);

    connect(parent, &DolphinMainWindow::urlChanged, this, &WorkerIntegration::updateActAsAdminAction);

    connect(actAsAdminAction, &QAction::triggered, this, &WorkerIntegration::toggleActAsAdmin);
}

void WorkerIntegration::createActAsAdminAction(KActionCollection *actionCollection, DolphinMainWindow *dolphinMainWindow)
{
    Q_ASSERT(!instance);
    if (KProtocolInfo::isKnownProtocol(QStringLiteral("admin"))) {
        QAction *actAsAdminAction = actionCollection->addAction(QStringLiteral("act_as_admin"));
        actAsAdminAction->setText(i18nc("@action:inmenu", "Act as Administrator"));
        actAsAdminAction->setIcon(QIcon::fromTheme(QStringLiteral("system-switch-user")));
        actAsAdminAction->setCheckable(true);
        actionCollection->setDefaultShortcut(actAsAdminAction, Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_A);

        instance = new WorkerIntegration(dolphinMainWindow, actAsAdminAction);
    }
}

void WorkerIntegration::exitAdminMode()
{
    if (instance->m_actAsAdminAction->isChecked()) {
        instance->m_actAsAdminAction->trigger();
    }
}

void WorkerIntegration::toggleActAsAdmin()
{
    auto dolphinMainWindow = static_cast<DolphinMainWindow *>(parent());
    QUrl url = dolphinMainWindow->activeViewContainer()->urlNavigator()->locationUrl();
    if (url.scheme() == QStringLiteral("file")) {
        bool risksAccepted = !KMessageBox::shouldBeShownContinue(warningDontShowAgainName);

        if (!risksAccepted) {
            KMessageDialog warningDialog{KMessageDialog::QuestionTwoActions, warningMessage(), dolphinMainWindow};
            warningDialog.setCaption(i18nc("@title:window", "Risks of Acting as an Administrator"));
            warningDialog.setIcon(QIcon::fromTheme(QStringLiteral("security-low")));
            warningDialog.setButtons(KGuiItem{i18nc("@action:button", "I Understand and Accept These Risks"), QStringLiteral("data-warning")},
                                     KStandardGuiItem::cancel());
            warningDialog.setDontAskAgainText(i18nc("@option:check", "Do not warn me about these risks again"));

            risksAccepted = warningDialog.exec() != 4 /* Cancel */;
            if (warningDialog.isDontAskAgainChecked()) {
                KMessageBox::saveDontShowAgainContinue(warningDontShowAgainName);
            }

            if (!risksAccepted) {
                updateActAsAdminAction(); // Uncheck the action
                return;
            }
        }

        url.setScheme(QStringLiteral("admin"));
    } else if (url.scheme() == QStringLiteral("admin")) {
        url.setScheme(QStringLiteral("file"));
    }
    dolphinMainWindow->changeUrl(url);
}

void WorkerIntegration::updateActAsAdminAction()
{
    if (instance) {
        const QString currentUrlScheme = static_cast<DolphinMainWindow *>(instance->parent())->activeViewContainer()->url().scheme();
        if (currentUrlScheme == QStringLiteral("file")) {
            instance->m_actAsAdminAction->setEnabled(true);
            instance->m_actAsAdminAction->setChecked(false);
        } else if (currentUrlScheme == QStringLiteral("admin")) {
            instance->m_actAsAdminAction->setEnabled(true);
            instance->m_actAsAdminAction->setChecked(true);
        } else {
            instance->m_actAsAdminAction->setEnabled(false);
        }
    }
}
