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

#include "konq_main.h"
#include "konq_misc.h"
#include "konq_factory.h"
#include "konq_mainwindow.h"
#include "konq_view.h"
#include "KonquerorIface.h"

#include <ktempfile.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kimageio.h>
#include <qfile.h>

#include <qwidgetlist.h>

static KCmdLineOptions options[] =
{
  { "silent", I18N_NOOP("Start without a default window."), 0 },
  { "preload", I18N_NOOP("Preload for later use."), 0 },
  { "profile <profile>",   I18N_NOOP("Profile to open."), 0 },
  { "mimetype <mimetype>",   I18N_NOOP("Mimetype to use for this URL, (e.g. text/html or inode/directory)."), 0 },
  { "+[URL]",   I18N_NOOP("Location to open."), 0 },
  { 0, 0, 0}
};

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, KonqFactory::aboutData() );

  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KonquerorApplication app;

  app.dcopClient()->registerAs( "konqueror" );

  app.dcopClient()->setDefaultObject( (new KonquerorIface())->objId() );

  KGlobal::locale()->insertCatalogue("libkonq"); // needed for apps using libkonq
  KImageIO::registerFormats();

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KTempFile crashlog_file(locateLocal("tmp", "konqueror-crash-"), ".log");
  KonqMainWindow::s_crashlog_file = crashlog_file.file();

  if ( kapp->isRestored() )
  {
    int n = 1;
    while ( KonqMainWindow::canBeRestored( n ) )
    {
      (new KonqMainWindow( KURL(), false ) )->restore( n );
      n++;
    }
  }
  else
  {
     if (args->isSet("profile"))
     {
       QString profile = QString::fromLocal8Bit(args->getOption("profile"));
       QString profilePath = profile;
       if (profile[0] != '/')
           profilePath = locate( "data", QString::fromLatin1("konqueror/profiles/")+profile );
       QString url;
       if (args->count() == 1)
           url = QString::fromLocal8Bit(args->arg(0));
       KParts::URLArgs urlargs;
       if (args->isSet("mimetype"))
           urlargs.serviceType = QString::fromLocal8Bit(args->getOption("mimetype"));
       kdDebug(1202) << "main() -> createBrowserWindowFromProfile servicetype=" << urlargs.serviceType << endl;
       KonqMisc::createBrowserWindowFromProfile( profilePath, profile, url, urlargs );
     }
     else
     {
         if (args->count() == 0)
         {
             if (args->isSet("preload"))
             {
                 KConfigGroupSaver group( app.config(), "Reusing" );
                 if( app.config()->readNumEntry( "MaxPreloadCount", 0 ) > 0 )
                 {
                     DCOPRef ref( "kded", "konqy_preloader" );
                     if( !ref.call( "registerPreloadedKonqy", app.dcopClient()->appId()))
                         return 0; // too many preloaded or failed
                     KonqMainWindow::setPreloadedFlag( true );
                     kdDebug(1202) << "Konqy preloaded :" << app.dcopClient()->appId() << endl;
                 }
                 else
                 {
                     return 0; // no preloading
                 }
             }
             else if (!args->isSet("silent"))
             {
                 KonqMainWindow *mainWindow = new KonqMainWindow;
                 mainWindow->show();
             }
             kdDebug(1202) << "main() -> no args" << endl;
         }
         else
         {
             for ( int i = 0; i < args->count(); i++ )
             {
                 // KonqMisc::konqFilteredURL doesn't cope with local files... A bit of hackery below
                 KURL url = args->url(i);
                 KURL urlToOpen;
                 if (url.isLocalFile() && QFile::exists(url.path())) // "konqueror index.html"
                     urlToOpen = url;
                 else
                     urlToOpen = KURL( KonqMisc::konqFilteredURL(0L, args->arg(i)) ); // "konqueror slashdot.org"

                 KParts::URLArgs urlargs;
                 if (args->isSet("mimetype"))
                 {
                     urlargs.serviceType = QString::fromLocal8Bit(args->getOption("mimetype"));
                     kdDebug(1202) << "main() : setting serviceType to " << urlargs.serviceType << endl;
                 }
                 KonqMisc::createNewWindow( urlToOpen, urlargs );
             }
         }
     }
  }
  args->clear();

  app.ref(); // for preloading
  
  app.exec();

  // Delete all KonqMainWindows, so that we don't have
  // any parts loaded when KLibLoader::cleanUp is called.
  // Their deletion was postponed in their event()
  // (and Qt doesn't delete WDestructiveClose widgets on exit anyway :(  )
  while( KonqMainWindow::mainWindowList() != NULL )
  { // the list will be deleted by last KonqMainWindow
      delete KonqMainWindow::mainWindowList()->first();
  }

  crashlog_file.unlink();

  return 0;
}
