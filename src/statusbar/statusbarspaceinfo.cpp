/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "statusbarspaceinfo.h"

#include "spaceinfoobserver.h"

#include <KIO/ApplicationLauncherJob>
#include <KIO/Global>
#include <KLocalizedString>
#include <KService>

#include <QMenu>
#include <QMouseEvent>
#include <QStorageInfo>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget *parent)
    : KCapacityBar(KCapacityBar::DrawTextInline, parent)
    , m_observer(nullptr)
{
    setCursor(Qt::PointingHandCursor);
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
            KCapacityBar::showEvent(event);
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
    KCapacityBar::hideEvent(event);
}

void StatusBarSpaceInfo::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Creates a menu with tools that help to find out more about free
        // disk space for the given url.

        const KService::Ptr filelight = KService::serviceByDesktopName(QStringLiteral("org.kde.filelight"));
        const KService::Ptr kdiskfree = KService::serviceByDesktopName(QStringLiteral("org.kde.kdf"));

        if (!filelight && !kdiskfree) {
            // nothing to show
            return;
        }

        QMenu *menu = new QMenu(this);

        if (filelight) {
            QAction *filelightFolderAction = menu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - current folder"));

            menu->connect(filelightFolderAction, &QAction::triggered, menu, [this, filelight](bool) {
                auto *job = new KIO::ApplicationLauncherJob(filelight);
                job->setUrls({m_url});
                job->start();
            });

            // For remote URLs like FTP analyzing the device makes no sense
            if (m_url.isLocalFile()) {
                QAction *filelightDiskAction = menu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - current device"));

                menu->connect(filelightDiskAction, &QAction::triggered, menu, [this, filelight](bool) {
                    const QStorageInfo info(m_url.toLocalFile());

                    if (info.isValid() && info.isReady()) {
                        auto *job = new KIO::ApplicationLauncherJob(filelight);
                        job->setUrls({QUrl::fromLocalFile(info.rootPath())});
                        job->start();
                    }
                });
            }

            QAction *filelightAllAction = menu->addAction(QIcon::fromTheme(QStringLiteral("filelight")), i18n("Disk Usage Statistics - all devices"));

            menu->connect(filelightAllAction, &QAction::triggered, menu, [this, filelight](bool) {
                const QStorageInfo info(m_url.toLocalFile());

                if (info.isValid() && info.isReady()) {
                    auto *job = new KIO::ApplicationLauncherJob(filelight);
                    job->start();
                }
            });
        }

        if (kdiskfree) {
            QAction *kdiskfreeAction = menu->addAction(QIcon::fromTheme(QStringLiteral("kdf")), i18n("KDiskFree"));

            connect(kdiskfreeAction, &QAction::triggered, this, [kdiskfree] {
                auto *job = new KIO::ApplicationLauncherJob(kdiskfree);
                job->start();
            });
        }

        menu->exec(QCursor::pos());
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

    setText(i18nc("@info:status Free disk space", "%1 free", KIO::convertSize(available)));
    setToolTip(i18nc("tooltip:status Free disk space", "%1 free out of %2 (%3% used)", KIO::convertSize(available), KIO::convertSize(size), percentUsed));
    setUpdatesEnabled(false);
    setValue(percentUsed);
    setUpdatesEnabled(true);

    if (!isVisible()) {
        show();
    } else {
        update();
    }
}

#include "moc_statusbarspaceinfo.cpp"
