/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

// Popup menus for kfm icons. Only the 'New' submenu for the moment.
// (c) David Faure, 1998

#include <qdir.h>
#include <qmsgbox.h>

#include <kapp.h>
#include <kurl.h>
#include <ksimpleconfig.h>
#include <klineeditdlg.h>

#include <kio_interface.h>
#include <kio_job.h>

#include "userpaths.h"
// --- Sven's check if this is global apps/mime start ---
//#include "kfm.h"
// --- Sven's check if this is global apps/mime end ---
#include "kfmpopup.h"

QStringList * KNewMenu::templatesList = 0L;
int KNewMenu::templatesVersion = 0;

KNewMenu::KNewMenu( OpenPartsUI::Menu_ptr menu) : menuItemsVersion(0)
{
    if ( CORBA::is_nil( menu ) )
    {
      m_bUseOPMenu = false;
      m_pMenu = new QPopupMenu;
      m_vMenu = 0L;

      QObject::connect( m_pMenu, SIGNAL(activated(int)),
                        this, SLOT(slotNewFile(int)) );
      QObject::connect( m_pMenu, SIGNAL(aboutToShow()),
                        this, SLOT(slotCheckUpToDate()) );
    }
    else
    {
      m_bUseOPMenu = true;
      m_vMenu = OpenPartsUI::Menu::_duplicate( menu );
      m_pMenu = 0L;
    }

    fillMenu();
}

void KNewMenu::setPopupFiles(QStringList & _files)
{
  popupFiles = _files;
  /*
  popupFiles.clear();
  const char *s;
  for ( s = _files.first(); s != 0L; s = _files.next() )
    popupFiles.append( s );
  */
}

void KNewMenu::slotCheckUpToDate( )
{
    if (menuItemsVersion < templatesVersion)
    {
        fillMenu();
        menuItemsVersion = templatesVersion;
    }
}

void KNewMenu::fillMenu()
{
    if ( m_bUseOPMenu && CORBA::is_nil( m_vMenu ) )
      return;

    if (!templatesList) { // No templates list up to now
        templatesList = new QStringList();
        fillTemplates();
        menuItemsVersion = templatesVersion;
    }

    if ( m_bUseOPMenu )
    {
      m_vMenu->clear();
      m_vMenu->insertItem( i18n( "Folder" ), CORBA::Object::_nil(), 0L, 0 );
    }
    else
    {
      m_pMenu->clear();
      m_pMenu->insertItem( i18n( "Folder" ) );
    }

    QStringList::Iterator templ = templatesList->begin(); // skip 'Folder'
    for ( ++templ; templ != templatesList->end(); ++templ)
    {
        KSimpleConfig config(UserPaths::templatesPath() + *templ, true);
        config.setGroup( "KDE Desktop Entry" );
        if ( templ->right(7) == ".kdelnk" )
            templ->truncate( templ->length() - 7 );
        if ( m_bUseOPMenu )
	  {
	    m_vMenu->insertItem( config.readEntry("Name", *templ ).data(), CORBA::Object::_nil(), 0L, 0 );
	  }    
	else
	  m_pMenu->insertItem( config.readEntry("Name", *templ ) );
    }
}

void KNewMenu::fillTemplates()
{
    templatesVersion++;

    templatesList->clear();
    templatesList->append( "Folder" );

    QDir d( UserPaths::templatesPath() );
    const QFileInfoList *list = d.entryInfoList();
    if ( list == 0L )
        warning(i18n("ERROR: Template does not exist '%s'"),
		UserPaths::templatesPath().data());
    else
    {
	QFileInfoListIterator it( *list );      // create list iterator
	QFileInfo *fi;                          // pointer for traversing

	while ( ( fi = it.current() ) != 0L )
	{
	    if ( strcmp( fi->fileName(), "." ) != 0 &&
		 strcmp( fi->fileName(), ".." ) != 0 &&
                 !fi->isDir() && fi->isReadable())
	    {
		templatesList->append( fi->fileName() );
	    }
	    ++it;                               // goto next list element
	}
    }
}

void KNewMenu::slotNewFile( int _id )
{
    if ( m_bUseOPMenu )
    {
      if ( CORBA::is_nil( m_vMenu ) )
        return;

      if ( m_vMenu->text( (CORBA::Long)_id ) == 0)
	return;
    }
    else if ( m_pMenu->text( _id ) == 0 )
      return;

    m_sDest.clear();
    m_sDest.setAutoDelete(true);

    QString p;
    
    if ( m_bUseOPMenu )
      p = *(templatesList->at( m_vMenu->indexOf( _id ) ));
    else
      p = *(templatesList->at( m_pMenu->indexOf( _id ) ));
      
    QString tmp = p;

    if ( strcmp( tmp, "Folder" ) != 0 ) {
      QString x = UserPaths::templatesPath() + p;
      if (!QFile::exists(x)) {
          QMessageBox::critical( 0L, i18n( "Error" ), i18n(
              "Source file doesn't exist anymore ! \n"
              "Use \"Rescan Bindings\" in View menu to update the menu"));
          return;
      }
      KSimpleConfig config(x, true);
      config.setGroup( "KDE Desktop Entry" );
      if ( tmp.right(7) == ".kdelnk" )
	tmp.truncate( tmp.length() - 7 );
      tmp = config.readEntry("Name", tmp);
    }

    QString text = i18n("New ");
    text += tmp;
    text += ":";
    const char *value = p;

    if ( strcmp( tmp, "Folder" ) == 0 ) {
	value = "";
	text = i18n("New Folder");
	text += ":";
    }

    KLineEditDlg l( text, value, 0L, true );
    if ( l.exec() )
    {
	QString name = l.text();
	if ( name.length() == 0 )
	    return;

        QStringList::Iterator it = popupFiles.begin();
	if ( strcmp( p, "Folder" ) == 0 )
	{
            for ( ; it != popupFiles.end(); ++it )
	    {
     	      KIOJob * job = new KIOJob;
	      KURL url( *it );
	      url.addPath( name );
	      QString u2 = url.url();
	      // --- Sven's check if this is global apps/mime start ---
	      // This is a bug fix for bug report from A. Pour from
	      // Mietierra (sp?)
	      // User wants to create dir in global mime/apps dir;

	      //	      Kfm::setUpDest(&u2); // this checks & repairs destination
	      // --- Sven's check if global apps/mime end ---
	      job->mkdir( u2, -1 );
            }
	}
	else
	{
	    QString src = UserPaths::templatesPath() + p;
            for ( ; it != popupFiles.end(); ++it )
            {
                KIOJob * job = new KIOJob( );
		KURL dest( *it );
		dest.addPath( name );

		//QString u2 = url.url();
		// debugT("Command copy '%s' '%s'\n",src.data(),dest.data());

		// --- Sven's check if this is global apps/mime start ---
		// This is a bug fix for bug report from A. Pour from
		// Mietierra (sp?)
		// User wants to create new entry in global mime/apps dir;
		//		Kfm::setUpDest(&u2);
		// --- Sven's check if global apps/mime end ---

                if ( name.right(7) ==".kdelnk" )
                {
                  m_sDest.insert( job->id(), new QString( dest.url() ) );
                  connect(job, SIGNAL( finished( int ) ), this, SLOT( slotCopyFinished( int ) ) );
                }

                job->copy( src, dest.url() );
            }
	}
    }
}

void KNewMenu::slotCopyFinished( int id )
{
  // Now open the properties dialog on the file, as it was a kdelnk
  // TODO
  //(void) new Properties( m_sDest.find( id )->data() );
  m_sDest.remove( id );
}

#include "kfmpopup.moc"
