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
#include "konq_iconviewwidget.h"
#include "konq_undo.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qfile.h>
#include <qdragobject.h>
#include <qlist.h>
#include <qregion.h>
#include <qtimer.h>
#include <qpainter.h>

#include <kapp.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/previewjob.h>
#include <klocale.h>
#include <kfileivi.h>
#include <konq_fileitem.h>
#include <kmessagebox.h>
#include <konq_defaults.h>
#include <konq_settings.h>
#include <konq_drag.h>
#include <konq_operations.h>
#include <konq_imagepreviewjob.h>
#include <kglobalsettings.h>
#include <kstddirs.h>
#include <kpropsdlg.h>
#include <kipc.h>
#include <kiconeffect.h>
#include <kurldrag.h>
#include <klibloader.h>

#include <assert.h>
#include <unistd.h>

struct KonqIconViewWidgetPrivate
{
    KFileIVI *pActiveItem;
    bool bSoundPreviews;
    KFileIVI *pSoundItem;
    QObject *pSoundPlayer;
    QTimer *pSoundTimer;
    KIO::PreviewJob *pPreviewJob;
};

KonqIconViewWidget::KonqIconViewWidget( QWidget * parent, const char * name, WFlags f, bool kdesktop )
    : KIconView( parent, name, f ),
      m_rootItem( 0L ), m_size( 0 ) /* default is DesktopIcon size */,
      m_bDesktop( kdesktop ),
      m_bSetGridX( !kdesktop ) /* No line breaking on the desktop */
{
    d = new KonqIconViewWidgetPrivate;
    connect( this, SIGNAL( dropped( QDropEvent *, const QValueList<QIconDragItem> & ) ),
             this, SLOT( slotDropped( QDropEvent*, const QValueList<QIconDragItem> & ) ) );

    connect( this, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    connect( horizontalScrollBar(),  SIGNAL(valueChanged(int)),
             this,                   SIGNAL(viewportAdjusted()));

    connect( verticalScrollBar(),  SIGNAL(valueChanged(int)),
             this,                 SIGNAL(viewportAdjusted()));

    kapp->addKipcEventMask( KIPC::IconChanged );
    connect( kapp, SIGNAL(iconChanged(int)), SLOT(slotIconChanged(int)) );
    connect( this, SIGNAL(onItem(QIconViewItem *)), SLOT(slotOnItem(QIconViewItem *)) );
    connect( this, SIGNAL(onViewport()), SLOT(slotOnViewport()) );
    connect( this, SIGNAL(itemRenamed(QIconViewItem *, const QString &)), SLOT(slotItemRenamed(QIconViewItem *, const QString &)) );

    // hardcoded settings
    setSelectionMode( QIconView::Extended );
    setItemTextPos( QIconView::Bottom );

    calculateGridX();
    setAutoArrange( true );
    setSorting( true, sortDirection() );
    m_bSortDirsFirst = true;
    d->pActiveItem = 0;
    d->bSoundPreviews = false;
    d->pSoundItem = 0;
    d->pSoundPlayer = 0;
    d->pSoundTimer = 0;
    d->pPreviewJob = 0;
    m_bMousePressed = false;
    m_LineupMode = LineupBoth;
    // emit our signals
    slotSelectionChanged();
    m_iconPositionGroupPrefix = QString::fromLatin1( "IconPosition::" );
    KonqUndoManager::incRef();
}

KonqIconViewWidget::~KonqIconViewWidget()
{
    stopImagePreview();
    KonqUndoManager::decRef();
    if (d->pSoundPlayer)
        delete d->pSoundPlayer;
    delete d;
}

void KonqIconViewWidget::focusOutEvent( QFocusEvent * /* ev */ )
{
    // We can't possibly have the mouse pressed and still lose focus.
    // Well, we can, but when we regain focus we should assume the mouse is
    // not down anymore or the slotOnItem code will break with highlighting!
    m_bMousePressed = false;
}

void KonqIconViewWidget::slotItemRenamed(QIconViewItem *item, const QString &name)
{
    kdDebug(1203) << "KonqIconViewWidget::slotItemRenamed" << endl;
    KFileItem * fileItem = static_cast<KFileIVI *>(item)->item();
    KonqOperations::rename( this, fileItem->url(), name );
}

void KonqIconViewWidget::slotIconChanged( int group )
{
    if (group != KIcon::Desktop)
        return;

    int size = m_size;
    if ( m_size == 0 )
      m_size = -1; // little trick to force grid change in setIcons
    setIcons( size ); // force re-determining all icons
}

void KonqIconViewWidget::slotOnItem( QIconViewItem *item )
{
    // Reset icon of previous item
    if( d->pActiveItem != 0L && d->pActiveItem != item )
    {
	d->pActiveItem->setEffect( KIcon::Desktop, KIcon::DefaultState );
	d->pActiveItem = 0L;
    }

    // Stop sound
    if (d->pSoundPlayer != 0 && static_cast<KFileIVI *>(item) != d->pSoundItem)
    {
	delete d->pSoundPlayer;
	d->pSoundPlayer = 0;

	d->pSoundItem = 0;
	if (d->pSoundTimer && d->pSoundTimer->isActive())
    	    d->pSoundTimer->stop();
    }

    if ( !m_bMousePressed )
    {
	if( item != d->pActiveItem )
	{
	    d->pActiveItem = static_cast<KFileIVI *>(item);
	    d->pActiveItem->setEffect( KIcon::Desktop, KIcon::ActiveState );

	    //kdDebug(1203) << "desktop;defaultstate=" <<
	    //      KGlobal::iconLoader()->iconEffect()->
	    //      fingerprint(KIcon::Desktop, KIcon::DefaultState) <<
	    //      endl;
	    //kdDebug(1203) << "desktop;activestate=" <<
	    //      KGlobal::iconLoader()->iconEffect()->
	    //      fingerprint(KIcon::Desktop, KIcon::ActiveState) <<
	    //      endl;
	}
	else
	{
	    // No effect. If we want to underline on hover, we should
	    // force the IVI to repaint here, though!
	    d->pActiveItem = 0L;
	}
    }	// bMousePressed
    else
    {
      // All features disabled during mouse clicking, e.g. rectangular
      // selection
      d->pActiveItem = 0L;
    }
    
    if (d->bSoundPreviews && static_cast<KFileIVI *>(item)->item()->mimetype().startsWith("audio/"))
    {
      d->pSoundItem = static_cast<KFileIVI *>(item);
      if (!d->pSoundTimer)
      {
        d->pSoundTimer = new QTimer(this);
        connect(d->pSoundTimer, SIGNAL(timeout()), SLOT(slotStartSoundPreview()));
      }
      if (d->pSoundTimer->isActive())
        d->pSoundTimer->stop();
      d->pSoundTimer->start(500, true);
    }
    else
    {
      if (d->pSoundPlayer)
      {
        delete d->pSoundPlayer;
        d->pSoundPlayer = 0;
      }
      d->pSoundItem = 0;
      if (d->pSoundTimer && d->pSoundTimer->isActive())
        d->pSoundTimer->stop();
    }
}

void KonqIconViewWidget::slotOnViewport()
{
    if (d->pSoundPlayer)
    {
      delete d->pSoundPlayer;
      d->pSoundPlayer = 0;
    }
    d->pSoundItem = 0;
    if (d->pSoundTimer && d->pSoundTimer->isActive())
      d->pSoundTimer->stop();

    if (d->pActiveItem == 0L)
        return;
				
    d->pActiveItem->setEffect( KIcon::Desktop, KIcon::DefaultState );
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::slotStartSoundPreview()
{
  if (!d->pSoundItem)
    return;
  KLibFactory *factory = KLibLoader::self()->factory("libkonqsound");
  if (factory)
  {
    QStringList args;
    args << d->pSoundItem->item()->url().url();
    d->pSoundPlayer = factory->create(this, 0, "KPlayObject", args);
  }
}

void KonqIconViewWidget::slotPreview(const KFileItem *item, const QPixmap &pix)
{
    for (QIconViewItem *it = firstItem(); it; it = it->nextItem())
        if (static_cast<KFileIVI *>(it)->item() == item)
            static_cast<KFileIVI *>(it)->setThumbnailPixmap(pix);
}

void KonqIconViewWidget::slotPreviewResult()
{
    d->pPreviewJob = 0;
    if (autoArrange())
        arrangeItemsInGrid();
}

void KonqIconViewWidget::clear()
{
    KIconView::clear();
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::takeItem( QIconViewItem *item )
{
    if ( d->pActiveItem == static_cast<KFileIVI *>(item) )
        d->pActiveItem = 0L;

    if ( d->pPreviewJob )
      d->pPreviewJob->removeItem( static_cast<KFileIVI *>(item)->item() );

    KIconView::takeItem( item );
}

void KonqIconViewWidget::setThumbnailPixmap( KFileIVI * item, const QPixmap & pixmap )
{
    if ( item )
    {
        if ( d->pActiveItem == item )
            d->pActiveItem = 0L;
        item->setThumbnailPixmap( pixmap );
        if ( m_bSetGridX &&  item->width() > gridX() )
        {
          setGridX( item->width() );
          if (autoArrange())
            arrangeItemsInGrid();
        }
    }
}

void KonqIconViewWidget::initConfig( bool bInit )
{
    m_pSettings = KonqFMSettings::settings();

    // Color settings
    QColor normalTextColor       = m_pSettings->normalTextColor();
    setItemColor( normalTextColor );

    if (m_bDesktop)
    {
      QColor itemTextBg = m_pSettings->itemTextBackground();
      if ( itemTextBg.isValid() )
          setItemTextBackground( itemTextBg );
      else
          setItemTextBackground( NoBrush );
    }

    // Font settings
    QFont font( m_pSettings->standardFont() );
    font.setUnderline( m_pSettings->underlineLink() );
    setItemFont( font );

    setWordWrapIconText( m_pSettings->wordWrapText() );

    if (!bInit)
        updateContents();
}

void KonqIconViewWidget::setIcons( int size, const char * stopImagePreviewFor )
{
    kdDebug(1203) << "KonqIconViewWidget::setIcons( " << size << " , " << stopImagePreviewFor << ")" << endl;
    bool sizeChanged = (m_size != size);
    int oldGridX = gridX();
    m_size = size;
    if ( sizeChanged || stopImagePreviewFor )
    {
        calculateGridX();
    }
    // Do this even if size didn't change, since this is used by refreshMimeTypes...
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
        KFileIVI * ivi = static_cast<KFileIVI *>( it );
	if ( !ivi->isThumbnail() ||
	    ( stopImagePreviewFor && strlen(stopImagePreviewFor) == 0) ||
	    ( stopImagePreviewFor &&
	    ivi->thumbnailName() == stopImagePreviewFor ) )
	{
            // perhaps we should do one big redraw instead ?
	    ivi->setIcon( size, ivi->state(), true, true );
	}
	else
	    ivi->invalidateThumb( ivi->state(), true );
    }
    if ( autoArrange() && (oldGridX != gridX() || stopImagePreviewFor) )
    {
        arrangeItemsInGrid( true ); // take new grid into account
    }
}

void KonqIconViewWidget::setItemTextPos( ItemTextPos pos )
{
    if ( m_bSetGridX )
    {
        calculateGridX();
        if ( itemTextPos() != pos )
        {
            if ( pos == QIconView::Right )
                setGridX( gridX() + 100 );
            else
                setGridX( gridX() - 100 );
        }
    }

    KIconView::setItemTextPos( pos );
}

void KonqIconViewWidget::calculateGridX()
{
    if ( m_bSetGridX )
    {
        int sz = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
        int newGridX = sz + 50 + (( itemTextPos() == QIconView::Right ) ? 100 : 0);

        kdDebug(1203) << "calculateGridX: newGridX=" << newGridX
                      << "sz=" << sz << endl;

        setGridX( newGridX );
    }
}

void KonqIconViewWidget::refreshMimeTypes()
{
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
        (static_cast<KFileIVI *>( it ))->item()->refreshMimeType();
    setIcons( m_size );
}

void KonqIconViewWidget::setURL( const KURL &kurl )
{
    stopImagePreview();
    m_url = kurl;
    if ( m_url.isLocalFile() )
        m_dotDirectoryPath = m_url.path().append( ".directory" );
    else
        m_dotDirectoryPath = QString::null;
}

void KonqIconViewWidget::startImagePreview( const QStringList &previewSettings, bool force )
{
    stopImagePreview(); // just in case

    KFileItemList items;
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
        if ( force || !static_cast<KFileIVI *>( it )->isThumbnail() )
            items.append( static_cast<KFileIVI *>( it )->item() );

    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int size;
    if (iconSize < 28)
        size = 48;
    else if (iconSize < 40)
        size = 60;
    else
        size = 90;

    d->pPreviewJob = KIO::filePreview( items, size, size, iconSize,
        m_pSettings->textPreviewIconTransparency(), true /* scale */,
        true /* save */, &previewSettings );
    connect( d->pPreviewJob, SIGNAL( gotPreview( const KFileItem *, const QPixmap & ) ),
             this, SLOT( slotPreview( const KFileItem *, const QPixmap & ) ) );
    connect( d->pPreviewJob, SIGNAL( result( KIO::Job * ) ),
             this, SIGNAL( imagePreviewFinished() ) );
    connect( d->pPreviewJob, SIGNAL( result( KIO::Job * ) ),
             this, SLOT( slotPreviewResult() ) );
    d->bSoundPreviews = previewSettings.contains( "audio/" );
}

void KonqIconViewWidget::stopImagePreview()
{
    if (d->pPreviewJob)
    {
        d->pPreviewJob->kill();
        d->pPreviewJob = 0;
        if (autoArrange())
            arrangeItemsInGrid();
    }
}

KFileItemList KonqIconViewWidget::selectedFileItems()
{
    KFileItemList lstItems;

    QIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
        if ( it->isSelected() ) {
            KFileItem *fItem = (static_cast<KFileIVI *>(it))->item();
            lstItems.append( fItem );
        }
    return lstItems;
}

void KonqIconViewWidget::slotDropped( QDropEvent *ev, const QValueList<QIconDragItem> & )
{
    // Drop on background
    KonqOperations::doDrop( m_rootItem /* may be 0L */, url(), ev, this );
}

void KonqIconViewWidget::drawBackground( QPainter *p, const QRect &r )
{
    drawBackground(p, r, r.topLeft());
}

void KonqIconViewWidget::drawBackground( QPainter *p, const QRect &r , const QPoint &pt)
{
    const QPixmap *pm  = backgroundPixmap();
    bool hasPixmap = pm && !pm->isNull();
    if ( !hasPixmap ) {
        pm = viewport()->backgroundPixmap();
        hasPixmap = pm && !pm->isNull();
    }

    QRect rtgt(r);
    rtgt.moveTopLeft(pt);
    if (!hasPixmap && backgroundMode() != NoBackground) {
        p->fillRect(rtgt, viewport()->backgroundColor());
        return;
    }

    if (hasPixmap) {
        int ax = (r.x() + contentsX() + leftMargin()) % pm->width();
        int ay = (r.y() + contentsY() + topMargin()) % pm->height();
        p->drawTiledPixmap(rtgt, *pm, QPoint(ax, ay));
    }
}

QDragObject * KonqIconViewWidget::dragObject()
{
    if ( !currentItem() )
        return 0;

    return konqDragObject( viewport() );
}

KonqIconDrag * KonqIconViewWidget::konqDragObject( QWidget * dragSource )
{
    //kdDebug(1203) << "KonqIconViewWidget::konqDragObject" << endl;

    KonqIconDrag * drag = new KonqIconDrag( dragSource );
    // Position of the mouse in the view
    QPoint orig = viewportToContents( viewport()->mapFromGlobal( m_mousePos ) );
    // Position of the item clicked in the view
    QPoint itempos = currentItem()->pixmapRect( FALSE ).topLeft();
    // Set pixmap, with the correct offset
    drag->setPixmap( *currentItem()->pixmap(), orig - itempos );
    // Append all items to the drag object
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
        if ( it->isSelected() ) {
          QString itemURL = (static_cast<KFileIVI *>(it))->item()->url().url(0, 106); // 106 is mib enum for utf8 codec
          kdDebug(1203) << "itemURL=" << itemURL << endl;
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

void KonqIconViewWidget::contentsDragEnterEvent( QDragEnterEvent *e )
{
    if ( e->provides( "text/uri-list" ) )
    {
        QByteArray payload = e->encodedData( "text/uri-list" );
        if ( !payload.size() )
            kdError() << "Empty data !" << endl;
        // Cache the URLs, since we need them every time we move over a file
        // (see KFileIVI)
        bool ok = KURLDrag::decode( e, m_lstDragURLs );
        if( !ok )
            kdError() << "Couldn't decode urls dragged !" << endl;
    }
    KIconView::contentsDragEnterEvent( e );
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

void KonqIconViewWidget::disableIcons( const KURL::List & lst )
{
  for ( QIconViewItem *kit = firstItem(); kit; kit = kit->nextItem() )
  {
      bool bFound = false;
      // Wow. This is ugly. Matching two lists together....
      // Some sorting to optimise this would be a good idea ?
      for (KURL::List::ConstIterator it = lst.begin(); !bFound && it != lst.end(); ++it)
      {
          if ( static_cast<KFileIVI *>( kit )->item()->url() == *it )
          {
              bFound = true;
              // maybe remove "it" from lst here ?
          }
      }
      static_cast<KFileIVI *>( kit )->setDisabled( bFound );
  }
}

void KonqIconViewWidget::slotSelectionChanged()
{
    // This code is very related to TreeViewBrowserExtension::updateActions
    bool cutcopy, del;
    bool bInTrash = false;
    int iCount = 0;
    KFileItem * firstSelectedItem = 0L;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
    {
        if ( it->isSelected() )
        {
            iCount++;
            if ( ! firstSelectedItem )
                firstSelectedItem = (static_cast<KFileIVI *>( it ))->item();

            if ( (static_cast<KFileIVI *>( it ))->item()->url().directory(false) == KGlobalSettings::trashPath() )
                bInTrash = true;
        }
    }
    cutcopy = del = ( iCount > 0 );

    emit enableAction( "cut", cutcopy );
    emit enableAction( "copy", cutcopy );
    emit enableAction( "trash", del && !bInTrash && m_url.isLocalFile() );
    emit enableAction( "del", del );
    emit enableAction( "shred", del );

    KFileItemList lstItems;
    if ( firstSelectedItem )
        lstItems.append( firstSelectedItem );
    emit enableAction( "properties", ( iCount == 1 ) &&
                       KPropertiesDialog::canDisplay( lstItems ) );
    emit enableAction( "editMimeType", ( iCount == 1 ) );
    emit enableAction( "rename", ( iCount == 1 ) );
}

void KonqIconViewWidget::renameSelectedItem()
{
    kdDebug(1203) << " -- KonqIconViewWidget::renameSelectedItem() -- " << endl;
    QIconViewItem * item = 0L;
    QIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
        if ( it->isSelected() && !item )
        {
            item = it;
            break;
        }
    if (!item)
    {
        Q_ASSERT(item);
        return;
    }
    item->rename();
}

void KonqIconViewWidget::cutSelection()
{
    kdDebug(1203) << " -- KonqIconViewWidget::cutSelection() -- " << endl;
    KonqIconDrag * obj = konqDragObject( /* no parent ! */ );
    obj->setMoveSelection( true );
    QApplication::clipboard()->setData( obj );
}

void KonqIconViewWidget::copySelection()
{
    kdDebug(1203) << " -- KonqIconViewWidget::copySelection() -- " << endl;
    KonqIconDrag * obj = konqDragObject( /* no parent ! */ );
    QApplication::clipboard()->setData( obj );
}

void KonqIconViewWidget::pasteSelection()
{
    KURL::List lst = selectedUrls();

    // nonsense.
    //assert ( lst.count() <= 1 );
    /*KURL pasteURL;
    if ( lst.count() == 1 )
      pasteURL = lst.first();
    else
      pasteURL = url(); */

    KonqOperations::doPaste( this, /*pasteURL*/url() );
}

KURL::List KonqIconViewWidget::selectedUrls()
{
    KURL::List lstURLs;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
        if ( it->isSelected() )
            lstURLs.append( (static_cast<KFileIVI *>( it ))->item()->url() );
    return lstURLs;
}

QRect KonqIconViewWidget::iconArea() const
{
    return m_IconRect;
}

void KonqIconViewWidget::setIconArea(const QRect &rect)
{
    m_IconRect = rect;
}

int KonqIconViewWidget::lineupMode() const
{
    return m_LineupMode;
}

void KonqIconViewWidget::setLineupMode(int mode)
{
    m_LineupMode = mode;
}

bool KonqIconViewWidget::sortDirectoriesFirst() const
{
  return m_bSortDirsFirst;
}

void KonqIconViewWidget::setSortDirectoriesFirst( bool b )
{
  m_bSortDirsFirst = b;
}

void KonqIconViewWidget::viewportResizeEvent(QResizeEvent * e)
{
  KIconView::viewportResizeEvent(e);
  emit viewportAdjusted();
}

void KonqIconViewWidget::contentsDropEvent( QDropEvent * ev )
{
  QIconViewItem *i = findItem( ev->pos() );
  // Short-circuit QIconView if Ctrl is pressed, so that it's possible
  // to drop a file into its own parent widget to copy it.
  if ( !i && (ev->action() == QDropEvent::Copy || ev->action() == QDropEvent::Link)
          && ev->source() && ev->source() == viewport())
  {
    // First we need to call QIconView though, to clear the drag shape
    setItemsMovable(false); // hack ? call it what you want :-)
    KIconView::contentsDropEvent( ev );
    setItemsMovable(true);

    QValueList<QIconDragItem> lst;
    slotDropped(ev, lst);
  }
  else
  {
    KIconView::contentsDropEvent( ev );
    emit dropped(); // What is this for ? (David)
  }
  slotSaveIconPositions();
}

void KonqIconViewWidget::contentsMousePressEvent( QMouseEvent *e )
{
  //kdDebug(1203) << "KonqIconViewWidget::contentsMousePressEvent" << endl;
  m_mousePos = QCursor::pos();
  m_bMousePressed = true;
  KIconView::contentsMousePressEvent( e );
}

void KonqIconViewWidget::contentsMouseReleaseEvent( QMouseEvent *e )
{
  m_bMousePressed = false;
  KIconView::contentsMouseReleaseEvent( e );
}

void KonqIconViewWidget::slotSaveIconPositions()
{
  if ( m_dotDirectoryPath.isEmpty() )
    return;
  //kdDebug(1203) << "KonqIconViewWidget::slotSaveIconPositions" << endl;
  KSimpleConfig dotDirectory( m_dotDirectoryPath );
  QIconViewItem *it = firstItem();
  if ( !it )
    return; // No more icons. Maybe we're closing and they've been removed already
  while ( it )
  {
    KFileIVI *ivi = static_cast<KFileIVI *>( it );
    KonqFileItem *item = ivi->item();

    dotDirectory.setGroup( QString( m_iconPositionGroupPrefix ).append( item->url().fileName() ) );
    //kdDebug(1203) << "KonqIconViewWidget::slotSaveIconPositions " << item->url().fileName() << " " << it->x() << " " << it->y() << endl;
    dotDirectory.writeEntry( "X", it->x() );
    dotDirectory.writeEntry( "Y", it->y() );
    dotDirectory.writeEntry( "Exists", true );

    it = it->nextItem();
  }

  QStringList groups = dotDirectory.groupList();
  QStringList::ConstIterator gIt = groups.begin();
  QStringList::ConstIterator gEnd = groups.end();
  for (; gIt != gEnd; ++gIt )
    if ( (*gIt).left( m_iconPositionGroupPrefix.length() ) == m_iconPositionGroupPrefix )
    {
      dotDirectory.setGroup( *gIt );
      if ( dotDirectory.hasKey( "Exists" ) )
        dotDirectory.deleteEntry( "Exists", false );
      else
        dotDirectory.deleteGroup( *gIt );
    }

  dotDirectory.sync();
}

// Adapted version of QIconView::insertInGrid, that works relative to
// m_IconRect, instead of the entire viewport.

void KonqIconViewWidget::insertInGrid(QIconViewItem *item)
{
    if (0L == item)
        return;

    if (!m_IconRect.isValid())
    {
        QIconView::insertInGrid(item);
        return;
    }

    QRegion r(m_IconRect);
    QIconViewItem *i = firstItem();
    int y = -1;
    for (; i; i = i->nextItem() )
    {
        r = r.subtract(i->rect());
        y = QMAX(y, i->y() + i->height());
    }

    QArray<QRect> rects = r.rects();
    QArray<QRect>::Iterator it = rects.begin();
    bool foundPlace = FALSE;
    for (; it != rects.end(); ++it)
    {
        QRect rect = *it;
        if (rect.width() >= item->width() && rect.height() >= item->height())
        {
            int sx = 0, sy = 0;
            if (rect.width() >= item->width() + spacing())
                sx = spacing();
            if (rect.height() >= item->height() + spacing())
                sy = spacing();
            item->move(rect.x() + sx, rect.y() + sy);
            foundPlace = true;
            break;
        }
    }

    if (!foundPlace)
        item->move(m_IconRect.topLeft());

    //item->dirty = false;
    return;
}


/*
 * Utility class QIVItemBin is used by KonqIconViewWidget::lineupIcons().
 */

class QIVItemBin
{
public:
    QIVItemBin() {}
    ~QIVItemBin() {}

    int count() { return mData.count(); }
    void add(QIconViewItem *item) { mData.append(item); }

    QIconViewItem *top();
    QIconViewItem *bottom();
    QIconViewItem *left();
    QIconViewItem *right();

private:
    QList<QIconViewItem> mData;
};

QIconViewItem *QIVItemBin::top()
{
    if (mData.count() == 0)
        return 0L;

    QIconViewItem *it = mData.first();
    QIconViewItem *item = it;
    int y = it->y();
    for (it=mData.next(); it; it=mData.next())
    {
        if (it->y() < y)
        {
            y = it->y();
            item = it;
        }
    }
    mData.remove(item);
    return item;
}

QIconViewItem *QIVItemBin::bottom()
{
    if (mData.count() == 0)
        return 0L;

    QIconViewItem *it = mData.first();
    QIconViewItem *item = it;
    int y = it->y();
    for (it=mData.next(); it; it=mData.next())
    {
        if (it->y() > y)
        {
            y = it->y();
            item = it;
        }
    }
    mData.remove(item);
    return item;
}

QIconViewItem *QIVItemBin::left()
{
    if (mData.count() == 0)
        return 0L;

    QIconViewItem *it=mData.first();
    QIconViewItem *item = it;
    int x = it->x();
    for (it=mData.next(); it; it=mData.next())
    {
        if (it->x() < x)
        {
            x = it->x();
            item = it;
        }
    }
    mData.remove(item);
    return item;
}

QIconViewItem *QIVItemBin::right()
{
    if (mData.count() == 0)
        return 0L;

    QIconViewItem *it=mData.first();
    QIconViewItem *item = it;
    int x = it->x();
    for (it=mData.next(); it; it=mData.next())
    {
        if (it->x() > x)
        {
            x = it->x();
            item = it;
        }
    }
    mData.remove(item);
    return item;
}


/*
 * The algorithm used for lineing up the icons could be called
 * "beating flat the icon field". Imagine the icon field to be some height
 * field on a regular grid, with the height being the number of icons in
 * each grid element. Now imagine slamming on the field with a shovel or
 * some other flat surface. The high peaks will be flattened and spread out
 * over their adjacent areas. This is basically what the algorithm tries to
 * simulate.
 *
 * First, the icons are binned to a grid of the desired size. If all bins
 * are containing at most one icon, we're done, of course. We just have to
 * move all icons to the center of each grid element.
 * For each bin which has more than one icon in it, we calculate 4
 * "friction coefficients", one for each cardinal direction. The friction
 * coefficient of a direction is the number of icons adjacent in that
 * direction. The idea is that this number is somewhat a measure in which
 * direction the icons should flow: icons flow in the direction of lowest
 * friction coefficient. We move a maximum of one icon per bin and loop over
 * all bins. This procedure is repeated some maximum number of times or until
 * no icons are moved anymore.
 *
 * I don't know if this algorithm is good or bad, I don't even know if it will
 * work all the time. It seems a correct thing to do, however, and it seems to
 * work particularly well. In any case, the number of runs is limited so there
 * can be no races.
 */

#define MIN3(a,b,c) ((a) < QMIN((b),(c)) ? (a) : ((b) < QMIN((a),(c)) ? (b) : (c)))

void KonqIconViewWidget::lineupIcons()
{
    if ( !firstItem() )
    {
        kdDebug(1203) << "No icons at all ?\n";
        return;
    }

    // For dx, use what used to be the gridX
    int sz = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int dx = sz + 30 + (( itemTextPos() == QIconView::Right ) ? 50 : 0);
    // For dy, well, let's use any icon, it should do
    int dy = firstItem()->height();

    dx += spacing();
    dy += spacing();

    kdDebug(1203) << "dx = " << dx << ", dy = " << dy << "\n";

    if ((dx < 15) || (dy < 15))
    {
        kdWarning(1203) << "Do you really have that fine a grid?\n";
        return;
    }

    int x1, x2, y1, y2;
    if (m_IconRect.isValid())
    {
        x1 = m_IconRect.left(); x2 = m_IconRect.right();
        y1 = m_IconRect.top(); y2 = m_IconRect.bottom();
    } else
    {
        x1 = 0; x2 = viewport()->width();
        y1 = 0; y2 = viewport()->height();
    }

    int nx = (x2 - x1) / dx;
    int ny = (y2 - y1) / dy;

    kdDebug(1203) << "nx = " << nx << " ny = " << ny << "\n";
    if ((nx > 150) || (ny > 100))
    {
        kdDebug(1203) << "Do you really have that fine a grid?\n";
        return;
    }
    if ((nx <= 1) || (ny <= 1))
    {
        kdDebug(1203) << "Iconview is too small, not doing anything.\n";
        return;
    }

    // Create a grid of (ny x nx) bins.
    typedef QIVItemBin *QIVItemPtr;
    QIVItemPtr **bins = new QIVItemPtr*[ny];

    int i, j;
    for (j=0; j<ny; j++)
    {
        bins[j] = new QIVItemPtr[nx];
        for (i=0; i<nx; i++)
            bins[j][i] = 0;
    }

    QValueList<QIconViewItem*> items;

    // Put each ivi in its corresponding bin.
    QIconViewItem *item;
    for (item=firstItem(); item; item=item->nextItem())
    {
        items.append(item);
    }

    int left = x1;
    int right = x1 + dx;
    i = 0;

    while (items.count())
    {
        int max_icon_x = dx;
        right = left + dx;

        for (QValueList<QIconViewItem*>::Iterator it = items.begin(); it != items.end(); ++it)
        {
            item = *it;
            if (item->x() < right && max_icon_x < item->width() )
                max_icon_x = item->width();
        }

        right = left + max_icon_x;

        for (QValueList<QIconViewItem*>::Iterator it = items.begin(); it != items.end();)
        {
            item = *it;
            int mid = item->x() + item->width()/2 - x1;
            kdDebug() << "matching " << mid << " left " << left << " right " << right << endl;
            if (mid < left || (mid >= left && mid < right)) {
                it = items.remove(it);
                j = (item->y() + item->height()/2 - y1) / dy;
                if (j < 0) j = 0;
                else if (j >= ny) j = ny - 1;

                kdDebug(1203) << "putting " << item->text() << " " << i << " " << j << endl;
                if (bins[j][i] == 0L)
                    bins[j][i] = new QIVItemBin;
                bins[j][i]->add(item);
            } else
                ++it;
        }
        kdDebug() << "next round " << items.count() << endl;
        i = QMIN(i+1, nx - 1);
        left += max_icon_x + spacing();
    }

    // The shuffle code
    int n, k;
    int infinity = 100000, nmoves = 1;
    for (n=0; (n < 10) && (nmoves != 0); n++)
    {
        nmoves = 0;
        for (j=0; j<ny; j++)
        {
            for (i=0; i<nx; i++)
            {
                if (!bins[j][i] || (bins[j][i]->count() < 2))
                    continue;

                kdDebug(1203) << "calc for " << i << " " << j << endl;
                // Calculate the 4 "friction coefficients".
                int tf = 0;
                for (k=j-1; (k >= 0) && bins[k][i] && bins[k][i]->count(); k--)
                    tf += bins[k][i]->count();
                if (k == -1)
                    tf += infinity;

                int bf = 0;
                for (k=j+1; (k < ny) && bins[k][i] && bins[k][i]->count(); k++)
                    bf += bins[k][i]->count();
                if (k == ny)
                    bf += infinity;

                int lf = 0;
                for (k=i-1; (k >= 0) && bins[j][k] && bins[j][k]->count(); k--)
                    lf += bins[j][k]->count();
                if (k == -1)
                    lf += infinity;

                int rf = 0;
                for (k=i+1; (k < nx) && bins[j][k] && bins[j][k]->count(); k++)
                    rf += bins[j][k]->count();
                if (k == nx)
                    rf += infinity;

                // If we are stuck between walls, continue
                if ( (tf >= infinity) && (bf >= infinity) &&
                     (lf >= infinity) && (rf >= infinity)
                   )
                    continue;

                // Is there a preferred lineup direction?
                if (m_LineupMode == LineupHorizontal)
                {
                    tf += infinity;
                    bf += infinity;
                } else if (m_LineupMode == LineupVertical)
                {
                    lf += infinity;
                    rf += infinity;
                }

                // Move one item in the direction of the least friction.
                if (tf <= MIN3(bf,lf,rf))
                {
                    if (!bins[j-1][i])
                        bins[j-1][i] = new QIVItemBin;
                    bins[j-1][i]->add(bins[j][i]->top());
                } else if (bf <= MIN3(tf,lf,rf))
                {
                    if (!bins[j+1][i])
                        bins[j+1][i] = new QIVItemBin;
                    bins[j+1][i]->add(bins[j][i]->bottom());
                } else if (lf <= MIN3(tf,bf,rf))
                {
                    if (!bins[j][i-1])
                        bins[j][i-1] = new QIVItemBin;
                    bins[j][i-1]->add(bins[j][i]->left());
                } else
                {
                    if (!bins[j][i+1])
                        bins[j][i+1] = new QIVItemBin;
                    bins[j][i+1]->add(bins[j][i]->right());
                }

                nmoves++;
            }
        }
        kdDebug(1203) << "nmoves = " << nmoves << "\n";
    }

    // Perform the actual moving
    n = 0;
    QIconViewItem **its = new QIconViewItem*[ny];
    for (i=0; i<nx; i++)
    {
        int max_icon_x = dx;
        for (j=0; j<ny; j++)
        {
            its[j] = 0;
            if (!bins[j][i] || !bins[j][i]->count())
                continue;

            item = its[j] = bins[j][i]->top();
            if ( max_icon_x < item->width() )
                max_icon_x = item->width();
        }

        for (j=0; j<ny; j++)
        {
            if ( its[j] == 0 )
                continue;

            item = its[j];
            int x = x1 + spacing() + ( max_icon_x - item->width() )/2;
            int y = y1 + j * dy;
            if (item->pos() != QPoint(x, y))
            {
                kdDebug(1203) << "moving " << item->text() << " " << x << " " << y << endl;
                item->move(x, y);
            }
            if (bins[j][i]->count())
            {
                kdDebug(1203) << "Lineup incomplete..\n";
                item = bins[j][i]->top();
                for (k=1; item; k++)
                {
                    x = x1 + i*dx + spacing() + 10*k; y = y1 + j*dy + spacing() + 5*k;
                    if (item->pos() != QPoint(x, y))
                    {
                        item->move(x, y);
                    }
                    item = bins[j][i]->top();
                }
            }
            delete bins[j][i];
            bins[j][i] = 0;
            n++;
        }
        x1 += max_icon_x  + spacing();
    }
    delete[] its;

    updateContents();
    for (int j=0; j<ny; j++)
        delete [] bins[j];
    delete[] bins;
    kdDebug(1203) << n << " icons successfully moved.\n";
    return;
}

void KonqIconViewWidget::visualActivate(QIconViewItem * item)
{
    // Rect of the QIconViewItem.
    QRect irect = item->rect();

    // Rect of the QIconViewItem's pixmap area.
    QRect rect = item->pixmapRect();

    // Adjust to correct position. If this isn't done, the fact that the
    // text may be wider than the pixmap puts us off-centre.
    rect.moveBy(irect.x(), irect.y());

    // Adjust for scrolling (David)
    rect.moveBy( -contentsX(), -contentsY() );

    KIconEffect::visualActivate(viewport(), rect);
}

void KonqIconViewWidget::backgroundPixmapChange( const QPixmap & )
{
    viewport()->update();
}

#include "konq_iconviewwidget.moc"

/* vim: set noet sw=4 ts=8 softtabstop=4: */
