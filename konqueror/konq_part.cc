/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#include "konq_part.h"
#include "konq_mainview.h"
#include "konq_shell.h"
#include "konq_factory.h"
#include "konq_viewmgr.h"

#include <qdir.h>

#include <kstddirs.h>
#include <kconfig.h>

KonqPart::KonqPart( QObject *parent, const char *name )
 : Part( parent, name )
{
  m_bOpenInitialURL = true;
}

KonqPart::~KonqPart()
{
  while ( firstView() )
    delete firstView();
}

View *KonqPart::createView( QWidget *parent, const char *name )
{
  KonqMainView *view = new KonqMainView( this, parent, name ? name : "KonqMainView" );

  addView( view );

  if ( m_bOpenInitialURL )
  {
    KConfig *config = KonqFactory::instance()->config();
    
    if ( config->hasGroup( "Default View Profile" ) )
    {
      config->setGroup( "Default View Profile" );
      view->viewManager()->loadViewProfile( *config );
    }
    else
      view->openURL( 0L, QDir::homeDirPath().prepend( "file:" ) );
  }    
  
  return view;
}

Shell *KonqPart::createShell()
{
  KonqShell *shell = new KonqShell;
  shell->setRootPart( this );
  shell->show();
  return shell;
}

void KonqPart::paintEverything( QPainter &/*painter*/, const QRect &/*rect*/,
                                bool /*transparent*/  , View */*view*/ )
{
  //TODO, probably
}				

QString KonqPart::configFile() const
{
  return readConfigFile( locate( "data", "konqueror/konqueror_part.rc", KonqFactory::instance() ) );
}

#include "konq_part.moc"
