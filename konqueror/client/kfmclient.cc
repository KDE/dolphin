/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

// $Id$

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <fstream.h>

#include <qdir.h>
#include <qstring.h>
#include <qtimer.h>

#include <kio/job.h>
#include <kcmdlineargs.h>
#include <kpropsdlg.h>
#include <krun.h>
#include <klocale.h>
#include <kservice.h>
#include <ktrader.h>
#include <kprocess.h>
#include <kstddirs.h>
#include <kopenwith.h>
#include <kdebug.h>
#include <dcopclient.h>

#include "kfmclient.h"
#include "KonquerorIface_stub.h"
#include "KDesktopIface_stub.h"

static const char *appName = "kfmclient";

static const char *description = I18N_NOOP("KDE tool for opening URLs from the command line");

static const char *version = "2.0";

static const KCmdLineOptions options[] =
{
   { "commands", I18N_NOOP("Show available commands."), 0},
   { "+command", I18N_NOOP("Command (see --commands)."), 0},
   { "+[URL(s)]", I18N_NOOP("Arguments for command."), 0},
   {0,0,0}
};

int main( int argc, char **argv )
{
  KCmdLineArgs::init(argc, argv, appName, description, version, false);

  KCmdLineArgs::addCmdLineOptions( options );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->isSet("commands") )
  {
    printf(i18n("\nSyntax:\n").local8Bit());
    printf(i18n("  kfmclient openURL 'url' ['mimetype']\n"
                "            # Opens a window showing 'url'.\n"
                "            #  'url' may be a relative path\n"
                "            #   or file name, such as . or subdir/\n"
                "            #   If 'url' is omitted, $HOME is used instead.\n\n").local8Bit());
    printf(i18n("            # If 'mimetype' is specified, it will be used to determine the\n"
                "            #   component that konqueror should use. For instance, set it to\n"
                "            #   text/html for a web page, to make it appear faster\n\n").local8Bit());

    printf(i18n("  kfmclient openProfile 'profile' ['url']\n"
                "            # Opens a window using the given profile.\n"
                "            #   'profile' is a file under ~/.kde/share/apps/konqueror/profiles.\n"
                "            #   'url' is an optional URL to open.\n\n").local8Bit());

    printf(i18n("  kfmclient openProperties 'url'\n"
                "            # Opens a properties menu\n\n").local8Bit());
    printf(i18n("  kfmclient exec ['url' ['binding']]\n"
                "            # Tries to execute 'url'. 'url' may be a usual\n"
                "            #   URL, this URL will be opened. You may omit\n"
                "            #   'binding'. In this case the default binding\n").local8Bit());
    printf(i18n("            #   is tried. Of course URL may be the URL of a\n"
                "            #   document, or it may be a *.desktop file.\n").local8Bit());
    printf(i18n("            #   This way you could for example mount a device\n"
                "            #   by passing 'Mount default' as binding to \n"
                "            #   'cdrom.desktop'\n\n").local8Bit());
    printf(i18n("  kfmclient move 'src' 'dest'\n"
                "            # Moves the URL 'src' to 'dest'.\n"
                "            #   'src' may be a list of URLs.\n").local8Bit());
    //printf(i18n("            #   'dest' may be \"trash:/\" to move the files\n"
    //            "            #   in the trash bin.\n\n").local8Bit());
    printf(i18n("  kfmclient copy 'src' 'dest'\n"
                "            # Copies the URL 'src' to 'dest'.\n"
                "            #   'src' may be a list of URLs.\n\n").local8Bit());
    printf(i18n("  kfmclient sortDesktop\n"
                "            # Rearranges all icons on the desktop.\n\n").local8Bit());
    printf(i18n("  kfmclient selectDesktopIcons x y w h add\n"
                "            # Selects the icons on the desktop in the given rectangle\n"
                "            # If add is 1, adds selection to the current one\n\n").local8Bit());
    printf(i18n("  kfmclient configure\n"
                "            # Re-read konqueror's configuration.\n\n").local8Bit());
    printf(i18n("  kfmclient configureDesktop\n"
                "            # Re-read kdesktop's configuration.\n\n").local8Bit());

    printf(i18n("*** Examples:\n"
                "  kfmclient exec file:/root/Desktop/cdrom.desktop \"Mount default\"\n"
                "             // Mounts the CDROM\n\n").local8Bit());
    printf(i18n("  kfmclient exec file:/home/weis/data/test.html\n"
                "             // Opens the file with default binding\n\n").local8Bit());
    printf(i18n("  kfmclient exec file:/home/weis/data/test.html Netscape\n"
                "             // Opens the file with netscape\n\n").local8Bit());
    printf(i18n("  kfmclient exec ftp://localhost/\n"
                "             // Opens new window with URL\n\n").local8Bit());
    printf(i18n("  kfmclient exec file:/root/Desktop/emacs.desktop\n"
                "             // Starts emacs\n\n").local8Bit());
    printf(i18n("  kfmclient exec file:/root/Desktop/cdrom.desktop\n"
                "             // Opens the CD-ROM's mount directory\n\n").local8Bit());
    printf(i18n("  kfmclient exec .\n"
                "             // Opens the current directory. Very convenient.\n\n").local8Bit());
    return 0;
  }

  clientApp a;
  a.dcopClient()->attach();

  return a.doIt() ? 0 /*no error*/ : 1 /*error*/;
}

