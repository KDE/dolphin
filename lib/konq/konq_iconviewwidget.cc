/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000 - 2005 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "konq_iconviewwidget.h"
#include "konq_operations.h"
#include "konq_undo.h"
#include "konq_sound.h"
#include "konq_filetip.h"

#include <QClipboard>
#include <QLayout>
#include <QTimer>
#include <QPainter>
#include <QToolTip>
#include <QLabel>
#include <QMovie>
#include <QRegExp>
#include <QCursor>
//Added by qt3to4:
#include <QMouseEvent>
#include <QFocusEvent>
#include <Q3MemArray>
#include <QEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QList>
#include <QWheelEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QPixmap>
#include <QBuffer>

#include <kapplication.h>
#include <kdebug.h>
#include <kio/previewjob.h>
#include <kfileivi.h>
#include <konq_settings.h>
#include <konqmimedata.h>
#include <kglobalsettings.h>
#include <kpropertiesdialog.h>
#include <kipc.h>
#include <kicontheme.h>
#include <kiconeffect.h>
#include <kstandarddirs.h>
#include <kprotocolmanager.h>
#include <kservicetypetrader.h>

#include <assert.h>
#include <unistd.h>
#include <klocale.h>


struct KonqIconViewWidgetPrivate
{
    KonqIconViewWidgetPrivate() {
        pActiveItem = 0;
        bSoundPreviews = false;
        pSoundItem = 0;
        bSoundItemClicked = false;
        pSoundPlayer = 0;
        pSoundTimer = 0;
        pPreviewJob = 0;
        bAllowSetWallpaper = false;

        doAnimations = true;
        m_movie = 0L;
        m_movieBlocked = 0;
        pFileTip = 0;
        pActivateDoubleClick = 0L;
        bCaseInsensitive = true;
        pPreviewMimeTypes = 0L;
	bProgramsURLdrag = false;
    }
    ~KonqIconViewWidgetPrivate() {
        delete pSoundPlayer;
        delete pSoundTimer;
        delete m_movie;
        delete pFileTip;
        delete pActivateDoubleClick;
        delete pPreviewMimeTypes;
        //delete pPreviewJob; done by stopImagePreview
    }
    KFileIVI *pActiveItem;
    // Sound preview
    KFileIVI *pSoundItem;
    KonqSoundPlayer *pSoundPlayer;
    QTimer *pSoundTimer;
    bool bSoundPreviews;
    bool bSoundItemClicked;
    bool bAllowSetWallpaper;
    bool bCaseInsensitive;
    bool bBoostPreview;

    // Animated icons support
    bool doAnimations;
    QMovie* m_movie;
    int m_movieBlocked;
    QString movieFileName;

    KIO::PreviewJob *pPreviewJob;
    KonqFileTip* pFileTip;
    QStringList previewSettings;
    bool renameItem;
    bool firstClick;
    bool releaseMouseEvent;
    QPoint mousePos;
    Qt::KeyboardModifiers mouseModifiers;
    Qt::MouseButtons mouseButtons;
    QTimer *pActivateDoubleClick;
    QStringList* pPreviewMimeTypes;
    bool bProgramsURLdrag;
};

