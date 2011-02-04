/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "panel.h"
#include <KUrl>

Panel::Panel(QWidget* parent) :
    QWidget(parent),
    m_url(),
    m_customContextMenuActions()
{
}

Panel::~Panel()
{
}

KUrl Panel::url() const
{
    return m_url;
}

void Panel::setCustomContextMenuActions(const QList<QAction*>& actions)
{
    m_customContextMenuActions = actions;
}

QList<QAction*> Panel::customContextMenuActions() const
{
    return m_customContextMenuActions;
}

QSize Panel::sizeHint() const
{
    // The size hint will be requested already when starting Dolphin even
    // if the panel is invisible. For performance reasons most panels delay
    // the creation and initialization of widgets until a showEvent() is called.
    // Because of this the size-hint of the embedded widgets cannot be used
    // and a default size is provided:
    return QSize(180, 180);
}

void Panel::setUrl(const KUrl& url)
{
    if (url.equals(m_url, KUrl::CompareWithoutTrailingSlash)) {
        return;
    }

    const KUrl oldUrl = m_url;
    m_url = url;
    const bool accepted = urlChanged();
    if (!accepted) {
        m_url = oldUrl;
    }
}

#include "panel.moc"
