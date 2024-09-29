/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "workerintegration.h"

#include "config-dolphin.h"
#include "dolphinmainwindow.h"
#include "dolphinpackageinstaller.h"
#include "dolphinviewcontainer.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageDialog>
#include <KProtocolInfo>

#include <QAction>

#include <iostream>

using namespace Admin;

/** Free file-local functions */
namespace
{
/** @returns the translated name of the actAsAdminAction. */
QString actionName()
{
    return i18nc("@action:inmenu", "Act as Administrator");
};

/** @returns the default keyboard shortcut of the actAsAdminAction. */
QKeySequence actionDefaultShortcut()
{
    return Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_A;
};

/** @returns whether any worker for the protocol "admin" is available. */
bool isWorkerInstalled()
{
    return KProtocolInfo::isKnownProtocol(QStringLiteral("admin"));
}
}

void Admin::guideUserTowardsInstallingAdminWorker()
{
    if (!isWorkerInstalled()) {
        std::cout << qPrintable(
            xi18nc("@info:shell",
                   "<application>Dolphin</application> requires <application>%1</application> to manage system-controlled files, but it is not installed.<nl/>"
                   "Press %2 to install <application>%1</application> or %3 to cancel.",
                   ADMIN_WORKER_PACKAGE_NAME,
                   QKeySequence{Qt::Key_Enter}.toString(QKeySequence::NativeText),
                   QKeySequence{Qt::CTRL | Qt::Key_C}.toString(QKeySequence::NativeText)));
        std::cin.ignore();

        /// Installing admin worker
        DolphinPackageInstaller adminWorkerInstaller{ADMIN_WORKER_PACKAGE_NAME, QUrl(QStringLiteral("appstream://org.kde.kio.admin")), isWorkerInstalled};
        QObject::connect(&adminWorkerInstaller, &KJob::result, [](KJob *job) {
            if (job->error()) {
                std::cout << qPrintable(job->errorString()) << std::endl;
                exit(1);
            }
        });
        adminWorkerInstaller.exec();
    }
}

void Admin::guideUserTowardsUsingAdminWorker()
{
    KuitSetup *kuitSetup = &Kuit::setupForDomain("dolphin");
    kuitSetup->setTagPattern(QStringLiteral("numberedlist"), QStringList{}, Kuit::RichText, ki18nc("tag-format-pattern <numberedlist> rich", "<ol>%1</ol>"));
    kuitSetup->setTagPattern(QStringLiteral("numbereditem"), QStringList{}, Kuit::RichText, ki18nc("tag-format-pattern <numbereditem> rich", "<li>%1</li>"));

    KMessageBox::information(
        nullptr,
        xi18nc("@info",
               "<para>Make use of your administrator rights in Dolphin:<numberedlist>"
               "<numbereditem>Navigate to the file or folder you want to change.</numbereditem>"
               "<numbereditem>Activate the \"%1\" action either under <interface>Open Menu|More|View</interface> or <interface>Menu Bar|View</interface>.<nl/>"
               "Default shortcut: <shortcut>%2</shortcut></numbereditem>"
               "<numbereditem>After authorization you can manage files as an administrator.</numbereditem></numberedlist></para>",
               actionName(),
               actionDefaultShortcut().toString(QKeySequence::NativeText)),
        i18nc("@title:window", "How to Administrate"),
        "",
        KMessageBox::WindowModal);
}

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
    Q_ASSERT(!instance /* We never want to construct more than one instance,
    however in automatic testing sometimes multiple DolphinMainWindows are created, so this assert is diluted to accommodate for that: */
             || instance->parent() != dolphinMainWindow);
    if (isWorkerInstalled()) {
        QAction *actAsAdminAction = actionCollection->addAction(QStringLiteral("act_as_admin"));
        actAsAdminAction->setText(actionName());
        actAsAdminAction->setIcon(QIcon::fromTheme(QStringLiteral("system-switch-user")));
        actAsAdminAction->setCheckable(true);
        actionCollection->setDefaultShortcut(actAsAdminAction, actionDefaultShortcut());

        instance = new WorkerIntegration(dolphinMainWindow, actAsAdminAction);
    }
}

QAction *WorkerIntegration::FriendAccess::actAsAdminAction()
{
    return instance->m_actAsAdminAction;
}

void WorkerIntegration::toggleActAsAdmin()
{
    auto dolphinMainWindow = static_cast<DolphinMainWindow *>(parent());
    QUrl url = dolphinMainWindow->activeViewContainer()->urlNavigator()->locationUrl();

    if (url.scheme() == QStringLiteral("admin")) {
        url.setScheme(QStringLiteral("file"));
        dolphinMainWindow->changeUrl(url);
        return;
    } else if (url.scheme() != QStringLiteral("file")) {
        return;
    }

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

#include "moc_workerintegration.cpp"
