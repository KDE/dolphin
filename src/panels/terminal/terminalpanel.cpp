/*
 * SPDX-FileCopyrightText: 2007-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "terminalpanel.h"

#include <KActionCollection>
#include <KIO/DesktopExecParser>
#include <KIO/Job>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageWidget>
#include <KMountPoint>
#include <KParts/ReadOnlyPart>
#include <KPluginFactory>
#include <KProtocolInfo>
#include <KShell>
#include <KXMLGUIBuilder>
#include <KXMLGUIFactory>
#include <kde_terminal_interface.h>

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

TerminalPanel::TerminalPanel(QWidget *parent)
    : Panel(parent)
    , m_clearTerminal(true)
    , m_mostLocalUrlJob(nullptr)
    , m_layout(nullptr)
    , m_terminal(nullptr)
    , m_terminalWidget(nullptr)
    , m_konsolePartMissingMessage(nullptr)
    , m_konsolePart(nullptr)
    , m_konsolePartCurrentDirectory()
    , m_sendCdToTerminalHistory()
    , m_kiofuseInterface(QStringLiteral("org.kde.KIOFuse"), QStringLiteral("/org/kde/KIOFuse"), QDBusConnection::sessionBus())
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

TerminalPanel::~TerminalPanel()
{
}

void TerminalPanel::goHome()
{
    sendCdToTerminal(QDir::homePath(), HistoryPolicy::SkipHistory);
}

QString TerminalPanel::currentWorkingDirectory()
{
    if (m_terminal) {
        return m_terminal->currentWorkingDirectory();
    }
    return QString();
}

void TerminalPanel::terminalExited()
{
    m_terminal = nullptr;
    Q_EMIT hideTerminalPanel();
}

bool TerminalPanel::isHiddenInVisibleWindow() const
{
    return parentWidget() && parentWidget()->isHidden();
}

void TerminalPanel::dockVisibilityChanged()
{
    // Only react when the DockWidget itself (not some parent) is hidden. This way we don't
    // respond when e.g. Dolphin is minimized.
    if (isHiddenInVisibleWindow() && m_terminal && !hasProgramRunning()) {
        // Make sure that the following "cd /" command will not affect the view.
        disconnect(m_konsolePart, SIGNAL(currentDirectoryChanged(QString)), this, SLOT(slotKonsolePartCurrentDirectoryChanged(QString)));

        // Make sure this terminal does not prevent unmounting any removable drives
        changeDir(QUrl::fromLocalFile(QStringLiteral("/")));

        // Because we have disconnected from the part's currentDirectoryChanged()
        // signal, we have to update m_konsolePartCurrentDirectory manually. If this
        // was not done, showing the panel again might not set the part's working
        // directory correctly.
        m_konsolePartCurrentDirectory = '/';
    }
}

QString TerminalPanel::runningProgramName() const
{
    return m_terminal ? m_terminal->foregroundProcessName() : QString();
}

KActionCollection *TerminalPanel::actionCollection()
{
    // m_terminal is the only reference reset to nullptr in case the terminal is
    // closed again
    if (m_terminal && m_konsolePart && m_terminalWidget) {
        const auto guiClients = m_konsolePart->childClients();
        for (auto *client : guiClients) {
            if (client->actionCollection()->associatedWidgets().contains(m_terminalWidget)) {
                return client->actionCollection();
            }
        }
    }
    return nullptr;
}

bool TerminalPanel::hasProgramRunning() const
{
    return m_terminal && (m_terminal->foregroundProcessId() != -1);
}

bool TerminalPanel::urlChanged()
{
    if (!url().isValid()) {
        return false;
    }

    const bool sendInput = m_terminal && !hasProgramRunning() && isVisible();
    if (sendInput) {
        changeDir(url());
    }

    return true;
}

void TerminalPanel::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (!m_terminal) {
        m_clearTerminal = true;
        KPluginFactory *factory = KPluginFactory::loadFactory(KPluginMetaData(QStringLiteral("konsolepart"))).plugin;
        m_konsolePart = factory ? (factory->create<KParts::ReadOnlyPart>(this)) : nullptr;
        if (m_konsolePart) {
            connect(m_konsolePart, &KParts::ReadOnlyPart::destroyed, this, &TerminalPanel::terminalExited);
            m_terminalWidget = m_konsolePart->widget();
            setFocusProxy(m_terminalWidget);
            m_layout->addWidget(m_terminalWidget);
            if (m_konsolePartMissingMessage) {
                m_layout->removeWidget(m_konsolePartMissingMessage);
            }
            m_terminal = qobject_cast<TerminalInterface *>(m_konsolePart);

            // needed to collect the correct KonsolePart actionCollection
            // namely the one of the single inner terminal and not the outer KonsolePart
            if (!m_konsolePart->factory() && m_terminalWidget) {
                if (!m_konsolePart->clientBuilder()) {
                    m_konsolePart->setClientBuilder(new KXMLGUIBuilder(m_terminalWidget));
                }

                auto factory = new KXMLGUIFactory(m_konsolePart->clientBuilder(), this);
                factory->addClient(m_konsolePart);

                // Prevents the KXMLGui warning about removing the client
                connect(m_terminalWidget, &QObject::destroyed, this, [factory, this] {
                    factory->removeClient(m_konsolePart);
                });
            }

        } else if (!m_konsolePartMissingMessage) {
            const auto konsoleInstallUrl = QUrl("appstream://org.kde.konsole.desktop");
            const auto konsoleNotInstalledText = i18n(
                "Terminal cannot be shown because Konsole is not installed. "
                "Please install it and then reopen the panel.");
            m_konsolePartMissingMessage = new KMessageWidget(konsoleNotInstalledText, this);
            m_konsolePartMissingMessage->setCloseButtonVisible(false);
            m_konsolePartMissingMessage->hide();
            if (KIO::DesktopExecParser::hasSchemeHandler(konsoleInstallUrl)) {
                auto installKonsoleAction = new QAction(i18n("Install Konsole"), this);
                connect(installKonsoleAction, &QAction::triggered, [konsoleInstallUrl]() {
                    QDesktopServices::openUrl(konsoleInstallUrl);
                });
                m_konsolePartMissingMessage->addAction(installKonsoleAction);
            }
            m_layout->addWidget(m_konsolePartMissingMessage);
            m_layout->addStretch();
            QTimer::singleShot(0, m_konsolePartMissingMessage, &KMessageWidget::animatedShow);
        } else {
            m_konsolePartMissingMessage->animatedShow();
        }
    }
    if (m_terminal) {
        m_terminal->showShellInDir(url().toLocalFile());
        if (!hasProgramRunning()) {
            changeDir(url());
        }
        m_terminalWidget->setFocus();
        connect(m_konsolePart, SIGNAL(currentDirectoryChanged(QString)), this, SLOT(slotKonsolePartCurrentDirectoryChanged(QString)));
    }

    Panel::showEvent(event);
}

void TerminalPanel::changeDir(const QUrl &url)
{
    delete m_mostLocalUrlJob;
    m_mostLocalUrlJob = nullptr;

    if (url.isLocalFile()) {
        sendCdToTerminal(url.toLocalFile());
        return;
    }

    // Try stat'ing the url; note that mostLocalUrl only works with ":local" protocols
    if (KProtocolInfo::protocolClass(url.scheme()) == QLatin1String(":local")) {
        m_mostLocalUrlJob = KIO::mostLocalUrl(url, KIO::HideProgressInfo);
        if (m_mostLocalUrlJob->uiDelegate()) {
            KJobWidgets::setWindow(m_mostLocalUrlJob, this);
        }
        connect(m_mostLocalUrlJob, &KIO::StatJob::result, this, &TerminalPanel::slotMostLocalUrlResult);
        return;
    }

    // Last chance, try KIOFuse
    sendCdToTerminalKIOFuse(url);
}

void TerminalPanel::sendCdToTerminal(const QString &dir, HistoryPolicy addToHistory)
{
    if (dir == m_konsolePartCurrentDirectory // We are already there
        && m_sendCdToTerminalHistory.isEmpty() // …and that is not because the terminal couldn't keep up
    ) {
        m_clearTerminal = false;
        return;
    }

    // Send prior Ctrl-E, Ctrl-U to ensure the line is empty. This is
    // mandatory, otherwise sending a 'cd x\n' to a prompt with 'rm -rf *'
    // would result in data loss.
    m_terminal->sendInput(QStringLiteral("\x05\x15"));

    // We want to ignore the currentDirectoryChanged(QString) signal, which we will receive after
    // the directory change, because this directory change is not caused by a "cd" command that the
    // user entered in the panel. Therefore, we have to remember 'dir'. Note that it could also be
    // a symbolic link -> remember the 'canonical' path.
    if (addToHistory == HistoryPolicy::AddToHistory)
        m_sendCdToTerminalHistory.enqueue(QDir(dir).canonicalPath());

    m_terminal->sendInput(" cd " + KShell::quoteArg(dir) + '\r');

    if (m_clearTerminal) {
        m_terminal->sendInput(QStringLiteral(" clear\r"));
        m_clearTerminal = false;
    }
}

void TerminalPanel::sendCdToTerminalKIOFuse(const QUrl &url)
{
    // URL isn't local, only hope for the terminal to be in sync with the
    // DolphinView is to mount the remote URL in KIOFuse and point to it.
    // If we can't do that for any reason, silently fail.
    auto reply = m_kiofuseInterface.mountUrl(url.toString());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (!reply.isError()) {
            // Successfully mounted, point to the KIOFuse equivalent path.
            sendCdToTerminal(reply.value());
        }
    });
}

void TerminalPanel::slotMostLocalUrlResult(KJob *job)
{
    KIO::StatJob *statJob = static_cast<KIO::StatJob *>(job);
    const QUrl url = statJob->mostLocalUrl();
    if (url.isLocalFile()) {
        sendCdToTerminal(url.toLocalFile());
    } else {
        sendCdToTerminalKIOFuse(url);
    }

    m_mostLocalUrlJob = nullptr;
}

void TerminalPanel::slotKonsolePartCurrentDirectoryChanged(const QString &dir)
{
    m_konsolePartCurrentDirectory = QDir(dir).canonicalPath();

    // Only emit a changeUrl signal if the directory change was caused by the user inside the
    // terminal, and not by sendCdToTerminal(QString).
    while (!m_sendCdToTerminalHistory.empty()) {
        if (m_konsolePartCurrentDirectory == m_sendCdToTerminalHistory.dequeue()) {
            return;
        }
    }

    // User may potentially be browsing inside a KIOFuse mount.
    // If so lets try and change the DolphinView to point to the remote URL equivalent.
    // instead of into the KIOFuse mount itself (which can cause performance issues!)
    const QUrl url(QUrl::fromLocalFile(dir));

    KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByPath(m_konsolePartCurrentDirectory);
    if (mountPoint && mountPoint->mountType() != QStringLiteral("fuse.kio-fuse")) {
        // Not in KIOFUse mount, so just switch to the corresponding URL.
        Q_EMIT changeUrl(url);
        return;
    }

    auto reply = m_kiofuseInterface.remoteUrl(m_konsolePartCurrentDirectory);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (reply.isError()) {
            // KIOFuse errored out... just show the normal URL
            Q_EMIT changeUrl(url);
        } else {
            // Our location happens to be in a KIOFuse mount and is mounted.
            // Let's change the DolphinView to point to the remote URL equivalent.
            Q_EMIT changeUrl(QUrl::fromUserInput(reply.value()));
        }
    });
}

bool TerminalPanel::terminalHasFocus() const
{
    if (m_terminalWidget) {
        return m_terminalWidget->hasFocus();
    }

    return hasFocus();
}
