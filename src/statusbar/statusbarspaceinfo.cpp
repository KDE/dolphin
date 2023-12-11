/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "statusbarspaceinfo.h"

#include "spaceinfoobserver.h"

#include <KCapacityBar>
#include <KIO/ApplicationLauncherJob>
#include <KIO/Global>
#include <KLocalizedString>
#include <KService>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QStorageInfo>
#include <QToolButton>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget *parent)
    : QWidget(parent)
    , m_observer(nullptr)
{
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
        m_ready = false;
    }
}

void StatusBarSpaceInfo::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        m_ready = false;
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
    if (m_shown) {
        if (m_ready) {
            QWidget::showEvent(event);
        }

        if (m_observer.isNull()) {
            m_observer.reset(new SpaceInfoObserver(m_url, this));
            connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
        }
    }
}

void StatusBarSpaceInfo::hideEvent(QHideEvent *event)
{
    if (m_ready) {
        m_observer.reset();
        m_ready = false;
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
        QAction *installFilelight =
            m_buttonMenu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Install Filelight to View Disk Usage Statistics…"));

        connect(installFilelight, &QAction::triggered, this, [] {
#ifdef Q_OS_WIN
            QDesktopServices::openUrl(QUrl("https://apps.kde.org/filelight"));
#else
            QDesktopServices::openUrl(QUrl("appstream://org.kde.filelight.desktop"));
#endif
        });

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

void StatusBarSpaceInfo::slotValuesChanged()
{
    Q_ASSERT(m_observer);
    const quint64 size = m_observer->size();

    if (!m_shown || size == 0) {
        hide();
        return;
    }

    m_ready = true;

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

#include "moc_statusbarspaceinfo.cpp"
