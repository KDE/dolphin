/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "KonqViewIface.h"
#include "konq_view.h"

#include <kapplication.h>
#include <dcopclient.h>

KonqViewIface::KonqViewIface( KonqView * view )
  : DCOPObject( view->name() ), m_pView ( view )
{
}

KonqViewIface::~KonqViewIface()
{
}

void KonqViewIface::openURL( QString url, const QString & locationBarURL, const QString & nameFilter )
{
  KURL u(url);
  m_pView->openURL( u, locationBarURL, nameFilter );
}

bool KonqViewIface::changeViewMode( const QString &serviceType,
                                    const QString &serviceName )
{
  return m_pView->changeViewMode( serviceType, serviceName );
}

void KonqViewIface::lockHistory()

{
  m_pView->lockHistory();
}

void KonqViewIface::stop()
{
  m_pView->stop();
}

QString KonqViewIface::url()
{
  return m_pView->url().url();
}

QString KonqViewIface::locationBarURL()
{
  return m_pView->locationBarURL();
}

QString KonqViewIface::serviceType()
{
  return m_pView->serviceType();
}

QStringList KonqViewIface::serviceTypes()
{
  return m_pView->serviceTypes();
}

DCOPRef KonqViewIface::part()
{
  DCOPRef res;

  KParts::ReadOnlyPart *part = m_pView->part();

  if ( !part )
    return res;

  QVariant dcopProperty = part->property( "dcopObjectId" );

  if ( dcopProperty.type() != QVariant::CString )
    return res;

  res.setRef( kapp->dcopClient()->appId(), dcopProperty.toCString() );
  return res;
}

void KonqViewIface::enablePopupMenu( bool b )
{
  m_pView->enablePopupMenu( b );
}

uint KonqViewIface::historyLength()const
{
    return m_pView->historyLength();
}

bool KonqViewIface::allowHTML() const
{
    return m_pView->allowHTML();
}

void KonqViewIface::goForward()
{
    m_pView->go(-1);
}

void KonqViewIface::goBack()
{
    m_pView->go(+1);
}

bool KonqViewIface::isPopupMenuEnabled() const
{
    return m_pView->isPopupMenuEnabled();
}

bool KonqViewIface::canGoBack()const
{
    return m_pView->canGoBack();
}

bool KonqViewIface::canGoForward()const
{
    return m_pView->canGoForward();
}
