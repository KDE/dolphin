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

class TerminalInterfaceV2;
class QVBoxLayout;
class QWidget;

namespace KIO {
    class StatJob;
}

/**
 * @brief Shows the terminal which is synchronized with the URL of the
 *        active view.
 */
class TerminalPanel : public Panel
{
    Q_OBJECT

public:
    TerminalPanel(QWidget* parent = 0);
    virtual ~TerminalPanel();

public slots:
    void terminalExited();
    void dockVisibilityChanged();

signals:
    void hideTerminalPanel();

protected:
    /** @see Panel::urlChanged() */
    virtual bool urlChanged();

    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

private slots:
    void slotMostLocalUrlResult(KJob* job);

private:
    void changeDir(const KUrl& url);
    void sendCdToTerminal(const QString& path);

private:
    bool m_clearTerminal;
    KIO::StatJob* m_mostLocalUrlJob;

    QVBoxLayout* m_layout;
    TerminalInterfaceV2* m_terminal;
    QWidget* m_terminalWidget;
};

#endif // TERMINALPANEL_H
