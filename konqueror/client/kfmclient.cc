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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <QDir>

#include <ktoolinvocation.h>
#include <kio/job.h>
#include <kcmdlineargs.h>
#include <kpropertiesdialog.h>
#include <klocale.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kopenwith.h>
#include <kurlrequesterdlg.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kfiledialog.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <kservice.h>
#include <QRegExp>

#include "kfmclient.h"
#include "KonquerorIface_stub.h"
#include "KDesktopIface_stub.h"
#include "kwin.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <QX11Info>
#endif

static const char appName[] = "kfmclient";
static const char programName[] = I18N_NOOP("kfmclient");

static const char description[] = I18N_NOOP("KDE tool for opening URLs from the command line");

static const char version[] = "2.0";

QByteArray clientApp::startup_id_str;
bool clientApp::m_ok = true;
bool s_interactive = true;

static KInstance* s_instance = 0;

static const KCmdLineOptions options[] =
{
   { "noninteractive", I18N_NOOP("Non interactive use: no message boxes"), 0},
   { "commands", I18N_NOOP("Show available commands"), 0},
   { "+command", I18N_NOOP("Command (see --commands)"), 0},
   { "+[URL(s)]", I18N_NOOP("Arguments for command"), 0},
   KCmdLineLastOption
};

extern "C" KDE_EXPORT int kdemain( int argc, char **argv )
{
  KCmdLineArgs::init(argc, argv, appName, programName, description, version, false);

  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs::addTempFileOption();

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->isSet("commands") )
  {
    KCmdLineArgs::enable_i18n();
    puts(i18n("\nSyntax:\n").toLocal8Bit());
    puts(i18n("  kfmclient openURL 'url' ['mimetype']\n"
                "            # Opens a window showing 'url'.\n"
                "            #  'url' may be a relative path\n"
                "            #   or file name, such as . or subdir/\n"
                "            #   If 'url' is omitted, $HOME is used instead.\n\n").toLocal8Bit());
    puts(i18n("            # If 'mimetype' is specified, it will be used to determine the\n"
                "            #   component that Konqueror should use. For instance, set it to\n"
                "            #   text/html for a web page, to make it appear faster\n\n").toLocal8Bit());

    puts(i18n("  kfmclient newTab 'url' ['mimetype']\n"
                "            # Same as above but opens a new tab with 'url' in an existing Konqueror\n"
                "            #   window on the current active desktop if possible.\n\n").toLocal8Bit());

    puts(i18n("  kfmclient openProfile 'profile' ['url']\n"
                "            # Opens a window using the given profile.\n"
                "            #   'profile' is a file under ~/.kde/share/apps/konqueror/profiles.\n"
                "            #   'url' is an optional URL to open.\n\n").toLocal8Bit());

    puts(i18n("  kfmclient openProperties 'url'\n"
                "            # Opens a properties menu\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec ['url' ['binding']]\n"
                "            # Tries to execute 'url'. 'url' may be a usual\n"
                "            #   URL, this URL will be opened. You may omit\n"
                "            #   'binding'. In this case the default binding\n").toLocal8Bit());
    puts(i18n("            #   is tried. Of course URL may be the URL of a\n"
                "            #   document, or it may be a *.desktop file.\n").toLocal8Bit());
    puts(i18n("            #   This way you could for example mount a device\n"
                "            #   by passing 'Mount default' as binding to \n"
                "            #   'cdrom.desktop'\n\n").toLocal8Bit());
    puts(i18n("  kfmclient move 'src' 'dest'\n"
                "            # Moves the URL 'src' to 'dest'.\n"
                "            #   'src' may be a list of URLs.\n").toLocal8Bit());
    //puts(i18n("            #   'dest' may be \"trash:/\" to move the files\n"
    //            "            #   in the trash bin.\n\n").toLocal8Bit());
    puts(i18n("  kfmclient download ['src']\n"
                "            # Copies the URL 'src' to a user specified location'.\n"
                "            #   'src' may be a list of URLs, if not present then\n"
                "            #   a URL will be requested.\n\n").toLocal8Bit());
    puts(i18n("  kfmclient copy 'src' 'dest'\n"
                "            # Copies the URL 'src' to 'dest'.\n"
                "            #   'src' may be a list of URLs.\n\n").toLocal8Bit());
    puts(i18n("  kfmclient sortDesktop\n"
                "            # Rearranges all icons on the desktop.\n\n").toLocal8Bit());
    puts(i18n("  kfmclient configure\n"
                "            # Re-read Konqueror's configuration.\n\n").toLocal8Bit());
    puts(i18n("  kfmclient configureDesktop\n"
                "            # Re-read kdesktop's configuration.\n\n").toLocal8Bit());

    puts(i18n("*** Examples:\n"
                "  kfmclient exec file:/root/Desktop/cdrom.desktop \"Mount default\"\n"
                "             // Mounts the CD-ROM\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec file:/home/weis/data/test.html\n"
                "             // Opens the file with default binding\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec file:/home/weis/data/test.html Netscape\n"
                "             // Opens the file with netscape\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec ftp://localhost/\n"
                "             // Opens new window with URL\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec file:/root/Desktop/emacs.desktop\n"
                "             // Starts emacs\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec file:/root/Desktop/cdrom.desktop\n"
                "             // Opens the CD-ROM's mount directory\n\n").toLocal8Bit());
    puts(i18n("  kfmclient exec .\n"
                "             // Opens the current directory. Very convenient.\n\n").toLocal8Bit());
    return 0;
  }

  return clientApp::doIt() ? 0 /*no error*/ : 1 /*error*/;
}

