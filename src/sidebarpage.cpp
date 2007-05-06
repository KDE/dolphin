/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "sidebarpage.h"
#include <QWidget>
#include <kfileitem.h>
#include <kurl.h>

SidebarPage::SidebarPage(QWidget* parent) :
    QWidget(parent),
    m_url(KUrl()),
    m_currentSelection(KFileItemList())
{
}

SidebarPage::~SidebarPage()
{
}

const KUrl& SidebarPage::url() const
{
    return m_url;
}

const KFileItemList& SidebarPage::selection() const
{
    return m_currentSelection;
}

void SidebarPage::setUrl(const KUrl& url)
{
    m_url = url;
}

void SidebarPage::setSelection(const KFileItemList& selection)
{
    m_currentSelection = selection;
}

#include "sidebarpage.moc"
