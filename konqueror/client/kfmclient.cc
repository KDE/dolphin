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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <fstream.h>

#include <qdir.h>
#include <qmessagebox.h>
#include <qstring.h>

#include <kio_propsdlg.h>
#include <kregistry.h>
#include <kregfactories.h>

#include "kfmclient.h"

/*
QString displayName()
{
    QString d( getenv( "DISPLAY" ) );
    int i = d.find( ':' );
    if ( i != -1 )
	d[i] = '_';
    if ( d.find( '.' ) == -1 )
	d += ".0";    
    return d;
}
*/

int main( int argc, char **argv )
{
  if ( argc == 1 )
  {
    // Should all the following be i18n'ed ?
    printf("\nSyntax:\n");
    printf("  kfmclient openURL\n"
           "            # Opens a dialog to ask you for the URL\n\n");
    printf("  kfmclient openURL 'url'\n"
           "            # Opens a window showing 'url'. If such a window\n");
    printf("            #   exists, it is shown. 'url' may be \"trash:/\"\n"
           "            #   to open the trash bin.\n\n");
    printf("  kfmclient refreshDesktop\n"
           "            # Refreshes the desktop\n\n");
    printf("  kfmclient refreshDirectory 'url'\n"
           "            # Tells KFM that an URL has changes. If KFM\n");
    printf("            #   is displaying that URL, it will be reloaded.\n\n");
    printf("  kfmclient openProperties 'url'\n"
           "            # Opens a properties menu\n\n");
    printf("  kfmclient exec 'url' ['binding']\n"
           "            # Tries to execute 'url'. 'url' may be a usual\n"
           "            #   URL, this URL will be opened. You may omit\n"
           "            #   'binding'. In this case the default binding\n");
    printf("            #   is tried. Of course URL may be the URL of a\n"
           "            #   document, or it may be a *.kdelnk file.\n");
    printf("            #   This way you could for example mount a device\n"
           "            #   by passing 'Mount default' as binding to \n"
           "            #   'cdrom.kdelnk'\n\n");
    printf("  kfmclient move 'src' 'dest'\n"
           "            # Copies the URL 'src' to 'dest'.\n"
           "            #   'src' may be a list of URLs.\n");
    printf("            #   'dest' may be \"trash:/\" to move the files\n"
           "            #   in the trash bin.\n\n");
    printf("  kfmclient folder 'src' 'dest'\n"
           "            # Like move if 'src' is given,\n"
           "            #   otherwise like openURL src \n\n");
    printf("  kfmclient sortDesktop\n"
           "            # Rearranges all icons on the desktop.\n\n");
    printf("  kfmclient configure\n"
           "            # Re-read KFM's configuration.\n\n");
    printf("*** Examples:\n"
           "  kfmclient exec file:/usr/local/kde/bin/kdehelp Open\n"
           "             // Starts kdehelp\n\n");
    printf("  kfmclient exec file:/root/Desktop/cdrom.kdelnk \"Mount default\"\n"
           "             // Mounts the CDROM\n\n");	
    printf("  kfmclient exec file:/home/weis/data/test.html\n"
           "             // Opens the file with default binding\n\n");
    printf("  kfmclient exec file:/home/weis/data/test.html Netscape\n"
           "             // Opens the file with netscape\n\n");
    printf("  kfmclient exec ftp://localhost/ Open\n"
           "             // Opens new window with URL\n\n");
    printf("  kfmclient exec file:/root/Desktop/emacs.kdelnk\n"
           "             // Starts emacs\n\n");
    printf("  kfmclient exec file:/root/Desktop/cdrom.kdelnk\n"
           "             // Opens the CD-ROM's mount directory\n\n");
    return 0;
  }
    
  clientApp a( argc, argv );

  // Register mimetypes and services, for kio_propsdlg (and krun, later)
  KRegistry registry;
  registry.addFactory( new KMimeTypeFactory );
  registry.addFactory( new KServiceFactory );
  registry.load( );

  return a.doIt( argc, argv );
}

void clientApp::openURL( const char * url)
{
  if ( !getIOR() )
    return ;

  CORBA::String_var s = CORBA::string_dup( url );
   //  Konqueror::MainWindow_ptr mainWindow = Konqueror::MainWindow::_duplicate( 
  (void) m_vApp->createMainWindow( s );
}

