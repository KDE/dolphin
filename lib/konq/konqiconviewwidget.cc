/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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
#include <qdragobject.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <klocale.h>
#include <kfileivi.h>
#include <kfileitem.h>
#include <kmessagebox.h>
#include <konqsettings.h>
#include <konqdrag.h>
#include <kuserpaths.h>

#include <assert.h>
#include <unistd.h>

KonqIconViewWidget::KonqIconViewWidget( QWidget * parent, const char * name, WFlags f )
    : KIconView( parent, name, f ),
      m_bImagePreviewAllowed( false )
{
    QObject::connect( this, SIGNAL( dropped( QDropEvent * ) ),
		      this, SLOT( slotDrop( QDropEvent* ) ) );
	
    QObject::connect( this, SIGNAL( selectionChanged() ),
                      this, SLOT( slotSelectionChanged() ) );

    // hardcoded settings
    setSelectionMode( QIconView::Extended );
    setItemTextPos( QIconView::Bottom );
    setGridX( 70 );
    setAutoArrange( true );
    setSorting( true, sortDirection() );
    // configurable settings
    initConfig();
    // emit our signals
    slotSelectionChanged();
}

void KonqIconViewWidget::initConfig()
{
    m_pSettings = KonqFMSettings::defaultIconSettings();

    // Color settings
    QColor normalTextColor	 = m_pSettings->normalTextColor();
    // QColor highlightedTextColor	 = m_pSettings->highlightedTextColor();
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

    // Behaviour ( changecursor, autoselect, ...)
    setChangeCursor( m_pSettings->changeCursor() );
    // TODO autoselect, here or in KIconView
    setWordWrapIconText( m_pSettings->wordWrapText() );
}

void KonqIconViewWidget::setIcons( KIconLoader::Size size )
{
    m_size = size;
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
	((KFileIVI*)it)->setIcon( size, m_bImagePreviewAllowed );
    }
}

void KonqIconViewWidget::refreshMimeTypes()
{
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
	((KFileIVI*)it)->item()->refreshMimeType();
    setIcons( m_size );
}

void KonqIconViewWidget::setImagePreviewAllowed( bool b )
{
    m_bImagePreviewAllowed = b;
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
	((KFileIVI*)it)->setIcon( m_size, m_bImagePreviewAllowed );
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


void KonqIconViewWidget::slotDrop( QDropEvent *ev )
{
    // Drop on background
    KonqDrag::doDrop( m_url, ev, this );
}

void KonqIconViewWidget::slotDropItem( KFileIVI *item, QDropEvent *ev )
{
    assert( item );
    KonqDrag::doDrop( item->item()->url(), ev, this );
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
    drag->setPixmap( *currentItem()->pixmap(),
		     QPoint( currentItem()->pixmapRect().width() / 2,
			     currentItem()->pixmapRect().height() / 2 ) );
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
	if ( it->isSelected() ) {
	    drag->append( KonqDragItem( QRect( it->pixmapRect( FALSE ).x() - orig.x(),
					       it->pixmapRect( FALSE ).y() - orig.y(),
					       it->pixmapRect().width(), it->pixmapRect().height() ),
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

void KonqIconViewWidget::setItemFont( const QFont &f )
{
    setFont( f );
}

void KonqIconViewWidget::setItemColor( const QColor &c )
{
    iColor = c;
}

QColor KonqIconViewWidget::itemColor() const
{
    return iColor;
}


void KonqIconViewWidget::slotSelectionChanged()
{
    bool bInTrash = false;
    int iCount = 0;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
    {
	if ( it->isSelected() )
	    iCount++;

	if ( ((KFileIVI *)it)->item()->url().directory(false) == KUserPaths::trashPath() )
	    bInTrash = true;
    }

    emit enableAction( "cut", iCount > 0 );
    emit enableAction( "copy", iCount > 0 );
    emit enableAction( "del", iCount > 0 && !bInTrash );
    emit enableAction( "trash", iCount > 0 && !bInTrash );

    bool bKIOClipboard = !KIO::isClipboardEmpty();
    QMimeSource *data = QApplication::clipboard()->data();
    bool bPaste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
	(iCount <= 1); // We can't paste to more than one destination, can we ?

    emit enableAction( "pastecopy", bPaste );
    emit enableAction( "pastecut", bPaste );

    // TODO : if only one url, check that it's a dir
}

void KonqIconViewWidget::cutSelection()
{
    //TODO: grey out items
    copySelection();
}

void KonqIconViewWidget::copySelection()
{
    QDragObject * obj = dragObject();
    QApplication::clipboard()->setData( obj );
}

void KonqIconViewWidget::pasteSelection( bool move )
{
    KFileItemList lstItems = selectedFileItems();
    assert ( lstItems.count() <= 1 );
    if ( lstItems.count() == 1 )
	KIO::pasteClipboard( lstItems.first()->url(), move );
    else
	KIO::pasteClipboard( url(), move );
}

void KonqIconViewWidget::deleteSelection()
{
    KURL::List urls = selectedUrls();

    KConfig *config = KGlobal::config();
    config->setGroup( "Misc Defaults" );
    if ( config->readBoolEntry( "ConfirmDestructive", true ) )
    {
      KURL::List::Iterator it = urls.begin();
      QStringList decodedList;
      for ( ; it != urls.end(); ++it )
        decodedList.append( (*it).decodedURL() );

      if ( KMessageBox::questionYesNoList(0, i18n( "Do you really want to delete the file(s) ?" ), decodedList )
           == KMessageBox::No )
        return;
    }

    KIO::Job *job = KIO::del( urls );
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotResult( KIO::Job * ) ) );
}

void KonqIconViewWidget::trashSelection()
{
    KIO::Job *job = KIO::move( selectedUrls(), KUserPaths::trashPath() );
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotResult( KIO::Job * ) ) );
}

KURL::List KonqIconViewWidget::selectedUrls()
{
    KURL::List lstURLs;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
	if ( it->isSelected() )
	    lstURLs.append( ( (KFileIVI *)it )->item()->url() );
    return lstURLs;
}

void KonqIconViewWidget::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
}

#include "konqiconviewwidget.moc"
