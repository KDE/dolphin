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
#include <kdebug.h>
#include <ksimpleconfig.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirwatch.h>

#include <kio/global.h>
#include <kio/job.h>
#include <kuserpaths.h>

#include "kpropsdlg.h"
#include "knewmenu.h"

QStringList * KNewMenu::templatesList = 0L;
int KNewMenu::templatesVersion = 1; // one step ahead, to force filling the menu
KDirWatch * KNewMenu::m_pDirWatch = 0L;

KNewMenu::KNewMenu( QActionCollection * _collec, const char *name ) :
  KActionMenu( i18n( "&New" ), _collec, name ), m_actionCollection( _collec ),
  menuItemsVersion( 0 )
{
  // Don't fill the menu yet
  // We'll do that in slotCheckUpToDate (should be connected to abouttoshow)
}

void KNewMenu::slotCheckUpToDate( )
{
    if (menuItemsVersion < templatesVersion)
    {
        // We need to clean up the action collection
        // We look for our actions using the group
        QValueList<QAction*> actions = m_actionCollection->actions( "KNewMenu" );
        for( QValueListIterator<QAction*> it = actions.begin(); it != actions.end(); ++it )
            m_actionCollection->remove( *it );

        fillMenu();
        menuItemsVersion = templatesVersion;
    }
}

void KNewMenu::fillMenu()
{
    if (!templatesList) { // No templates list up to now
        templatesList = new QStringList();
        slotFillTemplates();
        menuItemsVersion = templatesVersion;
    }

    popupMenu()->clear();
    KAction * act = new KAction( i18n( "Folder" ), 0, this, SLOT( slotNewFile() ),
                                 m_actionCollection, QString("newmenu1") );

    act->setGroup( "KNewMenu" );
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
        {
            name.truncate( name.length() - 7 );
        }
        name = config.readEntry("Name", name );

        // There might be a .desktop for that one already
        bool bSkip = false;

        QValueList<QAction*> actions = m_actionCollection->actions();
        QValueListIterator<QAction*> it = actions.begin();
        for( ; it != actions.end(); ++it )
        {
          if ( (*it)->text() == name )
          {
            kdDebug(1203) << "skipping" << (*templ) << endl;
            bSkip = true;
          }
        }

        if ( !bSkip )
        {
          KAction * act = new KAction( name, 0, this, SLOT( slotNewFile() ),
                                       m_actionCollection, QString("newmenu%1").arg( i ) );
          act->setGroup( "KNewMenu" );
          act->plug( popupMenu() );
        }
    }
}

void KNewMenu::slotFillTemplates()
{
    // Ensure any changes in the templates dir will call this
    if ( ! m_pDirWatch )
    {
        m_pDirWatch = new KDirWatch( 5000 ); // 5 seconds
        m_pDirWatch->addDir( KUserPaths::templatesPath() );
        connect ( m_pDirWatch, SIGNAL( dirty( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        connect ( m_pDirWatch, SIGNAL( fileDirty( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
    }
    templatesVersion++;

    templatesList->clear();
    templatesList->append( "Folder" );

    QDir d( KUserPaths::templatesPath() );
    const QFileInfoList *list = d.entryInfoList();
    if ( list == 0L )
        KMessageBox::error( 0L, i18n("ERROR: Template does not exist '%1'").arg(
		KUserPaths::templatesPath()));
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

    QString sFile = *(templatesList->at( id - 1 ));
    //kDebugInfo( 1203, QString("sFile = %1").arg(sFile));

    QString sName ( sFile );
    QString text, value;

    if ( sName != "Folder" ) {
      QString x = KUserPaths::templatesPath() + sFile;
      if (!QFile::exists(x)) {
          kDebugWarning( 1203, "%s doesn't exist", x.ascii());
          KMessageBox::sorry( 0L, i18n("Source file doesn't exist anymore !"));
          return;
      }
      if ( KDesktopFile::isDesktopFile( x ) )
      {
          KURL::List::Iterator it = popupFiles.begin();
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

        KURL::List::Iterator it = popupFiles.begin();
	if ( sFile =="Folder" )
	{
            for ( ; it != popupFiles.end(); ++it )
	    {
              QString url = (*it).path(1) + KIO::encodeFileName(name);
	      KURL::encode(url); // hopefully will disappear with next KURL
     	      KIO::Job * job = KIO::mkdir( url );
              connect( job, SIGNAL( result( KIO::Job * ) ),
                       SLOT( slotResult( KIO::Job * ) ) );
            }
	}
	else
	{
	    QString src = KUserPaths::templatesPath() + sFile;
            for ( ; it != popupFiles.end(); ++it )
            {
		KURL dest( *it );
		dest.addPath( name );

                KIO::Job * job = KIO::copy( src, dest );
                connect( job, SIGNAL( result( KIO::Job * ) ),
                         SLOT( slotResult( KIO::Job * ) ) );
            }
	}
    }
}

void KNewMenu::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
}

#include "knewmenu.moc"