KonqIconViewWidget::KonqIconViewWidget( QWidget * parent, const char * name, Qt::WFlags f, bool kdesktop )
    : K3IconView( parent, name, f ),
      m_rootItem( 0L ), m_size( 0 ) /* default is DesktopIcon size */,
      m_bDesktop( kdesktop ),
      m_bSetGridX( !kdesktop ) /* No line breaking on the desktop */
{
    d = new KonqIconViewWidgetPrivate;
    connect( this, SIGNAL( dropped( QDropEvent *, const QList<Q3IconDragItem> & ) ),
             this, SLOT( slotDropped( QDropEvent*, const QList<Q3IconDragItem> & ) ) );

    connect( this, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    kapp->addKipcEventMask( KIPC::IconChanged );
    connect( kapp, SIGNAL(iconChanged(int)), SLOT(slotIconChanged(int)) );
    connect( this, SIGNAL(onItem(Q3IconViewItem *)), SLOT(slotOnItem(Q3IconViewItem *)) );
    connect( this, SIGNAL(onViewport()), SLOT(slotOnViewport()) );
    connect( this, SIGNAL(itemRenamed(Q3IconViewItem *, const QString &)), SLOT(slotItemRenamed(Q3IconViewItem *, const QString &)) );

    m_pSettings = KonqFMSettings::settings();  // already needed in setItemTextPos(), calculateGridX()
    d->bBoostPreview = boostPreview();

    // hardcoded settings
    setSelectionMode( Q3IconView::Extended );
    setItemTextPos( Q3IconView::Bottom );
    d->releaseMouseEvent = false;
    d->pFileTip = new KonqFileTip(this);
    d->firstClick = false;
    calculateGridX();
    setAutoArrange( true );
    setSorting( true, sortDirection() );
    readAnimatedIconsConfig();
    m_bSortDirsFirst = true;
    m_bMousePressed = false;
    m_LineupMode = LineupBoth;
    // emit our signals
    slotSelectionChanged();
    m_iconPositionGroupPrefix = QLatin1String( "IconPosition::" );
    KonqUndoManager::incRef();
}

KonqIconViewWidget::~KonqIconViewWidget()
{
    stopImagePreview();
    KonqUndoManager::decRef();
    delete d;
}

bool KonqIconViewWidget::maySetWallpaper()
{
    return d->bAllowSetWallpaper;
}

void KonqIconViewWidget::setMaySetWallpaper(bool b)
{
    d->bAllowSetWallpaper = b;
}

void KonqIconViewWidget::focusOutEvent( QFocusEvent * ev )
{
    // We can't possibly have the mouse pressed and still lose focus.
    // Well, we can, but when we regain focus we should assume the mouse is
    // not down anymore or the slotOnItem code will break with highlighting!
    m_bMousePressed = false;

    // This will ensure that tooltips don't pop up and the mouseover icon
    // effect will go away if the mouse goes out of the view without
    // first moving into an empty portion of the view
    // Fixes part of #86968, and #85204
    // Matt Newell 2004-09-24
    slotOnViewport();

    K3IconView::focusOutEvent( ev );
}

void KonqIconViewWidget::slotItemRenamed(Q3IconViewItem *item, const QString &name)
{
    kDebug(1203) << "KonqIconViewWidget::slotItemRenamed" << endl;
    KFileIVI *viewItem = static_cast<KFileIVI *>(item);
    KFileItem *fileItem = viewItem->item();

    // The correct behavior is to show the old name until the rename has successfully
    // completed. Unfortunately, K3IconView forces us to allow the text to be changed
    // before we try the rename, so set it back to the pre-rename state.
    viewItem->setText( fileItem->text() );
    kDebug(1203)<<" fileItem->text() ;"<<fileItem->text()<<endl;
    // Don't do anything if the user renamed to a blank name.
    if( !name.isEmpty() )
    {
        // Actually attempt the rename. If it succeeds, KDirLister will update the name.
        KUrl oldurl( fileItem->url() );
        KUrl newurl( oldurl );
        newurl.setPath( newurl.directory( KUrl::AppendTrailingSlash ) + KIO::encodeFileName( name ) );
        kDebug(1203)<<" newurl :"<<newurl<<endl;
        // We use url()+name so that it also works if the name is a relative path (#51176)
        KonqOperations::rename( this, oldurl, newurl );
    }
}

void KonqIconViewWidget::slotIconChanged( int group )
{
    if (group != K3Icon::Desktop)
        return;

    int size = m_size;
    if ( m_size == 0 )
      m_size = -1; // little trick to force grid change in setIcons
    setIcons( size ); // force re-determining all icons
    readAnimatedIconsConfig();
}

void KonqIconViewWidget::readAnimatedIconsConfig()
{
    KConfigGroup cfgGroup( KGlobal::config(), "DesktopIcons" );
    d->doAnimations = cfgGroup.readEntry( "Animated", QVariant(true /*default*/ )).toBool();
}

void KonqIconViewWidget::slotOnItem( Q3IconViewItem *_item )
{
    KFileIVI* item = static_cast<KFileIVI *>( _item );
    // Reset icon of previous item
    if( d->pActiveItem != 0L && d->pActiveItem != item )
    {
        if ( d->m_movie && d->pActiveItem->isAnimated() )
        {
            d->m_movie->setPaused( true ); // we'll see below what we do with it
            d->pActiveItem->setAnimated( false );
            d->pActiveItem->refreshIcon( true );
        }
        else {
            d->pActiveItem->setActive( false );
        }
        d->pActiveItem = 0L;
        d->pFileTip->setItem( 0L );
    }

    // Stop sound
    if (d->pSoundPlayer != 0 && item != d->pSoundItem)
        stopSound();

    if ( !m_bMousePressed )
    {
        if( item != d->pActiveItem )
        {
            d->pActiveItem = item;
            d->pFileTip->setItem( d->pActiveItem->item(),
                                  item->rect(),
                                  item->pixmap() );

            if ( d->doAnimations && d->pActiveItem && d->pActiveItem->hasAnimation() )
            {
                //kDebug(1203) << "Playing animation for: " << d->pActiveItem->mouseOverAnimation() << endl;
                // Check if cached movie can be used
                if ( d->m_movie && d->movieFileName == d->pActiveItem->mouseOverAnimation() )
                {
                    d->pActiveItem->setAnimated( true );
                    if (d->m_movieBlocked) {
                        kDebug(1203) << "onitem, but blocked" << endl;
                        d->m_movie->start();
                        d->m_movie->setPaused(true);
                    }
                    else {
                        kDebug(1203) << "we go ahead.." << endl;
                        d->m_movieBlocked++;
                        QTimer::singleShot(300, this, SLOT(slotReenableAnimation()));
                        d->m_movie->start();
                    }
                }
                else
                {
                    delete d->m_movie;
                    d->m_movie = KGlobal::iconLoader()->loadMovie( d->pActiveItem->mouseOverAnimation(), K3Icon::Desktop, d->pActiveItem->iconSize() );
#if 0
                    if ( d->m_movie && d->m_movie->isValid() )
                    {
                        // Fix alpha-channel - currently only if no background pixmap,
                        // the bg pixmap case requires to uncomment the code at qmovie.cpp:404
                        const QPixmap* pm = backgroundPixmap();
                        bool hasPixmap = pm && !pm->isNull();
                        if ( !hasPixmap ) {
                            pm = viewport()->backgroundPixmap();
                            hasPixmap = pm && !pm->isNull();
                        }
                        if (!hasPixmap && backgroundMode() != Qt::NoBackground)
                           d->m_movie->setBackgroundColor( viewport()->backgroundColor() );
                        connect(d->m_movie, SIGNAL(updated(const QRect&)), this, SLOT(slotMovieUpdate(const QRect&)) );
                        connect(d->m_movie, SIGNAL(stateChanged(QMovie::MovieState)), this, SLOT(slotMovieStatus(QMovie::MovieState)) );
                        d->movieFileName = d->pActiveItem->mouseOverAnimation();
                        d->pActiveItem->setAnimated( true );
                    }
                    else
                    {
                        d->pActiveItem->setAnimated( false );
                        delete d->m_movie;
                        d->m_movie = 0;
                        // No movie available, remember it
                        d->pActiveItem->setMouseOverAnimation( QString() );
                    }
#endif
                }
            } // animations
            // Only do the normal "mouseover" effect if no animation is in use
            if (d->pActiveItem && !d->pActiveItem->isAnimated())
            {
                d->pActiveItem->setActive( true );
            }
        }
        else // No change in current item
        {
            // No effect. If we want to underline on hover, we should
            // force the IVI to repaint here, though!
            d->pActiveItem = 0L;
            d->pFileTip->setItem( 0L );
        }
    } // bMousePressed
    else
    {
        // All features disabled during mouse clicking, e.g. rectangular
        // selection
        d->pActiveItem = 0L;
        d->pFileTip->setItem( 0L );
    }

    // ## shouldn't this be disabled during rectangular selection too ?
    if (d->bSoundPreviews && d->pSoundPlayer &&
        d->pSoundPlayer->mimeTypes().contains(
            item->item()->mimetype())
        && KGlobalSettings::showFilePreview(item->item()->url())
        && topLevelWidget() == kapp->activeWindow())
    {
        d->pSoundItem = item;
        d->bSoundItemClicked = false;
        if (!d->pSoundTimer)
        {
            d->pSoundTimer = new QTimer(this);
            d->pSoundTimer->setSingleShot( true );
            d->pSoundTimer->setInterval( 500 );
            connect(d->pSoundTimer, SIGNAL(timeout()), SLOT(slotStartSoundPreview()));
        }
        d->pSoundTimer->start();
    }
    else
        stopSound();
}

void KonqIconViewWidget::slotOnViewport()
{
    d->pFileTip->setItem( 0L );

    stopSound();

    if (d->pActiveItem == 0L)
        return;

    if ( d->doAnimations && d->m_movie && d->pActiveItem->isAnimated() )
    {
        d->pActiveItem->setAnimated( false );
        // Aborting before the end of the animation ?
        if (d->m_movie->state() == QMovie::Running) {
            d->m_movie->setPaused( true );
            d->m_movieBlocked++;
            kDebug(1203) << "on viewport, blocking" << endl;
            QTimer::singleShot(300, this, SLOT(slotReenableAnimation()));
        }
        d->pActiveItem->refreshIcon( true );
        Q_ASSERT( d->pActiveItem->state() == K3Icon::DefaultState );
        //delete d->m_movie;
        //d->m_movie = 0L;
        // TODO a timer to delete the movie after some time if unused?
    }
    else
    {
        d->pActiveItem->setActive( false );
    }
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::slotStartSoundPreview()
{
  if (!d->pSoundItem || d->bSoundItemClicked)
    return;

  d->pSoundPlayer->play(d->pSoundItem->item()->url().url());
}


void KonqIconViewWidget::slotPreview(const KFileItem *item, const QPixmap &pix)
{
    // ### slow. Idea: move KonqKfmIconView's m_itemDict into this class
    for (Q3IconViewItem *it = firstItem(); it; it = it->nextItem())
    {
        KFileIVI* current = static_cast<KFileIVI *>(it);
        if (current->item() == item)
        {
            if (item->overlays() & K3Icon::HiddenOverlay) {
                QPixmap p(pix);

                KIconEffect::semiTransparent(p);
                current->setThumbnailPixmap(p);
            } else {
                current->setThumbnailPixmap(pix);
            }
            break;
        }
    }
}

void KonqIconViewWidget::slotPreviewResult()
{
    d->pPreviewJob = 0;
    emit imagePreviewFinished();
}


void KonqIconViewWidget::slotMovieUpdate( const QRect& rect )
{
    //kDebug(1203) << "KonqIconViewWidget::slotMovieUpdate " << rect.x() << "," << rect.y() << " " << rect.width() << "x" << rect.height() << endl;
    Q_ASSERT( d );
    Q_ASSERT( d->m_movie );
    // seems stopAnimation triggers one last update
    if ( d->pActiveItem && d->m_movie && d->pActiveItem->isAnimated() ) {
        const QPixmap &frame = d->m_movie->currentPixmap();
        // This can happen if the icon was scaled to the desired size, so KIconLoader
        // will happily return a movie with different dimensions than the icon
        int iconSize=d->pActiveItem->iconSize();
        if (iconSize==0) iconSize = KGlobal::iconLoader()->currentSize( K3Icon::Desktop );
        if ( frame.width() != iconSize || frame.height() != iconSize ) {
            d->pActiveItem->setAnimated( false );
            d->m_movie->setPaused(true);
            // No movie available, remember it
            d->pActiveItem->setMouseOverAnimation( QString() );
            d->pActiveItem->setActive( true );
            return;
        }
        d->pActiveItem->setPixmapDirect( frame, false, false /*no redraw*/ );
        QRect pixRect = d->pActiveItem->pixmapRect(false);
        repaintContents( pixRect.x() + rect.x(), pixRect.y() + rect.y(), rect.width(), rect.height(), false );
    }
}

void KonqIconViewWidget::slotMovieStatus( int status )
{
    if ( status < 0 ) {
        // Error playing the MNG -> forget about it and do normal iconeffect
        if ( d->pActiveItem && d->pActiveItem->isAnimated() ) {
            d->pActiveItem->setAnimated( false );
            d->pActiveItem->setMouseOverAnimation( QString() );
            d->pActiveItem->setActive( true );
        }
    }
}

void KonqIconViewWidget::slotReenableAnimation()
{
    if (!--d->m_movieBlocked) {
        if ( d->pActiveItem && d->m_movie && d->m_movie->state() == QMovie::Paused ) {
            kDebug(1203) << "reenabled animation" << endl;
            d->m_movie->setPaused(false);
        }
    }
}

void KonqIconViewWidget::clear()
{
    d->pFileTip->setItem( 0L );
    stopImagePreview(); // Just in case
    K3IconView::clear();
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::takeItem( Q3IconViewItem *item )
{
    if ( d->pActiveItem == static_cast<KFileIVI *>(item) )
    {
        d->pFileTip->setItem( 0L );
        d->pActiveItem = 0L;
    }

    if ( d->pPreviewJob )
      d->pPreviewJob->removeItem( static_cast<KFileIVI *>(item)->item() );

    K3IconView::takeItem( item );
}

// Currently unused - remove in KDE 4.0
void KonqIconViewWidget::setThumbnailPixmap( KFileIVI * item, const QPixmap & pixmap )
{
    if ( item )
    {
        if ( d->pActiveItem == item )
        {
            d->pFileTip->setItem( 0L );
            d->pActiveItem = 0L;
        }
        item->setThumbnailPixmap( pixmap );
        if ( m_bSetGridX &&  item->width() > gridX() )
        {
          setGridX( item->width() );
          if (autoArrange())
            arrangeItemsInGrid();
        }
    }
}

bool KonqIconViewWidget::initConfig( bool bInit )
{
    bool fontChanged = false;

    // Color settings
    QColor normalTextColor       = m_pSettings->normalTextColor();
    setItemColor( normalTextColor );

    if (m_bDesktop)
    {
      QColor itemTextBg = m_pSettings->itemTextBackground();
      if ( itemTextBg.isValid() )
          setItemTextBackground( itemTextBg );
      else
          setItemTextBackground( Qt::NoBrush );
    }

    bool on = m_pSettings->showFileTips();
    d->pFileTip->setOptions(on,
                            m_pSettings->showPreviewsInFileTips(),
                            m_pSettings->numFileTips());

    // if the user wants our own tooltip, don't show the one from Qts ListView
    setShowToolTips(!on);

    // Font settings
    QFont font( m_pSettings->standardFont() );
    if (!m_bDesktop)
        font.setUnderline( m_pSettings->underlineLink() );

    if ( font != KonqIconViewWidget::font() )
    {
        setFont( font );
        if (!bInit)
        {
            // QIconView doesn't do it by default... but if the font is made much
            // bigger, we really need to give more space between the icons
            fontChanged = true;
        }
    }

    setIconTextHeight( m_pSettings->iconTextHeight() );

    if ( (itemTextPos() == Q3IconView::Right) && (maxItemWidth() != gridXValue()) )
    {
        int size = m_size;
        m_size = -1; // little trick to force grid change in setIcons
        setIcons( size ); // force re-determining all icons
    }
    else if ( d->bBoostPreview != boostPreview() ) // Update icons if settings for preview icon size have changed
        setIcons(m_size);
    else if (!bInit)
        updateContents();
    return fontChanged;
}

bool KonqIconViewWidget::boostPreview() const
{
    if ( m_bDesktop ) return false;

    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    return group.readEntry( "BoostSize", QVariant(false )).toBool();
}

void KonqIconViewWidget::disableSoundPreviews()
{
    d->bSoundPreviews = false;
    stopSound();
}

void KonqIconViewWidget::setIcons( int size, const QStringList& stopImagePreviewFor )
{
    // size has changed?
    bool sizeChanged = (m_size != size);
    int oldGridX = gridX();
    m_size = size;

    // boost preview option has changed?
    bool boost = boostPreview();
    bool previewSizeChanged = ( d->bBoostPreview != boost );
    d->bBoostPreview = boost;

    if ( sizeChanged || previewSizeChanged )
    {
        int realSize = size ? size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );
        // choose spacing depending on font, but min 5 (due to KFileIVI  move limit)
        setSpacing( ( m_bDesktop || ( realSize > K3Icon::SizeSmall ) ) ?
                    qMax( 5, QFontMetrics(font()).width('n') ) : 0 );
    }

    if ( sizeChanged || previewSizeChanged || !stopImagePreviewFor.isEmpty() )
    {
        calculateGridX();
    }
    bool stopAll = !stopImagePreviewFor.isEmpty() && stopImagePreviewFor.first() == "*";

    // Disable repaints that can be triggered by ivi->setIcon(). Since icons are
    // resized in-place, if the icon size is increasing it can happens that the right
    // or bottom icons exceed the size of the viewport.. here we prevent the repaint
    // event that will be triggered in that case.
    bool prevUpdatesState = viewport()->updatesEnabled();
    viewport()->setUpdatesEnabled( false );

    // Do this even if size didn't change, since this is used by refreshMimeTypes...
    for ( Q3IconViewItem *it = firstItem(); it; it = it->nextItem() ) {
        KFileIVI * ivi = static_cast<KFileIVI *>( it );
        // Set a normal icon for files that are not thumbnails, and for files
        // that are thumbnails but for which it should be stopped
        if ( !ivi->isThumbnail() ||
             sizeChanged ||
             previewSizeChanged ||
             stopAll ||
             mimeTypeMatch( ivi->item()->mimetype(), stopImagePreviewFor ) )
        {
            ivi->setIcon( size, ivi->state(), true, false );
        }
        else
            ivi->invalidateThumb( ivi->state(), true );
    }

    // Restore viewport update to previous state
    viewport()->setUpdatesEnabled( prevUpdatesState );

    if ( ( sizeChanged || previewSizeChanged || oldGridX != gridX() ||
         !stopImagePreviewFor.isEmpty() ) && autoArrange() )
        arrangeItemsInGrid( true ); // take new grid into account and repaint
    else
        viewport()->update(); //Repaint later..
}

bool KonqIconViewWidget::mimeTypeMatch( const QString& mimeType, const QStringList& mimeList ) const
{
    for (QStringList::ConstIterator mt = mimeList.begin(); mt != mimeList.end(); ++mt)
    {
        if ( mimeType == *mt )
            return true;
        // Support for *mt == "image/*"
        QString tmp( mimeType );
        if ( (*mt).endsWith("*") && tmp.replace(QRegExp("/.*"), "/*") == (*mt) )
            return true;
    }
    return false;
}

void KonqIconViewWidget::setItemTextPos( ItemTextPos pos )
{
    // can't call gridXValue() because this already would need the new itemTextPos()
    int sz = m_size ? m_size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );

    if ( m_bSetGridX )
        if ( pos == Q3IconView::Bottom )
            setGridX( qMax( sz + 50, previewIconSize( sz ) + 13 ) );
        else
        {
            setMaxItemWidth( qMax( sz, previewIconSize( sz ) ) + m_pSettings->iconTextWidth() );
            setGridX( -1 );
        }

    K3IconView::setItemTextPos( pos );
}

void KonqIconViewWidget::gridValues( int* x, int* y, int* dx, int* dy,
                                     int* nx, int* ny )
{
    int previewSize = previewIconSize( m_size );
    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );

    // Grid size
    // as KFileIVI limits to move an icon to x >= 5, y >= 5, we define a grid cell as:
    // spacing() must be >= 5 (currently set to 5 in setIcons())
    // horizontal: left spacing() + <width>
    // vertical  : top spacing(), <height>, bottom spacing()
    // The doubled space in y-direction gives a better visual separation and makes it clearer
    // to which item the text belongs
    *dx = spacing() + qMax( qMax( iconSize, previewSize ), m_pSettings->iconTextWidth() );
    int textHeight = iconTextHeight() * fontMetrics().height();
    *dy = spacing() + qMax( iconSize, previewSize ) + 2 + textHeight + spacing();

    // Icon Area
    int w, h;
    if ( m_IconRect.isValid() ) {  // w and h must be != 0, otherwise we would get a div by zero
        *x = m_IconRect.left(); w = m_IconRect.width();
        *y = m_IconRect.top();  h = m_IconRect.height();
    }
    else {
        *x = 0; w = viewport()->width();
        *y = 0; h = viewport()->height();
    }

    // bug:110775 avoid div by zero (happens e.g. when iconTextHeight or iconTextWidth are very large)
    if ( *dx > w )
        *dx = w;

    if ( *dy > h )
        *dy = h;

    *nx = w / *dx;
    *ny = h / *dy;
    // TODO: Check that items->count() <= nx * ny

    // Let have exactly nx columns and ny rows
    if(*nx && *ny) {
      *dx = w / *nx;
      *dy = h / *ny;
    }
    kDebug(1203) << "x=" << *x << " y=" << *y << " spacing=" << spacing() << " iconSize=" << iconSize
                  << " w=" << w << " h=" << h
                  << " nx=" << *nx << " ny=" << *ny
                  << " dx=" << *dx << " dy=" << *dy << endl;
}

