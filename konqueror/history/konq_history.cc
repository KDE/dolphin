/* This file is part of the KDE project
   Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
   Copyright (C) 2000 Carsten Pfeiffer  <pfeiffer@kde.org>

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

#include <kdebug.h>
#include <kinstance.h>
#include <kparts/factory.h>

#include "konq_history.h"
#include "historywidget.h"


class KonqHistoryFactory : public KParts::Factory
{
public:
  KonqHistoryFactory() {}

  virtual ~KonqHistoryFactory()
  {
      delete s_instance;
      s_instance = 0L;
  }

  virtual KParts::Part* createPart( QWidget *parentWidget, const char *, 
				    QObject *parent, const char *name, 
				    const char*, const QStringList & )
  {
    KParts::Part *obj = new KonqHistoryPart( parentWidget, parent, name );
    emit objectCreated( obj );
    return obj;
  }

  static KInstance *instance()
  {
    if ( !s_instance )
      s_instance = new KInstance( "konqueror" );
    return s_instance;
  }

private:
  static KInstance *s_instance;
};

KInstance *KonqHistoryFactory::s_instance = 0;

extern "C"
{
  void *init_libkonqhistory()
  {
    return new KonqHistoryFactory;
  }
};


/****************************************************************************/


KonqHistoryPart::KonqHistoryPart( QWidget *parentWidget, QObject *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
    m_history = new HistoryWidget( parentWidget );

    setWidget( m_history );
    setInstance( KonqHistoryFactory::instance(), false );
    (void) new KonqHistoryExtension( this );
}

KonqHistoryPart::~KonqHistoryPart()
{
}

#include "konq_history.moc"