// Call needInstance before any use of KConfig
static void needInstance()
{
    if ( !s_instance ) {
        // TODO KStaticDeleter
        s_instance = new KInstance( appName );
    }
}

/*
 Whether to start a new konqueror or reuse an existing process.

 First of all, this concept is actually broken, as the view used to show
 the data may change at any time, and therefore Konqy reused to browse
 "safe" data may eventually browse something completely different.
 Moreover, it's quite difficult to find out when to reuse, and thus this
 function is an ugly hack. You've been warned.

 Kfmclient will attempt to find an instance for reusing if either reusing
 is configured to reuse always,
 or it's not configured to never reuse, and the URL to-be-opened is "safe".
 The URL is safe, if the view used to view it is listed in the allowed KPart's.
 In order to find out the part, mimetype is needed, and KMimeTypeTrader is needed.
 If mimetype is not known, KMimeType is used (which doesn't work e.g. for remote
 URLs, but oh well). Since this function may be running without a KApplication
 instance, I'm actually quite surprised it works, and it may sooner or later break.
 Nice, isn't it?

 If a profile is being used, and no url has been explicitly given, it needs to be
 read from the profile. If there's more than one URL listed in the profile, no reusing
 will be done (oh well), if there's no URL, no reusing will be done either (also
 because the webbrowsing profile doesn't have any URL listed).
*/
static bool startNewKonqueror( QString url, QString mimetype, const QString& profile )
{
    needInstance();
    KConfig cfg( QLatin1String( "konquerorrc" ), true );
    cfg.setGroup( "Reusing" );
    QStringList allowed_parts;
    // is duplicated in ../KonquerorIface.cc
    allowed_parts << QLatin1String( "konq_iconview.desktop" )
                  << QLatin1String( "konq_multicolumnview.desktop" )
                  << QLatin1String( "konq_sidebartng.desktop" )
                  << QLatin1String( "konq_infolistview.desktop" )
                  << QLatin1String( "konq_treeview.desktop" )
                  << QLatin1String( "konq_detailedlistview.desktop" );
    if( cfg.hasKey( "SafeParts" )
        && cfg.readEntry( "SafeParts" ) != QLatin1String( "SAFE" ))
        allowed_parts = cfg.readEntry( "SafeParts",QStringList() );
    if( allowed_parts.count() == 1 && allowed_parts.first() == QLatin1String( "ALL" ))
	return false; // all parts allowed
    if( url.isEmpty())
    {
        if( profile.isEmpty())
            return true;
	QString profilepath = locate( "data", QLatin1String("konqueror/profiles/") + profile );
	if( profilepath.isEmpty())
	    return true;
	KConfig cfg( profilepath, true );
	cfg.setDollarExpansion( true );
        cfg.setGroup( "Profile" );
	QMap< QString, QString > entries = cfg.entryMap( QLatin1String( "Profile" ));
	QRegExp urlregexp( QLatin1String( "^View[0-9]*_URL$" ));
	QStringList urls;
	for( QMap< QString, QString >::ConstIterator it = entries.begin();
	     it != entries.end();
	     ++it )
	{
            // don't read value from map, dollar expansion is needed
            QString value = cfg.readEntry( it.key(), QString());
	    if( urlregexp.indexIn( it.key()) >= 0 && !value.isEmpty())
		urls << value;
	}
	if( urls.count() != 1 )
	    return true;
	url = urls.first();
	mimetype = QLatin1String( "" );
    }
    if( mimetype.isEmpty())
	mimetype = KMimeType::findByURL( KUrl( url ) )->name();
    KService::List offers = KMimeTypeTrader::self()->query( mimetype, QLatin1String( "KParts/ReadOnlyPart" ) );
    KService::Ptr serv;
    if( offers.count() > 0 )
        serv = offers.first();
    return !serv || !allowed_parts.contains( serv->desktopEntryName() + QLatin1String(".desktop") );
}

