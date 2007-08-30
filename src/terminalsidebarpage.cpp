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

#include "terminalsidebarpage.h"

#include <klibloader.h>
#include <kde_terminal_interface.h>
#include <kparts/part.h>
#include <kshell.h>

#include <QBoxLayout>
#include <QShowEvent>

TerminalSidebarPage::TerminalSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_layout(0),
    m_terminal(0)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setMargin(0);
}

TerminalSidebarPage::~TerminalSidebarPage()
{
}

QSize TerminalSidebarPage::sizeHint() const
{
    QSize size = SidebarPage::sizeHint();
    size.setHeight(200);
    return size;
}

void TerminalSidebarPage::setUrl(const KUrl& url)
{
    if (!SidebarPage::url().equals(url, KUrl::CompareWithoutTrailingSlash)) {
        SidebarPage::setUrl(url);
        if ((m_terminal != 0) && isVisible()) {
            m_terminal->sendInput("cd " + KShell::quoteArg(url.path()) + '\n');
        }
    }
}

void TerminalSidebarPage::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        SidebarPage::showEvent(event);
        return;
    }

    if (m_terminal == 0) {
        KPluginFactory* factory = KLibLoader::self()->factory("libkonsolepart");
        KParts::ReadOnlyPart* part = factory ? (factory->create<KParts::ReadOnlyPart>(this)) : 0;
        if (part != 0) {
            m_layout->addWidget(part->widget());
            m_terminal = qobject_cast<TerminalInterface *>(part);
        }
    }
    if (m_terminal != 0) {
        m_terminal->showShellInDir(url().path());
        m_terminal->sendInput("clear\n");
    }

    SidebarPage::showEvent(event);
}

#include "terminalsidebarpage.moc"