/** Whether to start a new konqueror or reuse an existing process */
static bool startNewKonqueror( const KURL & url )
{
    KConfig config( QString::fromLatin1("kfmclientrc") );
    config.setGroup( QString::fromLatin1("Settings") );
    // Current default: reuse an existing process for local urls, create a new one for remote ones
    QString val = config.readEntry( QString::fromLatin1("StartNewKonqueror"), QString::fromLatin1("Web only") );
    if ( (val == QString::fromLatin1("Web only")   && !url.isLocalFile()) ||
         (val == QString::fromLatin1("Local only") &&  url.isLocalFile()) ||
         (  val == QString::fromLatin1("Always") ||
            val == QString::fromLatin1("true") ||
            val == QString::fromLatin1("TRUE") ||
            val == QString::fromLatin1("1")))
        return true;
    else
        return false; // means (val == "Never") or one of the two above combinations return false
}

bool clientApp::createNewWindow(const KURL & url, const QString & mimetype)
{
    kdDebug() << "clientApp::createNewWindow " << url.url() << " mimetype=" << mimetype << endl;
    QByteArray data;
    QCString appId, appObj;

	// check if user wants to use external browser
	KConfig config( QString::fromLatin1("kfmclientrc"));
	config.setGroup( QString::fromLatin1("Settings"));
	QString strBrowser = config.readEntry( QString::fromLatin1("ExternalBrowser"));
	if (!strBrowser.isEmpty())
	{
		KProcess proc;
		proc << strBrowser << url.url();
		proc.start( KProcess::DontCare );
		return true;
	}

    if ( !startNewKonqueror(url) &&
         dcopClient()->findObject( "konqueror*", "KonquerorIface", "", data,
                                   appId, appObj ) )
    {
        kdDebug() << "clientApp::createNewWindow using existing konqueror" << endl;
        KonquerorIface_stub konqy( appId, appObj );
        konqy.createNewWindow( url.url(), mimetype );
    }
    else
    {
        QString error;
        /* Well, we can't pass a mimetype through startServiceByDesktopPath !
        if ( KApplication::startServiceByDesktopPath( QString::fromLatin1("konqueror.desktop"),
                                                      url.url(), &error ) > 0 )
        {
            kdError() << "Couldn't start konqueror from konqueror.desktop: " << error << endl;
            */
            KProcess proc;
            if ( mimetype.isEmpty() )
                proc << QString::fromLatin1("kdeinit_wrapper") << QString::fromLatin1("konqueror") << url.url();
            else
                proc << QString::fromLatin1("kdeinit_wrapper") << QString::fromLatin1("konqueror") << QString::fromLatin1("-mimetype") << mimetype << url.url();
            proc.start( KProcess::DontCare );
            kdDebug() << "clientApp::createNewWindow KProcess started" << endl;
        //}
    }
    return true;
}

bool clientApp::openProfile( const QString & filename, const QString & url, const QString & mimetype )
{
  m_profileName = filename;
  m_url = url;
  m_mimetype = mimetype;
  QByteArray data;
  QCString appId, appObj;
  if ( !startNewKonqueror(KURL(url)) &&
       dcopClient()->findObject( "konqueror*", "KonquerorIface", "", data,
                                 appId, appObj ) )
  {
      slotAppRegistered( "konqueror" );
  }
  else
  {
    dcopClient()->setNotifications( true );
    QObject::connect( dcopClient(), SIGNAL( applicationRegistered( const QCString& ) ),
                    this, SLOT( slotAppRegistered( const QCString & ) ) );
    QString error;
    if ( KApplication::startServiceByDesktopPath( QString::fromLatin1("konqueror.desktop"), QString::fromLatin1("--silent"), &error ) > 0 )
    {
      kdError() << "Couldn't start konqueror from konqueror.desktop: " << error << endl;
      return false;
    }

    exec();
  }

  return true;
}

void clientApp::slotAppRegistered( const QCString &appId )
{
    if ( appId.left( 9 ) == "konqueror" )
    {
        QString profile = locate( "data", QString::fromLatin1("konqueror/profiles/") + m_profileName );
        if ( profile.isEmpty() )
        {
            fprintf( stderr, i18n("Profile %1 not found\n").arg(m_profileName).local8Bit() );
            ::exit( 0 );
        }
        KonquerorIface_stub konqy( appId, "KonquerorIface" );
        if ( m_url.isEmpty() )
            konqy.createBrowserWindowFromProfile( profile, m_profileName );
        else if ( m_mimetype.isEmpty() )
            konqy.createBrowserWindowFromProfileAndURL( profile, m_profileName, m_url );
        else
            konqy.createBrowserWindowFromProfileAndURL( profile, m_profileName, m_url, m_mimetype );
        sleep(2); // Martin Schenk <martin@schenk.com> says this is necessary to let the server read from the socket
        ::exit( 0 );
    }
}