void KonqIconViewWidget::calculateGridX()
{
    if ( m_bSetGridX )
        if ( itemTextPos() == Q3IconView::Bottom )
            setGridX( gridXValue() );
        else
        {
            setMaxItemWidth( gridXValue() );
            setGridX( -1 );
        }
}

int KonqIconViewWidget::gridXValue() const
{
    // this method is only used in konqi as filemanager (not desktop)
    int sz = m_size ? m_size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );
    int newGridX;

    if ( itemTextPos() == Q3IconView::Bottom )
        newGridX = qMax( sz + 50, previewIconSize( sz ) + 13 );
    else
        newGridX = qMax( sz, previewIconSize( sz ) ) + m_pSettings->iconTextWidth();

    //kDebug(1203) << "gridXValue: " << newGridX << " sz=" << sz << endl;
    return newGridX;
}

void KonqIconViewWidget::refreshMimeTypes()
{
    updatePreviewMimeTypes();
    for ( Q3IconViewItem *it = firstItem(); it; it = it->nextItem() )
        (static_cast<KFileIVI *>( it ))->item()->refreshMimeType();
    setIcons( m_size );
}

void KonqIconViewWidget::setURL( const KUrl &kurl )
{
    stopImagePreview();
    m_url = kurl;

    d->pFileTip->setPreview( KGlobalSettings::showFilePreview(m_url) );

    if ( m_url.isLocalFile() )
        m_dotDirectoryPath = m_url.path( KUrl::AddTrailingSlash ).append( ".directory" );
    else
        m_dotDirectoryPath.clear();
}

