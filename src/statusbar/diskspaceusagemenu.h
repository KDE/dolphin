/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DISKSPACEUSAGEMENU_H
#define DISKSPACEUSAGEMENU_H

#include <KMessageWidget>

#include <QMenu>
#include <QPointer>
#include <QUrl>

class QShowEvent;
class QWidgetAction;

/**
 * A menu that allows launching tools to view disk usage statistics like Filelight and KDiskFree when those are installed.
 * If none are installed, this menu instead allows installing Filelight.
 */
class DiskSpaceUsageMenu : public QMenu
{
    Q_OBJECT

public:
    explicit DiskSpaceUsageMenu(QWidget *parent);

    inline void setUrl(const QUrl &url)
    {
        m_url = url;
    };

Q_SIGNALS:
    /**
     * Requests for @p message with the given @p messageType to be shown to the user in a non-modal way.
     */
    void showMessage(const QString &message, KMessageWidget::MessageType messageType);

    /**
     * Requests for a progress update to be shown to the user in a non-modal way.
     * @param currentlyRunningTaskTitle     The task that is currently progressing.
     * @param installationProgressPercent   The current percentage of completion.
     */
    void showInstallationProgress(const QString &currentlyRunningTaskTitle, int installationProgressPercent);

private Q_SLOTS:
    /**
     * Asynchronously starts a Filelight installation using DolphinPackageInstaller. @see DolphinPackageInstaller.
     * Installation success or failure is reported through showMessage(). @see StatusBarSpaceInfo::showMessage().
     * Installation progress is reported through showInstallationProgress(). @see StatusBarSpaceInfo::showInstallationProgress().
     */
    void slotInstallFilelightButtonClicked();

    void updateMenu();

protected:
    /** Moves keyboard focus to the "Install Filelight" button if the Installation UI is shown. */
    void showEvent(QShowEvent *event) override;

private:
    /**
     * Creates a new QWidgetAction that contains a UI to install Filelight.
     * m_installFilelightWidgetAction is initialised after calling this method once.
     */
    void initialiseInstallFilelightWidgetAction();

private:
    /** An action containing a UI to install Filelight. */
    QPointer<QWidgetAction> m_installFilelightWidgetAction = nullptr;
    /** The current url of the view. Filelight can be launched to show this directory. */
    QUrl m_url;
};

#endif // DISKSPACEUSAGEMENU_H
