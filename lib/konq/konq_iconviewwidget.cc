/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000, 2001, 2002 David Faure <david@mandrakesoft.com>

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
#include "konq_sound.h"

#include <qclipboard.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <qmovie.h>
#include <qregexp.h>
#include <qcursor.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kio/previewjob.h>
#include <kfileivi.h>
#include <konq_settings.h>
#include <konq_drag.h>
#include <konq_operations.h>
#include <kglobalsettings.h>
#include <kpropertiesdialog.h>
#include <kipc.h>
#include <kicontheme.h>
#include <kiconeffect.h>
#include <kurldrag.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <ktrader.h>

#include <assert.h>
#include <unistd.h>

class KFileTip: public QFrame
{
public:
    KFileTip( KonqIconViewWidget* parent ) : QFrame( 0, 0, WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WStyle_StaysOnTop | WX11BypassWM ),

          m_corner( 0 ),
          m_filter( false ),
          m_view( parent ),
          m_item( 0 ),
          m_previewJob( 0 ),
          m_ivi( 0 )
    {
        m_iconLabel = new QLabel(this);
        m_textLabel = new QLabel(this);
        m_textLabel->setAlignment(Qt::AlignAuto | Qt::AlignTop);

        QGridLayout* layout = new QGridLayout(this, 1, 2, 8, 0);
        layout->addWidget(m_iconLabel, 0, 0);
        layout->addWidget(m_textLabel, 0, 1);
        layout->setResizeMode(QLayout::Fixed);

        setPalette( QToolTip::palette() );
        setMargin( 1 );
        setFrameStyle( QFrame::Plain | QFrame::Box );

        hide();
    }
    ~KFileTip();

    void setPreview(bool on)
    {
        m_preview = on;
        if(on)
            m_iconLabel->show();
        else
            m_iconLabel->hide();
    }

    void setOptions( bool on, bool preview, int num)
    {
        m_num = num;
        setPreview(preview);
        m_on = on;
    }

    void setItem( KFileIVI *ivi );

    virtual bool eventFilter( QObject *, QEvent *e );

    void gotPreview( const KFileItem*, const QPixmap& );
    void gotPreviewResult();

protected:
    virtual void drawContents( QPainter *p );
    virtual void timerEvent( QTimerEvent * );
    virtual void resizeEvent( QResizeEvent * );

private:
    void setFilter( bool enable );

    void reposition();

    QLabel*    m_iconLabel;
    QLabel*    m_textLabel;
    int        m_num;
    bool       m_on;
    bool       m_preview;
    QPixmap    m_corners[4];
    int        m_corner;
    bool       m_filter;
    KonqIconViewWidget*       m_view;
    KFileItem* m_item;
    KIO::PreviewJob* m_previewJob;
    KFileIVI*  m_ivi;
};

KFileTip::~KFileTip()
{
   if ( m_previewJob ) {
        m_previewJob->kill();
        m_previewJob = 0;
    }
}

void KFileTip::setItem( KFileIVI *ivi )
{
    if (!m_on) return;
    if (m_ivi == ivi) return;

    if ( m_previewJob ) {
        m_previewJob->kill();
        m_previewJob = 0;
    }

    m_ivi = ivi;
    m_item = ivi ? ivi->item() : 0;

    QString text = ivi ? ivi->item()->getToolTipText( m_num ) : QString::null;
    if ( !text.isEmpty() ) {
        hide();
        m_textLabel -> setText( text );

        killTimers();
        setFilter( true );

        if (m_preview) {
            m_iconLabel -> setPixmap(*(ivi->pixmap()));
            KFileItemList oneItem;
            oneItem.append( ivi->item() );

            m_previewJob = KIO::filePreview( oneItem, 256, 256, 64, 70, true, true, 0);
            connect( m_previewJob, SIGNAL( gotPreview( const KFileItem *, const QPixmap & ) ),
                    m_view, SLOT( slotToolTipPreview( const KFileItem *, const QPixmap & ) ) );
            connect( m_previewJob, SIGNAL( result( KIO::Job * ) ),
                    m_view, SLOT( slotToolTipPreviewResult() ) );
        }

        startTimer( 700 );
    }
    else {
        killTimers();
        if ( isVisible() ) {
            setFilter( false );
            hide();
        }
    }
}

