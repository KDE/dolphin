/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include "konqiconviewwidget.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qfile.h>
#include <qpopupmenu.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kio_job.h>
#include <kio_paste.h>
#include <klocale.h>
#include <kfileivi.h>
#include <kfileitem.h>
#include <konqsettings.h>
#include <konqdrag.h>
#include <kuserpaths.h>

#include <assert.h>
#include <X11/Xlib.h>

// for the link method only (to move with it)
#include <kmessagebox.h>
#include <errno.h>

KonqIconViewWidget::KonqIconViewWidget( QWidget * parent, const char * name, WFlags f )
  : QIconView( parent, name, f ),
    m_bImagePreviewAllowed( false )
{
  QObject::connect( this, SIGNAL( dropped( QDropEvent * ) ),
	            this, SLOT( slotDrop( QDropEvent* ) ) );
	
  // hardcoded settings
  setSelectionMode( QIconView::Extended );
  setItemTextPos( QIconView::Bottom );
  setGridX( 70 );
  setAligning( true );
  setSorting( true, sortDirection() );
  // configurable settings
  initConfig();
}

void KonqIconViewWidget::initConfig()
{
  m_pSettings = KonqFMSettings::defaultIconSettings();

  // Color settings
  QColor normalTextColor         = m_pSettings->normalTextColor();
  QColor highlightedTextColor    = m_pSettings->highlightedTextColor();
  setItemColor( normalTextColor );

  /*
    // What does this do ? (David)
  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );
  */

  // Font settings
  QFont font( m_pSettings->stdFontName(), m_pSettings->fontSize() );
  font.setUnderline( m_pSettings->underlineLink() );
  setItemFont( font );

  // Behaviour (single click/double click, autoselect, ...)
  bool bChangeCursor = m_pSettings->changeCursor();
  setSingleClickConfiguration( new QFont(font), new QColor(normalTextColor), 
                               new QFont(font), new QColor(highlightedTextColor),
                    new QCursor(bChangeCursor ? KCursor().handCursor() : KCursor().arrowCursor()),
                    m_pSettings->autoSelect() );
  setUseSingleClickMode( m_pSettings->singleClick() );
  setWordWrapIconText( m_pSettings->wordWrapText() );
}

void KonqIconViewWidget::setSize( KIconLoader::Size size )
{
  m_size = size;
  for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
    ((KFileIVI*)it)->setSize( size, m_bImagePreviewAllowed );
  }
  setViewMode( (size == KIconLoader::Small) ? QIconSet::Small : QIconSet::Large );
}

void KonqIconViewWidget::setImagePreviewAllowed( bool b )
{
  m_bImagePreviewAllowed = b;
  for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
    ((KFileIVI*)it)->setSize( m_size, m_bImagePreviewAllowed );
  }
}

KFileItemList KonqIconViewWidget::selectedFileItems()
{
  KFileItemList lstItems;

  QIconViewItem *it = firstItem();
  for (; it; it = it->nextItem() )
    if ( it->isSelected() ) {
      KFileItem *fItem = ((KFileIVI *)it)->item();
      lstItems.append( fItem );
    }
  return lstItems;
}

