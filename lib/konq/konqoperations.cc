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

#include <qwidget.h>
#include <konqoperations.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kpropsdlg.h>

#include <dcopclient.h>
#include <konq_dirwatcher_stub.h>

// For doDrop
#include <qpopupmenu.h>
#include <qdir.h>
#include <kio/paste.h>
#include <kio/job.h>
#include <klocale.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kapp.h>
#include <kstddirs.h>
#include <konqfileitem.h>
#include <konqdrag.h>
#include <assert.h>
#include <unistd.h>
#include <kglobalsettings.h>
#include <X11/Xlib.h>

KonqOperations::KonqOperations( QWidget *parent )
: QObject( parent, "KonqOperations" )
{
  m_bSkipConfirmation = false;
}

void KonqOperations::editMimeType( const QString & mimeType )
{
    QString mimeTypeFile = locate("mime", mimeType + ".desktop");
  if ( mimeTypeFile.isEmpty() )
  {
    mimeTypeFile = locate("mime", mimeType + ".kdelnk");
    if ( mimeTypeFile.isEmpty() )
    {
      mimeTypeFile = locate("mime", mimeType );
      if ( mimeTypeFile.isEmpty() )
        return; // hmmm
    }
  }

  (void) new KPropertiesDialog( mimeTypeFile  );
}

void KonqOperations::del( QWidget * parent, int method, const KURL::List & selectedURLs )
{
  kdDebug(1203) << "KonqOperations::del " << parent->className() << endl;
  if ( selectedURLs.isEmpty() )
  {
    kdWarning(1203) << "Empty URL list !" << endl;
    return;
  }
  KonqOperations * op = new KonqOperations( parent );
  op->_del( method, selectedURLs );
}

void KonqOperations::emptyTrash()
{
  KonqOperations *op = new KonqOperations( 0L );;
  op->m_bSkipConfirmation = true;

  QDir trashDir( KGlobalSettings::trashPath() );
  QStringList files = trashDir.entryList( QDir::Files | QDir::Dirs );
  files.remove(QString("."));
  files.remove(QString(".."));

  QStringList::Iterator it(files.begin());
  for (; it != files.end(); ++it )
    (*it).prepend( KGlobalSettings::trashPath() );

  KURL::List urls;
  it = files.begin();
  for (; it != files.end(); ++it )
    urls.append( *it );

  if ( urls.count() > 0 )
    op->_del( DEL, urls );
}

void KonqOperations::_del( int method, const KURL::List & selectedURLs )
{
  if ( m_bSkipConfirmation || askDeleteConfirmation( selectedURLs ) )
  {
    m_method = method;
    m_srcURLs = selectedURLs;
    KIO::Job *job;
    switch( method )
    {
      case TRASH:
        job = KIO::move( selectedURLs, KGlobalSettings::trashPath() );
        break;
      case DEL:
        job = KIO::del( selectedURLs );
        break;
      case SHRED:
        job = KIO::del( selectedURLs, true );
        break;
      default:
        ASSERT(0);
        delete this;
        return;
    }
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotResult( KIO::Job * ) ) );
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
      QStringList prettyList;
      for ( ; it != selectedURLs.end(); ++it )
        prettyList.append( (*it).prettyURL() );

      return ( KMessageBox::questionYesNoList(0,
                   i18n( "Do you really want to delete the file(s) ?" ), prettyList )
               == KMessageBox::Yes );
    }
    return true;
}

//static
void KonqOperations::doDrop( const KonqFileItem * destItem, QDropEvent * ev, QWidget * parent )
{
    KURL dest = destItem->url();
    //kdDebug() << "dest : " << dest.url() << endl;
    KURL::List lst;
    if ( KonqDrag::decode( ev, lst ) ) // Are they urls ?
    {
	if( lst.count() == 0 )
	{
	    kdWarning(1203) << "Oooops, no data ...." << endl;
	    return;
	}
        // Check if we dropped something on itself
        KURL::List::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
        {
            kdDebug() << "URL : " << (*it).url() << endl;
            if ( dest.cmp( *it, true /*ignore trailing slashes*/ ) )
                return; // do nothing instead of displaying kfm's annoying error box
        }

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

            if ( dest.path( 1 ) == KGlobalSettings::trashPath() )
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

            KonqOperations * op = new KonqOperations( parent );
            KIO::Job * job;
            switch ( ev->action() ) {
                case QDropEvent::Move :
                  job = KIO::move( lst, dest );
                  op->setOperation( job, MOVE, lst, dest );
                  break;
                case QDropEvent::Copy :
                  job = KIO::copy( lst, dest );
                  op->setOperation( job, COPY, lst, dest );
                  break;
                case QDropEvent::Link :
                  KIO::link( lst, dest );
                  op->setOperation( 0L, LINK, lst, dest ); // triggers slotResult at once
                  break;
                default : kdError(1203) << "Unknown action " << (int)ev->action() << endl; delete op; return;
            }
        } else
        {
            // (If this fails, there is a bug in KonqFileItem::acceptsDrops)
            assert( dest.isLocalFile() );
            if ( destItem->mimetype() == "application/x-desktop")
            {
                // Local .desktop file
                QString error;
                QStringList stringList;
                KURL::List::Iterator it = lst.begin();
                for ( ; it != lst.end() ; it++ )
                {
                    stringList.append((*it).url());
                }
                if ( KApplication::startServiceByDesktopPath( dest.path(), stringList, &error ) > 0 )
                    KMessageBox::error( 0L, error );
            } else
            {
                // Should be a local executable
                // (If this fails, there is a bug in KonqFileItem::acceptsDrops)
                assert ( access( dest.path(), X_OK ) == 0 );
                // Launch executable for each of the files
                KURL::List::Iterator it = lst.begin();
                for ( ; it != lst.end() ; it++ )
                {
                    KProcess proc;
                    proc << dest.path() << (*it).path(); // assume local files
                    kdDebug(1203) << "starting " << dest.path() << " " << (*it).path() << endl;
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
            kdDebug(1203) << "Pasting to " << dest.url() << endl;
            KIO::pasteData( dest, ev->data( formats.first() ) );
        }
    }
}

void KonqOperations::setOperation( KIO::Job * job, int method, const KURL::List & src, const KURL & dest )
{
  m_method = method;
  m_srcURLs = src;
  m_destURL = dest;
  if ( job )
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotResult( KIO::Job * ) ) );
  else // for link
    slotResult( 0L );
}

void KonqOperations::slotResult( KIO::Job * job )
{
    if (job && job->error())
        job->showErrorDialog( (QWidget*)parent() );
    else
    {
      kdDebug() << "KonqOperations::slotResult : notifying the KonqDirListers" << endl;
      KonqDirWatcher_stub allDirWatchers("*", "KonqDirWatcher*");
      switch (m_method) {
        case TRASH:
            // Notify the listers showing the trash that new files are there
            allDirWatchers.FilesAdded( KGlobalSettings::trashPath() );
            // fall through
        case SHRED:
        case DEL:
            // Notify about the deletions
            allDirWatchers.FilesRemoved( m_srcURLs );
            break;
        case MOVE:
            allDirWatchers.FilesRemoved( m_srcURLs );
            // fall through
        case COPY:
        case LINK:
            allDirWatchers.FilesAdded( m_destURL );
            break;
        default:
            ASSERT(0);
      }
    }
    delete this;
}

#include <konqoperations.moc>
