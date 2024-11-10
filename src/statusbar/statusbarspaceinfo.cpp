/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "statusbarspaceinfo.h"

#include "config-dolphin.h"
#include "dolphinpackageinstaller.h"
#include "global.h"
#include "spaceinfoobserver.h"

#include <KCapacityBar>
#include <KIO/ApplicationLauncherJob>
#include <KIO/Global>
#include <KLocalizedString>
#include <KService>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QStorageInfo>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget *parent)
    : QWidget(parent)
    , m_observer(nullptr)
    , m_installFilelightWidgetAction{nullptr}
    , m_hasSpaceInfo{false}
    , m_shown{false}
{
    hide(); // Only become visible when we have space info to show. @see StatusBarSpaceInfo::setShown().

    m_capacityBar = new KCapacityBar(KCapacityBar::DrawTextInline, this);
    m_textInfoButton = new QToolButton(this);
    m_textInfoButton->setAutoRaise(true);
    m_textInfoButton->setPopupMode(QToolButton::InstantPopup);
    m_buttonMenu = new QMenu(this);
    m_textInfoButton->setMenu(m_buttonMenu);
    connect(m_buttonMenu, &QMenu::aboutToShow, this, &StatusBarSpaceInfo::updateMenu);

    auto layout = new QHBoxLayout(this);
    // We reduce the outside margin of the flat button so it visually has the same margin as the status bar text label on the other end of the bar.
    layout->setContentsMargins(2, -1, 0, -1); // "-1" makes it so the fixed height won't be ignored.
    layout->addWidget(m_capacityBar);
    layout->addWidget(m_textInfoButton);
}

StatusBarSpaceInfo::~StatusBarSpaceInfo()
{
}

void StatusBarSpaceInfo::setShown(bool shown)
{
    m_shown = shown;
    if (!m_shown) {
        hide();
        return;
    }

    // We only show() this widget in slotValueChanged() when it m_hasSpaceInfo.
    if (m_observer.isNull()) {
        m_observer.reset(new SpaceInfoObserver(m_url, this));
        connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
    }

    if (m_hasSpaceInfo) {
        slotValuesChanged();
    }
}

void StatusBarSpaceInfo::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        m_hasSpaceInfo = false;
        if (m_observer) {
            m_observer.reset(new SpaceInfoObserver(m_url, this));
            connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
        }
    }
}

QUrl StatusBarSpaceInfo::url() const
{
    return m_url;
}

void StatusBarSpaceInfo::update()
{
    if (m_observer) {
        m_observer->update();
    }
}

void StatusBarSpaceInfo::showEvent(QShowEvent *event)
{
    if (m_shown && m_observer.isNull()) {
        m_observer.reset(new SpaceInfoObserver(m_url, this));
        connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
    }
    QWidget::showEvent(event);
}

void StatusBarSpaceInfo::hideEvent(QHideEvent *event)
{
    if (m_hasSpaceInfo) {
        m_observer.reset();
        m_hasSpaceInfo = false;
    }
    QWidget::hideEvent(event);
}

QSize StatusBarSpaceInfo::minimumSizeHint() const
{
    return QSize();
}

