/*  This file is part of the KDE project
    Copyright (C) 2000  David Faure <faure@kde.org>

#include "konq_operations.h"

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
#include <qclipboard.h>
#include <konq_operations.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>

#include <kdirwatch.h>
#include <kpropsdlg.h>
#include <kdirnotify_stub.h>

#include <dcopclient.h>
#include "konq_undo.h"
#include "konq_defaults.h"

// For doDrop
#include <qdir.h>//first
#include <assert.h>
#include <kapp.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kurldrag.h>
#include <kglobalsettings.h>
#include <kimageio.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <klocale.h>
#include <konq_drag.h>
#include <konq_fileitem.h>
#include <konq_iconviewwidget.h>
#include <kprocess.h>
#include <kstringhandler.h>
#include <kstddirs.h>
#include <qpopupmenu.h>
#include <qtextstream.h>
#include <unistd.h>
#include <X11/Xlib.h>

KonqOperations::KonqOperations( QWidget *parent )
    : QObject( parent, "KonqOperations" ), m_info(0L)
{
}

KonqOperations::~KonqOperations()
{
    delete m_info;
}

void KonqOperations::editMimeType( const QString & mimeType )
{
  QString keditfiletype = QString::fromLatin1("keditfiletype");
  KRun::runCommand( keditfiletype + " " + mimeType,
                    keditfiletype, keditfiletype /*unused*/);
}

void KonqOperations::del( QWidget * parent, int method, const KURL::List & selectedURLs )
{
  kdDebug(1203) << "KonqOperations::del " << parent->className() << endl;
  if ( selectedURLs.isEmpty() )
  {
    kdWarning(1203) << "Empty URL list !" << endl;
    return;
  }
  // We have to check the trash itself isn't part of the selected
  // URLs.
  bool bTrashIncluded = false;
  KURL::List::ConstIterator it = selectedURLs.begin();
  for ( ; it != selectedURLs.end() && !bTrashIncluded; ++it )
      if ( (*it).isLocalFile() && (*it).path(1) == KGlobalSettings::trashPath() )
          bTrashIncluded = true;
  int confirmation = DEFAULT_CONFIRMATION;
  if ( bTrashIncluded )
  {
      switch ( method ) {
          case TRASH:
              // Can't trash the trash
              // TODO KMessageBox
              return;
          case DEL:
          case SHRED:
              confirmation = FORCE_CONFIRMATION;
              break;
      }
  }
  KonqOperations * op = new KonqOperations( parent );
  op->_del( method, selectedURLs, confirmation );
}

void KonqOperations::emptyTrash()
{
  KonqOperations *op = new KonqOperations( 0L );

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
  {
    KURL u;
    u.setPath( *it );
    urls.append( u );
  }

  if ( urls.count() > 0 )
    op->_del( EMPTYTRASH, urls, SKIP_CONFIRMATION );

}

void KonqOperations::doPaste( QWidget * parent, const KURL & destURL )
{
    // move or not move ?
    bool move = false;
    QMimeSource *data = QApplication::clipboard()->data();
    if ( data->provides( "application/x-kde-cutselection" ) ) {
      move = KonqDrag::decodeIsCutSelection( data );
      kdDebug(1203) << "move (from clipboard data) = " << move << endl;
    }

    KIO::Job *job = KIO::pasteClipboard( destURL, move );
    if ( job )
    {
        KonqOperations * op = new KonqOperations( parent );
        KIO::CopyJob * copyJob = static_cast<KIO::CopyJob *>(job);
        op->setOperation( job, move ? MOVE : COPY, copyJob->srcURLs(), copyJob->destURL() );
        (void) new KonqCommandRecorder( move ? KonqCommand::MOVE : KonqCommand::COPY, KURL::List(), destURL, job );
    }
}

