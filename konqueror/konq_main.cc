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

#include <iostream.h>

#include "konq_run.h"
#include "konq_misc.h"
#include "konq_mainview.h"
#include "konq_viewmgr.h"
#include "KonquerorIface.h"
#include "version.h"

#include <ktrader.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kapp.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <dcopclient.h>
#include <kimgio.h>
#include <ksimpleconfig.h>

static KCmdLineOptions options[] =
{
  { "silent", I18N_NOOP("Start without a default window."), 0 },
  { "+[URL]",   I18N_NOOP("Location to open."), 0 },
  { 0, 0, 0}
};

class KonquerorIfaceImpl : virtual public KonquerorIface
{
public:
  KonquerorIfaceImpl();
  virtual ~KonquerorIfaceImpl();

  virtual void openBrowserWindow( const QString &url );

  virtual void createBrowserWindowFromProfile( const QString &filename );

  virtual void setMoveSelection( int move );
};

KonquerorIfaceImpl::KonquerorIfaceImpl()
 : DCOPObject( "KonquerorIface" )
{
}

KonquerorIfaceImpl::~KonquerorIfaceImpl()
{
}

void KonquerorIfaceImpl::openBrowserWindow( const QString &url )
{
  KFileManager::getFileManager()->openFileManagerWindow( url );
}

void KonquerorIfaceImpl::createBrowserWindowFromProfile( const QString &filename )
{
  kDebugInfo( 1202, "void KonquerorIfaceImpl::createBrowserWindowFromProfile( const QString &filename ) " );
  kDebugInfo( 1202, "%s", filename.ascii() );

  KonqMainView *mainView = new KonqMainView( QString::null, false );

  KSimpleConfig cfg( filename, true );
  cfg.setGroup( "Profile" );
  mainView->viewManager()->loadViewProfile( cfg );
  mainView->enableAllActions( true );

  mainView->show();
}

void KonquerorIfaceImpl::setMoveSelection( int move )
{
  kDebugInfo( 1202, "setMoveSelection: %i", move );
  KonqMainView::setMoveSelection( (bool)move );
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "konqueror", I18N_NOOP("Konqueror"),
                        KONQUEROR_VERSION,
                        I18N_NOOP("Web browser, file manager, ..."),
                        KAboutData::License_GPL,
                        "(c) 1999-2000, The Konqueror developers" );
  aboutData.addAuthor( "Torben Weis", I18N_NOOP("kfm author"), "weis@kde.org" );
  aboutData.addAuthor( "David Faure", I18N_NOOP("developer (parts, I/O lib) and maintainer"), "faure@kde.org" );
  aboutData.addAuthor( "Simon Hausmann", I18N_NOOP("developer (framework, parts)"), "hausmann@kde.org" );
  aboutData.addAuthor( "Michael Reiher", I18N_NOOP("developer (framework)"), "michael.reiher@gmx.de" );
  aboutData.addAuthor( "Matthias Welk", I18N_NOOP("developer"), "welk@fokus.gmd.de" );
  aboutData.addAuthor( "Lars Knoll", I18N_NOOP("developer (HTML rendering engine)"), "knoll@kde.org" );
  aboutData.addAuthor( "Antti Koivisto", I18N_NOOP("developer (HTML rendering engine)"), "koivisto@kde.org" );
  aboutData.addAuthor( "Waldo Bastian", I18N_NOOP("developer (HTML rendering engine, I/O lib)"), "bastian@kde.org" );
  aboutData.addAuthor( "Matt Koss", I18N_NOOP("developer (I/O lib)"), "koss@napri.sk" );
  aboutData.addAuthor( "Alex Zepeda", I18N_NOOP("developer (I/O lib)"), "jazepeda@pacbell.net" );
  aboutData.addAuthor( "Stephan Kulow", I18N_NOOP("developer (I/O lib)"), "coolo@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;

  app.dcopClient()->registerAs( "konqueror" );

  (void)new KonquerorIfaceImpl();

  KGlobal::locale()->insertCatalogue("libkonq"); // needed for apps using libkonq
  kimgioRegister();

  QString path = KGlobal::dirs()->saveLocation("data", "kfm/bookmarks", true);
  KonqBookmarkManager bm ( path );
  KonqFileManager fm;

  // Launch the cookiejar if not already running
  KConfig *kioConfig = new KConfig("kioslaverc", false, false);
  if (kioConfig->readBoolEntry( "Cookies", true ))
  {
     QCString dcopService;
     QString error;
     if (KApplication::startServiceByDesktopName("kcookiejar", QString::null,
		dcopService, error ))
     {
        // Error starting kcookiejar.
        kDebugInfo( 1202, "Error starting KCookiejar: %s\n", error.ascii());
     }
  }
  delete kioConfig;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (args->count() == 0)
  {
     if (!args->isSet("silent"))
     {
        fm.openFileManagerWindow( KURL() );
     }
  }
  else
  {
     for ( int i = 0; i < args->count(); i++ )
        fm.openFileManagerWindow( args->url(i) );
  }
  args->clear();

  QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

  app.exec();

  return 0;
}
