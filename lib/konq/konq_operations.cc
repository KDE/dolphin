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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <QClipboard>
//Added by qt3to4:
#include <QDropEvent>
#include <QList>
#include "konq_operations.h"
#include <ktoolinvocation.h>
#include <kautomount.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <krun.h>
#include <kshell.h>
#include <kshortcut.h>
#include <kio/jobuidelegate.h>

#include <QtDBus/QtDBus>
#include <kdirnotify.h>
#include "konq_undo.h"
#include "konq_defaults.h"
#include "konqmimedata.h"
#include "konqbookmarkmanager.h"

// For doDrop
#include <QDir>//first
#include <assert.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kdesktopfile.h>
#include <k3urldrag.h>
#include <kglobalsettings.h>
#include <kimageio.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/paste.h>
#include <kio/netaccess.h>
#include <kio/renamedlg.h>
#include <konq_iconviewwidget.h>
#include <kprotocolmanager.h>
#include <kprocess.h>
#include <kstringhandler.h>
#include <QMenu>
#include <unistd.h>
#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <QX11Info>
#endif
#include <kauthorized.h>
#include <kglobal.h>

KBookmarkManager * KonqBookmarkManager::s_bookmarkManager;

KonqOperations::KonqOperations( QWidget *parent )
    : QObject( parent ),
      m_method( UNKNOWN ), m_info(0L), m_pasteInfo(0L)
{
    setObjectName( "KonqOperations" );
}

KonqOperations::~KonqOperations()
{
    delete m_info;
    delete m_pasteInfo;
}

void KonqOperations::editMimeType( const QString & mimeType )
{
  QString keditfiletype = QLatin1String("keditfiletype");
  KRun::runCommand( keditfiletype + " " + KProcess::quote(mimeType),
                    keditfiletype, keditfiletype /*unused*/);
}

void KonqOperations::del( QWidget * parent, int method, const KUrl::List & selectedURLs )
{
  kDebug(1203) << "KonqOperations::del " << parent->metaObject()->className() << endl;
  if ( selectedURLs.isEmpty() )
  {
    kWarning(1203) << "Empty URL list !" << endl;
    return;
  }

  KonqOperations * op = new KonqOperations( parent );
  int confirmation = DEFAULT_CONFIRMATION;
  op->_del( method, selectedURLs, confirmation );
}

void KonqOperations::emptyTrash()
{
  KonqOperations *op = new KonqOperations( 0L );
  op->_del( EMPTYTRASH, KUrl("trash:/"), SKIP_CONFIRMATION );
}

void KonqOperations::restoreTrashedItems( const KUrl::List& urls )
{
  KonqOperations *op = new KonqOperations( 0L );
  op->_restoreTrashedItems( urls );
}

void KonqOperations::mkdir( QWidget *parent, const KUrl & url )
{
    KIO::Job * job = KIO::mkdir( url );
    KonqOperations * op = new KonqOperations( parent );
    op->setOperation( job, MKDIR, KUrl::List(), url );
    (void) new KonqCommandRecorder( KonqCommand::MKDIR, KUrl(), url, job ); // no support yet, apparently
}

void KonqOperations::doPaste( QWidget * parent, const KUrl & destURL )
{
   doPaste(parent, destURL, QPoint());
}

void KonqOperations::doPaste( QWidget * parent, const KUrl & destURL, const QPoint &pos )
{
    // move or not move ?
    bool move = false;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if ( data->hasFormat( "application/x-kde-cutselection" ) ) {
      move = KonqMimeData::decodeIsCutSelection( data );
      kDebug(1203) << "move (from clipboard data) = " << move << endl;
    }

    KIO::Job *job = KIO::pasteClipboard( destURL, parent, move );
    if ( job )
    {
        KonqOperations * op = new KonqOperations( parent );
        KIO::CopyJob * copyJob = static_cast<KIO::CopyJob *>(job);
        KIOPasteInfo * pi = new KIOPasteInfo;
        pi->mousePos = pos;
        op->setPasteInfo( pi );
        op->setOperation( job, move ? MOVE : COPY, copyJob->srcUrls(), copyJob->destUrl() );
        (void) new KonqCommandRecorder( move ? KonqCommand::MOVE : KonqCommand::COPY, KUrl::List(), destURL, job );
    }
}

