/* This file is part of the KDE project
   Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_history_h__
#define __konq_history_h__

#include <kurl.h>
#include <kparts/browserextension.h>
#include <kparts/part.h>

#include <klistview.h>

class HistoryWidget;

class KonqHistoryPart : public KParts::ReadOnlyPart
{
  Q_OBJECT

public:
  KonqHistoryPart( QWidget *parentWidget, QObject *parent, 
		   const char *name = 0L );
  virtual ~KonqHistoryPart();

  virtual bool openFile() { return true; }

private:
  HistoryWidget *m_history;
};

class KonqHistoryExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    KonqHistoryExtension( KonqHistoryPart *parent )
	: KParts::BrowserExtension( parent, "dummy history browserextension" )
    {}
    
};

#endif
