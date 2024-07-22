/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ADMINWORKERINTEGRATION_H
#define ADMINWORKERINTEGRATION_H

#include <QObject>

class DolphinMainWindow;
class DolphinViewContainer;
class KActionCollection;
class QAction;
class QUrl;

/**
 * @brief This namespace contains everything that is necessary to nicely integrate "KIO Admin Worker" into Dolphin.
 *
 * @see https://commits.kde.org/kio-admin
 */
namespace Admin
{
/**
 * When a user starts Dolphin with arguments that imply that they want to use administrative rights, this method is called.
 * This function acts like a command line program that guides users towards installing kio-admin. It will not return until this is accomplished.
 * This function will do nothing if kio-admin is already installed.
 */
void guideUserTowardsInstallingAdminWorker();

void guideUserTowardsUsingAdminWorker();

/**
 * Used with the KMessageBox API so users can disable the warning.
 * @see KMessageBox::saveDontShowAgainContinue()
 * @see KMessageBox::enableMessage()
 */
constexpr QLatin1String warningDontShowAgainName{"warnAboutRisksBeforeActingAsAdmin"};

/** @returns an elaborate warning about the dangers of acting with administrative privileges. */
QString warningMessage();

/**
 * @brief A class encapsulating the "Act as Admin"-toggle action.
 *
 * @see https://commits.kde.org/kio-admin
 */
class WorkerIntegration : public QObject
{
    Q_OBJECT

public:
    /**
     * Adds a toggle action to the \a actionCollection.
     * The action switches between acting as a normal user or acting as an administrator.
     */
    static void createActAsAdminAction(KActionCollection *actionCollection, DolphinMainWindow *dolphinMainWindow);

    /**
     * An interface that only allows friend classes to show the WorkerIntegration::m_actAsAdminAction to users.
     * Aside from these friend classes the action is only accessible through the actionCollection of DolphinMainWindow.
     */
    class FriendAccess
    {
        /** @returns WorkerIntegration::m_actAsAdminAction or crashes if WorkerIntegration::createActAsAdminAction() has not been called previously. */
        static QAction *actAsAdminAction();

        friend class Bar; /// Allows the bar to access the actAsAdminAction, so users can use the bar to change who they are acting as.
        friend DolphinViewContainer; // Allows the view container to access the actAsAdminAction, so the action can be shown to users when they are trying to
                                     // view a folder for which they are lacking read permissions.
    };

private:
    WorkerIntegration(DolphinMainWindow *parent, QAction *actAsAdminAction);

    /**
     * Toggles between acting with admin rights or not.
     * This enables editing more files than the normal user account would be allowed to but requires re-authorization.
     */
    void toggleActAsAdmin();

    /** Updates the toggled/checked state of the action depending on the state of the currently active view. */
    static void updateActAsAdminAction();

private:
    /** @see createActAsAdminAction() */
    QAction *const m_actAsAdminAction = nullptr;
};

}

#endif // ADMINWORKERINTEGRATION_H