void KonqOperations::copy( QWidget * parent, int method, const KUrl::List & selectedURLs, const KUrl& destUrl )
{
  kDebug(1203) << "KonqOperations::copy() " << parent->metaObject()->className() << endl;
  if ((method!=COPY) && (method!=MOVE) && (method!=LINK))
  {
    kWarning(1203) << "Illegal copy method !" << endl;
    return;
  }
  if ( selectedURLs.isEmpty() )
  {
    kWarning(1203) << "Empty URL list !" << endl;
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

  op->setOperation( job, method, selectedURLs, destUrl );

  if (method==COPY)
     (void) new KonqCommandRecorder( KonqCommand::COPY, selectedURLs, destUrl, job );
  else
     (void) new KonqCommandRecorder( method==MOVE?KonqCommand::MOVE:KonqCommand::LINK, selectedURLs, destUrl, job );
}

void KonqOperations::_del( int method, const KUrl::List & _selectedURLs, int confirmation )
{
    KUrl::List selectedURLs;
    for (KUrl::List::ConstIterator it = _selectedURLs.begin(); it != _selectedURLs.end(); ++it)
        if (KProtocolManager::supportsDeleting(*it))
            selectedURLs.append(*it);
    if (selectedURLs.isEmpty()) {
        delete this;
        return;
    }

  m_method = method;
  if ( confirmation == SKIP_CONFIRMATION || askDeleteConfirmation( selectedURLs, confirmation ) )
  {
    //m_srcURLs = selectedURLs;
    KIO::Job *job;
    switch( method )
    {
      case TRASH:
      {
        job = KIO::trash( selectedURLs );
        (void) new KonqCommandRecorder( KonqCommand::TRASH, selectedURLs, KUrl("trash:/"), job );
         break;
      }
      case EMPTYTRASH:
      {
        // Same as in ktrash --empty
        QByteArray packedArgs;
        QDataStream stream( &packedArgs, QIODevice::WriteOnly );
        stream << (int)1;
        job = KIO::special( KUrl("trash:/"), packedArgs );
       KNotification::event("Trash: emptied", QString() , QPixmap() , 0l /*, QStringList() , KNotification::ContextList() */, KNotification::DefaultEvent );
       break;
      }
      case DEL:
        job = KIO::del( selectedURLs );
        break;
      case SHRED:
        job = KIO::del( selectedURLs, true );
        break;
      default:
        kWarning() << "Unknown operation: " << method << endl;
        delete this;
        return;
    }
    connect( job, SIGNAL( result( KJob * ) ),
             SLOT( slotResult( KJob * ) ) );
  } else
    delete this;
}

void KonqOperations::_restoreTrashedItems( const KUrl::List& urls )
{
    m_method = RESTORE;
    KonqMultiRestoreJob* job = new KonqMultiRestoreJob( urls, true );
    connect( job, SIGNAL( result( KJob * ) ),
             SLOT( slotResult( KJob * ) ) );
}

bool KonqOperations::askDeleteConfirmation( const KUrl::List & selectedURLs, int confirmation )
{
    QString keyName;
    bool ask = ( confirmation == FORCE_CONFIRMATION );
    if ( !ask )
    {
        KConfig config("konquerorrc", true, false);
        config.setGroup( "Trash" );
        keyName = ( m_method == DEL ? "ConfirmDelete" : m_method == SHRED ? "ConfirmShred" : "ConfirmTrash" );
        bool defaultValue = ( m_method == DEL ? DEFAULT_CONFIRMDELETE : m_method == SHRED ? DEFAULT_CONFIRMSHRED : DEFAULT_CONFIRMTRASH );
        ask = config.readEntry( keyName, QVariant(defaultValue )).toBool();
    }
    if ( ask )
    {
      KUrl::List::ConstIterator it = selectedURLs.begin();
      QStringList prettyList;
      for ( ; it != selectedURLs.end(); ++it ) {
        if ( (*it).protocol() == "trash" ) {
          QString path = (*it).path();
          // HACK (#98983): remove "0-foo". Note that it works better than
	  // displaying KFileItem::name(), for files under a subdir.
          prettyList.append( path.remove(QRegExp("^/[0-9]*-")) );
        } else
          prettyList.append( (*it).pathOrUrl() );
      }

      int result;
      switch(m_method)
      {
      case DEL:
          result = KMessageBox::warningContinueCancelList( 0,
             	i18np( "Do you really want to delete this item?", "Do you really want to delete these %n items?", prettyList.count()),
             	prettyList,
		i18n( "Delete Files" ),
		KStdGuiItem::del(),
		keyName, KMessageBox::Dangerous);
	 break;

      case SHRED:
          result = KMessageBox::warningContinueCancelList( 0,
                i18np( "Do you really want to shred this item?", "Do you really want to shred these %n items?", prettyList.count()),
                prettyList,
                i18n( "Shred Files" ),
		KGuiItem( i18n( "Shred" ), "editshred" ),
		keyName, KMessageBox::Dangerous);
        break;

      case MOVE:
      default:
          result = KMessageBox::warningContinueCancelList( 0,
                i18np( "Do you really want to move this item to the trash?", "Do you really want to move these %n items to the trash?", prettyList.count()),
                prettyList,
		i18n( "Move to Trash" ),
		KGuiItem( i18nc( "Verb", "&Trash" ), "edittrash"),
		keyName, KMessageBox::Dangerous);
      }
      if (!keyName.isEmpty())
      {
         // Check kmessagebox setting... erase & copy to konquerorrc.
         KConfig *config = KGlobal::config();
         KConfigGroup saver(config, "Notification Messages");
         if (!saver.readEntry(keyName, QVariant(true)).toBool())
         {
            saver.writeEntry(keyName, true);
            saver.sync();
            KConfig konq_config("konquerorrc", false);
            konq_config.setGroup( "Trash" );
            konq_config.writeEntry( keyName, false );
         }
      }
      return (result == KMessageBox::Continue);
    }
    return true;
}

void KonqOperations::doDrop( const KFileItem * destItem, const KUrl & dest, QDropEvent * ev, QWidget * parent )
{
    kDebug(1203) << "doDrop: dest : " << dest.url() << endl;
    QMap<QString, QString> metaData;
    const KUrl::List lst = KUrl::List::fromMimeData( ev->mimeData(), &metaData );
    if ( !lst.isEmpty() ) // Are they urls ?
    {
        kDebug(1203) << "KonqOperations::doDrop metaData: " << metaData.count() << " entries." << endl;
        QMap<QString,QString>::ConstIterator mit;
        for( mit = metaData.begin(); mit != metaData.end(); ++mit )
        {
            kDebug(1203) << "metaData: key=" << mit.key() << " value=" << mit.value() << endl;
        }
        // Check if we dropped something on itself
        KUrl::List::ConstIterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
        {
            kDebug(1203) << "URL : " << (*it).url() << endl;
            if ( dest.equals( *it, KUrl::CompareWithoutTrailingSlash ) )
            {
                // The event source may be the view or an item (icon)
                // Note: ev->source() can be 0L! (in case of kdesktop) (Simon)
                if ( !ev->source() || ev->source() != parent && ev->source()->parent() != parent )
                    KMessageBox::sorry( parent, i18n("You cannot drop a folder on to itself") );
                kDebug(1203) << "Dropped on itself" << endl;
                ev->setAccepted( false );
                return; // do nothing instead of displaying kfm's annoying error box
            }
        }

        // Check the state of the modifiers key at the time of the drop
        // TODO port to QApplication::keyboardModifiers()
#ifdef Q_WS_X11 // removing for now, it should be OK once it uses keyboardModifiers()
        Window root;
        Window child;
        int root_x, root_y, win_x, win_y;
        uint keybstate;
        XQueryPointer( QX11Info::display(), QX11Info::appRootWindow(), &root, &child,
                       &root_x, &root_y, &win_x, &win_y, &keybstate );

        Qt::DropAction action = ev->dropAction();
        // Check for the drop of a bookmark -> we want a Link action
        if ( ev->provides("application/x-xbel") )
        {
            keybstate |= ControlMask | ShiftMask;
            action = Qt::LinkAction;
            kDebug(1203) << "KonqOperations::doDrop Bookmark -> emulating Link" << endl;
        }

        KonqOperations * op = new KonqOperations(parent);
        op->setDropInfo( new DropInfo( keybstate, lst, metaData, win_x, win_y, action ) );

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

        ev->acceptProposedAction();
#else
        kDebug(1203) << "we don't handle keyboard modifiers yet, skip check" << endl;
        ev->setAccepted( false );
        return;
#endif
    }
    else
    {
        //kDebug(1203) << "Pasting to " << dest.url() << endl;
        KonqOperations * op = new KonqOperations(parent);
        KIO::CopyJob* job = KIO::pasteMimeSource( ev->mimeData(), dest,
                                                  i18n( "File name for dropped contents:" ),
                                                  parent );
        if ( job ) // 0 if canceled by user
        {
            op->setOperation( job, COPY, KUrl::List(), job->destUrl() );
            (void) new KonqCommandRecorder( KonqCommand::COPY, KUrl::List(), dest, job );
        }
        ev->acceptProposedAction();
    }
}

void KonqOperations::asyncDrop( const KFileItem * destItem )
{
    assert(m_info); // setDropInfo should have been called before asyncDrop
    m_destURL = destItem->url();

    //kDebug(1203) << "KonqOperations::asyncDrop destItem->mode=" << destItem->mode() << " url=" << m_destURL << endl;
    // Check what the destination is
    if ( destItem->isDir() )
    {
        doFileCopy();
        return;
    }
    if ( !m_destURL.isLocalFile() )
    {
        // We dropped onto a remote URL that is not a directory!
        // (e.g. an HTTP link in the sidebar).
        // Can't do that, but we can't prevent it before stating the dest....
        kWarning(1203) << "Cannot drop onto " << m_destURL << endl;
        delete this;
        return;
    }
    if ( destItem->mimetype() == "application/x-desktop")
    {
        // Local .desktop file. What type ?
        KDesktopFile desktopFile( m_destURL.path() );
        if ( desktopFile.hasApplicationType() )
        {
            QString error;
            QStringList stringList;
            KUrl::List lst = m_info->lst;
            KUrl::List::Iterator it = lst.begin();
            for ( ; it != lst.end() ; it++ )
            {
                stringList.append((*it).url());
            }
            if ( KToolInvocation::startServiceByDesktopPath( m_destURL.path(), stringList, &error ) > 0 )
                KMessageBox::error( 0L, error );
        }
        else
        {
            // Device or Link -> adjust dest
            if ( desktopFile.hasDeviceType() && desktopFile.hasKey("MountPoint") ) {
                QString point = desktopFile.readEntry( "MountPoint" );
                m_destURL.setPath( point );
                QString dev = desktopFile.readDevice();
                QString mp = KIO::findDeviceMountPoint( dev );
                // Is the device already mounted ?
                if ( !mp.isNull() )
                    doFileCopy();
                else
                {
                    bool ro = desktopFile.readEntry( "ReadOnly", QVariant(false )).toBool();
                    QByteArray fstype = desktopFile.readEntry( "FSType" ).toLatin1();
#ifndef Q_WS_WIN
                    KAutoMount* am = new KAutoMount( ro, fstype, dev, point, m_destURL.path(), false );
                    connect( am, SIGNAL( finished() ), this, SLOT( doFileCopy() ) );
#endif
                }
                return;
            }
            else if ( desktopFile.hasLinkType() && desktopFile.hasKey("URL") ) {
                m_destURL = desktopFile.readPathEntry("URL");
                doFileCopy();
                return;
            }
            // else, well: mimetype, service, servicetype or .directory. Can't really drop anything on those.
        }
    }
    else
    {
        // Should be a local executable
        // (If this fails, there is a bug in KFileItem::acceptsDrops)
        kDebug(1203) << "KonqOperations::doDrop " << m_destURL.path() << "should be an executable" << endl;
        Q_ASSERT ( access( QFile::encodeName(m_destURL.path()), X_OK ) == 0 );
        KProcess proc;
        proc << m_destURL.path() ;
        // Launch executable for each of the files
        KUrl::List lst = m_info->lst;
        KUrl::List::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
            proc << (*it).path(); // assume local files
        kDebug(1203) << "starting " << m_destURL.path() << " with " << lst.count() << " arguments" << endl;
        proc.start( KProcess::DontCare );
    }
    delete this;
}

void KonqOperations::doFileCopy()
{
    assert(m_info); // setDropInfo - and asyncDrop - should have been called before asyncDrop
    KUrl::List lst = m_info->lst;
    Qt::DropAction action = m_info->action;
    bool isDesktopFile = false;
    bool itemIsOnDesktop = false;
    bool allItemsAreFromTrash = true;
    KUrl::List mlst; // list of items that can be moved
    for (KUrl::List::ConstIterator it = lst.begin(); it != lst.end(); ++it)
    {
        bool local = (*it).isLocalFile();
        if ( KProtocolManager::supportsDeleting( *it ) && (!local || QFileInfo((*it).directory()).isWritable() ))
            mlst.append(*it);
        if ( local && KDesktopFile::isDesktopFile((*it).path()))
            isDesktopFile = true;
        if ( local && (*it).path().startsWith(KGlobalSettings::desktopPath()))
            itemIsOnDesktop = true;
        if ( local || (*it).protocol() != "trash" )
            allItemsAreFromTrash = false;
    }

    bool linkOnly = false;
    if (isDesktopFile && !KAuthorized::authorizeKAction("run_desktop_files") &&
        (m_destURL.path( KUrl::AddTrailingSlash ) == KGlobalSettings::desktopPath()) )
    {
       linkOnly = true;
    }

    if ( !mlst.isEmpty() && m_destURL.protocol() == "trash" )
    {
        if ( itemIsOnDesktop && !KAuthorized::authorizeKAction("editable_desktop_icons") )
        {
            delete this;
            return;
        }

        m_method = TRASH;
        if ( askDeleteConfirmation( mlst, DEFAULT_CONFIRMATION ) )
            action = Qt::MoveAction;
        else
        {
            delete this;
            return;
        }
    }
    else if ( allItemsAreFromTrash || m_destURL.protocol() == "trash" ) {
        // No point in asking copy/move/link when using dnd from or to the trash.
        action = Qt::MoveAction;
    }
    else if ( (
        ((QApplication::keyboardModifiers() & Qt::ControlModifier) == 0) &&
        ((QApplication::keyboardModifiers() & Qt::ShiftModifier) == 0) ) || linkOnly )
    {
        // Neither control nor shift are pressed => show popup menu
        KonqIconViewWidget *iconView = dynamic_cast<KonqIconViewWidget*>(parent());
        bool bSetWallpaper = false;
        if ( iconView && iconView->maySetWallpaper() && lst.count() == 1 )
	{
            KUrl url = lst.first();
            KMimeType::Ptr mime = KMimeType::findByUrl( url );
            if ( mime && ( ( KImageIO::isSupported(mime->name(), KImageIO::Reading) ) ||
                 mime->is( "image/svg+xml" ) ) )
            {
                bSetWallpaper = true;
            }
        }

        // Check what the source can do
        KUrl url = lst.first(); // we'll assume it's the same for all URLs (hack)
        bool sReading = KProtocolManager::supportsReading( url );
        bool sDeleting = KProtocolManager::supportsDeleting( url );
        bool sMoving = KProtocolManager::supportsMoving( url );
        // Check what the destination can do
        bool dWriting = KProtocolManager::supportsWriting( m_destURL );
        if ( !dWriting )
        {
            delete this;
            return;
        }

        QMenu popup;
        QString seq = QKeySequence( Qt::ShiftModifier ).toString();
        seq.chop(1); // chop superfluous '+'
        QAction* popupMoveAction = new QAction(i18n( "&Move Here" ) + "\t" + seq, this);
        popupMoveAction->setIcon(SmallIconSet("goto"));
        seq = QKeySequence( Qt::ControlModifier ).toString();
        seq.chop(1);
        QAction* popupCopyAction = new QAction(i18n( "&Copy Here" ) + "\t" + seq, this);
        popupCopyAction->setIcon(SmallIconSet("editcopy"));
        seq = QKeySequence( Qt::ControlModifier + Qt::ShiftModifier ).toString();
        seq.chop(1);
        QAction* popupLinkAction = new QAction(i18n( "&Link Here" ) + "\t" + seq, this);
        popupLinkAction->setIcon(SmallIconSet("www"));
        QAction* popupWallAction = new QAction( i18n( "Set as &Wallpaper" ), this );
        popupWallAction->setIcon(SmallIconSet("background"));
        QAction* popupCancelAction = new QAction(i18n( "C&ancel" ) + "\t" + QKeySequence( Qt::Key_Escape ).toString(), this);
        popupCancelAction->setIcon(SmallIconSet("cancel"));

        if ( sReading && !linkOnly)
            popup.addAction(popupCopyAction);

        if (!mlst.isEmpty() && (sMoving || (sReading && sDeleting)) && !linkOnly )
            popup.addAction(popupMoveAction);

        popup.addAction(popupLinkAction);

        if (bSetWallpaper)
            popup.addAction(popupWallAction);

        popup.addSeparator();
        popup.addAction(popupCancelAction);

        QAction* result = popup.exec( m_info->mousePos );

        if(result == popupCopyAction)
            action = Qt::CopyAction;
        else if(result == popupMoveAction)
            action = Qt::MoveAction;
        else if(result == popupLinkAction)
            action = Qt::LinkAction;
        else if(result == popupWallAction)
        {
            kDebug(1203) << "setWallpaper iconView=" << iconView << " url=" << lst.first().url() << endl;
            if (iconView && iconView->isDesktop() ) iconView->setWallpaper(lst.first());
            delete this;
            return;
        }
        else if(result == popupCancelAction || !result)
        {
            delete this;
            return;
        }
    }

    KIO::Job * job = 0;
    switch ( action ) {
    case Qt::MoveAction :
        job = KIO::move( lst, m_destURL );
        job->setMetaData( m_info->metaData );
        setOperation( job, m_method == TRASH ? TRASH : MOVE, lst, m_destURL );
        (void) new KonqCommandRecorder(
            m_method == TRASH ? KonqCommand::TRASH : KonqCommand::MOVE,
            lst, m_destURL, job );
        return; // we still have stuff to do -> don't delete ourselves
    case Qt::CopyAction :
        job = KIO::copy( lst, m_destURL );
        job->setMetaData( m_info->metaData );
        setOperation( job, COPY, lst, m_destURL );
        (void) new KonqCommandRecorder( KonqCommand::COPY, lst, m_destURL, job );
        return;
    case Qt::LinkAction :
        kDebug(1203) << "KonqOperations::asyncDrop lst.count=" << lst.count() << endl;
        job = KIO::link( lst, m_destURL );
        job->setMetaData( m_info->metaData );
        setOperation( job, LINK, lst, m_destURL );
        (void) new KonqCommandRecorder( KonqCommand::LINK, lst, m_destURL, job );
        return;
    default : kError(1203) << "Unknown action " << (int)action << endl;
    }
    delete this;
}

void KonqOperations::rename( QWidget * parent, const KUrl & oldurl, const KUrl& newurl )
{
    kDebug(1203) << "KonqOperations::rename oldurl=" << oldurl << " newurl=" << newurl << endl;
    if ( oldurl == newurl )
        return;

    KUrl::List lst;
    lst.append(oldurl);
    KIO::Job * job = KIO::moveAs( oldurl, newurl, !oldurl.isLocalFile() );
    KonqOperations * op = new KonqOperations( parent );
    op->setOperation( job, MOVE, lst, newurl );
    (void) new KonqCommandRecorder( KonqCommand::MOVE, lst, newurl, job );
    // if moving the desktop then update config file and emit
    if ( oldurl.isLocalFile() && oldurl.path( KUrl::AddTrailingSlash ) == KGlobalSettings::desktopPath() )
    {
        kDebug(1203) << "That rename was the Desktop path, updating config files" << endl;
        KConfig *globalConfig = KGlobal::config();
        KConfigGroup cgs( globalConfig, "Paths" );
        cgs.writePathEntry("Desktop" , newurl.path(), KConfigBase::Persistent|KConfigBase::Global );
        cgs.sync();
        KGlobalSettings::self()->emitChange(KGlobalSettings::SettingsChanged, KGlobalSettings::SETTINGS_PATHS);
    }
}

void KonqOperations::setOperation( KIO::Job * job, int method, const KUrl::List & /*src*/, const KUrl & dest )
{
    m_method = method;
    //m_srcURLs = src;
    m_destURL = dest;
    if ( job )
    {
        connect( job, SIGNAL( result( KJob * ) ),
                 SLOT( slotResult( KJob * ) ) );
        KIO::CopyJob *copyJob = dynamic_cast<KIO::CopyJob*>(job);
        KonqIconViewWidget *iconView = dynamic_cast<KonqIconViewWidget*>(parent());
        if (copyJob && iconView)
        {
            connect(copyJob, SIGNAL(aboutToCreate(KIO::Job *,const QList<KIO::CopyInfo> &)),
                 this, SLOT(slotAboutToCreate(KIO::Job *,const QList<KIO::CopyInfo> &)));
            connect(this, SIGNAL(aboutToCreate(const QPoint &, const QList<KIO::CopyInfo> &)),
                 iconView, SLOT(slotAboutToCreate(const QPoint &, const QList<KIO::CopyInfo> &)));
        }
    }
    else // for link
        slotResult( 0L );
}

void KonqOperations::slotAboutToCreate(KIO::Job *, const QList<KIO::CopyInfo> &files)
{
    emit aboutToCreate( m_info ? m_info->mousePos : m_pasteInfo ? m_pasteInfo->mousePos : QPoint(), files);
}

void KonqOperations::statURL( const KUrl & url, const QObject *receiver, const char *member )
{
    KonqOperations * op = new KonqOperations( 0L );
    op->_statURL( url, receiver, member );
    op->m_method = STAT;
}

void KonqOperations::_statURL( const KUrl & url, const QObject *receiver, const char *member )
{
    connect( this, SIGNAL( statFinished( const KFileItem * ) ), receiver, member );
    KIO::StatJob * job = KIO::stat( url /*, false?*/ );
    connect( job, SIGNAL( result( KJob * ) ),
             SLOT( slotStatResult( KJob * ) ) );
}

void KonqOperations::slotStatResult( KJob * job )
{
    if ( job->error())
    {
        static_cast<KIO::Job*>( job )->ui()->setWindow((QWidget*)parent());
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();
    }
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

void KonqOperations::slotResult( KJob * job )
{
    if (job && job->error())
    {
        static_cast<KIO::Job*>( job )->ui()->setWindow((QWidget*)parent());
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();
    }
    if ( m_method == EMPTYTRASH ) {
        // Update konq windows opened on trash:/
        org::kde::KDirNotify::emitFilesAdded( "trash:/" ); // yeah, files were removed, but we don't know which ones...
    }
    delete this;
}

void KonqOperations::rename( QWidget * parent, const KUrl & oldurl, const QString & name )
{
    KUrl newurl( oldurl );
    newurl.setPath( oldurl.directory( KUrl::AppendTrailingSlash ) + name );
    kDebug(1203) << "KonqOperations::rename("<<name<<") called. newurl=" << newurl << endl;
    rename( parent, oldurl, newurl );
}

void KonqOperations::newDir( QWidget * parent, const KUrl & baseURL )
{
    bool ok;
    QString name = i18n( "New Folder" );
    if ( baseURL.isLocalFile() && QFileInfo( baseURL.path( KUrl::AddTrailingSlash ) + name ).exists() )
        name = KIO::RenameDlg::suggestName( baseURL, i18n( "New Folder" ) );

    name = KInputDialog::getText ( i18n( "New Folder" ),
        i18n( "Enter folder name:" ), name, &ok, parent );
    if ( ok && !name.isEmpty() )
    {
        KUrl url;
        if ((name[0] == '/') || (name[0] == '~'))
        {
           url.setPath(KShell::tildeExpand(name));
        }
        else
        {
           name = KIO::encodeFileName( name );
           url = baseURL;
           url.addPath( name );
        }
        KonqOperations::mkdir( 0L, url );
    }
}

////

KonqMultiRestoreJob::KonqMultiRestoreJob( const KUrl::List& urls, bool showProgressInfo )
    : KIO::Job( showProgressInfo ),
      m_urls( urls ), m_urlsIterator( m_urls.begin() ),
      m_progress( 0 )
{
  QTimer::singleShot(0, this, SLOT(slotStart()));
}

void KonqMultiRestoreJob::slotStart()
{
    // Well, it's not a total in bytes, so this would look weird
    //if ( m_urlsIterator == m_urls.begin() ) // first time: emit total
    //    emit totalSize( m_urls.count() );

    if ( m_urlsIterator != m_urls.end() )
    {
        const KUrl& url = *m_urlsIterator;

        KUrl new_url = url;
        if ( new_url.protocol()=="system"
          && new_url.path().startsWith("/trash") )
        {
            QString path = new_url.path();
	    path.remove(0, 6);
	    new_url.setProtocol("trash");
	    new_url.setPath(path);
        }

        Q_ASSERT( new_url.protocol() == "trash" );
        QByteArray packedArgs;
        QDataStream stream( &packedArgs, QIODevice::WriteOnly );
        stream << (int)3 << new_url;
        KIO::Job* job = KIO::special( new_url, packedArgs );
        addSubjob( job );
    }
    else // done!
    {
        org::kde::KDirNotify::emitFilesRemoved(m_urls.toStringList() );
        emitResult();
    }
}

void KonqMultiRestoreJob::slotResult( KJob *job )
{
    if ( job->error() )
    {
        KIO::Job::slotResult( job ); // will set the error and emit result(this)
        return;
    }
    removeSubjob(job);
    // Move on to next one
    ++m_urlsIterator;
    ++m_progress;
    //emit processedSize( this, m_progress );
    emitPercent( m_progress, m_urls.count() );
    slotStart();
}

#include "konq_operations.moc"
