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
#include <konqfileitem.h>
#include <kmessagebox.h>
#include <konqdefaults.h>
#include <konqsettings.h>
#include <konqdrag.h>
#include <konqoperations.h>
#include <kuserpaths.h>

#include <assert.h>
#include <unistd.h>

KonqIconViewWidget::KonqIconViewWidget( QWidget * parent, const char * name, WFlags f )
    : KIconView( parent, name, f ),
      m_rootItem( 0L ),
      m_bImagePreviewAllowed( false )
{
    QObject::connect( this, SIGNAL( dropped( QDropEvent *, const QValueList<QIconDragItem> & ) ),
		      this, SLOT( slotDropped( QDropEvent*, const QValueList<QIconDragItem> & ) ) );
	
    QObject::connect( this, SIGNAL( selectionChanged() ),
                      this, SLOT( slotSelectionChanged() ) );

    QObject::connect(
      horizontalScrollBar(),  SIGNAL(valueChanged(int)),
      this,                   SLOT(slotViewportScrolled(int)));
      
    QObject::connect(
      verticalScrollBar(),  SIGNAL(valueChanged(int)),
      this,                 SLOT(slotViewportScrolled(int)));

    // hardcoded settings
    setSelectionMode( QIconView::Extended );
    setItemTextPos( QIconView::Bottom );
    setGridX( 70 );
    setAutoArrange( true );
    setSorting( true, sortDirection() );
    m_bSortDirsFirst = true;
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

KonqFileItemList KonqIconViewWidget::selectedFileItems()
{
    KonqFileItemList lstItems;

    QIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
	if ( it->isSelected() ) {
	    KonqFileItem *fItem = ((KFileIVI *)it)->item();
	    lstItems.append( fItem );
	}
    return lstItems;
}

void KonqIconViewWidget::slotDropped( QDropEvent *ev, const QValueList<QIconDragItem> & )
{
    // Drop on background
    if ( m_rootItem ) // otherwise, it's too early, '.' is not yet listed
        KonqOperations::doDrop( m_rootItem, ev, this );
}

void KonqIconViewWidget::slotDropItem( KFileIVI *item, QDropEvent *ev )
{
    assert( item );
    KonqOperations::doDrop( item->item(), ev, this );
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

    KonqDrag *drag = new KonqDrag( viewport() );
    // Position of the mouse in the view
    QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );
    // Position of the item clicked in the view
    QPoint itempos = currentItem()->pixmapRect( FALSE ).topLeft();
    // Set pixmap, with the correct offset
    drag->setPixmap( *currentItem()->pixmap(), orig - itempos );
    // Append all items to the drag object
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
	if ( it->isSelected() ) {
          QString itemURL = ((KFileIVI *)it)->item()->url().url();
          QIconDragItem id;
          id.setData( QCString(itemURL.latin1()) );
          drag->append( id,
                        QRect( it->pixmapRect( FALSE ).x() - orig.x(),
                               it->pixmapRect( FALSE ).y() - orig.y(),
                               it->pixmapRect().width(), it->pixmapRect().height() ),
                        QRect( it->textRect( FALSE ).x() - orig.x(),
                               it->textRect( FALSE ).y() - orig.y(),	
                               it->textRect().width(), it->textRect().height() ),
                        itemURL );
	}
    }
    return drag;
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
    // This code is very related to TreeViewBrowserExtension::updateActions
    bool cutcopy, del;
    bool bInTrash = false;
    int iCount = 0;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
    {
	if ( it->isSelected() )
	    iCount++;

	if ( ((KFileIVI *)it)->item()->url().directory(false) == KUserPaths::trashPath() )
	    bInTrash = true;
    }
    cutcopy = del = ( iCount > 0 );

    emit enableAction( "cut", cutcopy );
    emit enableAction( "copy", cutcopy );
    emit enableAction( "trash", del && !bInTrash );
    emit enableAction( "del", del );
    emit enableAction( "shred", del );

    bool bKIOClipboard = !KIO::isClipboardEmpty();
    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
	(iCount <= 1); // We can't paste to more than one destination, can we ?

    emit enableAction( "pastecopy", paste );
    emit enableAction( "pastecut", paste );

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
    KURL::List lst = selectedUrls();
    assert ( lst.count() <= 1 );
    if ( lst.count() == 1 )
	KIO::pasteClipboard( lst.first(), move );
    else
	KIO::pasteClipboard( url(), move );
}

KURL::List KonqIconViewWidget::selectedUrls()
{
    KURL::List lstURLs;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
	if ( it->isSelected() )
	    lstURLs.append( ( (KFileIVI *)it )->item()->url() );
    return lstURLs;
}

bool KonqIconViewWidget::sortDirectoriesFirst() const
{
  return m_bSortDirsFirst;
}

void KonqIconViewWidget::setSortDirectoriesFirst( bool b )
{
  m_bSortDirsFirst = b;
}

// For KonqOperations
void KonqIconViewWidget::slotResult( KIO::Job * job )
{
  if (job->error())
    job->showErrorDialog();
}

void KonqIconViewWidget::slotViewportScrolled(int)
{
  emit(viewportAdjusted());
}

void KonqIconViewWidget::viewportResizeEvent(QResizeEvent * e)
{
  KIconView::viewportResizeEvent(e);
  emit(viewportAdjusted());
}



#include "konqiconviewwidget.moc"