static int currentScreen()
{
#ifdef Q_WS_X11
	QX11Info info;
    if( QX11Info::display() != NULL )
        return info.screen();
    // case when there's no KApplication instance
    const char* env = getenv( "DISPLAY" );
    if( env == NULL )
        return 0;
    const char* dotpos = strrchr( env, '.' );
    const char* colonpos = strrchr( env, ':' );
    if( dotpos != NULL && colonpos != NULL && dotpos > colonpos )
        return atoi( dotpos + 1 );
#endif
    return 0;
}

static void needDCOP()
{
    if( !KApplication::dcopClient()->attach())
    {
       KApplication::startKdeinit();
       KApplication::dcopClient()->attach();
    }
}

// when reusing a preloaded konqy, make sure your always use a DCOP call which opens a profile !
static DCOPCString getPreloadedKonqy()
{
    needInstance();
    KConfig cfg( QLatin1String( "konquerorrc" ), true );
    cfg.setGroup( "Reusing" );
    if( cfg.readEntry( "MaxPreloadCount", 1 ) == 0 )
        return "";
    needDCOP();
    DCOPRef ref( "kded", "konqy_preloader" );
    DCOPCString ret;
    if( ref.callExt( "getPreloadedKonqy", DCOPRef::NoEventLoop, 3000, currentScreen()).get( ret ))
        return ret;
    return DCOPCString();
}

static DCOPCString konqyToReuse( const QString& url, const QString& mimetype, const QString& profile )
{ // prefer(?) preloaded ones
    DCOPCString ret = getPreloadedKonqy();
    if( !ret.isEmpty())
        return ret;
    if( startNewKonqueror( url, mimetype, profile ))
        return "";
    needDCOP();
    DCOPCString appObj;
    QByteArray data;
    QDataStream str( &data, QIODevice::WriteOnly );

    str.setVersion(QDataStream::Qt_3_1);
    str << currentScreen();
    if( !KApplication::dcopClient()->findObject( "konqueror*", "KonquerorIface",
             "processCanBeReused( int )", data, ret, appObj, false, 3000 ) )
        return "";
    return ret;
}