void KonqIconViewWidget::startImagePreview( const QStringList &, bool force )
{
    stopImagePreview(); // just in case

    // Check config
    if ( !KGlobalSettings::showFilePreview( url() ) ) {
        kDebug(1203) << "Previews disabled for protocol " << url().protocol() << endl;
        emit imagePreviewFinished();
        return;
    }

    if ((d->bSoundPreviews = d->previewSettings.contains( "audio/" )) &&
        !d->pSoundPlayer)
    {
      KLibFactory *factory = KLibLoader::self()->factory("konq_sound");
      if (factory)
      {
          d->pSoundPlayer = static_cast<KonqSoundPlayer *>(
            factory->create(this, "KonqSoundPlayer"));
      }
      d->bSoundPreviews = (d->pSoundPlayer != 0L);
    }

    KFileItemList items;
    for ( Q3IconViewItem *it = firstItem(); it; it = it->nextItem() )
        if ( force || !static_cast<KFileIVI *>( it )->hasValidThumbnail() )
            items.append( static_cast<KFileIVI *>( it )->item() );

    bool onlyAudio = true;
    for ( QStringList::ConstIterator it = d->previewSettings.begin(); it != d->previewSettings.end(); ++it ) {
        if ( (*it).startsWith( "audio/" ) )
            d->bSoundPreviews = true;
        else
            onlyAudio = false;
    }

    if ( items.isEmpty() || onlyAudio ) {
        emit imagePreviewFinished();
        return; // don't start the preview job if not really necessary
    }

    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );
    int size;

    d->bBoostPreview = boostPreview();
    size = previewIconSize( iconSize );

    if ( !d->bBoostPreview )
         iconSize /= 2;

    d->pPreviewJob = KIO::filePreview( items, size, size, iconSize,
        m_pSettings->textPreviewIconTransparency(), true /* scale */,
        true /* save */, &(d->previewSettings) );
    connect( d->pPreviewJob, SIGNAL( gotPreview( const KFileItem *, const QPixmap & ) ),
             this, SLOT( slotPreview( const KFileItem *, const QPixmap & ) ) );
    connect( d->pPreviewJob, SIGNAL( result( KJob * ) ),
             this, SLOT( slotPreviewResult() ) );
}

void KonqIconViewWidget::stopImagePreview()
{
    if (d->pPreviewJob)
    {
        d->pPreviewJob->doKill();
        d->pPreviewJob = 0;
        // Now that previews are updated in-place, calling
        // arrangeItemsInGrid() here is not needed anymore
    }
}

bool KonqIconViewWidget::isPreviewRunning() const
{
    return d->pPreviewJob;
}

KFileItemList KonqIconViewWidget::selectedFileItems()
{
    KFileItemList lstItems;

    Q3IconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
        if ( it->isSelected() ) {
            KFileItem *fItem = (static_cast<KFileIVI *>(it))->item();
            lstItems.append( fItem );
        }
    return lstItems;
}

void KonqIconViewWidget::slotDropped( QDropEvent *ev, const QList<Q3IconDragItem> & )
{
    // Drop on background
    KUrl dirURL = url();
    if ( m_rootItem ) {
        bool dummy;
        dirURL = m_rootItem->mostLocalURL(dummy);
    }
    KonqOperations::doDrop( m_rootItem /* may be 0L */, dirURL, ev, this );
}

