/*  This file is part of the KDE project
    Copyright (C) 2000  David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <konqoperations.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>

// For doDrop
#include <qpopupmenu.h>
#include <kio/paste.h>
#include <kio/job.h>
#include <klocale.h>
#include <kdebug.h>
#include <kprocess.h>
#include <konqfileitem.h>
#include <konqdrag.h>
#include <assert.h>
#include <unistd.h>
#include <kuserpaths.h>
#include <X11/Xlib.h>

void KonqOperations::del( int method, const KURL::List & selectedURLs )
{
  KonqOperations * op = new KonqOperations;
  op->_del( method, selectedURLs );
}

void KonqOperations::_del( int method, const KURL::List & selectedURLs )
{
  if (askDeleteConfirmation( selectedURLs ))
  {
    switch( method )
    {
      case TRASH:
      {
        KIO::Job *job = KIO::move( selectedURLs, KUserPaths::trashPath() );
        connect( job, SIGNAL( result( KIO::Job * ) ),
                 SLOT( slotResult( KIO::Job * ) ) );
        break;
      }
      case DEL:
      {
        KIO::Job *job = KIO::del( selectedURLs );
        connect( job, SIGNAL( result( KIO::Job * ) ),
                 SLOT( slotResult( KIO::Job * ) ) );
        break;
      }
      case SHRED:
      {
        KIO::Job *job = KIO::del( selectedURLs, true );
        connect( job, SIGNAL( result( KIO::Job * ) ),
                 SLOT( slotResult( KIO::Job * ) ) );
        break;
      }
    }
  } else
    delete this;
}

bool KonqOperations::askDeleteConfirmation( const KURL::List & selectedURLs )
{
    KConfig *config = new KConfig("konquerorrc", false, true);
    config->setGroup( "Trash" );
    if ( config->readBoolEntry( "ConfirmDestructive", true ) )
    {
      KURL::List::ConstIterator it = selectedURLs.begin();
      QStringList decodedList;
      for ( ; it != selectedURLs.end(); ++it )
        decodedList.append( (*it).decodedURL() );

      return ( KMessageBox::questionYesNoList(0,
                   i18n( "Do you really want to delete the file(s) ?" ), decodedList )
               == KMessageBox::Yes );
    }
    return true;
}

//static
void KonqOperations::doDrop( const KonqFileItem * destItem, QDropEvent * ev, QObject * receiver )
{
    KURL dest = destItem->url();
    //kdDebug() << "dest : " << dest.url() << endl;
    QStringList lst;
    if ( KonqDrag::decode( ev, lst ) ) // Are they urls ?
    {
	if( lst.count() == 0 )
	{
	    kDebugWarning(1202,"Oooops, no data ....");
	    return;
	}
        // Check if we dropped something on itself
        QStringList::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
            if ( dest.cmp( KURL(*it), true /*ignore trailing slashes*/ ) )
                return; // do nothing instead of diaplying kfm's annoying error box

        // Check what the destination is
        if ( S_ISDIR(destItem->mode()) )
        {
            // Check the state of the modifiers key at the time of the drop
            Window root;
            Window child;
            int root_x, root_y, win_x, win_y;
            uint keybstate;
            XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                           &root_x, &root_y, &win_x, &win_y, &keybstate );

            if ( dest.path( 1 ) == KUserPaths::trashPath() )
                ev->setAction( QDropEvent::Move );
            else if ( ((keybstate & ControlMask) == 0) && ((keybstate & ShiftMask) == 0) )
            {
                // Nor control nor shift are pressed => show popup menu
                QPopupMenu popup;
                popup.insertItem( i18n( "&Copy Here" ), 1 );
                popup.insertItem( i18n( "&Move Here" ), 2 );
                popup.insertItem( i18n( "&Link Here" ), 3 );

                int result = popup.exec( QPoint( win_x, win_y ) );
                switch (result) {
                    case 1 : ev->setAction( QDropEvent::Copy ); break;
                    case 2 : ev->setAction( QDropEvent::Move ); break;
                    case 3 : ev->setAction( QDropEvent::Link ); break;
                    default : return;
                }
            }

            KIO::Job * job = 0L;
            switch ( ev->action() ) {
                case QDropEvent::Move : job = KIO::move( lst, dest ); break;
                case QDropEvent::Copy : job = KIO::copy( lst, dest ); break;
                case QDropEvent::Link : KIO::link( lst, dest ); break;
                default : kDebugError( 1202, "Unknown action %d", ev->action() ); return;
            }
            connect( job, SIGNAL( result( KIO::Job * ) ),
                     receiver, SLOT( slotResult( KIO::Job * ) ) );
        } else
        {
            // (If this fails, there is a bug in KonqFileItem::acceptsDrops)
            assert( dest.isLocalFile() );
            if ( destItem->mimetype() == "application/x-desktop")
            {
                // Local .desktop file
                KDesktopFile desktopFile( dest.path() );
                // Launch .desktop file for each dropped URL
                // (TODO honour multiple urls for the same process, needs
                //  extension of KService::startService)
                KService service( &desktopFile );
                QCString dcopService;
                QString error;
                QStringList::Iterator it = lst.begin();
                for ( ; it != lst.end() ; it++ )
                    if ( service.startService( *it, dcopService, error ) > 0 )
                        KMessageBox::error( 0L, error );
            } else
            {
                // Should be a local executable
                // (If this fails, there is a bug in KonqFileItem::acceptsDrops)
                assert ( access( dest.path(), X_OK ) == 0 );
                // Launch executable for each of the files
                QStringList::Iterator it = lst.begin();
                for ( ; it != lst.end() ; it++ )
                {
                    KProcess proc;
                    proc << dest.path() << KURL(*it).path(); // assume local files
                    kdDebug(1203) << "starting " << dest.path() << " " << KURL(*it).path() << endl;
                    proc.start( KProcess::DontCare );
                }
            }
        }

        ev->acceptAction(TRUE);
        ev->accept();
    }
    else
    {
        QStringList formats;

        for ( int i = 0; ev->format( i ); i++ )
            if ( *( ev->format( i ) ) )
                formats.append( ev->format( i ) );
        if ( formats.count() >= 1 )
        {
            kDebugInfo(1202,"Pasting to %s", dest.url().ascii());
            KIO::pasteData( dest, ev->data( formats.first() ) );
        }
    }
}

void KonqOperations::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
    delete this;
}

#include <konqoperations.moc>