// TODO : move this to libkonq or libkio
void link( QStringList srcUrls, KURL destDir )
{
  kdebug( KDEBUG_INFO, 1202, "%s", QString("destDir = %1").arg(destDir.url()).ascii() );
  bool overwriteExistingFiles = false;
  if ( destDir.isMalformed() )
  {
    KMessageBox::sorry( 0L, i18n( "Malformed URL\n%1" ).arg( destDir.url() ) );
    return;
  }
  else if ( !destDir.isLocalFile() )
  {
    // I can only make links on the local file system.
    KMessageBox::sorry( 0L, i18n( "Can only make links on local file system" ) );
    return;
  }
  QStringList::ConstIterator it = srcUrls.begin();
  for ( ; it != srcUrls.end() ; ++it )
  {
    KURL srcUrl( *it );
    if ( srcUrl.isMalformed() )
    {
      KMessageBox::sorry( 0L, i18n( "Malformed URL\n%1" ).arg( *it ) );
      return;
    }

    // The destination URL is the destination dir + the filename
    KURL destUrl( destDir.url(1) + srcUrl.filename() );
    kdebug( KDEBUG_INFO, 1202, "%s", QString("destUrl = %1").arg(destUrl.url()).ascii() );

    // Do we link a file on the local disk?
    if ( srcUrl.isLocalFile() )
    {
      // Make a symlink
      if ( symlink( srcUrl.path().local8Bit(), destUrl.path().local8Bit() ) == -1 )
      {
        // Does the destination already exist ?
        if ( errno == EEXIST )
        {
          // Are we allowed to overwrite the files ?
          if ( overwriteExistingFiles )
          {
            // Try to delete the destination
            if ( unlink( destUrl.path().local8Bit() ) != 0 )
            {
              KMessageBox::sorry( 0L, i18n( "Could not overwrite\n%1"), destUrl.path() );
              return;
            }
          }
          else
          {
            // Ask the user what to do
            // TODO
            KMessageBox::sorry( 0L, i18n( "Destination exists (real dialog box not implemented yet)\n%1"), destUrl.path() );
            return;
          }
        }
        else
        {
          // Some error occured while we tried to symlink
          KMessageBox::sorry( 0L, i18n( "Failed to make symlink from \n%1\nto\n%2\n" ).
                              arg(srcUrl.url()).arg(destUrl.url()) );
          return;
        }
      } // else : no problem
    }
    // Make a link from a file in a tar archive, ftp, http or what ever
    else
    {
      // Encode slashes and so on
      QString destPath = destDir.path(1) + KFileItem::encodeFileName( srcUrl.url() );
      QFile f( destPath );
      if ( f.open( IO_ReadWrite ) )
      {
        f.close(); // kalle
        KSimpleConfig config( destPath ); // kalle
        config.setDesktopGroup();
        config.writeEntry( "URL", srcUrl.url() );
        config.writeEntry( "Type", "Link" );
        QString protocol = srcUrl.protocol();
        if ( protocol == "ftp" )
          config.writeEntry( "Icon", "ftp" );
        else if ( protocol == "http" )
          config.writeEntry( "Icon", "www" );
        else if ( protocol == "info" )
          config.writeEntry( "Icon", "info" );
        else if ( protocol == "mailto" )   // sven:
          config.writeEntry( "Icon", "kmail" ); // added mailto: support
        else
          config.writeEntry( "Icon", "unknown" );
        config.sync();
      }
      else
      {
        KMessageBox::sorry( 0L, i18n( "Could not write to\n%1").arg(destPath) );
        return;
      }
    }
  }
}
            
void KonqIconViewWidget::slotDrop( QDropEvent *e )
{
  slotDropItem( 0L, e );
}

void KonqIconViewWidget::slotDropItem( KFileIVI *item, QDropEvent *e )
{

  // Use either the root url or the item url
  KURL dest( ( item == 0L ) ? m_url /*m_dirLister->url()*/ : item->item()->url().url() );

  // Check the state of the modifiers key at the time of the drop
  Window root;
  Window child;
  int root_x, root_y, win_x, win_y;
  uint keybstate;
  XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                 &root_x, &root_y, &win_x, &win_y, &keybstate );

  if ( dest.path( 1 ) == KUserPaths::trashPath() )
    e->setAction( QDropEvent::Move );
  else if ( ((keybstate & ControlMask) == 0) && ((keybstate & ShiftMask) == 0) )
  {
    // Nor control nor shift are pressed => show popup menu
    QPopupMenu popup;
    popup.insertItem( i18n( "Copy" ), 1 );
    popup.insertItem( i18n( "Move" ), 2 );
    popup.insertItem( i18n( "Link" ), 3 );
    int result = popup.exec( QPoint( win_x, win_y ) );
    switch (result) {
    case 1 : e->setAction( QDropEvent::Copy ); break;
    case 2 : e->setAction( QDropEvent::Move ); break;
    case 3 : e->setAction( QDropEvent::Link ); break;
    default : return;
    }
  }

  dropStuff( item, e );
}

void KonqIconViewWidget::dropStuff( KFileIVI *item, QDropEvent *ev )
{
  QStringList lst;

  QStringList formats;

  for ( int i = 0; ev->format( i ); i++ )
    if ( *( ev->format( i ) ) )
      formats.append( ev->format( i ) );

  // Try to decode to the data you understand...
  if ( QUrlDrag::decodeToUnicodeUris( ev, lst ) )
  {
    if( lst.count() == 0 )
    {
      kdebug(KDEBUG_WARN,1202,"Oooops, no data ....");
      return;
    }
    KIOJob* job = new KIOJob;

    // Use either the root url or the item url
    KURL dest( ( item == 0L ) ? m_url /*m_dirLister->url()*/ : item->item()->url().url() );

    switch ( ev->action() ) {
      case QDropEvent::Move : job->move( lst, dest.url( 1 ) ); break;
      case QDropEvent::Copy : job->copy( lst, dest.url( 1 ) ); break;
      case QDropEvent::Link : {
        link( lst, dest );
        break;
      }
      default : kdebug( KDEBUG_ERROR, 1202, "Unknown action %d", ev->action() ); return;
    }
  }
  else if ( formats.count() >= 1 )
  {
    if ( item == 0L )
      pasteData( m_url /*m_dirLister->url()*/, ev->data( formats.first() ) );
    else
    {
      kdebug(0,1202,"Pasting to %s", item->item()->url().url().ascii() /* item's url */);
      pasteData( item->item()->url().url()/* item's url */, ev->data( formats.first() ) );
    }
  }
}
            