void KonqIconViewWidget::slotAboutToCreate(const QPoint &, const QList<KIO::CopyInfo> &)
{
   // Do nothing :-)
}

void KonqIconViewWidget::drawBackground( QPainter *p, const QRect &r )
{
    drawBackground(p, r, r.topLeft());
}

void KonqIconViewWidget::drawBackground( QPainter *p, const QRect &r , const QPoint &pt)
{
    Q_UNUSED(pt)

    Q3IconView::drawBackground(p, r);
    return;
#if 0
    const QPixmap *pm  = backgroundPixmap();
    bool hasPixmap = pm && !pm->isNull();
    if ( !hasPixmap ) {
        pm = viewport()->backgroundPixmap();
        hasPixmap = pm && !pm->isNull();
    }

    QRect rtgt(r);
    rtgt.moveTopLeft(pt);
    if (!hasPixmap && backgroundMode() != Qt::NoBackground) {
        p->fillRect(rtgt, viewport()->backgroundColor());
        return;
    }

    if (hasPixmap) {
        int ax = (r.x() + contentsX() + leftMargin()) % pm->width();
        int ay = (r.y() + contentsY() + topMargin()) % pm->height();
        p->drawTiledPixmap(rtgt, *pm, QPoint(ax, ay));
    }
#endif
}

Q3DragObject * KonqIconViewWidget::dragObject()
{
#ifdef __GNUC__
#warning make this nicer once now Q3 view is used
#endif
    if ( !currentItem() )
        return 0;

    //return konqDragObject( viewport() );
    QMimeData *data=konqMimeData(false);
    QDrag *drag=new QDrag(viewport());
    drag->setMimeData(data);
    if (data->hasFormat("application/x-kde-primaryIcon")) {
       QPixmap pixmap;
       if (pixmap.loadFromData(data->data("application/x-kde-primaryIcon"),"PNG"))
           drag->setPixmap(pixmap);
       QPoint hotspot(QString(data->data("application/x-kde-dragX")).toInt(),QString(data->data("application/x-kde-dragY")).toInt());
       drag->setHotSpot	(hotspot);
    }
    drag->start();
return 0;
}

QMimeData * KonqIconViewWidget::konqMimeData(bool moveSelection)
{
	KUrl::List urls;
	KUrl::List mostLocalUrls;
        Q3IconViewItem *primaryItem = currentItem();
	for ( Q3IconViewItem *it = firstItem(); it; it = it->nextItem() ) {
		if (it->isSelected()) {
	        	if (!primaryItem)
       				primaryItem = it;
			KFileItem* fileItem = static_cast<KFileIVI *>(it)->item();
			urls.append(fileItem->url());
			bool dummy;
			mostLocalUrls.append(fileItem->mostLocalURL(dummy));
#ifdef __GNUC__
#warning how much of the q3icondragitem stuff do we have to duplicate here.... (TODO: jowenn)
#endif
		}
	}
	QMimeData *data=new QMimeData();
	KonqMimeData::populateMimeData(data,urls,mostLocalUrls,moveSelection);
#ifdef __GNUC__
#warning perhaps move part of following code into populateMimeData;
#endif
        if (primaryItem) {
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		primaryItem->pixmap()->save(&buffer, "PNG");
		data->setData("application/x-kde-primaryIcon",bytes);
		QPoint dragPoint=m_mousePos - primaryItem->pixmapRect(false).topLeft();
		data->setData("application/x-kde-dragX",QString("%1").arg(dragPoint.x()).toAscii());
		data->setData("application/x-kde-dragY",QString("%2").arg(dragPoint.y()).toAscii());
	}
	return data;
}

void KonqIconViewWidget::contentsDragEnterEvent( QDragEnterEvent *e )
{
    // Cache the URLs, since we need them every time we move over a file
    // (see KFileIVI)
    m_lstDragURLs = KUrl::List::fromMimeData( e->mimeData() );

    if ( !m_lstDragURLs.isEmpty() && m_lstDragURLs.first().protocol() == "programs" )
    {
        e->ignore();
        emit dragEntered( false );
        d->bProgramsURLdrag = true;
        return;
    }

    K3IconView::contentsDragEnterEvent( e );
    emit dragEntered( true /*accepted*/ );
}

void KonqIconViewWidget::contentsDragMoveEvent( QDragMoveEvent *e )
{
    if ( d->bProgramsURLdrag ) {
        emit dragMove( false );
        e->ignore();
        cancelPendingHeldSignal();
        return;
    }

    Q3IconViewItem *item = findItem( e->pos() );
    if ( e->source() != viewport() &&
         !item && m_rootItem && !m_rootItem->isWritable() ) {
        emit dragMove( false );
        e->ignore();
        cancelPendingHeldSignal();
        return;
    }
    emit dragMove( true );
    K3IconView::contentsDragMoveEvent( e );
}

void KonqIconViewWidget::contentsDragLeaveEvent( QDragLeaveEvent *e )
{
    d->bProgramsURLdrag = false;
    K3IconView::contentsDragLeaveEvent(e);
    emit dragLeft();
}

void KonqIconViewWidget::setItemColor( const QColor &c )
{
    iColor = c;
}

QColor KonqIconViewWidget::itemColor() const
{
    return iColor;
}

