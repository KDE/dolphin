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
#include <kprotocolinfo.h>
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

void KonqOperations::copy( QWidget * parent, int method, const KURL::List & selectedURLs, const KURL& destUrl )
{
  kdDebug(1203) << "KonqOperations::copy() " << parent->className() << endl;
  if ((method!=COPY) && (method!=MOVE) && (method!=LINK))
  {
    kdWarning(1203) << "Illegal copy method !" << endl;
    return;
  }
  if ( selectedURLs.isEmpty() )
  {
    kdWarning(1203) << "Empty URL list !" << endl;
    return;
  }

  KonqOperations * op = new KonqOperations( parent );
  KIO::Job* job(0);
  if (method==LINK)
     job= KIO::link( selectedURLs, destUrl);
  else if (method==MOVE)
     job= KIO::move( selectedURLs, destUrl);
  else
     job= KIO::copy( selectedURLs, destUrl);

  op->setOperation( job, method,selectedURLs ,destUrl );

  if (method==COPY)
     (void) new KonqCommandRecorder( KonqCommand::COPY, selectedURLs, destUrl, job );
  else
     (void) new KonqCommandRecorder( method==MOVE?KonqCommand::MOVE:KonqCommand::LINK, selectedURLs, destUrl, job );
};

void KonqOperations::_del( int method, const KURL::List & selectedURLs, int confirmation )
{
  m_method = method;
  if ( confirmation == SKIP_CONFIRMATION || askDeleteConfirmation( selectedURLs, confirmation ) )
  {
    //m_srcURLs = selectedURLs;
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
    QString keyName;
    bool ask = ( confirmation == FORCE_CONFIRMATION );
    if ( !ask )
    {
        KConfig config("konquerorrc", true, false);
        config.setGroup( "Trash" );
        keyName = ( m_method == DEL ? "ConfirmDelete" : m_method == SHRED ? "ConfirmShred" : "ConfirmTrash" );
        bool defaultValue = ( m_method == DEL ? DEFAULT_CONFIRMDELETE : m_method == SHRED ? DEFAULT_CONFIRMSHRED : DEFAULT_CONFIRMTRASH );
        ask = config.readBoolEntry( keyName, defaultValue );
    }
    if ( ask )
    {
      KURL::List::ConstIterator it = selectedURLs.begin();
      QStringList prettyList;
      for ( ; it != selectedURLs.end(); ++it )
        prettyList.append( (*it).prettyURL() );

      int result;
      if ( prettyList.count() == 1 )
      {
        QString filename = KStringHandler::csqueeze(KIO::decodeFileName(selectedURLs.first().fileName()));
        QString directory = KStringHandler::csqueeze(selectedURLs.first().directory());
        switch(m_method)
        {
          case DEL:
             result = KMessageBox::warningContinueCancel( 0, 
             	i18n( "Do you really want to delete <b>%1</b> from '%2'?" ).arg( filename ).arg( directory ),
		i18n( "Delete File" ),
		i18n( "Delete" ),
		keyName, false);
	     break;
	     
	  case SHRED:
             result = KMessageBox::warningContinueCancel( 0, 
             	i18n( "Do you really want to shred <b>%1</b>?" ).arg( filename ),
		i18n( "Shred File" ),
		i18n( "Shred" ),
		keyName, false);
	     break;

          case MOVE:
 	  default:
             result = KMessageBox::warningContinueCancel( 0, 
             	i18n( "Do you really want to move <b>%1</b> to the trash?" ).arg( filename ),
		i18n( "Move to Trash" ),
		i18n( "Trash" ),
		keyName, false);
	     break;	
        }
      }
      else
      {
        switch(m_method)
        {
          case DEL:
             result = KMessageBox::warningContinueCancelList( 0, 
             	i18n( "Do you really want to delete these %1 items?" ).arg(prettyList.count()),
             	prettyList,
		i18n( "Delete Files" ),
		i18n( "Delete" ),
		keyName);
	     break;
	     
	  case SHRED:
             result = KMessageBox::warningContinueCancelList( 0, 
                i18n( "Do you really want to shred these %1 items?" ).arg(prettyList.count()),
                prettyList,
                i18n( "Shred Files" ),
		i18n( "Shred" ),
		keyName);
	     break;

          case MOVE:
 	  default:
             result = KMessageBox::warningContinueCancelList( 0, 
                i18n( "Do you really want to move these %1 items to the trashcan?" ).arg(prettyList.count()),
                prettyList,
		i18n( "Move to Trash" ),
		i18n( "Trash" ),
		keyName);
	     break;
        }
      }
      if (!keyName.isEmpty())
      {
         // Check kmessagebox setting... erase & copy to konquerorrc.
         KConfig *config = kapp->config();
         KConfigGroupSaver saver(config, "Notification Messages");
         if (!config->readBoolEntry(keyName, true))
         {
            config->writeEntry(keyName, true);
            config->sync();
            KConfig konq_config("konquerorrc", false);
            konq_config.setGroup( "Trash" );
            konq_config.writeEntry( keyName, false );
         }
      }
      return (result == KMessageBox::Continue);
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
            ev->accept(false);
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
                    KMessageBox::sorry( parent, i18n("You can't drop a directory onto itself") );
                kdDebug(1203) << "Dropped on itself" << endl;
                ev->accept(false);
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

        QDropEvent::Action action = ev->action();
        // Check for the drop of a bookmark -> we want a Link action
        if ( ev->provides("application/x-xbel") )
        {
            keybstate |= ControlMask | ShiftMask;
            action = QDropEvent::Link;
            kdDebug(1203) << "KonqOperations::doDrop Bookmark -> emulating Link" << endl;
        }

        KonqOperations * op = new KonqOperations(parent);
        op->setDropInfo( new DropInfo( keybstate, lst, win_x, win_y, action ) );

        // Ok, now we need destItem.
        if ( destItem )
        {
            op->asyncDrop( destItem ); // we have it already
        }
        else
        {
            // we need to stat to get it.
            op->_statURL( dest, op, SLOT( asyncDrop( const KFileItem * ) ) );
        }
        // In both cases asyncDrop will delete op when done

        ev->acceptAction();
    }
    else
    {
        QStrList formats;

        for ( int i = 0; ev->format( i ); i++ )
            if ( *( ev->format( i ) ) )
                formats.append( ev->format( i ) );
        if ( formats.count() >= 1 )
        {
            //kdDebug(1203) << "Pasting to " << dest.url() << endl;

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
        ev->acceptAction();
    }
}

void KonqOperations::asyncDrop( const KFileItem * destItem )
{
    assert(m_info); // setDropInfo should have been called before asyncDrop
    KURL dest = destItem->url();
    KURL::List lst = m_info->lst;
    kdDebug(1203) << "KonqOperations::asyncDrop destItem->mode=" << destItem->mode() << endl;
    // Check what the destination is
    if ( S_ISDIR(destItem->mode()) )
    {
        QDropEvent::Action action = m_info->action;
        if ( dest.path( 1 ) == KGlobalSettings::trashPath() )
        {
            if ( askDeleteConfirmation( lst, DEFAULT_CONFIRMATION ) )
                action = QDropEvent::Move;
            else
            {
                delete this;
                return;
            }
        }
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

            // Check what the source can do
            KURL url = lst.first(); // we'll assume it's the same for all URLs (hack)
            bool sReading = KProtocolInfo::supportsReading( url );
            bool sDeleting = KProtocolInfo::supportsDeleting( url );
            bool sMoving = KProtocolInfo::supportsMoving( url );
            // Check what the destination can do
            bool dWriting = KProtocolInfo::supportsWriting( dest );
            if ( !dWriting )
            {
                delete this;
                return;
            }

            // Nor control nor shift are pressed => show popup menu
            QPopupMenu popup;
            if ( sReading )
              popup.insertItem(SmallIconSet("editcopy"), i18n( "&Copy Here" ), 1 );
            if ( (sMoving || (sReading && sDeleting)) )
              popup.insertItem( i18n( "&Move Here" ), 2 );
            popup.insertItem(SmallIconSet("www"), i18n( "&Link Here" ), 3 );
            if (bSetWallpaper)
                popup.insertItem(SmallIconSet("background"), i18n( "Set as &Wallpaper"), 4 );

            int result = popup.exec( m_info->mousePos );
            switch (result) {
                case 1 : action = QDropEvent::Copy; break;
                case 2 : action = QDropEvent::Move; break;
                case 3 : action = QDropEvent::Link; break;
                case 4 :
                {
                    kdDebug(1203) << "setWallpaper iconView=" << iconView << " url=" << lst.first().url() << endl;
                    if (iconView && iconView->isDesktop() ) iconView->setWallpaper(lst.first());
                    delete this;
                    return;
                }
                default : delete this; return;
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
                kdDebug(1203) << "KonqOperations::asyncDrop lst.count=" << lst.count() << endl;
                job = KIO::link( lst, dest );
                setOperation( job, LINK, lst, dest );
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
            kdDebug(1203) << "KonqOperations::doDrop " << dest.path() << "should be an executable" << endl;
            ASSERT ( access( QFile::encodeName(dest.path()), X_OK ) == 0 );
            KProcess proc;
            proc << dest.path() ;
            // Launch executable for each of the files
            KURL::List::Iterator it = lst.begin();
            for ( ; it != lst.end() ; it++ )
                proc << (*it).path(); // assume local files
            kdDebug(1203) << "starting " << dest.path() << " with " << lst.count() << " arguments" << endl;
            proc.start( KProcess::DontCare );
        }
    }
    delete this;
}

void KonqOperations::rename( QWidget * parent, const KURL & oldurl, const QString & name )
{
    QString newPath = oldurl.directory(false,true) + name;
    kdDebug(1203) << "KonqOperations::rename newPath=" << newPath << endl;
    KURL newurl(oldurl);
    newurl.setPath(newPath);
    if ( oldurl != newurl )
    {
        KURL::List lst;
        lst.append(oldurl);
        KIO::Job * job = KIO::moveAs( oldurl, newurl );
        KonqOperations * op = new KonqOperations( parent );
        op->setOperation( job, MOVE, lst, newurl );
        (void) new KonqCommandRecorder( KonqCommand::MOVE, lst, newurl, job );
    }
}

void KonqOperations::setOperation( KIO::Job * job, int method, const KURL::List & /*src*/, const KURL & dest )
{
  m_method = method;
  //m_srcURLs = src;
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
    op->m_method = STAT;
}

void KonqOperations::_statURL( const KURL & url, const QObject *receiver, const char *member )
{
    connect( this, SIGNAL( statFinished( const KFileItem * ) ), receiver, member );
    KIO::StatJob * job = KIO::stat( url /*, false?*/ );
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotStatResult( KIO::Job * ) ) );
}

void KonqOperations::slotStatResult( KIO::Job * job )
{
    if ( job->error())
        job->showErrorDialog( (QWidget*)parent() );
    else
    {
        KIO::StatJob * statJob = static_cast<KIO::StatJob*>(job);
        KFileItem * item = new KFileItem( statJob->statResult(), statJob->url() );
        emit statFinished( item );
        delete item;
    }
    // If we're only here for a stat, we're done. But not if we used _statURL internally
    if ( m_method == STAT )
        delete this;
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
    delete this;
}

#include <konq_operations.moc>