void KonqIconViewWidget::drawBackground( QPainter *p, const QRect &r )
{
  const QPixmap *pm = viewport()->backgroundPixmap();
  if (!pm || pm->isNull()) {
    p->fillRect(r, viewport()->backgroundColor());
    return;
  }
  
  int ax = (r.x() % pm->width());
  int ay = (r.y() % pm->height());
  p->drawTiledPixmap(r, *pm, QPoint(ax, ay)); 
}

QDragObject * KonqIconViewWidget::dragObject()
{
    if ( !currentItem() )
	return 0;

    QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );
    KonqDrag *drag = new KonqDrag( viewport() );
    drag->setPixmap( QPixmap( currentItem()->icon().pixmap( QIconView::viewMode(), QIconSet::Normal ) ),
		     QPoint( currentItem()->iconRect().width() / 2,
			     currentItem()->iconRect().height() / 2 ) );
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
	if ( it->isSelected() ) {
	    drag->append( KonqDragItem( QRect( it->iconRect( FALSE ).x() - orig.x(),
					       it->iconRect( FALSE ).y() - orig.y(),
					       it->iconRect().width(), it->iconRect().height() ),
					QRect( it->textRect( FALSE ).x() - orig.x(),
					       it->textRect( FALSE ).y() - orig.y(), 	
					       it->textRect().width(), it->textRect().height() ),
					((KFileIVI *)it)->item()->url().url() ) );
	}
    }
    return drag;
}


void KonqIconViewWidget::initDragEnter( QDropEvent *e )
{
    if ( KonqDrag::canDecode( e ) ) {	
	QValueList<KonqDragItem> lst;
	KonqDrag::decode( e, lst );
	if ( lst.count() != 0 ) {
	    setDragObjectIsKnown( e );
	} else {
	    QStringList l;
	    KonqDrag::decode( e, l );
	    setNumDragItems( l.count() );
	}
    } else if ( QUriDrag::canDecode( e ) ) {
	QStringList l;
	QUriDrag::decodeLocalFiles( e, l );
	setNumDragItems( l.count() );
    } else {
	QIconView::initDragEnter( e );
    }
}

////////////////////////////////////////////////////////////////////////////

IconEditExtension::IconEditExtension( KonqIconViewWidget *iconView )
 : EditExtension( iconView, "IconEditExtension" )
{
  m_iconView = iconView;
  connect( m_iconView, SIGNAL( selectionChanged() ),
           this, SIGNAL( selectionChanged() ) );
}

void IconEditExtension::can( bool &cut, bool &copy, bool &paste, bool &move )
{
  bool bInTrash = false;
  int iCount = 0;

  for ( QIconViewItem *it = m_iconView->firstItem(); it; it = it->nextItem() )
  {
    if ( it->isSelected() )
      iCount++;

    if ( ((KFileIVI *)it)->item()->url().directory(false) == KUserPaths::trashPath() )
      bInTrash = true;
  }

  cut = move = copy = iCount > 0;
  move = move && !bInTrash;

  bool bKIOClipboard = !isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
    (iCount <= 1); // We can't paste to more than one destination, can we ?
  // TODO : if only one url, check that it's a dir
}

void IconEditExtension::cutSelection()
{
  //TODO: grey out items
  copySelection();
}

void IconEditExtension::copySelection()
{
  QDragObject * obj = m_iconView->dragObject();
  QApplication::clipboard()->setData( obj );
}

void IconEditExtension::pasteSelection( bool move )
{
  KFileItemList lstItems = m_iconView->selectedFileItems();
  assert ( lstItems.count() <= 1 );
  if ( lstItems.count() == 1 )
    pasteClipboard( lstItems.first()->url().url(), move );
  else
    pasteClipboard( m_iconView->url(), move );
}

void IconEditExtension::moveSelection( const QString &destinationURL )
{
  QStringList lstURLs;

  for ( QIconViewItem *it = m_iconView->firstItem(); it; it = it->nextItem() )
    if ( it->isSelected() )
      lstURLs.append( ( (KFileIVI *)it )->item()->url().url() );

  KIOJob *job = new KIOJob;

  if ( !destinationURL.isEmpty() )
    job->move( lstURLs, destinationURL );
  else
    job->del( lstURLs );
}

#include "konqiconviewwidget.moc"