void KonqIconViewWidget::disableIcons( const KUrl::List & lst )
{
  for ( Q3IconViewItem *kit = firstItem(); kit; kit = kit->nextItem() )
  {
      bool bFound = false;
      // Wow. This is ugly. Matching two lists together....
      // Some sorting to optimise this would be a good idea ?
      for (KUrl::List::ConstIterator it = lst.begin(); !bFound && it != lst.end(); ++it)
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
    // This code is very related to ListViewBrowserExtension::updateActions
    int canCopy = 0;
    int canDel = 0;
    int canTrash = 0;
    bool bInTrash = false;
    int iCount = 0;

    for ( Q3IconViewItem *it = firstItem(); it; it = it->nextItem() )
    {
        if ( it->isSelected() )
        {
            iCount++;
            canCopy++;

            KFileItem *item = ( static_cast<KFileIVI *>( it ) )->item();
            KUrl url = item->url();
            QString local_path = item->localPath();

            /*if ( url.directory(KUrl::AppendTrailingSlash) == KGlobalSettings::trashPath() )
                bInTrash = true;*/
            if ( KProtocolManager::supportsDeleting( url ) )
                canDel++;
            if ( !local_path.isEmpty() )
                canTrash++;
        }
    }

    emit enableAction( "cut", canDel > 0 );
    emit enableAction( "copy", canCopy > 0 );
    emit enableAction( "trash", canDel > 0 && !bInTrash && canTrash==canDel );
    emit enableAction( "del", canDel > 0 );
    emit enableAction( "properties", iCount > 0 && KPropertiesDialog::canDisplay( selectedFileItems() ) );
    emit enableAction( "editMimeType", ( iCount == 1 ) );
    emit enableAction( "rename", ( iCount == 1) && !bInTrash );
}

void KonqIconViewWidget::renameCurrentItem()
{
    if ( currentItem() )
        currentItem()->rename();
}

void KonqIconViewWidget::renameSelectedItem()
{
    kDebug(1203) << " -- KonqIconViewWidget::renameSelectedItem() -- " << endl;
    Q3IconViewItem * item = 0L;
    Q3IconViewItem *it = firstItem();
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
    kDebug(1203) << " -- KonqIconViewWidget::cutSelection() -- " << endl;
//    KonqIconDrag * obj = konqDragObject( /* no parent ! */ );
//    obj->setMoveSelection( true );
    QMimeData *data=konqMimeData(true);
//    QApplication::clipboard()->setData( obj );
    QApplication::clipboard()->setMimeData(data);
}

void KonqIconViewWidget::copySelection()
{
    kDebug(1203) << " -- KonqIconViewWidget::copySelection() -- " << endl;
//    KonqIconDrag * obj = konqDragObject( /* no parent ! */ );
    QMimeData *data=konqMimeData(false);
//    QApplication::clipboard()->setData( obj );
    QApplication::clipboard()->setMimeData(data);
}

void KonqIconViewWidget::pasteSelection()
{
    paste( url() );
}

void KonqIconViewWidget::paste( const KUrl &url )
{
    KonqOperations::doPaste( this, url );
}

KUrl::List KonqIconViewWidget::selectedUrls( UrlFlags flags ) const
{
    KUrl::List lstURLs;

    bool dummy;
    for ( Q3IconViewItem *it = firstItem(); it; it = it->nextItem() )
        if ( it->isSelected() ) {
            KFileItem* item = (static_cast<KFileIVI *>( it ))->item();
            lstURLs.append( flags == MostLocalUrls ? item->mostLocalURL( dummy ) : item->url() );
        }
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

void KonqIconViewWidget::stopSound()
{
    if (d->pSoundPlayer)
        d->pSoundPlayer->stop();
    d->pSoundItem = 0;
    if (d->pSoundTimer && d->pSoundTimer->isActive())
        d->pSoundTimer->stop();
}

void KonqIconViewWidget::contentsMouseMoveEvent( QMouseEvent *e )
{
    if ( (d->pSoundPlayer && d->pSoundPlayer->isPlaying()) || (d->pSoundTimer && d->pSoundTimer->isActive()))
    {
        // The following call is SO expensive (the ::widgetAt call eats up to 80%
        // of the mouse move cpucycles!), so it's mandatory to place that function
        // under strict checks, such as d->pSoundPlayer->isPlaying()
        if ( QApplication::topLevelAt( QCursor::pos() ) != topLevelWidget() )
            stopSound();
    }
    d->renameItem= false;
    K3IconView::contentsMouseMoveEvent( e );
}

void KonqIconViewWidget::contentsDropEvent( QDropEvent * ev )
{
  Q3IconViewItem *i = findItem( ev->pos() );

    if ( ev->source() != viewport() &&
         !i && m_rootItem && !m_rootItem->isWritable() ) {
        ev->setAccepted( false );
        return;
    }

  // Short-circuit QIconView if Ctrl is pressed, so that it's possible
  // to drop a file into its own parent widget to copy it.
  if ( !i && (ev->dropAction() == Qt::CopyAction || ev->dropAction() == Qt::LinkAction)
          && ev->source() && ev->source() == viewport())
  {
    // First we need to call QIconView though, to clear the drag shape
    bool bMovable = itemsMovable();
    setItemsMovable(false); // hack ? call it what you want :-)
    K3IconView::contentsDropEvent( ev );
    setItemsMovable(bMovable);

    QList<Q3IconDragItem> lst;
    slotDropped(ev, lst);
  }
  else
  {
    K3IconView::contentsDropEvent( ev );
    emit dropped(); // What is this for ? (David)      KDE4: remove
  }
  // Don't do this here, it's too early !
  // slotSaveIconPositions();
  // If we want to save after the new file gets listed, though,
  // we could reimplement contentsDropEvent in KDIconView and set m_bNeedSave. Bah.

  // This signal is sent last because we need to ensure it is
  // taken in account when all the slots triggered by the dropped() signal
  // are executed. This way we know that the Drag and Drop is truly finished
  emit dragFinished();
}

void KonqIconViewWidget::doubleClickTimeout()
{
    d->renameItem= true;
    mousePressChangeValue();
    if ( d->releaseMouseEvent )
    {
        QMouseEvent e( QEvent::MouseButtonPress, d->mousePos, Qt::LeftButton, d->mouseButtons, d->mouseModifiers);
        Q3IconViewItem* item = findItem( e.pos() );
        KUrl url;
        if ( item )
        {
            url= ( static_cast<KFileIVI *>( item ) )->item()->url();
            bool brenameTrash =false;
            /*if ( url.isLocalFile() && (url.directory(KUrl::AppendTrailingSlash) == KGlobalSettings::trashPath() || url.path(1).startsWith(KGlobalSettings::trashPath())))
                brenameTrash = true;
            */
            if ( url.isLocalFile() && !brenameTrash && d->renameItem && m_pSettings->renameIconDirectly() && e.button() == Qt::LeftButton && item->textRect( false ).contains(e.pos()))
            {
                if( d->pActivateDoubleClick->isActive () )
                    d->pActivateDoubleClick->stop();
                item->rename();
                m_bMousePressed = false;
            }
        }
    }
    else
    {
        QMouseEvent e( QEvent::MouseMove, d->mousePos, Qt::LeftButton, d->mouseButtons, d->mouseModifiers);
        K3IconView::contentsMousePressEvent( &e );
    }
    if( d->pActivateDoubleClick->isActive() )
        d->pActivateDoubleClick->stop();

    d->releaseMouseEvent = false;
    d->renameItem= false;
}

void KonqIconViewWidget::wheelEvent(QWheelEvent* e)
{
    // when scrolling with mousewheel, stop possible pending filetip
    d->pFileTip->setItem( 0 );

    if (e->modifiers() == Qt::ControlModifier)
    {
        if (e->delta() >= 0)
        {
            emit incIconSize();
        }
        else
        {
            emit decIconSize();
        }
        e->accept();
        return;
    }

    K3IconView::wheelEvent(e);
}

void KonqIconViewWidget::leaveEvent( QEvent *e )
{
    // when leaving the widget, stop possible pending filetip
    d->pFileTip->setItem( 0 );

    K3IconView::leaveEvent(e);
}

void KonqIconViewWidget::mousePressChangeValue()
{
  //kDebug(1203) << "KonqIconViewWidget::contentsMousePressEvent" << endl;
  m_bMousePressed = true;
  if (d->pSoundPlayer)
    d->pSoundPlayer->stop();
  d->bSoundItemClicked = true;
  d->firstClick = false;

  // Once we click on the item, we don't want a tooltip
  // Fixes part of #86968
  d->pFileTip->setItem( 0 );
}

void KonqIconViewWidget::contentsMousePressEvent( QMouseEvent *e )
{
    if(d->pActivateDoubleClick && d->pActivateDoubleClick->isActive ())
        d->pActivateDoubleClick->stop();
     Q3IconViewItem* item = findItem( e->pos() );
     m_mousePos = e->pos();
     KUrl url;
     if ( item )
     {
         url = ( static_cast<KFileIVI *>( item ) )->item()->url();
         bool brenameTrash =false;
         /*if ( url.isLocalFile() && (url.directory(KUrl::AppendTrailingSlash) == KGlobalSettings::trashPath() || url.path(1).startsWith(KGlobalSettings::trashPath())))
             brenameTrash = true;*/
         if ( !brenameTrash && !KGlobalSettings::singleClick() && m_pSettings->renameIconDirectly() && e->button() == Qt::LeftButton && item->textRect( false ).contains(e->pos())&& !d->firstClick &&  url.isLocalFile() && (!url.protocol().indexOf( "device", 0, Qt::CaseInsensitive )==0))
         {
             d->firstClick = true;
             d->mousePos = e->pos();
             d->mouseModifiers = e->modifiers();
             d->mouseButtons = e->buttons();
             if (!d->pActivateDoubleClick)
             {
                 d->pActivateDoubleClick = new QTimer(this);
                 connect(d->pActivateDoubleClick, SIGNAL(timeout()), this, SLOT(doubleClickTimeout()));
             }
             if( d->pActivateDoubleClick->isActive () )
                 d->pActivateDoubleClick->stop();
             else
                 d->pActivateDoubleClick->start(QApplication::doubleClickInterval());
             d->releaseMouseEvent = false;
             return;
         }
         else
             d->renameItem= false;
     }
     else
         d->renameItem= false;
    mousePressChangeValue();
    if(d->pActivateDoubleClick && d->pActivateDoubleClick->isActive())
        d->pActivateDoubleClick->stop();
    K3IconView::contentsMousePressEvent( e );

}

void KonqIconViewWidget::contentsMouseReleaseEvent( QMouseEvent *e )
{
    K3IconView::contentsMouseReleaseEvent( e );
    if(d->releaseMouseEvent && d->pActivateDoubleClick && d->pActivateDoubleClick->isActive ())
        d->pActivateDoubleClick->stop();
    slotSelectionChanged();
    d->releaseMouseEvent = true;
    m_bMousePressed = false;
}

void KonqIconViewWidget::slotSaveIconPositions()
{
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  // This code is currently not used but left in for compatibility reasons.
  // It can be removed in KDE 4.0
  // Saving of desktop icon positions is now done in KDIconView::saveIconPositions()
  // in kdebase/kdesktop/kdiconview.cc
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

  if ( m_dotDirectoryPath.isEmpty() )
    return;
  if ( !m_bDesktop )
    return; // Currently not available in Konqueror
  kDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions" << endl;
  KSimpleConfig dotDirectory( m_dotDirectoryPath );
  Q3IconViewItem *it = firstItem();
  if ( !it )
    return; // No more icons. Maybe we're closing and they've been removed already
  while ( it )
  {
    KFileIVI *ivi = static_cast<KFileIVI *>( it );
    KFileItem *item = ivi->item();

    dotDirectory.setGroup( QString( m_iconPositionGroupPrefix ).append( item->url().fileName() ) );
    kDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions " << item->url().fileName() << " " << it->x() << " " << it->y() << endl;
    dotDirectory.writeEntry( QString( "X %1" ).arg( width() ), it->x() );
    dotDirectory.writeEntry( QString( "Y %1" ).arg( height() ), it->y() );
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
      {
        kDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions deleting group " << *gIt << endl;
        dotDirectory.deleteGroup( *gIt );
      }
    }

  dotDirectory.sync();

  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  // This code is currently not used but left in for compatibility reasons.
  // It can be removed in KDE 4.0
  // Saving of desktop icon positions is now done in KDIconView::saveIconPositions()
  // in kdebase/kdesktop/kdiconview.cc
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
}

// Adapted version of QIconView::insertInGrid, that works relative to
// m_IconRect, instead of the entire viewport.

void KonqIconViewWidget::insertInGrid(Q3IconViewItem *item)
{
    if (0L == item)
        return;

    if (!m_IconRect.isValid())
    {
        K3IconView::insertInGrid(item);
        return;
    }

    QRegion r(m_IconRect);
    Q3IconViewItem *i = firstItem();
    int y = -1;
    for (; i; i = i->nextItem() )
    {
        r = r.subtract(i->rect());
        y = qMax(y, i->y() + i->height());
    }

    Q3MemArray<QRect> rects = r.rects();
    Q3MemArray<QRect>::Iterator it = rects.begin();
    bool foundPlace = false;
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

void KonqIconViewWidget::lineupIcons()
{
    // even if there are no items yet, calculate the maxItemWidth to have the correct
    // item rect when we insert new items

    // Create a grid of (ny x nx) bins.
    int x0, y0, dx, dy, nx, ny;
    gridValues( &x0, &y0, &dx, &dy, &nx, &ny );

    int itemWidth = dx - spacing();
    bool newItemWidth = false;
    if ( maxItemWidth() != itemWidth ) {
        newItemWidth = true;
        setMaxItemWidth( itemWidth );
        setFont( font() );  // Force calcRect()
    }

    if ( !firstItem() ) {
        kDebug(1203) << "No icons at all ?\n";
        return;
    }

    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );

    typedef QList<Q3IconViewItem*> Bin;
    Bin*** bins = new Bin**[nx];
    int i;
    int j;
    for ( i = 0; i < nx ; i++ ) {
        bins[i] = new Bin*[ny];
        for ( j = 0; j < ny; j++ )
            bins[i][j] = 0L;
    }

    // Insert items into grid
    int textHeight = iconTextHeight() * fontMetrics().height();

    for ( Q3IconViewItem* item = firstItem(); item; item = item->nextItem() ) {
        int x = item->x() + item->width() / 2 - x0;
        int y = item->pixmapRect( false ).bottom() - iconSize / 2
                - ( dy - ( iconSize + textHeight ) ) / 2 - y0;
        int posX = qMin( nx-1, qMax( 0, x / dx ) );
        int posY = qMin( ny-1, qMax( 0, y / dy ) );

        if ( !bins[posX][posY] )
            bins[posX][posY] = new Bin;
        bins[posX][posY]->prepend( item );
    }

    // The shuffle code
    int n, k;
    const int infinity = 10000;
    int nmoves = 1;
    for ( n = 0; n < 30 && nmoves > 0; n++ ) {
        nmoves = 0;
        for ( i = 0; i < nx; i++ ) {
            for ( j = 0; j < ny; j++ ) {
                if ( !bins[i][j] || ( bins[i][j]->count() <= 1 ) )
                    continue;

                // Calculate the 4 "friction coefficients".
                int tf = 0, bf = 0, lf = 0, rf = 0;
                for ( k = j-1; k >= 0 && bins[i][k] && bins[i][k]->count(); k-- )
                    tf += bins[i][k]->count();
                if ( k == -1 )
                    tf += infinity;

                for ( k = j+1; k < ny && bins[i][k] && bins[i][k]->count(); k++ )
                    bf += bins[i][k]->count();
                if ( k == ny )
                    bf += infinity;

                for ( k = i-1; k >= 0 && bins[k][j] && bins[k][j]->count(); k-- )
                    lf += bins[k][j]->count();
                if ( k == -1 )
                    lf += infinity;

                for ( k = i+1; k < nx && bins[k][j] && bins[k][j]->count(); k++ )
                    rf += bins[k][j]->count();
                if ( k == nx )
                    rf += infinity;

                // If we are stuck between walls, continue
                if ( tf >= infinity && bf >= infinity &&
                     lf >= infinity && rf >= infinity )
                    continue;

                // Is there a preferred lineup direction?
                if ( m_LineupMode == LineupHorizontal ) {
                    tf += infinity;
                    bf += infinity;
                }
                else if ( m_LineupMode == LineupVertical ) {
                    lf += infinity;
                    rf += infinity;
                }

                // Move one item in the direction of the least friction
                Q3IconViewItem* movedItem;
                Bin* items = bins[i][j];

                int mini = qMin( qMin( tf, bf ), qMin( lf, rf ) );
                if ( tf == mini ) {
                    // move top item in (i,j) to (i,j-1)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->y() < movedItem->y() )
                            movedItem = *it;
                    }
                    items->removeAll( movedItem );
                    if ( !bins[i][j-1] )
                        bins[i][j-1] = new Bin;
                    bins[i][j-1]->prepend( movedItem );
                }
                else if ( bf ==mini ) {
                    // move bottom item in (i,j) to (i,j+1)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->y() > movedItem->y() )
                            movedItem = *it;
                    }
                    items->removeAll( movedItem );
                    if ( !bins[i][j+1] )
                        bins[i][j+1] = new Bin;
                    bins[i][j+1]->prepend( movedItem );
                }
                else if ( lf == mini )
                {
                    // move left item in (i,j) to (i-1,j)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->x() < movedItem->x() )
                            movedItem = *it;
                    }
                    items->removeAll( movedItem );
                    if ( !bins[i-1][j] )
                        bins[i-1][j] = new Bin;
                    bins[i-1][j]->prepend( movedItem );
                }
                else {
                    // move right item in (i,j) to (i+1,j)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->x() > movedItem->x() )
                            movedItem = *it;
                    }
                    items->removeAll( movedItem );
                    if ( !bins[i+1][j] )
                        bins[i+1][j] = new Bin;
                    bins[i+1][j]->prepend( movedItem );
                }
                nmoves++;
            }
        }
    }

    // Perform the actual moving
    QRegion repaintRegion;
    QList<Q3IconViewItem*> movedItems;

    for ( i = 0; i < nx; i++ ) {
        for ( j = 0; j < ny; j++ ) {
            Bin* bin = bins[i][j];
            if ( !bin )
                continue;
            if ( !bin->isEmpty() ) {
                Q3IconViewItem* item = bin->first();
                int newX = x0 + i*dx + spacing() +
                           qMax(0, ( (dx-spacing()) - item->width() ) / 2);  // pixmap can be larger as iconsize
                // align all icons vertically to their text
                int newY = y0 + j*dy + dy - spacing() - ( item->pixmapRect().bottom() + 2 + textHeight );
                if ( item->x() != newX || item->y() != newY ) {
                    QRect oldRect = item->rect();
                    movedItems.prepend( item );
                    item->move( newX, newY );
                    if ( item->rect() != oldRect )
                        repaintRegion = repaintRegion.unite( oldRect );
                }
            }
            delete bin;
            bins[i][j] = 0L;
        }
    }

    // repaint
    if ( newItemWidth )
        updateContents();
    else {
        // Repaint only repaintRegion...
        QVector<QRect> rects = repaintRegion.rects();
        for ( uint l = 0; l < rects.count(); l++ ) {
            kDebug( 1203 ) << "Repainting (" << rects[l].x() << ","
                            << rects[l].y() << ")\n";
            repaintContents( rects[l], false );
        }
        // Repaint icons that were moved
        while ( !movedItems.isEmpty() ) {
            repaintItem( movedItems.first() );
            movedItems.removeAll( movedItems.first() );
        }
    }

    for ( i = 0; i < nx ; i++ ) {
            delete [] bins[i];
    }
    delete [] bins;
}

