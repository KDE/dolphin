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

#include "konq_history.h"

#include <qapplication.h>

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <klocale.h>
#include <konq_operations.h>
#include <konq_propsview.h>
#include <konq_settings.h>
#include <kparts/factory.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>

#include <assert.h>


/************************************************************************************/


class KonqHistoryFactory : public KParts::Factory
{
public:
  KonqHistoryFactory()
  {
  }

  virtual ~KonqHistoryFactory()
  {
    if ( s_instance )
    {
      delete s_instance;
      s_instance = 0L;
    }
    if ( s_defaultViewProps )
    {
      delete s_defaultViewProps;
      s_defaultViewProps = 0L;
    }
  }

  virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList & )
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

  static KonqPropsView *defaultViewProps()
  {
      if ( !s_defaultViewProps )
         s_defaultViewProps = new KonqPropsView( instance(), 0L );

      return s_defaultViewProps;
  }

private:
  static KInstance *s_instance;
  static KonqPropsView *s_defaultViewProps;
};

KInstance *KonqHistoryFactory::s_instance = 0;
KonqPropsView *KonqHistoryFactory::s_defaultViewProps = 0;

extern "C"
{
  void *init_libkonqdirtree()
  {
    return new KonqHistoryFactory;
  }
};


/************************************************************************************/


KonqHistoryBrowserExtension::KonqHistoryBrowserExtension( KonqHistoryPart *parent, KonqHistory *history )
 : KParts::BrowserExtension( parent )
{
  m_history = history;
}


/************************************************************************************/


KonqHistoryPart::KonqHistoryPart( QWidget *parentWidget, QObject *parent, const char *name )
 : KonqDirPart( parent, name )
{
  m_history = new KonqHistory( this, parentWidget );

  setBrowserExtension( new KonqHistoryBrowserExtension( this, m_history ) );

  // Create a properties instance for this view
  m_pProps = new KonqPropsView( KonqHistoryFactory::instance(),
                                KonqHistoryFactory::defaultViewProps() );

  setWidget( m_history );
  setInstance( KonqHistoryFactory::instance(), false );
}

KonqHistoryPart::~KonqHistoryPart()
{
}

bool KonqHistoryPart::openURL( const KURL & url )
{
  emit started( 0 );
  m_history->followURL( url );
  emit completed();
  return true;
}

bool KonqHistoryPart::closeURL()
{
  return true;
}


/************************************************************************************/


KonqHistory::KonqHistory( KonqHistoryPart */*parent*/, QWidget *parentWidget )
  : QWidget( parentWidget )
{
}

KonqHistory::~KonqHistory()
{
}


void KonqHistory::followURL( const KURL &/*_url*/ )
{
}


#include "konq_history.moc"
