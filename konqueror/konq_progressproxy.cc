/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "konq_progressproxy.h"

#include <kio_job.h>

KonqProgressProxy::KonqProgressProxy( OPPartIf *part, KIOJob *job )
{
  job->insertChild( this ); //let's make sure we get deleted when the job dies
  
  m_dctSignals = part->signalImplementations();

  if ( !part->supportsInterface( "IDL:Browser/View:1.0" ) ||
       !(*m_dctSignals)[ "loadingProgress" ] ||
       !(*m_dctSignals)[ "speedProgress" ] )
    assert( 0 );

  m_partId = part->id();
  
  connect( job, SIGNAL( sigTotalSize( int, unsigned long ) ), this, SLOT( slotTotalSize( int, unsigned long ) ) );
  connect( job, SIGNAL( sigProcessedSize( int, unsigned long ) ), this, SLOT( slotProcessedSize( int, unsigned long ) ) );
  connect( job, SIGNAL( sigSpeed( int, unsigned long ) ), this, SLOT( slotSpeed( int, unsigned long ) ) );

  m_ulTotalDocumentSize = 0;
}

void KonqProgressProxy::slotTotalSize( int, unsigned long size )
{
  m_ulTotalDocumentSize = size;
}

void KonqProgressProxy::slotProcessedSize( int, unsigned long size )
{
  if ( m_ulTotalDocumentSize > 0 && (*m_dctSignals)[ "loadingProgress" ] )
    signal_call2( "loadingProgress", m_dctSignals, m_partId, 
                  (CORBA::Long)( size * 100 / m_ulTotalDocumentSize ) );
}

void KonqProgressProxy::slotSpeed( int, unsigned long bytesPerSecond )
{
  if ( (*m_dctSignals)[ "speedProgress" ] )
    signal_call2( "speedProgress", m_dctSignals, m_partId, (CORBA::Long)bytesPerSecond );
}

#include "konq_progressproxy.moc"
