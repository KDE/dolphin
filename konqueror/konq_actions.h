/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_actions_h__
#define __konq_actions_h__ $Id$

#include <kaction.h>

class KonqComboAction : public KSelectAction
{
  Q_OBJECT
public:
    KonqComboAction( const QString& text, int accel = 0, QObject* parent = 0, const char* name = 0 );
    KonqComboAction( const QString& text, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqComboAction( const QString& text, const QIconSet& pix, int accel = 0,
	     QObject* parent = 0, const char* name = 0 );
    KonqComboAction( const QString& text, const QIconSet& pix, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqComboAction( QObject* parent = 0, const char* name = 0 );

    virtual int plug( QWidget *w );

    virtual void unplug( QWidget *w );

protected:
    virtual void changeItem( int id, int index, const QString &text );

};

#endif