void StatusBarSpaceInfo::updateMenu()
{
    m_buttonMenu->clear();

    // Creates a menu with tools that help to find out more about free
    // disk space for the given url.

    const KService::Ptr filelight = KService::serviceByDesktopName(QStringLiteral("org.kde.filelight"));
    const KService::Ptr kdiskfree = KService::serviceByDesktopName(QStringLiteral("org.kde.kdf"));

    if (!filelight && !kdiskfree) {
        // Show an UI to install a tool to free up disk space because this is what a user pressing on a "free space" button would want.
        if (!m_installFilelightWidgetAction) {
            initialiseInstallFilelightWidgetAction();
        }
        m_buttonMenu->addAction(m_installFilelightWidgetAction);
        return;
    }

    if (filelight) {
        QAction *filelightFolderAction = m_buttonMenu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - current folder"));

        m_buttonMenu->connect(filelightFolderAction, &QAction::triggered, m_buttonMenu, [this, filelight](bool) {
            auto *job = new KIO::ApplicationLauncherJob(filelight);
            job->setUrls({m_url});
            job->start();
        });

        // For remote URLs like FTP analyzing the device makes no sense
        if (m_url.isLocalFile()) {
            QAction *filelightDiskAction =
                m_buttonMenu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - current device"));

            m_buttonMenu->connect(filelightDiskAction, &QAction::triggered, m_buttonMenu, [this, filelight](bool) {
                const QStorageInfo info(m_url.toLocalFile());

                if (info.isValid() && info.isReady()) {
                    auto *job = new KIO::ApplicationLauncherJob(filelight);
                    job->setUrls({QUrl::fromLocalFile(info.rootPath())});
                    job->start();
                }
            });
        }

        QAction *filelightAllAction = m_buttonMenu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - all devices"));

        m_buttonMenu->connect(filelightAllAction, &QAction::triggered, m_buttonMenu, [this, filelight](bool) {
            const QStorageInfo info(m_url.toLocalFile());

            if (info.isValid() && info.isReady()) {
                auto *job = new KIO::ApplicationLauncherJob(filelight);
                job->start();
            }
        });
    }

    if (kdiskfree) {
        QAction *kdiskfreeAction = m_buttonMenu->addAction(QIcon::fromTheme(QStringLiteral("kdf")), i18n("KDiskFree"));

        connect(kdiskfreeAction, &QAction::triggered, this, [kdiskfree] {
            auto *job = new KIO::ApplicationLauncherJob(kdiskfree);
            job->start();
        });
    }
}

void StatusBarSpaceInfo::slotInstallFilelightButtonClicked()
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
            if (m_textInfoButton->menu()->isVisible()) {
                m_textInfoButton->menu()->hide();
                updateMenu();
                m_textInfoButton->menu()->show();
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

void StatusBarSpaceInfo::slotValuesChanged()
{
    Q_ASSERT(m_observer);
    const quint64 size = m_observer->size();

    if (!m_shown || size == 0) {
        hide();
        return;
    }

    m_hasSpaceInfo = true;

    const quint64 available = m_observer->available();
    const quint64 used = size - available;
    const int percentUsed = qRound(100.0 * qreal(used) / qreal(size));

    m_textInfoButton->setText(i18nc("@info:status Free disk space", "%1 free", KIO::convertSize(available)));
    setToolTip(i18nc("tooltip:status Free disk space", "%1 free out of %2 (%3% used)", KIO::convertSize(available), KIO::convertSize(size), percentUsed));
    m_textInfoButton->setToolTip(i18nc("@info:tooltip for the free disk space button",
                                       "%1 free out of %2 (%3% used)\nPress to manage disk space usage.",
                                       KIO::convertSize(available),
                                       KIO::convertSize(size),
                                       percentUsed));
    setUpdatesEnabled(false);
    m_capacityBar->setValue(percentUsed);
    setUpdatesEnabled(true);

    if (!isVisible()) {
        show();
    } else {
        update();
    }
}

void StatusBarSpaceInfo::initialiseInstallFilelightWidgetAction()
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
    m_buttonMenu->setFocusProxy(installFilelightButton);
    containerWidget->setFocusPolicy(Qt::TabFocus);
    containerWidget->setFocusProxy(installFilelightButton);
    installFilelightButton->setAccessibleDescription(installFilelightBody->text());
    connect(installFilelightButton, &QAbstractButton::clicked, this, &StatusBarSpaceInfo::slotInstallFilelightButtonClicked);

    m_installFilelightWidgetAction = new QWidgetAction{this};
    m_installFilelightWidgetAction->setDefaultWidget(containerWidget); // transfers ownership of containerWidget
}

#include "moc_statusbarspaceinfo.cpp"