void KFileTip::reposition()
{
    if (!m_ivi) return;

    QRect rect = m_ivi->rect();
    QPoint off = m_view->mapToGlobal( m_view->contentsToViewport( QPoint( 0, 0 ) ) );
    rect.moveBy( off.x(), off.y() );

    QPoint pos = rect.center();
    // m_corner:
    // 0: upperleft
    // 1: upperright
    // 2: lowerleft
    // 3: lowerright
    // 4+: none
    m_corner = 0;
    // should the tooltip be shown to the left or to the right of the ivi ?
    QRect desk = KGlobalSettings::desktopGeometry(rect.center());
    if (rect.center().x() + width() > desk.right())
    {
        // to the left
        if (pos.x() - width() < 0) {
            pos.setX(0);
            m_corner = 4;
        } else {
            pos.setX( pos.x() - width() );
            m_corner = 1;
        }
    }
    // should the tooltip be shown above or below the ivi ?
    if (rect.bottom() + height() > desk.bottom())
    {
        // above
        pos.setY( rect.top() - height() );
        m_corner += 2;
    }
    else pos.setY( rect.bottom() );

    move( pos );
    update();
}

void KFileTip::gotPreview( const KFileItem* item, const QPixmap& pixmap )
{
    m_previewJob = 0;
    if (item != m_item) return;

    m_iconLabel -> setPixmap(pixmap);
}

void KFileTip::gotPreviewResult()
{
    m_previewJob = 0;
}

void KFileTip::drawContents( QPainter *p )
{
    static const char * const names[] = {
        "arrow_topleft",
        "arrow_topright",
        "arrow_bottomleft",
        "arrow_bottomright"
    };

    if (m_corner >= 4) {  // 4 is empty, so don't draw anything
        QFrame::drawContents( p );
        return;
    }

    if ( m_corners[m_corner].isNull())
        m_corners[m_corner].load( locate( "data", QString::fromLatin1( "konqueror/pics/%1.png" ).arg( names[m_corner] ) ) );

    QPixmap &pix = m_corners[m_corner];

    switch ( m_corner )
    {
        case 0:
            p->drawPixmap( 3, 3, pix );
            break;
        case 1:
            p->drawPixmap( width() - pix.width() - 3, 3, pix );
            break;
        case 2:
            p->drawPixmap( 3, height() - pix.height() - 3, pix );
            break;
        case 3:
            p->drawPixmap( width() - pix.width() - 3, height() - pix.height() - 3, pix );
            break;
    }

    QFrame::drawContents( p );
}

void KFileTip::setFilter( bool enable )
{
    if ( enable == m_filter ) return;

    if ( enable ) {
        kapp->installEventFilter( this );
        QApplication::setGlobalMouseTracking( true );
    }
    else {
        QApplication::setGlobalMouseTracking( false );
        kapp->removeEventFilter( this );
    }
    m_filter = enable;
}

void KFileTip::timerEvent( QTimerEvent * )
{
    killTimers();
    if ( !isVisible() ) {
        startTimer( 15000 );
        reposition();
        show();
    }
    else {
        setFilter( false );
        hide();
    }
}

void KFileTip::resizeEvent( QResizeEvent* event )
{
    QFrame::resizeEvent(event);
    reposition();
}

bool KFileTip::eventFilter( QObject *, QEvent *e )
{
    switch ( e->type() )
    {
        case QEvent::Leave:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Wheel:
            killTimers();
            setFilter( false );
            hide();
        default: break;
    }

    return false;
}

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
	gridXspacing = 50;

        doAnimations = true;
        m_movie = 0L;
        m_movieBlocked = 0;
        pFileTip = 0;
        pActivateDoubleClick = 0L;
        bCaseInsensitive = true;
        pPreviewMimeTypes = 0L;
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
    QPoint desktopGridSpacing;
    int gridXspacing;

    // Animated icons support
    bool doAnimations;
    QMovie* m_movie;
    int m_movieBlocked;
    QString movieFileName;

    KIO::PreviewJob *pPreviewJob;
    KFileTip* pFileTip;
    QStringList previewSettings;
    bool renameItem;
    bool firstClick;
    bool releaseMouseEvent;
    QPoint mousePos;
    int mouseState;
    QTimer *pActivateDoubleClick;
    QStringList* pPreviewMimeTypes;
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

    kapp->addKipcEventMask( KIPC::IconChanged );
    connect( kapp, SIGNAL(iconChanged(int)), SLOT(slotIconChanged(int)) );
    connect( this, SIGNAL(onItem(QIconViewItem *)), SLOT(slotOnItem(QIconViewItem *)) );
    connect( this, SIGNAL(onViewport()), SLOT(slotOnViewport()) );
    connect( this, SIGNAL(itemRenamed(QIconViewItem *, const QString &)), SLOT(slotItemRenamed(QIconViewItem *, const QString &)) );

    if ( m_bDesktop ) {
        KConfigGroup group( KGlobal::config(), "DesktopIcons" );
        QPoint defaultSize;
        d->desktopGridSpacing = group.readPointEntry( "DesktopGridSpacing", &defaultSize );
        if ( d->desktopGridSpacing.isNull() ) {
            // read GridXSpacing (for compatibility with old settings)
            int compat = group.readNumEntry( "GridXSpacing", 0 );
            if ( compat > 0 ) {
                // make a large preview fit into the grid
                int previewSize = largestPreviewIconSize( m_size );
                int size = m_size ? m_size : KGlobal::iconLoader()->currentSize(  KIcon::Desktop );
                d->desktopGridSpacing.setX( compat );
                d->desktopGridSpacing.setY( 7 + QMAX( 0, previewSize - size ) );
            }
            // defaults (medium)
            else
                d->desktopGridSpacing = QPoint( 55, 40 );
        }
    }
    d->bBoostPreview = boostPreview();

    // hardcoded settings
    setSelectionMode( QIconView::Extended );
    setItemTextPos( QIconView::Bottom );
    d->releaseMouseEvent = false;
    d->pFileTip = new KFileTip(this);
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
    m_iconPositionGroupPrefix = QString::fromLatin1( "IconPosition::" );
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
    KIconView::focusOutEvent( ev );
}

