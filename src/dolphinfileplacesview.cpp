/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphinfileplacesview.h"
#include "draganddrophelper.h"
#include <kfileitem.h>
#include <konq_operations.h>

DolphinFilePlacesView::DolphinFilePlacesView(QWidget* parent) :
    KFilePlacesView(parent),
    m_mouseButtons(Qt::NoButton)
{
    setDropOnPlaceEnabled(true);
    connect(this, SIGNAL(urlsDropped(const KUrl&, QDropEvent*, QWidget*)),
            this, SLOT(slotUrlsDropped(const KUrl&, QDropEvent*, QWidget*)));
    connect(this, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(emitExtendedUrlChangedSignal(const KUrl&)));
}

DolphinFilePlacesView::~DolphinFilePlacesView()
{
}

void DolphinFilePlacesView::mousePressEvent(QMouseEvent* event)
{
    m_mouseButtons = event->buttons();
    KFilePlacesView::mousePressEvent(event);
}

void DolphinFilePlacesView::slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent)
{
    DragAndDropHelper::dropUrls(KFileItem(), dest, event, parent);
}

void DolphinFilePlacesView::emitExtendedUrlChangedSignal(const KUrl& url)
{
    emit urlChanged(url, m_mouseButtons);
}

#include "dolphinfileplacesview.moc"