void KonqIconViewWidget::lineupIcons( Q3IconView::Arrangement arrangement )
{
    int x0, y0, dx, dy, nxmax, nymax;
    gridValues( &x0, &y0, &dx, &dy, &nxmax, &nymax );
    int textHeight = iconTextHeight() * fontMetrics().height();

    QRegion repaintRegion;
    QList<Q3IconViewItem*> movedItems;
    int nx = 0, ny = 0;

    Q3IconViewItem* item;
    for ( item = firstItem(); item; item = item->nextItem() ) {
        int newX = x0 + nx*dx + spacing() +
                   qMax(0, ( (dx-spacing()) - item->width() ) / 2);  // icon can be larger as defined
        // align all icons vertically to their text
        int newY = y0 + ny*dy + dy - spacing() - ( item->pixmapRect().bottom() + 2 + textHeight );
        if ( item->x() != newX || item->y() != newY ) {
            QRect oldRect = item->rect();
            movedItems.prepend( item );
            item->move( newX, newY );
            if ( item->rect() != oldRect )
                repaintRegion = repaintRegion.unite( oldRect );
        }
        if ( arrangement == Q3IconView::LeftToRight ) {
            nx++;
            if ( nx >= nxmax ) {
                ny++;
                nx = 0;
            }
        }
        else {
            ny++;
            if ( ny >= nymax ) {
                nx++;
                ny = 0;
            }
        }
    }

    // Repaint only repaintRegion...
    Q3MemArray<QRect> rects = repaintRegion.rects();
    for ( uint l = 0; l < rects.count(); l++ ) {
        kDebug( 1203 ) << "Repainting (" << rects[(int)l].x() << ","
                        << rects[(int)l].y() << ")\n";
        repaintContents( rects[(int)l], false );
    }
    // Repaint icons that were moved
    while ( !movedItems.isEmpty() ) {
        repaintItem( movedItems.first() );
        movedItems.removeAll( movedItems.first() );
    }
}

