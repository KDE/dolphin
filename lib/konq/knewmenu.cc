/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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

#include <qdir.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kapp.h>
#include <kurl.h>
#include <ksimpleconfig.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kdesktopfile.h>

#include <kio_interface.h>
#include <kio_job.h>
#include <kuserpaths.h>

#include "kpropsdlg.h"
#include "knewmenu.h"

QStringList * KNewMenu::templatesList = 0L;
int KNewMenu::templatesVersion = 0;

KNewMenu::KNewMenu( QActionCollection * _collec, const char *name ) : 
  QActionMenu( i18n( "&New" ), _collec, name ), m_actionCollection( _collec ), menuItemsVersion(0)
{
    fillMenu();
}

void KNewMenu::setPopupFiles(QStringList & _files)
{
    popupFiles = _files;
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
    if (!templatesList) { // No templates list up to now
        templatesList = new QStringList();
        fillTemplates();
        menuItemsVersion = templatesVersion;
    }
    
    popupMenu()->clear();
    KAction * act = new KAction( i18n( "Folder" ), 0, this, SLOT( slotNewFile() ),
                                 m_actionCollection, QString("newmenu1") );

    act->plug( popupMenu() );

    int i = 2;
    QStringList::Iterator templ = templatesList->begin(); // skip 'Folder'
    for ( ++templ; templ != templatesList->end(); ++templ, ++i)
    {
        KSimpleConfig config(KUserPaths::templatesPath() + *templ, true);
        config.setDesktopGroup();
        QString name = *templ;
        if ( name.right(8) == ".desktop" )
            name.truncate( name.length() - 8 );
        if ( name.right(7) == ".kdelnk" )
            name.truncate( name.length() - 7 );
        name = config.readEntry("Name", name );
        KAction * act = new KAction( name, 0, this, SLOT( slotNewFile() ),
                                 m_actionCollection, QString("newmenu%1").arg( i ) );
        act->plug( popupMenu() );
    }
}

void KNewMenu::fillTemplates()
{
    templatesVersion++;

    templatesList->clear();
    templatesList->append( "Folder" );

    QDir d( KUserPaths::templatesPath() );
    const QFileInfoList *list = d.entryInfoList();
    if ( list == 0L )
        warning(i18n("ERROR: Template does not exist '%s'"),
		KUserPaths::templatesPath().ascii());
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

void KNewMenu::slotNewFile()
{
    int id = QString( sender()->name() + 7 ).toInt(); // skip "newmenu"
    if (id == 0) return;

    m_sDest.clear();
    m_sDest.setAutoDelete(true);

    QString sFile = *(templatesList->at( id - 1 ));
    kdebug(0, 1203, QString("sFile = %1").arg(sFile));
      
    QString sName ( sFile );
    QString text, value;

    if ( sName != "Folder" ) {
      QString x = KUserPaths::templatesPath() + sFile;
      if (!QFile::exists(x)) {
          kdebug(KDEBUG_WARN, 1203, "%s doesn't exist", x.ascii());
          KMessageBox::sorry( 0L, i18n("Source file doesn't exist anymore !"));
          return;
      }
      if ( KDesktopFile::isDesktopFile( x ) )
      {
          QStringList::Iterator it = popupFiles.begin();
          for ( ; it != popupFiles.end(); ++it )
          {
              (void) new PropertiesDialog( x, *it, sFile );
          }
          return; // done, exit.
      }

      // Not a desktop file, nor a folder.
      text = i18n("New ") + sName + ":";
      value = sFile;
    } else {
      value = "";
      text = i18n("New Folder :");
    }

    KLineEditDlg l( text, value, 0L, true );
    if ( l.exec() )
    {
	QString name = l.text();
	if ( name.length() == 0 )
	    return;

        QStringList::Iterator it = popupFiles.begin();
	if ( sFile =="Folder" )
	{
            for ( ; it != popupFiles.end(); ++it )
	    {
     	      KIOJob * job = new KIOJob;
	      KURL url( *it );
	      url.addPath( name );
	      job->mkdir( url.url(), -1 );
            }
	}
	else
	{
	    QString src = KUserPaths::templatesPath() + sFile;
            for ( ; it != popupFiles.end(); ++it )
            {
                KIOJob * job = new KIOJob( );
		KURL dest( *it );
		dest.addPath( name );

                job->copy( src, dest.url() );
            }
	}
    }
}

#include "knewmenu.moc"