void KonqOperations::_del( int method, const KURL::List & selectedURLs, int confirmation )
{
  m_method = method;
  if ( confirmation == SKIP_CONFIRMATION || askDeleteConfirmation( selectedURLs, confirmation ) )
  {
    m_srcURLs = selectedURLs;
    KIO::Job *job;
    switch( method )
    {
      case TRASH:
        job = KIO::move( selectedURLs, KGlobalSettings::trashPath() );
        (void) new KonqCommandRecorder( KonqCommand::MOVE, selectedURLs, KGlobalSettings::trashPath(), job );
        break;
      case EMPTYTRASH:
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

bool KonqOperations::askDeleteConfirmation( const KURL::List & selectedURLs, int confirmation )
{
    bool ask = ( confirmation == FORCE_CONFIRMATION );
    if ( !ask )
    {
        KConfig *config = new KConfig("konquerorrc", false, true);
        config->setGroup( "Trash" );
        QString groupName = ( m_method == DEL ? "ConfirmDelete" : m_method == SHRED ? "ConfirmShred" : "ConfirmTrash" );
        bool defaultValue = ( m_method == DEL ? DEFAULT_CONFIRMDELETE : m_method == SHRED ? DEFAULT_CONFIRMSHRED : DEFAULT_CONFIRMTRASH );
        ask = config->readBoolEntry( groupName, defaultValue );
        delete config;
    }
    if ( ask )
    {
      KURL::List::ConstIterator it = selectedURLs.begin();
      QStringList prettyList;
      for ( ; it != selectedURLs.end(); ++it )
        prettyList.append( (*it).prettyURL() );

      if ( prettyList.count() == 1 )
      {
        //TODO "Do you really want to delete <filename> from <directory>"
        // so that it's possible to use KIO::decodeName on the filename
        QString url = KStringHandler::csqueeze(prettyList.first());
        QString msg = (m_method == DEL ? i18n( "Do you really want to delete '%1'?" ).arg( url ) :
                       m_method == SHRED ? i18n( "Do you really want to shred '%1'?" ).arg( url ) :
                       i18n( "Do you really want to move '%1' to the trash?" ).arg( url ));
        return ( KMessageBox::questionYesNo( 0, msg ) == KMessageBox::Yes );
      }
      else
      {
        QString msg = (m_method == DEL ? i18n( "Do you really want to delete the files?" ) :
                       m_method == SHRED ? i18n( "Do you really want to shred the files?" ) :
                       i18n( "Do you really want to move the files to the trashcan?" ));

        return ( KMessageBox::questionYesNoList( 0, msg, prettyList ) == KMessageBox::Yes );
      }
    }
    return true;
}

//static
void KonqOperations::doDrop( const KonqFileItem * destItem, const KURL & dest, QDropEvent * ev, QWidget * parent )
{
    kdDebug(1203) << "dest : " << dest.url() << endl;
    KURL::List lst;
    if ( KURLDrag::decode( ev, lst ) ) // Are they urls ?
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
            kdDebug(1203) << "URL : " << (*it).url() << endl;
            if ( dest.cmp( *it, true /*ignore trailing slashes*/ ) )
            {
                // The event source may be the view or an item (icon)
                // Note: ev->source() can be 0L! (in case of kdesktop) (Simon)
                if ( !ev->source() || ev->source() != parent && ev->source()->parent() != parent )
                    KMessageBox::sorry( parent, i18n("You can't drop a directory on itself") );
                return; // do nothing instead of displaying kfm's annoying error box
            }
        }

        // Check the state of the modifiers key at the time of the drop
        Window root;
        Window child;
        int root_x, root_y, win_x, win_y;
        uint keybstate;
        XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                       &root_x, &root_y, &win_x, &win_y, &keybstate );

        KonqOperations * op = new KonqOperations(parent);
        op->setDropInfo( new DropInfo( keybstate, lst, win_x, win_y ) );

        // Ok, now we need destItem.
        if ( destItem )
            op->asyncDrop( destItem ); // we have it already
        else
        {
            // we need to stat to get it
            op->_statURL( dest, op, SLOT( asyncDrop( const KFileItem * ) ) );
        }

        ev->acceptAction(TRUE);
        ev->accept();
    }
    else
    {
        QStrList formats;

        for ( int i = 0; ev->format( i ); i++ )
            if ( *( ev->format( i ) ) )
                formats.append( ev->format( i ) );
        if ( formats.count() >= 1 )
        {
            kdDebug(1203) << "Pasting to " << dest.url() << endl;

            QByteArray data;

            QString text;
            if ( QTextDrag::canDecode( ev ) && QTextDrag::decode( ev, text ) )
            {
                QTextStream txtStream( data, IO_WriteOnly );
                txtStream << text;
            }
            else
                data = ev->data( formats.first() );

            KIO::pasteData( dest, data );
        }
    }
}