void clientApp::sendASNChange()
{
#ifdef Q_WS_X11
    KStartupInfoId id;
    id.initId( startup_id_str );
    KStartupInfoData data;
    data.addPid( 0 );   // say there's another process for this ASN with unknown PID
    data.setHostname(); // ( no need to bother to get this konqy's PID )
    Display* dpy = QX11Info::display();
    if( dpy == NULL ) // we may be running without QApplication here
        dpy = XOpenDisplay( NULL );
    if( dpy != NULL )
        KStartupInfo::sendChangeX( dpy, id, data );
    if( dpy != NULL && dpy != QX11Info::display())
        XCloseDisplay( dpy );
#endif
}

bool clientApp::createNewWindow(const KUrl & url, bool newTab, bool tempFile, const QString & mimetype)
{
    kDebug( 1202 ) << "clientApp::createNewWindow " << url.url() << " mimetype=" << mimetype << endl;
    needInstance();
#if 0
    // check if user wants to use external browser
    // ###### this option seems to have no GUI and to be redundant with BrowserApplication now.
    // ###### KDE4: remove
    KConfig config( QLatin1String("kfmclientrc"));
    config.setGroup( QLatin1String("Settings"));
    QString strBrowser = config.readPathEntry("ExternalBrowser");
    if (!strBrowser.isEmpty())
    {
        if ( tempFile )
            kWarning() << "kfmclient used with --tempfile but is passing to an external browser! Tempfile will never be deleted" << endl;
        KProcess proc;
        proc << strBrowser << url.url();
        proc.start( KProcess::DontCare );
        return true;
    }
#endif

    if (url.protocol().startsWith(QLatin1String("http")))
    {
#if 1
        KConfig config( QLatin1String("kfmclientrc"));
#endif
        config.setGroup("General");
        if (!config.readEntry("BrowserApplication").isEmpty())
        {
            kDebug() << config.readEntry( "BrowserApplication" ) << endl;
            Q_ASSERT( qApp );
            //clientApp app;
#ifdef Q_WS_X11
            KStartupInfo::appStarted();
#endif

            KRun * run = new KRun( url,0L ); // TODO pass tempFile [needs support in the KRun ctor]
            QObject::connect( run, SIGNAL( finished() ), qApp, SLOT( delayedQuit() ));
            QObject::connect( run, SIGNAL( error() ), qApp, SLOT( delayedQuit() ));
            qApp->exec();
            return !run->hasError();
        }
    }

    KConfig cfg( QLatin1String( "konquerorrc" ), true );
    cfg.setGroup( "FMSettings" );
    if ( newTab || cfg.readEntry( "KonquerorTabforExternalURL", QVariant(false )).toBool() )
    {
        needDCOP();
        DCOPCString foundApp, foundObj;
        QByteArray data;
        QDataStream str( &data, QIODevice::WriteOnly );

        str.setVersion(QDataStream::Qt_3_1);
        if( KApplication::dcopClient()->findObject( "konqueror*", "konqueror-mainwindow*",
            "windowCanBeUsedForTab()", data, foundApp, foundObj, false, 3000 ) )
        {
            DCOPRef ref( foundApp, foundObj );
            DCOPReply reply = ref.call( "newTabASN", url.url(), startup_id_str, tempFile );
            if ( reply.isValid() ) {
                sendASNChange();
                return true;
            }
      }
    }

    DCOPCString appId = konqyToReuse( url.url(), mimetype, QString() );
    if( !appId.isEmpty())
    {
        kDebug( 1202 ) << "clientApp::createNewWindow using existing konqueror" << endl;
        KonquerorIface_stub konqy( appId, "KonquerorIface" );
        konqy.createNewWindowASN( url.url(), mimetype, startup_id_str, tempFile );
        sendASNChange();
    }
    else
    {
        QString error;
        /* Well, we can't pass a mimetype through startServiceByDesktopPath !
        if ( KToolInvocation::startServiceByDesktopPath( QLatin1String("konqueror.desktop"),
                                                      url.url(), &error ) > 0 )
        {
            kError() << "Couldn't start konqueror from konqueror.desktop: " << error << endl;
            */
            // pass kfmclient's startup id to konqueror using kshell
#ifdef Q_WS_X11
            KStartupInfoId id;
            id.initId( startup_id_str );
            id.setupStartupEnv();
#endif
            KProcess proc;
            proc << "kshell" << "konqueror";
            if ( !mimetype.isEmpty() )
                proc << "-mimetype" << mimetype;
            if ( tempFile )
                proc << "-tempfile";
            proc << url.url();
            proc.start( KProcess::DontCare );
#ifdef Q_WS_X11
            KStartupInfo::resetStartupEnv();
#endif
            kDebug( 1202 ) << "clientApp::createNewWindow KProcess started" << endl;
        //}
    }
    return true;
}

