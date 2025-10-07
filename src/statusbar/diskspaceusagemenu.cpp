/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "diskspaceusagemenu.h"

#include "dolphinpackageinstaller.h"
#include "global.h"

#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KService>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QStorageInfo>
#include <QWidgetAction>

DiskSpaceUsageMenu::DiskSpaceUsageMenu(QWidget *parent)
    : QMenu{parent}
{
    connect(this, &QMenu::aboutToShow, this, &DiskSpaceUsageMenu::updateMenu);
}

void DiskSpaceUsageMenu::slotInstallFilelightButtonClicked()
{
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl("https://apps.kde.org/filelight"));
#else
    auto packageInstaller = new DolphinPackageInstaller(
        FILELIGHT_PACKAGE_NAME,
        QUrl("appstream://org.kde.filelight.desktop"),
        []() {
            return KService::serviceByDesktopName(QStringLiteral("org.kde.filelight"));
        },
        this);
    connect(packageInstaller, &KJob::result, this, [this](KJob *job) {
        Q_EMIT showInstallationProgress(QString(), 100); // Hides the progress information in the status bar.
        if (job->error()) {
            Q_EMIT showMessage(job->errorString(), KMessageWidget::Error);
        } else {
            Q_EMIT showMessage(xi18nc("@info", "<application>Filelight</application> installed successfully."), KMessageWidget::Positive);
            if (isVisible()) {
                hide();
                updateMenu();
                show();
            }
        }
    });
    const auto installationTaskText{i18nc("@info:status", "Installing Filelight…")};
    Q_EMIT showInstallationProgress(installationTaskText, -1);
    connect(packageInstaller, &KJob::percentChanged, this, [this, installationTaskText](KJob * /* job */, long unsigned int percent) {
        if (percent < 100) { // Ignore some weird reported values.
            Q_EMIT showInstallationProgress(installationTaskText, percent);
        }
    });
    packageInstaller->start();
#endif
}

void DiskSpaceUsageMenu::updateMenu()
{
    clear();

    // Creates a menu with tools that help to find out more about free
    // disk space for the given url.

    const KService::Ptr filelight = KService::serviceByDesktopName(QStringLiteral("org.kde.filelight"));
    const KService::Ptr kdiskfree = KService::serviceByDesktopName(QStringLiteral("org.kde.kdf"));

    if (!filelight && !kdiskfree) {
        // Show an UI to install a tool to free up disk space because this is what a user pressing on a "free space" button would want.
        if (!m_installFilelightWidgetAction) {
            initialiseInstallFilelightWidgetAction();
        }
        addAction(m_installFilelightWidgetAction);
        return;
    }

    if (filelight) {
        QAction *filelightFolderAction = addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - current folder"));

        connect(filelightFolderAction, &QAction::triggered, this, [this, filelight](bool) {
            auto *job = new KIO::ApplicationLauncherJob(filelight);
            job->setUrls({m_url});
            job->start();
        });

        // For remote URLs like FTP analyzing the device makes no sense
        if (m_url.isLocalFile()) {
            QAction *filelightDiskAction = addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - current device"));

            connect(filelightDiskAction, &QAction::triggered, this, [this, filelight](bool) {
                const QStorageInfo info(m_url.toLocalFile());

                if (info.isValid() && info.isReady()) {
                    auto *job = new KIO::ApplicationLauncherJob(filelight);
                    job->setUrls({QUrl::fromLocalFile(info.rootPath())});
                    job->start();
                }
            });
        }

        QAction *filelightAllAction = addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - all devices"));

        connect(filelightAllAction, &QAction::triggered, this, [this, filelight](bool) {
            const QStorageInfo info(m_url.toLocalFile());

            if (info.isValid() && info.isReady()) {
                auto *job = new KIO::ApplicationLauncherJob(filelight);
                job->start();
            }
        });
    }

    if (kdiskfree) {
        QAction *kdiskfreeAction = addAction(QIcon::fromTheme(QStringLiteral("kdf")), i18n("KDiskFree"));

        connect(kdiskfreeAction, &QAction::triggered, this, [kdiskfree] {
            auto *job = new KIO::ApplicationLauncherJob(kdiskfree);
            job->start();
        });
    }
}

void DiskSpaceUsageMenu::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !actions().isEmpty()) {
        auto widgetAction = qobject_cast<QWidgetAction *>(*actions().constBegin());
        if (widgetAction) {
            widgetAction->defaultWidget()->setFocus();
        }
    }
    QMenu::showEvent(event);
}

void DiskSpaceUsageMenu::initialiseInstallFilelightWidgetAction()
{
    Q_ASSERT(!m_installFilelightWidgetAction);

    auto containerWidget = new QWidget{this};
    containerWidget->setContentsMargins(Dolphin::VERTICAL_SPACER_HEIGHT,
                                        Dolphin::VERTICAL_SPACER_HEIGHT,
                                        Dolphin::VERTICAL_SPACER_HEIGHT, // Using the same value for every spacing in this containerWidget looks nice.
                                        Dolphin::VERTICAL_SPACER_HEIGHT);
    auto vLayout = new QVBoxLayout(containerWidget);

    auto installFilelightTitle = new QLabel(i18nc("@title", "Free Up Disk Space"), containerWidget);
    installFilelightTitle->setAlignment(Qt::AlignCenter);
    installFilelightTitle->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    QFont titleFont{installFilelightTitle->font()};
    titleFont.setPointSize(titleFont.pointSize() + 2);
    installFilelightTitle->setFont(titleFont);
    vLayout->addWidget(installFilelightTitle);

    vLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);

    auto installFilelightBody =
        // i18n: The new line ("<nl/>") tag is only there to format this text visually pleasing, i.e. to avoid having one very long line.
        new QLabel(xi18nc("@title", "<para>Install additional software to view disk usage statistics<nl/>and identify big files and folders.</para>"),
                   containerWidget);
    installFilelightBody->setAlignment(Qt::AlignCenter);
    installFilelightBody->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    vLayout->addWidget(installFilelightBody);

    vLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);

    auto installFilelightButton =
        new QPushButton(QIcon::fromTheme(QStringLiteral("filelight")), i18nc("@action:button", "Install Filelight…"), containerWidget);
    installFilelightButton->setMinimumWidth(std::max(installFilelightButton->sizeHint().width(), installFilelightTitle->sizeHint().width()));
    auto buttonLayout = new QHBoxLayout; // The parent is automatically set on addLayout() below.
    buttonLayout->addWidget(installFilelightButton, 0, Qt::AlignHCenter);
    vLayout->addLayout(buttonLayout);

    // Make sure one Tab press focuses the button after the UI opened.
    setFocusProxy(installFilelightButton);
    containerWidget->setFocusPolicy(Qt::TabFocus);
    containerWidget->setFocusProxy(installFilelightButton);
    installFilelightButton->setAccessibleDescription(installFilelightBody->text());
    connect(installFilelightButton, &QAbstractButton::clicked, this, &DiskSpaceUsageMenu::slotInstallFilelightButtonClicked);

    m_installFilelightWidgetAction = new QWidgetAction{this};
    m_installFilelightWidgetAction->setDefaultWidget(containerWidget); // transfers ownership of containerWidget
}
