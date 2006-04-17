/* This file is part of the KDE project
   Copyright (C) 2003 Simon Hausmann <hausmann@kde.org>

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


#include "delayedinitializer.h"
#include <qtimer.h>
//Added by qt3to4:
#include <QEvent>

DelayedInitializer::DelayedInitializer( int eventType, QObject *parent, const char *name )
    : QObject( parent ), m_eventType( eventType ), m_signalEmitted( false )
{
    setObjectName( name );
    parent->installEventFilter( this );
}

bool DelayedInitializer::eventFilter( QObject *receiver, QEvent *event )
{
    if ( m_signalEmitted || event->type() != m_eventType )
        return false;

    m_signalEmitted = true;
    receiver->removeEventFilter( this );

    // Move the emitting of the event to the end of the eventQueue
    // so we are absolutely sure the event we get here is handled before
    // the initialize is fired.
    QTimer::singleShot( 0, this, SLOT( slotInitialize() ) );

    return false;
}

void DelayedInitializer::slotInitialize()
{
    emit initialize();
    deleteLater();
}

#include "delayedinitializer.moc"

/* vim: et sw=4
 */