bool clientApp::openProfile( const QString & profileName, const QString & url, const QString & mimetype )
{
  needInstance();
  DCOPCString appId = konqyToReuse( url, mimetype, profileName );
  if( appId.isEmpty())
  {
    QString error;
    if ( KToolInvocation::startServiceByDesktopPath( QLatin1String("konqueror.desktop"),
        QLatin1String("--silent"), &error, &appId, NULL, startup_id_str ) > 0 )
    {
      kError() << "Couldn't start konqueror from konqueror.desktop: " << error << endl;
      return false;
    }
      // startServiceByDesktopPath waits for the app to register with DCOP
      // so when we arrive here, konq is up and running already, and appId contains the identification
  }

  QString profile = locate( "data", QLatin1String("konqueror/profiles/") + profileName );
  if ( profile.isEmpty() )
  {
      fprintf( stderr, "%s", i18n("Profile %1 not found\n", profileName).toLocal8Bit().data() );
      ::exit( 0 );
  }
  KonquerorIface_stub konqy( appId, "KonquerorIface" );
  if ( url.isEmpty() )
      konqy.createBrowserWindowFromProfileASN( profile, profileName, startup_id_str );
  else if ( mimetype.isEmpty() )
      konqy.createBrowserWindowFromProfileAndURLASN( profile, profileName, url, startup_id_str );
  else
      konqy.createBrowserWindowFromProfileAndURLASN( profile, profileName, url, mimetype, startup_id_str );
  sleep(2); // Martin Schenk <martin@schenk.com> says this is necessary to let the server read from the socket
  sendASNChange();
  return true;
}

void clientApp::delayedQuit()
{
    // Quit in 2 seconds. This leaves time for KRun to pop up
    // "app not found" in KProcessRunner, if that was the case.
    QTimer::singleShot( 2000, this, SLOT(deref()) );
}

static void checkArgumentCount(int count, int min, int max)
{
   if (count < min)
   {
      fputs( i18n("Syntax Error: Not enough arguments\n").toLocal8Bit(), stderr );
      ::exit(1);
   }
   if (max && (count > max))
   {
      fputs( i18n("Syntax Error: Too many arguments\n").toLocal8Bit(), stderr );
      ::exit(1);
   }
}

