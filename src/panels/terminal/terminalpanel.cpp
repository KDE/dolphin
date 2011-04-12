/***************************************************************************
 *   Copyright (C) 2007-2010 by Peter Penz <peter.penz@gmx.at>             *
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

#include "terminalpanel.h"

#include <signal.h>

#include <kpluginloader.h>
#include <kpluginfactory.h>
#include <kde_terminal_interface_v2.h>
#include <kparts/part.h>
#include <kshell.h>
#include <kio/job.h>
#include <KIO/JobUiDelegate>

#include <QBoxLayout>
#include <QShowEvent>

TerminalPanel::TerminalPanel(QWidget* parent) :
    Panel(parent),
    m_clearTerminal(true),
    m_mostLocalUrlJob(0),
    m_layout(0),
    m_terminal(0),
    m_terminalWidget(0)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setMargin(0);
}

TerminalPanel::~TerminalPanel()
{
}

void TerminalPanel::terminalExited()
{
    emit hideTerminalPanel();
    m_terminal = 0;
}

bool TerminalPanel::urlChanged()
{
    if (!url().isValid()) {
        return false;
    }

    const bool sendInput = (m_terminal != 0)
                           && (m_terminal->foregroundProcessId() == -1)
                           && isVisible();
    if (sendInput) {
        changeDir(url());
    }

    return true;
}

void TerminalPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (m_terminal == 0) {
        m_clearTerminal = true;
        KPluginFactory* factory = KPluginLoader("libkonsolepart").factory();
        KParts::ReadOnlyPart* part = factory ? (factory->create<KParts::ReadOnlyPart>(this)) : 0;
        if (part != 0) {
            connect(part, SIGNAL(destroyed(QObject*)), this, SLOT(terminalExited()));
            m_terminalWidget = part->widget();
            m_layout->addWidget(m_terminalWidget);
            m_terminal = qobject_cast<TerminalInterfaceV2 *>(part);
        }
    }
    if (m_terminal != 0) {
        m_terminal->showShellInDir(url().toLocalFile());
        changeDir(url());
        m_terminalWidget->setFocus();
    }

    Panel::showEvent(event);
}

void TerminalPanel::changeDir(const KUrl& url)
{
    delete m_mostLocalUrlJob;
    m_mostLocalUrlJob = 0;

    if (url.isLocalFile()) {
        sendCdToTerminal(url.toLocalFile());
    } else {
        m_mostLocalUrlJob = KIO::mostLocalUrl(url, KIO::HideProgressInfo);
        m_mostLocalUrlJob->ui()->setWindow(this);
        connect(m_mostLocalUrlJob, SIGNAL(result(KJob*)), this, SLOT(slotMostLocalUrlResult(KJob*)));
    }
}

void TerminalPanel::sendCdToTerminal(const QString& dir)
{
    if (!m_clearTerminal) {
        // The TerminalV2 interface does not provide a way to delete the
        // current line before sending a new input. This is mandatory,
        // otherwise sending a 'cd x' to a existing 'rm -rf *' might
        // result in data loss. As workaround SIGINT is send.
        kill(m_terminal->terminalProcessId(), SIGINT);
    }

    m_terminal->sendInput("cd " + KShell::quoteArg(dir) + '\n');

    if (m_clearTerminal) {
        m_terminal->sendInput("clear\n");
        m_clearTerminal = false;
    }
}

void TerminalPanel::slotMostLocalUrlResult(KJob* job)
{
    KIO::StatJob* statJob = static_cast<KIO::StatJob *>(job);
    const KUrl url = statJob->mostLocalUrl();
    if (url.isLocalFile()) {
        sendCdToTerminal(url.toLocalFile());
    }

    m_mostLocalUrlJob = 0;
}

#include "terminalpanel.moc"