void clientApp::delayedQuit()
{
    // Quit in 2 seconds. This leaves time for KRun to pop up
    // "app not found" in KProcessRunner, if that was the case.
    QTimer::singleShot( 2000, this, SLOT(slotDeref()) );
}

void clientApp::slotDeref()
{
    // Damn, I should have made kapp::deref a slot.....
    deref();
}

static void checkArgumentCount(int count, int min, int max)
{
   if (count < min)
   {
      fprintf( stderr, i18n("Syntax Error: Not enough arguments\n").local8Bit() );
      ::exit(1);
   }
   if (max && (count > max))
   {
      fprintf( stderr, i18n("Syntax Error: Too many arguments\n").local8Bit() );
      ::exit(1);
   }
}

bool clientApp::doIt()
{
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  int argc = args->count();
  checkArgumentCount(argc, 1, 0);

  QCString command = args->arg(0);

  if ( command == "openURL" )
  {
    checkArgumentCount(argc, 1, 3);
    if ( argc == 1 )
    {
      KURL url;
      url.setPath(QDir::homeDirPath());
      return createNewWindow( url );
    }
    if ( argc == 2 )
    {
      return createNewWindow( args->url(1) );
    }
    if ( argc == 3 )
    {
      return createNewWindow( args->url(1), QString::fromLatin1(args->arg(2)) );
    }
  }
  else if ( command == "openProfile" )
  {
    checkArgumentCount(argc, 2, 3);
    QString url;
    if ( argc == 3 )
      url = args->url(2).url();
    return openProfile( QString::fromLocal8Bit(args->arg(1)), url );
  }
  else if ( command == "openProperties" )
  {
    checkArgumentCount(argc, 2, 2);
    KPropertiesDialog * p = new KPropertiesDialog( args->url(1) );
    QObject::connect( p, SIGNAL( destroyed() ), this, SLOT( quit() ));
    exec();
    return m_ok;
  }
  else if ( command == "exec" )
  {
    checkArgumentCount(argc, 1, 3);
    if ( argc == 1 )
    {
      KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
      kdesky.popupExecuteCommand();
    }
    else if ( argc == 2 )
    {
      KFileOpenWithHandler fowh;
      KRun * run = new KRun( args->url(1) );
      QObject::connect( run, SIGNAL( finished() ), this, SLOT( delayedQuit() ));
      QObject::connect( run, SIGNAL( error() ), this, SLOT( delayedQuit() ));
      exec();
      return m_ok;
    }
    else if ( argc == 3 )
    {
      KURL::List urls;
      urls.append( args->url(1) );
      KService::Ptr serv = (*KTrader::self()->query( QString::fromLocal8Bit(args->arg(2)) ).begin());
      if (!serv) return 1;
      KFileOpenWithHandler fowh;
      return KRun::run( *serv, urls );
    }
  }
  else if ( command == "move" )
  {
    checkArgumentCount(argc, 2, 0);
    KURL::List srcLst;
    for ( int i = 1; i <= argc - 2; i++ )
      srcLst.append( args->url(i) );

    KIO::Job * job = KIO::move( srcLst, args->url(argc - 1) );
    connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
    exec();
    return m_ok;
  }
  else if ( command == "copy" )
  {
    checkArgumentCount(argc, 2, 0);
    KURL::List srcLst;
    for ( int i = 1; i <= argc - 2; i++ )
      srcLst.append( args->url(i) );

    KIO::Job * job = KIO::copy( srcLst, args->url(argc - 1) );
    connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
    exec();
    return m_ok;
  }
  else if ( command == "sortDesktop" )
  {
    checkArgumentCount(argc, 1, 1);

    KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
    kdesky.rearrangeIcons( (int)false );

    return true;
  }
  else if ( command == "selectDesktopIcons" )
  {
    checkArgumentCount(argc, 6, 6);
    int x = atoi( args->arg(1) );
    int y = atoi( args->arg(2) );
    int w = atoi( args->arg(3) );
    int h = atoi( args->arg(4) );
    // bool bAdd = (bool) atoi( args->arg(5) ); /* currently unused */ // TODO
    KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
    kdesky.selectIconsInRect( x, y, w, h );
  }
  else if ( command == "configure" )
  {
    checkArgumentCount(argc, 1, 1);
    QByteArray data;
    kapp->dcopClient()->send( "*", "KonqMainViewIface", "reparseConfiguration()", data );
    // Warning. In case something is added/changed here, keep kcontrol/konq/main.cpp in sync.
  }
  else if ( command == "configureDesktop" )
  {
    checkArgumentCount(argc, 1, 1);
    KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
    kdesky.configure();
  }
  else
  {
    fprintf( stderr, i18n("Syntax Error: Unknown command '%1'\n").arg(QString::fromLocal8Bit(command)).local8Bit() );
    return false;
  }
  return true;
}

void clientApp::slotResult( KIO::Job * job )
{
  if (job->error())
    job->showErrorDialog();
  m_ok = !job->error();
  quit();
}

#include "kfmclient.moc"