void KonqIconViewWidget::slotItemRenamed(QIconViewItem *item, const QString &name)
{
    kdDebug(1203) << "KonqIconViewWidget::slotItemRenamed" << endl;
    KFileIVI *viewItem = static_cast<KFileIVI *>(item);
    KFileItem *fileItem = viewItem->item();

    // The correct behavior is to show the old name until the rename has successfully
    // completed. Unfortunately, KIconView forces us to allow the text to be changed
    // before we try the rename, so set it back to the pre-rename state.
    viewItem->setText( fileItem->text() );
    kdDebug(1203)<<" fileItem->text() ;"<<fileItem->text()<<endl;
    // Don't do anything if the user renamed to a blank name.
    if( !name.isEmpty() )
    {
        // Actually attempt the rename. If it succeeds, KDirLister will update the name.
        KURL oldurl( fileItem->url() );
        KURL newurl( url() );
        newurl.setPath( url().path(1) + KIO::encodeFileName( name ) );
        kdDebug(1203)<<" newurl :"<<newurl.url()<<endl;
        // We use url()+name so that it also works if the name is a relative path (#51176)
        KonqOperations::rename( this, oldurl, newurl );
    }
}

void KonqIconViewWidget::slotIconChanged( int group )
{
    if (group != KIcon::Desktop)
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
    d->doAnimations = cfgGroup.readBoolEntry( "Animated", true /*default*/ );
    d->gridXspacing = cfgGroup.readNumEntry( "GridXSpacing", 50);
}