int KonqIconViewWidget::largestPreviewIconSize( int size ) const
{
    int iconSize = size ? size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );

    if (iconSize < 28)
        return 48;
    if (iconSize < 40)
        return 64;
    if (iconSize < 60)
        return 96;
    if (iconSize < 120)
        return 128;

    return 192;
}

int KonqIconViewWidget::previewIconSize( int size ) const
{
    int iconSize = size ? size : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );

    if (!d->bBoostPreview)
        return iconSize;

    return largestPreviewIconSize( iconSize );
}

void KonqIconViewWidget::visualActivate(Q3IconViewItem * item)
{
    // Rect of the QIconViewItem.
    QRect irect = item->rect();

    // Rect of the QIconViewItem's pixmap area.
    QRect rect = item->pixmapRect();

    // Adjust to correct position. If this isn't done, the fact that the
    // text may be wider than the pixmap puts us off-center.
    rect.translate(irect.x(), irect.y());

    // Adjust for scrolling (David)
    rect.translate( -contentsX(), -contentsY() );

    KIconEffect::visualActivate(viewport(), rect);
}

void KonqIconViewWidget::backgroundPixmapChange( const QPixmap & )
{
    viewport()->update();
}

void KonqIconViewWidget::setPreviewSettings( const QStringList& settings )
{
    d->previewSettings = settings;
    updatePreviewMimeTypes();

    int size = m_size;
    m_size = -1; // little trick to force grid change in setIcons
    setIcons( size ); // force re-determining all icons
}

const QStringList& KonqIconViewWidget::previewSettings()
{
    return d->previewSettings;
}

void KonqIconViewWidget::setNewURL( const QString& url )
{
    KUrl u;
    if ( url.startsWith( "/" ) )
        u.setPath( url );
    else
        u = url;
    setURL( u );
}

void KonqIconViewWidget::setCaseInsensitiveSort( bool b )
{
    d->bCaseInsensitive = b;
}

bool KonqIconViewWidget::caseInsensitiveSort() const
{
    return d->bCaseInsensitive;
}

bool KonqIconViewWidget::canPreview( KFileItem* item )
{
    if ( !KGlobalSettings::showFilePreview( url() ) )
        return false;

    if ( d->pPreviewMimeTypes == 0L )
        updatePreviewMimeTypes();

    return mimeTypeMatch( item->mimetype(), *( d->pPreviewMimeTypes ) );
}

void KonqIconViewWidget::updatePreviewMimeTypes()
{
    if ( d->pPreviewMimeTypes == 0L )
        d->pPreviewMimeTypes = new QStringList;
    else
        d->pPreviewMimeTypes->clear();

    // Load the list of plugins to determine which mimetypes are supported
    KService::List plugins = KServiceTypeTrader::self()->query("ThumbCreator");
    KService::List::ConstIterator it;

    for ( it = plugins.begin(); it != plugins.end(); ++it ) {
        if ( d->previewSettings.contains((*it)->desktopEntryName()) ) {
            QStringList mimeTypes = (*it)->property("MimeTypes").toStringList();
            for (QStringList::ConstIterator mt = mimeTypes.begin(); mt != mimeTypes.end(); ++mt)
                d->pPreviewMimeTypes->append(*mt);
        }
    }
}

#include "konq_iconviewwidget.moc"

/* vim: set et sw=4 ts=8 softtabstop=4: */