void KonqOperations::asyncDrop( const KFileItem * destItem )
{
    assert(m_info); // setDropInfo should have been called before asyncDrop
    KURL dest = destItem->url();
    KURL::List & lst = m_info->lst;
    kdDebug() << "KonqOperations::asyncDrop destItem->mode=" << destItem->mode() << endl;
    // Check what the destination is
    if ( S_ISDIR(destItem->mode()) )
    {
        QDropEvent::Action action = QDropEvent::Copy;
        if ( dest.path( 1 ) == KGlobalSettings::trashPath() )
            action = QDropEvent::Move;
        else if ( ((m_info->keybstate & ControlMask) == 0) && ((m_info->keybstate & ShiftMask) == 0) )
        {
            KonqIconViewWidget *iconView = dynamic_cast<KonqIconViewWidget*>(parent());
            bool bSetWallpaper = false;
            if (iconView && iconView->isDesktop() &&
                (lst.count() == 1) &&
                (!KImageIO::type(lst.first().path()).isEmpty()))
            {
                bSetWallpaper = true;
            }

            // Nor control nor shift are pressed => show popup menu
            QPopupMenu popup;
            popup.insertItem( i18n( "&Copy Here" ), 1 );
            popup.insertItem( i18n( "&Move Here" ), 2 );
            popup.insertItem( i18n( "&Link Here" ), 3 );
            if (bSetWallpaper)
                popup.insertItem( i18n( "Set as &Wallpaper"), 4 );

            int result = popup.exec( m_info->mousePos );
            switch (result) {
                case 1 : action = QDropEvent::Copy; break;
                case 2 : action = QDropEvent::Move; break;
                case 3 : action = QDropEvent::Link; break;
                case 4 :
                {
                    if (iconView) iconView->setWallpaper(lst.first());
                    delete this;
                    return;
                }
                default : return;
            }
        }

        KIO::Job * job = 0;
        switch ( action ) {
            case QDropEvent::Move :
                job = KIO::move( lst, dest );
                setOperation( job, MOVE, lst, dest );
                (void) new KonqCommandRecorder( KonqCommand::MOVE, lst, dest, job );
                return; // we still have stuff to do -> don't delete ourselves
            case QDropEvent::Copy :
                job = KIO::copy( lst, dest );
                setOperation( job, COPY, lst, dest );
                (void) new KonqCommandRecorder( KonqCommand::COPY, lst, dest, job );
                return;
            case QDropEvent::Link :
                job = KIO::link( lst, dest );
                setOperation( 0L, LINK, lst, dest );
                (void) new KonqCommandRecorder( KonqCommand::LINK, lst, dest, job );
                return;
            default : kdError(1203) << "Unknown action " << (int)action << endl;
        }
    } else
    {
        // (If this fails, there is a bug in KonqFileItem::acceptsDrops)
        assert( dest.isLocalFile() );
        if ( destItem->mimetype() == "application/x-desktop")
        {
            // Local .desktop file. What type ?
            KDesktopFile desktopFile( dest.path() );
            if ( desktopFile.hasApplicationType() )
            {
                QString error;
                QStringList stringList;
                KURL::List::Iterator it = lst.begin();
                for ( ; it != lst.end() ; it++ )
                {
                    stringList.append((*it).url());
                }
                if ( KApplication::startServiceByDesktopPath( dest.path(), stringList, &error ) > 0 )
                    KMessageBox::error( 0L, error );
            }
            // else, well: mimetype, link, .directory, device. Can't really drop anything on those.
        } else
        {
            // Should be a local executable
            // (If this fails, there is a bug in KonqFileItem::acceptsDrops)
            kdDebug() << "KonqOperations::doDrop " << dest.path() << "should be an executable" << endl;
            ASSERT ( access( QFile::encodeName(dest.path()), X_OK ) == 0 );
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
    delete this;
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

void KonqOperations::statURL( const KURL & url, const QObject *receiver, const char *member )
{
    KonqOperations * op = new KonqOperations( 0L );
    op->_statURL( url, receiver, member );
}

void KonqOperations::_statURL( const KURL & url, const QObject *receiver, const char *member )
{
    m_method = STAT;
    connect( this, SIGNAL( statFinished( const KFileItem * ) ), receiver, member );
    KIO::StatJob * job = KIO::stat( url /*, false?*/ );
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotResult( KIO::Job * ) ) );
}

void KonqOperations::slotResult( KIO::Job * job )
{
    if (job && job->error())
        job->showErrorDialog( (QWidget*)parent() );
    // Update trash bin icon if trashing files or emptying trash
    bool bUpdateTrash = m_method == TRASH || m_method == EMPTYTRASH;
    // ... or if creating a new file in the trash
    if ( m_method == MOVE || m_method == COPY || m_method == LINK )
    {
        KURL trash;
        trash.setPath( KGlobalSettings::trashPath() );
        if ( m_destURL.cmp( trash, true /*ignore trailing slash*/ ) )
            bUpdateTrash = true;
    }
    if (bUpdateTrash)
    {
        // Update trash bin icon
        KURL trash;
        trash.setPath( KGlobalSettings::trashPath() );
        KURL::List lst;
        lst.append(trash);
        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesChanged( lst );
    }
    if ( m_method == STAT && job && !job->error())
    {
        KIO::StatJob * statJob = static_cast<KIO::StatJob*>(job);
        KFileItem * item = new KFileItem( statJob->statResult(), statJob->url() );
        emit statFinished( item );
        delete item;
    }
    delete this;
}

#include <konq_operations.moc>
