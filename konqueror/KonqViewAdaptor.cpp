/* This file is part of the KDE project
   Copyright 2000       Simon Hausmann <hausmann@kde.org>
   Copyright 2000, 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KonqViewAdaptor.h"
#include "konq_view.h"

KonqViewAdaptor::KonqViewAdaptor( KonqView * view )
    : m_pView ( view )
{
}

KonqViewAdaptor::~KonqViewAdaptor()
{
}

void KonqViewAdaptor::openUrl( const QString& url, const QString & locationBarURL, const QString & nameFilter )
{
    m_pView->openUrl( KUrl(url), locationBarURL, nameFilter );
}

bool KonqViewAdaptor::changeViewMode( const QString &serviceType,
                                      const QString &serviceName )
{
    return m_pView->changeViewMode( serviceType, serviceName );
}

void KonqViewAdaptor::lockHistory()

{
    m_pView->lockHistory();
}

void KonqViewAdaptor::stop()
{
    m_pView->stop();
}

QString KonqViewAdaptor::url()
{
    return m_pView->url().url();
}

QString KonqViewAdaptor::locationBarURL()
{
    return m_pView->locationBarURL();
}

QString KonqViewAdaptor::serviceType()
{
    return m_pView->serviceType();
}

QStringList KonqViewAdaptor::serviceTypes()
{
    return m_pView->serviceTypes();
}

QDBusObjectPath KonqViewAdaptor::part()
{
    return QDBusObjectPath( m_pView->partObjectPath() );
}

void KonqViewAdaptor::enablePopupMenu( bool b )
{
    m_pView->enablePopupMenu( b );
}

uint KonqViewAdaptor::historyLength()const
{
    return m_pView->historyLength();
}

bool KonqViewAdaptor::allowHTML() const
{
    return m_pView->allowHTML();
}

void KonqViewAdaptor::goForward()
{
    m_pView->go(-1);
}

void KonqViewAdaptor::goBack()
{
    m_pView->go(+1);
}

bool KonqViewAdaptor::isPopupMenuEnabled() const
{
    return m_pView->isPopupMenuEnabled();
}

bool KonqViewAdaptor::canGoBack()const
{
    return m_pView->canGoBack();
}

bool KonqViewAdaptor::canGoForward()const
{
    return m_pView->canGoForward();
}

#include "KonqViewAdaptor.moc"
