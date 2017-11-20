/***************************************************************************
 *   Copyright (C) 2007-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef TERMINALPANEL_H
#define TERMINALPANEL_H

#include <panels/panel.h>

#include <QQueue>

class TerminalInterface;
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
     *        home when an unmounting request is receieved.
     */
    void goHome();
    QString currentWorkingDirectory();

public slots:
    void terminalExited();
    void dockVisibilityChanged();

signals:
    void hideTerminalPanel();

    /**
     * Is emitted if the an URL change is requested.
     */
    void changeUrl(const QUrl& url);

protected:
    bool urlChanged() override;

    void showEvent(QShowEvent* event) override;

private slots:
    void slotMostLocalUrlResult(KJob* job);
    void slotKonsolePartCurrentDirectoryChanged(const QString& dir);

private:
    enum class HistoryPolicy {
        AddToHistory,
        SkipHistory
    };

    void changeDir(const QUrl& url);
    void sendCdToTerminal(const QString& path, HistoryPolicy addToHistory = HistoryPolicy::AddToHistory);

private:
    bool m_clearTerminal;
    KIO::StatJob* m_mostLocalUrlJob;

    QVBoxLayout* m_layout;
    TerminalInterface* m_terminal;
    QWidget* m_terminalWidget;
    KParts::ReadOnlyPart* m_konsolePart;
    QString m_konsolePartCurrentDirectory;
    QQueue<QString> m_sendCdToTerminalHistory;
};

#endif // TERMINALPANEL_H