bool clientApp::doIt()
{
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  int argc = args->count();
  checkArgumentCount(argc, 1, 0);

  if ( !args->isSet( "ninteractive" ) ) {
      s_interactive = false;
  }
  QByteArray command = args->arg(0);

#ifdef Q_WS_X11
  // read ASN env. variable for non-KApp cases
  startup_id_str = KStartupInfo::currentStartupIdEnv().id();
#endif

  kDebug() << "Creating clientApp" << endl;
  int fake_argc = 0;
  char** fake_argv = 0;
  clientApp app( fake_argc, fake_argv );


  if ( command == "openURL" || command == "newTab" )
  {
    checkArgumentCount(argc, 1, 3);
    bool tempFile = KCmdLineArgs::isTempFileSet();
    if ( argc == 1 )
    {
      KUrl url;
      url.setPath(QDir::homePath());
      return createNewWindow( url, command == "newTab", tempFile );
    }
    if ( argc == 2 )
    {
      return createNewWindow( args->url(1), command == "newTab", tempFile );
    }
    if ( argc == 3 )
    {
      return createNewWindow( args->url(1), command == "newTab", tempFile, QLatin1String(args->arg(2)) );
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
    QObject::connect( p, SIGNAL( destroyed() ), &app, SLOT( quit() ));
    QObject::connect( p, SIGNAL( canceled() ), &app, SLOT( slotDialogCanceled() ));
    app.exec();
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
      KRun * run = new KRun( args->url(1), 0L);
      QObject::connect( run, SIGNAL( finished() ), &app, SLOT( delayedQuit() ));
      QObject::connect( run, SIGNAL( error() ), &app, SLOT( delayedQuit() ));
      app.exec();
      return !run->hasError();
    }
    else if ( argc == 3 )
    {
      KUrl::List urls;
      urls.append( args->url(1) );
      const KService::List offers = KMimeTypeTrader::self()->query( QString::fromLocal8Bit( args->arg( 2 ) ),
                                                                    QLatin1String( "Application" ) );
      if (offers.isEmpty()) return 1;
      KService::Ptr serv = offers.first();
      return KRun::run( *serv, urls, 0 );
    }
  }
  else if ( command == "move" )
  {
    checkArgumentCount(argc, 2, 0);
    KUrl::List srcLst;
    for ( int i = 1; i <= argc - 2; i++ )
      srcLst.append( args->url(i) );

    KIO::Job * job = KIO::move( srcLst, args->url(argc - 1) );
    if ( !s_interactive )
        job->setInteractive( false );
    connect( job, SIGNAL( result( KJob * ) ), &app, SLOT( slotResult( KJob * ) ) );
    app.exec();
    return m_ok;
  }
  else if ( command == "download" )
  {
    checkArgumentCount(argc, 0, 0);
    KUrl::List srcLst;
    if (argc == 1) {
       while(true) {
          KUrl src = KUrlRequesterDlg::getURL();
          if (!src.isEmpty()) {
             if (!src.isValid()) {
                KMessageBox::error(0, i18n("Unable to download from an invalid URL."));
                continue;
             }
             srcLst.append(src);
          }
          break;
       }
    } else {
       for ( int i = 1; i <= argc - 1; i++ )
          srcLst.append( args->url(i) );
    }
    if (srcLst.count() == 0)
       return m_ok;
    QString dst =
       KFileDialog::getSaveFileName( (argc<2) ? (QString::null) : (args->url(1).fileName()) );
    if (dst.isEmpty()) // canceled
       return m_ok; // AK - really okay?
    KUrl dsturl;
    dsturl.setPath( dst );
    KIO::Job * job = KIO::copy( srcLst, dsturl );
    if ( !s_interactive )
        job->setInteractive( false );
    connect( job, SIGNAL( result( KJob * ) ), &app, SLOT( slotResult( KJob * ) ) );
    app.exec();
    return m_ok;
  }
  else if ( command == "copy" )
  {
    checkArgumentCount(argc, 2, 0);
    KUrl::List srcLst;
    for ( int i = 1; i <= argc - 2; i++ )
      srcLst.append( args->url(i) );

    KIO::Job * job = KIO::copy( srcLst, args->url(argc - 1) );
    if ( !s_interactive )
        job->setInteractive( false );
    connect( job, SIGNAL( result( KJob * ) ), &app, SLOT( slotResult( KJob * ) ) );
    app.exec();
    return m_ok;
  }
  else if ( command == "sortDesktop" )
  {
    checkArgumentCount(argc, 1, 1);

    KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
    kdesky.rearrangeIcons( (int)false );

    return true;
  }
  else if ( command == "configure" )
  {
    needDCOP();
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
    fprintf( stderr, "%s", i18n("Syntax Error: Unknown command '%1'\n", QString::fromLocal8Bit(command)).toLocal8Bit().data() );
    return false;
  }
  return true;
}

void clientApp::slotResult( KJob * job )
{
  if (job->error() && s_interactive)
    static_cast<KIO::Job*>(job)->showErrorDialog();
  m_ok = !job->error();
  quit();
}

void clientApp::slotDialogCanceled()
{
    m_ok = false;
    quit();
}

#include "kfmclient.moc"
