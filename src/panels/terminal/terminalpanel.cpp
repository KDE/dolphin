/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#include <klibloader.h>
#include <kde_terminal_interface_v2.h>
#include <kparts/part.h>
#include <kshell.h>
#include <kio/netaccess.h>

#include <QBoxLayout>
#include <QShowEvent>

#include <kdebug.h>

TerminalPanel::TerminalPanel(QWidget* parent) :
    Panel(parent),
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

QSize TerminalPanel::sizeHint() const
{
    QSize size = Panel::sizeHint();
    size.setHeight(200);
    return size;
}

void TerminalPanel::setUrl(const KUrl& url)
{
    if (!url.isValid() || (url == Panel::url())) {
        return;
    }

    Panel::setUrl(url);
    KUrl mostLocalUrl = KIO::NetAccess::mostLocalUrl(url, 0);
    const bool sendInput = (m_terminal != 0)
                           && (m_terminal->foregroundProcessId() == -1)
                           && isVisible()
                           && mostLocalUrl.isLocalFile();
    if (sendInput) {
        m_terminal->sendInput("cd " + KShell::quoteArg(mostLocalUrl.toLocalFile()) + '\n');
    }

}

void TerminalPanel::terminalExited()
{
    emit hideTerminalPanel();
    m_terminal = 0;
}

void TerminalPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (m_terminal == 0) {
        KPluginFactory* factory = KPluginLoader("libkonsolepart").factory();
        KParts::ReadOnlyPart* part = factory ? (factory->create<KParts::ReadOnlyPart>(this)) : 0;
        if (part != 0) {
            connect(part, SIGNAL(destroyed(QObject*)), this, SLOT(terminalExited()));
            m_terminalWidget = part->widget();
            m_layout->addWidget(m_terminalWidget);
            m_terminal = qobject_cast<TerminalInterfaceV2 *>(part);
            m_terminal->showShellInDir(url().path());

        }        
    }
    if (m_terminal != 0) {
        m_terminal->sendInput("cd " + KShell::quoteArg(url().path()) + '\n');
        m_terminal->sendInput("clear\n");
        m_terminalWidget->setFocus();
    }

    Panel::showEvent(event);
}

#include "terminalpanel.moc"