int clientApp::doIt( int argc, char **argv )
{
  if ( argc < 2 )
  {
    printf( "Syntax Error: Too few arguments\n" );
    exit(1);
  }
    
  if ( strcmp( argv[1], "refreshDesktop" ) == 0 )
  {
    if ( argc != 2 )
    {
      printf( "Syntax Error: Too many arguments\n" );
      exit(1);
    }
    //	kfm.refreshDesktop();
  }
  else if ( strcmp( argv[1], "sortDesktop" ) == 0 )
  {
    if ( argc != 2 )
    {
      printf( "Syntax Error: Too many arguments\n" );
      exit(1);
    }
    //	kfm.sortDesktop();
  }
  else if ( strcmp( argv[1], "configure" ) == 0 )
  {
    if ( argc != 2 )
    {
      printf( "Syntax Error: Too many arguments\n" );
      exit(1);
    }
    //	kfm.configure();
  }
  else if ( strcmp( argv[1], "openURL" ) == 0 )
  {
    if ( argc == 2 )
    {
      openURL( QDir::homeDirPath() );
    }
    else if ( argc == 3 )
    {
      openURL( argv[2] );
    }
    else
    {
      printf( "Syntax Error: Too many arguments\n" );
      exit(1);
    }
  }
  else if ( strcmp( argv[1], "refreshDirectory" ) == 0 )
  {
    if ( argc == 2 )
    {
      openURL( QDir::homeDirPath() );
    }
    else if ( argc == 3 )
    {
      // kfm.refreshDirectory( argv[2] );
    }
    else
    {
      printf( "Syntax Error: Too many arguments\n" );
      exit(1);
    }
  }
  else if ( strcmp( argv[1], "openProperties" ) == 0 )
  {
    if ( argc == 3 )
    {
      PropertiesDialog * p = new PropertiesDialog( argv[2] );
      QObject::connect( p, SIGNAL( propertiesClosed() ), this, SLOT( quit() ));
      exec();
    }
    else
    {
      printf( "Syntax Error: Too many/few arguments\n" );
      exit(1);
    }
  }
  else if ( strcmp( argv[1], "exec" ) == 0 )
  {
    if ( argc == 3 )
    {
      // kfm.exec( argv[2], 0L );
    }
    else if ( argc == 4 )
    {
      // kfm.exec( argv[2], argv[3] );
    }
    else
    {
      printf( "Syntax Error: Too many/few arguments\n" );
      exit(1);
    }
  }
  else if ( strcmp( argv[1], "move" ) == 0 )
  {
    if ( argc <= 3 )
    {
      printf( "Syntax Error: Too many/few arguments\n" );
      exit(1);
    }
    QString src = "";
    int i = 2;
    while ( i <= argc - 2 )
    {
      src += argv[i];
      if ( i < argc - 2 )
        src += "\n";
      i++;
    }
	
    // kfm.moveClient( src.data(), argv[ argc - 1 ] );
  }
  else if ( strcmp( argv[1], "copy" ) == 0 )
  {
    if ( argc <= 3 )
    {
      printf( "Syntax Error: Too many/few arguments\n" );
      exit(1);
    }
    QString src = "";
    int i = 2;
    while ( i <= argc - 2 )
    {
      src += argv[i];
      if ( i < argc - 2 )
        src += "\n";
      i++;
    }
	
    // kfm.copy( src.data(), argv[ argc - 1 ] );
  }
  else if ( strcmp( argv[1], "folder" ) == 0 )
  {
    if ( argc <=2 )
    {
      printf( "Syntax Error: Too many/few arguments\n" );
      exit(1);
    }

    if (argc > 3) {
      QString src = "";
      int i = 2;
      while ( i <= argc - 2 )
      {
        src += argv[i];
        if ( i < argc - 2 )
          src += "\n";
        i++;
      }
	
      // kfm.moveClient( src.data(), argv[ argc - 1 ] );
    }
    else
    {
      openURL( argv[2] );
    }
  }
  else if ( strcmp( argv[1], "selectRootIcons" ) == 0 )
  {
    if ( argc == 7 )
    {
      int x = atoi( argv[2] );
      int y = atoi( argv[3] );	  
      int w = atoi( argv[4] );
      int h = atoi( argv[5] );
      int add = atoi( argv[6] );
      bool bAdd = (bool)add;
      // kfm.selectRootIcons( x, y, w, h, bAdd );
    }
    else
    {
      printf( "Syntax Error: Too many/few arguments\n" );
      exit(1);
    }
  }
  else
  {
    printf("Syntax Error: Unknown command '%s'\n",argv[1] );
    exit(1);
  }
  return 0;
}

bool clientApp::getIOR()
{
  // TODO : add display name, or switch to Torben's trader ;)
  const QString refFile = kapp->localkdedir()+"/share/apps/konqueror/konqueror.ior";
  //const QString refFile = QDir::homeDirPath() + "/.konqueror.ior"; // not the app !
  ifstream in( refFile.data() );

  if ( in.fail() )
  {
    cerr << " Can't open `" << refFile.data() << "': " << strerror(errno) << endl;
    return false;
  }
  else
  {
    char s[1000];
    in >> s;
    in.close();
    CORBA::Object_var obj = opapp_orb->string_to_object( s );
    
    m_vApp = Konqueror::Application::_narrow( obj );
    if( CORBA::is_nil( m_vApp ) )
    {
      QMessageBox::critical( (QWidget*)0L, i18n("ERROR"), i18n("Can't contact konqueror !"), i18n("Ok") );
      return false;
    } 
    return true;
  }
}

