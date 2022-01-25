/*
 * SPDX-FileCopyrightText: 2007-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TERMINALPANEL_H
#define TERMINALPANEL_H

#include "kiofuse_interface.h"
#include "panels/panel.h"

#include <QQueue>

class TerminalInterface;
class KActionCollection;
class KMessageWidget;
class QVBoxLayout;
class QWidget;

namespace KIO {
    class StatJob;
}

namespace KParts {
    class ReadOnlyPart;
}
class KJob;
/**
 * @brief Shows the terminal which is synchronized with the URL of the
 *        active view.
 */
class TerminalPanel : public Panel
{
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel() override;

    /**
     * @brief This function is used to set the terminal panels's cwd to
     *        home when an unmounting request is received.
     */
    void goHome();
    QString currentWorkingDirectory();
    bool isHiddenInVisibleWindow() const;
    bool terminalHasFocus() const;
    bool hasProgramRunning() const;
    QString runningProgramName() const;
    KActionCollection *actionCollection();

public Q_SLOTS:
    void terminalExited();
    void dockVisibilityChanged();

Q_SIGNALS:
    void hideTerminalPanel();

    /**
     * Is emitted if the an URL change is requested.
     */
    void changeUrl(const QUrl& url);

protected:
    bool urlChanged() override;

    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    void slotMostLocalUrlResult(KJob* job);
    void slotKonsolePartCurrentDirectoryChanged(const QString& dir);

private:
    enum class HistoryPolicy {
        AddToHistory,
        SkipHistory
    };

    void changeDir(const QUrl& url);
    void sendCdToTerminal(const QString& path, HistoryPolicy addToHistory = HistoryPolicy::AddToHistory);
    void sendCdToTerminalKIOFuse(const QUrl &url);
private:
    bool m_clearTerminal;
    KIO::StatJob* m_mostLocalUrlJob;

    QVBoxLayout* m_layout;
    TerminalInterface* m_terminal;
    QWidget* m_terminalWidget;
    KMessageWidget* m_konsolePartMissingMessage;
    KParts::ReadOnlyPart* m_konsolePart;
    QString m_konsolePartCurrentDirectory;
    QQueue<QString> m_sendCdToTerminalHistory;
    org::kde::KIOFuse::VFS m_kiofuseInterface;
};

#endif // TERMINALPANEL_H