void KonqIconViewWidget::slotOnItem( QIconViewItem *_item )
{
    KFileIVI* item = static_cast<KFileIVI *>( _item );
    // Reset icon of previous item
    if( d->pActiveItem != 0L && d->pActiveItem != item )
    {
        if ( d->m_movie && d->pActiveItem->isAnimated() )
        {
            d->m_movie->pause(); // we'll see below what we do with it
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
    {
        d->pSoundPlayer->stop();

        d->pSoundItem = 0;
        if (d->pSoundTimer && d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
    }

    if ( !m_bMousePressed )
    {
        if( item != d->pActiveItem )
        {
            d->pActiveItem = item;
            if ( topLevelWidget() == kapp->activeWindow() )
                d->pFileTip->setItem( d->pActiveItem );

            if ( d->doAnimations && d->pActiveItem && d->pActiveItem->hasAnimation() )
            {
                //kdDebug(1203) << "Playing animation for: " << d->pActiveItem->mouseOverAnimation() << endl;
                // Check if cached movie can be used
#if 0 // Qt-mng bug, reusing the movie doesn't work currently.
                if ( d->m_movie && d->movieFileName == d->pActiveItem->mouseOverAnimation() )
                {
                    d->pActiveItem->setAnimated( true );
                    if (d->m_movieBlocked) {
                        kdDebug(1203) << "onitem, but blocked" << endl;
                        d->m_movie->pause();
                    }
                    else {
                        kdDebug(1203) << "we go ahead.." << endl;
                        d->m_movieBlocked++;
                        QTimer::singleShot(300, this, SLOT(slotReenableAnimation()));
                        d->m_movie->restart();
                        d->m_movie->unpause();
                    }
                }
                else
#endif
                {
                    QMovie movie = KGlobal::iconLoader()->loadMovie( d->pActiveItem->mouseOverAnimation(), KIcon::Desktop, d->pActiveItem->iconSize() );
                    if ( !movie.isNull() )
                    {
                        delete d->m_movie;
                        d->m_movie = new QMovie( movie ); // shallow copy, don't worry
                        // Fix alpha-channel - currently only if no background pixmap,
                        // the bg pixmap case requires to uncomment the code at qmovie.cpp:404
                        const QPixmap* pm = backgroundPixmap();
                        bool hasPixmap = pm && !pm->isNull();
                        if ( !hasPixmap ) {
                            pm = viewport()->backgroundPixmap();
                            hasPixmap = pm && !pm->isNull();
                        }
                        if (!hasPixmap && backgroundMode() != NoBackground)
                           d->m_movie->setBackgroundColor( viewport()->backgroundColor() );
                        d->m_movie->connectUpdate( this, SLOT( slotMovieUpdate(const QRect &) ) );
                        d->m_movie->connectStatus( this, SLOT( slotMovieStatus(int) ) );
                        d->movieFileName = d->pActiveItem->mouseOverAnimation();
                        d->pActiveItem->setAnimated( true );
                    }
                    else
                    {
                        d->pActiveItem->setAnimated( false );
                        if (d->m_movie)
                            d->m_movie->pause();
                        // No movie available, remember it
                        d->pActiveItem->setMouseOverAnimation( QString::null );
                    }
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
            connect(d->pSoundTimer, SIGNAL(timeout()), SLOT(slotStartSoundPreview()));
        }
        if (d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
        d->pSoundTimer->start(500, true);
    }
    else
    {
        if (d->pSoundPlayer)
            d->pSoundPlayer->stop();
        d->pSoundItem = 0;
        if (d->pSoundTimer && d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
    }
}

void KonqIconViewWidget::slotOnViewport()
{
    d->pFileTip->setItem( 0L );

    if (d->pSoundPlayer)
      d->pSoundPlayer->stop();
    d->pSoundItem = 0;
    if (d->pSoundTimer && d->pSoundTimer->isActive())
      d->pSoundTimer->stop();

    if (d->pActiveItem == 0L)
        return;

    if ( d->doAnimations && d->m_movie && d->pActiveItem->isAnimated() )
    {
        d->pActiveItem->setAnimated( false );
#if 0
        // Aborting before the end of the animation ?
        if (d->m_movie->running()) {
            d->m_movie->pause();
            d->m_movieBlocked++;
            kdDebug(1203) << "on viewport, blocking" << endl;
            QTimer::singleShot(300, this, SLOT(slotReenableAnimation()));
        }
#endif
        d->pActiveItem->refreshIcon( true );
        Q_ASSERT( d->pActiveItem->state() == KIcon::DefaultState );
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
    for (QIconViewItem *it = firstItem(); it; it = it->nextItem())
    {
        KFileIVI* current = static_cast<KFileIVI *>(it);
        if (current->item() == item)
        {
            if (item->overlays() & KIcon::HiddenOverlay) {
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

void KonqIconViewWidget::slotToolTipPreview(const KFileItem* item, const QPixmap &pix)
{
    if (d->pFileTip) d->pFileTip->gotPreview( item, pix );
}

void KonqIconViewWidget::slotToolTipPreviewResult()
{
    if (d->pFileTip) d->pFileTip->gotPreviewResult();
}

void KonqIconViewWidget::slotMovieUpdate( const QRect& rect )
{
    //kdDebug(1203) << "KonqIconViewWidget::slotMovieUpdate " << rect.x() << "," << rect.y() << " " << rect.width() << "x" << rect.height() << endl;
    Q_ASSERT( d );
    Q_ASSERT( d->m_movie );
    // seems stopAnimation triggers one last update
    if ( d->pActiveItem && d->m_movie && d->pActiveItem->isAnimated() ) {
        const QPixmap &frame = d->m_movie->framePixmap();
        // This can happen if the icon was scaled to the desired size, so KIconLoader
        // will happily return a movie with different dimensions than the icon
        int iconSize=d->pActiveItem->iconSize();
        if (iconSize==0) iconSize = KGlobal::iconLoader()->currentSize( KIcon::Desktop );
        if ( frame.width() != iconSize || frame.height() != iconSize ) {
            d->pActiveItem->setAnimated( false );
            d->m_movie->pause();
            // No movie available, remember it
            d->pActiveItem->setMouseOverAnimation( QString::null );
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
            d->pActiveItem->setMouseOverAnimation( QString::null );
            d->pActiveItem->setActive( true );
        }
    }
}

void KonqIconViewWidget::slotReenableAnimation()
{
    if (!--d->m_movieBlocked) {
        if ( d->pActiveItem && d->m_movie && d->m_movie->paused()) {
            kdDebug(1203) << "reenabled animation" << endl;
            d->m_movie->restart();
            d->m_movie->unpause();
        }
    }
}

void KonqIconViewWidget::clear()
{
    d->pFileTip->setItem( 0L );
    stopImagePreview(); // Just in case
    KIconView::clear();
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::takeItem( QIconViewItem *item )
{
    if ( d->pActiveItem == static_cast<KFileIVI *>(item) )
    {
        d->pFileTip->setItem( 0L );
        d->pActiveItem = 0L;
    }

    if ( d->pPreviewJob )
      d->pPreviewJob->removeItem( static_cast<KFileIVI *>(item)->item() );

    KIconView::takeItem( item );
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


    d->pFileTip->setOptions(m_pSettings->showFileTips() && QToolTip::isGloballyEnabled(),
                            m_pSettings->showPreviewsInFileTips(),
                            m_pSettings->numFileTips());

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

    setIconTextHeight( m_bDesktop ? 2 : m_pSettings->iconTextHeight() );

    // Update icons if settings for preview icon size have changed
    if ( d->bBoostPreview != boostPreview() )
        setIcons(m_size);
    else if (!bInit)
        updateContents();
    return fontChanged;
}

bool KonqIconViewWidget::boostPreview() const
{
    if ( m_bDesktop ) {
        int size = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
        int mini = spacing() + QMAX( 0, largestPreviewIconSize( size ) - size );
        if ( d->desktopGridSpacing.x() < mini ||
             d->desktopGridSpacing.y() < mini )
            return false;
    }

    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    return group.readBoolEntry( "BoostSize", false );
}

void KonqIconViewWidget::disableSoundPreviews()
{
    d->bSoundPreviews = false;

    if (d->pSoundPlayer)
      d->pSoundPlayer->stop();
    d->pSoundItem = 0;
    if (d->pSoundTimer && d->pSoundTimer->isActive())
      d->pSoundTimer->stop();
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
        int realSize = size ? size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
        setSpacing( ( realSize > KIcon::SizeSmall ) ? 5 : 0 );
    }

    if ( sizeChanged || previewSizeChanged || !stopImagePreviewFor.isEmpty() )
    {
        calculateGridX();
    }
    bool stopAll = !stopImagePreviewFor.isEmpty() && stopImagePreviewFor.first() == "*";
    // Do this even if size didn't change, since this is used by refreshMimeTypes...
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
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

void KonqIconViewWidget::gridValues( int* x, int* y, int* dx, int* dy,
                                     int* nx, int* ny )
{
    int previewSize = previewIconSize( m_size );
    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );

    // Grid size
    *dx = QMAX( iconSize + d->desktopGridSpacing.x(),
                   previewSize + spacing() );
    int textHeight = QMIN( iconTextHeight(), 2 ) * fontMetrics().height();
    *dy = textHeight + 2 +
        QMAX( iconSize + d->desktopGridSpacing.y(), previewSize );

    // Icon Area
    int x1, x2, y1, y2;
    int yOffset = QMAX( 0, *dy - ( previewSize + textHeight ) );
    if ( m_IconRect.isValid() ) {
        *x = x1 = m_IconRect.left(); x2 = m_IconRect.right();
        y1 = m_IconRect.top(); y2 = m_IconRect.bottom();
    }
    else {
        *x = x1 = 0; x2 = viewport()->width();
        y1 = 0; y2 = viewport()->height();
    }
    *y = y1 -= yOffset / 2;
    y2 -= yOffset / 2;

    *nx = (x2 - x1) / *dx;
    *ny = (y2 - y1) / *dy;
    // TODO: Check that items->count() <= nx * ny

    // Let have exactly nx columns and ny rows
    *dx = (x2 - x1) / *nx;
    *dy = (y2 - y1) / *ny;
    kdDebug(1203) << "dx = " << *dx << ", dy = " << *dy << "\n";
}

void KonqIconViewWidget::calculateGridX()
{
    if ( m_bSetGridX )
        setGridX( gridXValue() );
}

int KonqIconViewWidget::gridXValue() const
{
    int sz = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int newGridX = sz + (!m_bSetGridX ? d->gridXspacing : 50) + (( itemTextPos() == QIconView::Right ) ? 100 : 0);
    newGridX = QMAX( newGridX, previewIconSize( sz ) + 13 );
    //kdDebug(1203) << "gridXValue: " << newGridX << " sz=" << sz << endl;
    return newGridX;
}

void KonqIconViewWidget::refreshMimeTypes()
{
    updatePreviewMimeTypes();
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
        (static_cast<KFileIVI *>( it ))->item()->refreshMimeType();
    setIcons( m_size );
}

void KonqIconViewWidget::setURL( const KURL &kurl )
{
    stopImagePreview();
    m_url = kurl;

    d->pFileTip->setPreview( KGlobalSettings::showFilePreview(m_url) );

    if ( m_url.isLocalFile() )
        m_dotDirectoryPath = m_url.path(1).append( ".directory" );
    else
        m_dotDirectoryPath = QString::null;
}

void KonqIconViewWidget::startImagePreview( const QStringList &, bool force )
{
    stopImagePreview(); // just in case

    // Check config
    if ( !KGlobalSettings::showFilePreview( url() ) ) {
        kdDebug(1203) << "Previews disabled for protocol " << url().protocol() << endl;
        emit imagePreviewFinished();
        return;
    }

    if ((d->bSoundPreviews = d->previewSettings.contains( "audio/" )) &&
        !d->pSoundPlayer)
    {
      KLibFactory *factory = KLibLoader::self()->factory("konq_sound");
      if (factory)
        d->pSoundPlayer = static_cast<KonqSoundPlayer *>(
          factory->create(this, 0, "KonqSoundPlayer"));
      d->bSoundPreviews = (d->pSoundPlayer != 0L);
    }

    KFileItemList items;
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
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

    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
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
    connect( d->pPreviewJob, SIGNAL( result( KIO::Job * ) ),
             this, SLOT( slotPreviewResult() ) );
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

bool KonqIconViewWidget::isPreviewRunning() const
{
    return d->pPreviewJob;
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

void KonqIconViewWidget::slotAboutToCreate(const QPoint &, const QValueList<KIO::CopyInfo> &)
{
   // Do nothing :-)
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

    QPoint offset(-10,-10);
    KonqIconDrag * drag = new KonqIconDrag( dragSource );
    QIconViewItem *primaryItem = currentItem();
    // Append all items to the drag object
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
        if ( it->isSelected() ) {
          if (!primaryItem)
             primaryItem = it;
          KURL url = (static_cast<KFileIVI *>(it))->item()->url();
          QString itemURL = KURLDrag::urlToString(url);
          kdDebug(1203) << "itemURL=" << itemURL << endl;
          QIconDragItem id;
          id.setData( QCString(itemURL.latin1()) );
          drag->append( id,
                        QRect( it->pixmapRect(false).topLeft() - m_mousePos - offset,
                               it->pixmapRect().size() ),
                        QRect( it->textRect(false).topLeft() - m_mousePos - offset,
                               it->textRect().size() ),
                        itemURL );
        }
    }

    if (primaryItem)
    {
       // Set pixmap, with the correct offset
       drag->setPixmap( *primaryItem->pixmap(), m_mousePos - primaryItem->pixmapRect(false).topLeft() + offset );
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
    // This code is very related to ListViewBrowserExtension::updateActions
    int canCopy = 0;
    int canDel = 0;
    bool bInTrash = false;
    int iCount = 0;

    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
    {
        if ( it->isSelected() )
        {
            iCount++;
            canCopy++;

            KURL url = ( static_cast<KFileIVI *>( it ) )->item()->url();
            if ( url.directory(false) == KGlobalSettings::trashPath() )
                bInTrash = true;
            if ( KProtocolInfo::supportsDeleting( url ) )
                canDel++;
        }
    }

    emit enableAction( "cut", canDel > 0 );
    emit enableAction( "copy", canCopy > 0 );
    emit enableAction( "trash", canDel > 0 && !bInTrash && m_url.isLocalFile() );
    emit enableAction( "del", canDel > 0 );
    emit enableAction( "properties", iCount > 0 && KPropertiesDialog::canDisplay( selectedFileItems() ) );
    emit enableAction( "editMimeType", ( iCount == 1 ) );
    emit enableAction( "rename", ( iCount == 1 ) && !bInTrash );
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
    paste( url() );
}

void KonqIconViewWidget::paste( const KURL &url )
{
    KonqOperations::doPaste( this, url );
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

void KonqIconViewWidget::contentsMouseMoveEvent( QMouseEvent *e )
{
    if ( QApplication::widgetAt( QCursor::pos() ) != topLevelWidget() )
    {
        if (d->pSoundPlayer)
            d->pSoundPlayer->stop();
        d->pSoundItem = 0;
        if (d->pSoundTimer && d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
    }
    d->renameItem= false;
    QIconView::contentsMouseMoveEvent( e );
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
    bool bMovable = itemsMovable();
    setItemsMovable(false); // hack ? call it what you want :-)
    KIconView::contentsDropEvent( ev );
    setItemsMovable(bMovable);

    QValueList<QIconDragItem> lst;
    slotDropped(ev, lst);
  }
  else
  {
    KIconView::contentsDropEvent( ev );
    emit dropped(); // What is this for ? (David)
  }
  // Don't do this here, it's too early !
  // slotSaveIconPositions();
  // If we want to save after the new file gets listed, though,
  // we could reimplement contentsDropEvent in KDIconView and set m_bNeedSave. Bah.
}

void KonqIconViewWidget::doubleClickTimeout()
{
    d->renameItem= true;
    mousePressChangeValue();
    if ( d->releaseMouseEvent )
    {
        QMouseEvent e( QEvent::MouseButtonPress,d->mousePos , 1, d->mouseState);
        QIconViewItem* item = findItem( e.pos() );
        KURL url;
        if ( item )
        {
            url= ( static_cast<KFileIVI *>( item ) )->item()->url();
            bool brenameTrash =false;
            if ( url.isLocalFile() && (url.directory(false) == KGlobalSettings::trashPath() || url.path(1).startsWith(KGlobalSettings::trashPath())))
                brenameTrash = true;

            if ( url.isLocalFile() && !brenameTrash && d->renameItem && m_pSettings->renameIconDirectly() && e.button() == LeftButton && item->textRect( false ).contains(e.pos()))
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
        QMouseEvent e( QEvent::MouseMove,d->mousePos , 1, d->mouseState);
        KIconView::contentsMousePressEvent( &e );
    }
    if( d->pActivateDoubleClick->isActive() )
        d->pActivateDoubleClick->stop();

    d->releaseMouseEvent = false;
    d->renameItem= false;
}

void KonqIconViewWidget::wheelEvent(QWheelEvent* e)
{
    if (e->state() == ControlButton)
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

    KIconView::wheelEvent(e);
}

void KonqIconViewWidget::mousePressChangeValue()
{
  //kdDebug(1203) << "KonqIconViewWidget::contentsMousePressEvent" << endl;
  m_bMousePressed = true;
  if (d->pSoundPlayer)
    d->pSoundPlayer->stop();
  d->bSoundItemClicked = true;
  d->firstClick = false;
}

void KonqIconViewWidget::contentsMousePressEvent( QMouseEvent *e )
{
    if(d->pActivateDoubleClick && d->pActivateDoubleClick->isActive ())
        d->pActivateDoubleClick->stop();
     QIconViewItem* item = findItem( e->pos() );
     m_mousePos = e->pos();
     KURL url;
     if ( item )
     {
         url = ( static_cast<KFileIVI *>( item ) )->item()->url();
         bool brenameTrash =false;
         if ( url.isLocalFile() && (url.directory(false) == KGlobalSettings::trashPath() || url.path(1).startsWith(KGlobalSettings::trashPath())))
             brenameTrash = true;
         if ( !brenameTrash && !KGlobalSettings::singleClick() && m_pSettings->renameIconDirectly() && e->button() == LeftButton && item->textRect( false ).contains(e->pos())&& !d->firstClick &&  url.isLocalFile() && (!url.protocol().find("device", 0, false)==0))
         {
             d->firstClick = true;
             d->mousePos = e->pos();
             d->mouseState = e->state();
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
    KIconView::contentsMousePressEvent( e );

}

void KonqIconViewWidget::contentsMouseReleaseEvent( QMouseEvent *e )
{
    KIconView::contentsMouseReleaseEvent( e );
    if(d->releaseMouseEvent && d->pActivateDoubleClick && d->pActivateDoubleClick->isActive ())
        d->pActivateDoubleClick->stop();
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
  kdDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions" << endl;
  KSimpleConfig dotDirectory( m_dotDirectoryPath );
  QIconViewItem *it = firstItem();
  if ( !it )
    return; // No more icons. Maybe we're closing and they've been removed already
  while ( it )
  {
    KFileIVI *ivi = static_cast<KFileIVI *>( it );
    KFileItem *item = ivi->item();

    dotDirectory.setGroup( QString( m_iconPositionGroupPrefix ).append( item->url().fileName() ) );
    kdDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions " << item->url().fileName() << " " << it->x() << " " << it->y() << endl;
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
        kdDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions deleting group " << *gIt << endl;
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

    QMemArray<QRect> rects = r.rects();
    QMemArray<QRect>::Iterator it = rects.begin();
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
    if ( !firstItem() ) {
        kdDebug(1203) << "No icons at all ?\n";
        return;
    }

    // Make a list of items
    QValueList<QIconViewItem*> items;
    for ( QIconViewItem* item = firstItem(); item; item = item->nextItem() )
        items.append(item);

    int iconSize = m_size ? m_size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );

    // Create a grid of (ny x nx) bins.
    int x0, y0, dx, dy, nx, ny;
    gridValues( &x0, &y0, &dx, &dy, &nx, &ny );
    typedef QValueList<QIconViewItem*> Bin;
    Bin* bins[nx][ny];
    int i;
    int j;
    for ( i = 0; i < nx ; i++ ) {
        for ( j = 0; j < ny; j++ )
            bins[i][j] = 0L;
    }

    // Insert items into grid
    int textHeight = QMIN( iconTextHeight(), 2 ) * fontMetrics().height();
    QValueList<QIconViewItem*>::Iterator it;
    for ( it = items.begin(); it != items.end(); it++ ) {
        QIconViewItem* item = *it;
        int x = item->x() + item->width() / 2 - x0;
        int y = item->pixmapRect( false ).bottom() - iconSize / 2
                - ( dy - ( iconSize + textHeight ) ) / 2 - y0;
        int posX = QMIN( nx-1, QMAX( 0, x / dx ) );
        int posY = QMIN( ny-1, QMAX( 0, y / dy ) );

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
                QIconViewItem* movedItem;
                Bin* items = bins[i][j];

                int mini = QMIN( QMIN( tf, bf ), QMIN( lf, rf ) );
                if ( tf == mini ) {
                    // move top item in (i,j) to (i,j-1)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->y() < movedItem->y() )
                            movedItem = *it;
                    }
                    items->remove( movedItem );
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
                    items->remove( movedItem );
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
                    items->remove( movedItem );
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
                    items->remove( movedItem );
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
    QValueList<QIconViewItem*> movedItems;

    for ( i = 0; i < nx; i++ ) {
        for ( j = 0; j < ny; j++ ) {
            Bin* bin = bins[i][j];
            if ( !bin )
                continue;
            if ( !bin->isEmpty() ) {
                QIconViewItem* item = bin->first();
                int newX = x0 + i*dx + ( dx - item->width() ) / 2;
                int newY = y0 + j*dy + dy - ( item->pixmapRect().bottom() + textHeight + 2 );
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
    int itemWidth = dx - 2 * spacing();
    if ( maxItemWidth() != itemWidth ) {
        setMaxItemWidth( itemWidth );
        setFont( font() );  // Force calcRect()
        updateContents();
    }
    else {
        // Repaint only repaintRegion...
        QMemArray<QRect> rects = repaintRegion.rects();
        for ( uint l = 0; l < rects.count(); l++ ) {
            kdDebug( 1203 ) << "Repainting (" << rects[l].x() << ","
                            << rects[l].y() << ")\n";
            repaintContents( rects[l], false );
        }
        // Repaint icons that were moved
        while ( !movedItems.isEmpty() ) {
            repaintItem( movedItems.first() );
            movedItems.remove( movedItems.first() );
        }
    }
}

void KonqIconViewWidget::lineupIcons( QIconView::Arrangement arrangement )
{
    int x0, y0, dx, dy, nxmax, nymax;
    gridValues( &x0, &y0, &dx, &dy, &nxmax, &nymax );
    int textHeight = QMIN( iconTextHeight(), 2 ) * fontMetrics().height();

    QRegion repaintRegion;
    QValueList<QIconViewItem*> movedItems;
    int nx = 0, ny = 0;

    QIconViewItem* item;
    for ( item = firstItem(); item; item = item->nextItem() ) {
        int newX = x0 + nx * dx + ( dx - item->width() ) / 2;
        int newY = y0 + ny * dy + dy - ( item->pixmapRect().bottom() + textHeight + 2 );
        if ( item->x() != newX || item->y() != newY ) {
            QRect oldRect = item->rect();
            movedItems.prepend( item );
            item->move( newX, newY );
            if ( item->rect() != oldRect )
                repaintRegion = repaintRegion.unite( oldRect );
        }
        if ( arrangement == QIconView::LeftToRight ) {
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
    QMemArray<QRect> rects = repaintRegion.rects();
    for ( uint l = 0; l < rects.count(); l++ ) {
        kdDebug( 1203 ) << "Repainting (" << rects[l].x() << ","
                        << rects[l].y() << ")\n";
        repaintContents( rects[l], false );
    }
    // Repaint icons that were moved
    while ( !movedItems.isEmpty() ) {
        repaintItem( movedItems.first() );
        movedItems.remove( movedItems.first() );
    }
}

int KonqIconViewWidget::largestPreviewIconSize( int size ) const
{
    int iconSize = size ? size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );

    if (iconSize < 28)
        return 48;
    if (iconSize < 40)
        return 64;
    if (iconSize < 60)
        return 96;

    return 128;
}

int KonqIconViewWidget::previewIconSize( int size ) const
{
    int iconSize = size ? size : KGlobal::iconLoader()->currentSize( KIcon::Desktop );

    if (!d->bBoostPreview)
        return iconSize;

    return largestPreviewIconSize( iconSize );
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
    KURL u;
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
    KTrader::OfferList plugins = KTrader::self()->query("ThumbCreator");
    KTrader::OfferList::ConstIterator it;

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
