/* This file is part of the KDE project
   Copyright (C) 1999, 2000, 2001, 2002 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfileivi.h"
#include "kivdirectoryoverlay.h"
#include "konq_iconviewwidget.h"
#include "konq_operations.h"
#include "konq_settings.h"

#include <QPainter>
//Added by qt3to4:
#include <QPixmap>
#include <QList>
#include <QDropEvent>
#include <QIcon>

#include <k3urldrag.h>
#include <kiconeffect.h>
#include <kfileitem.h>
#include <kdebug.h>
#include <krun.h>

#undef Bool

/**
 * Private data for KFileIVI
 */
struct KFileIVI::Private
{
    QPixmap cachedPixmap( QIcon::Mode ) const;
    void addCachedPixmap( const QPixmap&, QIcon::Mode );
    void setCachedPixmaps( const QPixmap&, QIcon::Mode = QIcon::Normal );

    // Icon states (cached to prevent re-applying icon effects every time)
    QMap<QIcon::Mode, QPixmap> cachedPixmaps;
    QPixmap  thumb; // Raw unprocessed thumbnail
    QString m_animatedIcon; // Name of animation
    bool m_animated;        // Animation currently running ?
	KIVDirectoryOverlay* m_directoryOverlay;
	QPixmap m_overlay;
	QString m_overlayName;
};

KFileIVI::KFileIVI( KonqIconViewWidget *iconview, KFileItem* fileitem, int size )
    : K3IconViewItem( iconview, fileitem->text() ),
    m_size( size ), m_state( K3Icon::DefaultState ),
    m_bDisabled( false ), m_bThumbnail( false ), m_fileitem( fileitem )
{
    d = new KFileIVI::Private;

    updatePixmapSize();
    setPixmap( m_fileitem->pixmap( m_size, m_state ) );
    setDropEnabled( S_ISDIR( m_fileitem->mode() ) );

    // Cache entry for the icon effects
    d->setCachedPixmaps( *pixmap() );
    d->m_animated = false;

    // iconName() requires the mimetype to be known
    if ( fileitem->isMimeTypeKnown() )
    {
        QString icon = fileitem->iconName();
        if ( !icon.isEmpty() )
            setMouseOverAnimation( icon );
        else
            setMouseOverAnimation( "unknown" );
    }
    d->m_directoryOverlay = 0;
}

KFileIVI::~KFileIVI()
{
    delete d->m_directoryOverlay;
    delete d;
}

void KFileIVI::invalidateThumb( int state, bool redraw )
{
    QIcon::Mode mode;
    switch( state )
    {
	case K3Icon::DisabledState:
	    mode = QIcon::Disabled;
	    break;
	case K3Icon::ActiveState:
	    mode = QIcon::Active;
	    break;
	case K3Icon::DefaultState:
	default:
	    mode = QIcon::Normal;
	    break;
    }

    const QPixmap newThumb( KGlobal::iconLoader()->iconEffect()->
                            apply( d->thumb, K3Icon::Desktop, state ) );
    d->setCachedPixmaps( newThumb, mode );

    m_state = state;

    Q3IconViewItem::setPixmap( newThumb, false, redraw );
}

void KFileIVI::setIcon( int size, int state, bool recalc, bool redraw )
{
    m_size = size;
    m_bThumbnail = false;
    if ( m_bDisabled )
      m_state = K3Icon::DisabledState;
    else
      m_state = state;

    if ( d->m_overlayName.isNull() )
        d->m_overlay = QPixmap();
    else {
        int halfSize;
        if (m_size == 0) {
            halfSize = IconSize(K3Icon::Desktop) / 2;
        } else {
            halfSize = m_size / 2;
        }
        d->m_overlay = DesktopIcon(d->m_overlayName, halfSize);
    }

    setPixmapDirect(m_fileitem->pixmap( m_size, m_state ) , recalc, redraw );
}

void KFileIVI::setOverlay( const QString& iconName )
{
    d->m_overlayName = iconName;

    refreshIcon(true);
}

KIVDirectoryOverlay* KFileIVI::setShowDirectoryOverlay( bool show )
{
    if ( !m_fileitem->isDir() || m_fileitem->iconName() != "folder" )
        return 0;

    if (show) {
        if (!d->m_directoryOverlay)
            d->m_directoryOverlay = new KIVDirectoryOverlay(this);
        return d->m_directoryOverlay;
    } else {
        delete d->m_directoryOverlay;
        d->m_directoryOverlay = 0;
        setOverlay(QString());
        return 0;
    }
}

bool KFileIVI::showDirectoryOverlay(  )
{
    return (bool)d->m_directoryOverlay;
}

void KFileIVI::setPixmapDirect( const QPixmap& pixmap, bool recalc, bool redraw )
{
    QIcon::Mode mode;
    switch( m_state )
    {
	case K3Icon::DisabledState:
	    mode = QIcon::Disabled;
	    break;
	case K3Icon::ActiveState:
	    mode = QIcon::Active;
	    break;
	case K3Icon::DefaultState:
	default:
	    mode = QIcon::Normal;
	    break;
    }

    d->setCachedPixmaps( pixmap, mode );

    updatePixmapSize();
    Q3IconViewItem::setPixmap( pixmap, recalc, redraw );
}

void KFileIVI::setDisabled( bool disabled )
{
    if ( m_bDisabled != disabled )
    {
        m_bDisabled = disabled;
        bool active = ( m_state == K3Icon::ActiveState );
        setEffect( m_bDisabled ? K3Icon::DisabledState : 
                   ( active ? K3Icon::ActiveState : K3Icon::DefaultState ) );
    }
}

void KFileIVI::setThumbnailPixmap( const QPixmap & pixmap )
{
    m_bThumbnail = true;
    d->thumb = pixmap;

    const QPixmap newThumb( KGlobal::iconLoader()->iconEffect()
                            ->apply( pixmap, K3Icon::Desktop, K3Icon::DefaultState ) );
    d->setCachedPixmaps( newThumb );

    m_state = K3Icon::DefaultState;

    // Recalc when setting this pixmap!
    updatePixmapSize();
    Q3IconViewItem::setPixmap( newThumb, true );
}

void KFileIVI::setActive( bool active )
{
    if ( active )
        setEffect( K3Icon::ActiveState );
    else
        setEffect( m_bDisabled ? K3Icon::DisabledState : K3Icon::DefaultState );
}

void KFileIVI::setEffect( int state )
{
    QIcon::Mode mode;
    switch( state )
    {
	case K3Icon::DisabledState:
	    mode = QIcon::Disabled;
	    break;
	case K3Icon::ActiveState:
	    mode = QIcon::Active;
	    break;
	case K3Icon::DefaultState:
	default:
	    mode = QIcon::Normal;
	    break;
    }
    // Do not update if the fingerprint is identical (prevents flicker)!

    KIconEffect *effect = KGlobal::iconLoader()->iconEffect();

    bool haveEffect = effect->hasEffect( K3Icon::Desktop, m_state ) !=
                      effect->hasEffect( K3Icon::Desktop, state );

                //kDebug(1203) << "desktop;defaultstate=" <<
                //      effect->fingerprint(K3Icon::Desktop, K3Icon::DefaultState) <<
                //      endl;
                //kDebug(1203) << "desktop;activestate=" <<
                //      effect->fingerprint(K3Icon::Desktop, K3Icon::ActiveState) <<
                //      endl;

    if( haveEffect &&
        effect->fingerprint( K3Icon::Desktop, m_state ) !=
	effect->fingerprint( K3Icon::Desktop, state ) )
    {
	// Effects on are not applied until they are first accessed to
	// save memory. Do this now when needed
        QPixmap pixmap( d->cachedPixmap( mode ) );
        if( pixmap.isNull() )
        {
            if( m_bThumbnail )
                pixmap = effect->apply( d->thumb, K3Icon::Desktop, state );
            else
                pixmap = m_fileitem->pixmap( m_size, state );
            d->addCachedPixmap( pixmap, mode );
        }
        Q3IconViewItem::setPixmap( pixmap );
    }
    m_state = state;
}

void KFileIVI::refreshIcon( bool redraw )
{
    if (!isThumbnail())
        setIcon( m_size, m_state, true, redraw );
}

void KFileIVI::invalidateThumbnail()
{
    d->thumb = QPixmap();
}

bool KFileIVI::isThumbnailInvalid() const
{
    return d->thumb.isNull();
}

bool KFileIVI::acceptDrop( const QMimeSource *mime ) const
{
    if ( mime->provides( "text/uri-list" ) ) // We're dragging URLs
    {
        if ( m_fileitem->acceptsDrops() ) // Directory, executables, ...
            return true;

        // Use cache
        KUrl::List uris = ( static_cast<KonqIconViewWidget*>(iconView()) )->dragURLs();

        // Check if we want to drop something on itself
        // (Nothing will happen, but it's a convenient way to move icons)
        KUrl::List::Iterator it = uris.begin();
        for ( ; it != uris.end() ; it++ )
        {
            if ( m_fileitem->url().equals( *it, KUrl::CompareWithoutTrailingSlash ) )
                return true;
        }
    }
    return Q3IconViewItem::acceptDrop( mime );
}

void KFileIVI::setKey( const QString &key )
{
    QString theKey = key;

    QVariant sortDirProp = iconView()->property( "sortDirectoriesFirst" );

    bool isdir = ( S_ISDIR( m_fileitem->mode() ) && ( !sortDirProp.isValid() || ( sortDirProp.type() == QVariant::Bool && sortDirProp.toBool() ) ) );

    // The order is: .dir (0), dir (1), .file (2), file (3)
    int sortChar = isdir ? 1 : 3;
    if ( m_fileitem->text()[0] == '.' )
        --sortChar;

    if ( !iconView()->sortDirection() ) // reverse sorting
        sortChar = 3 - sortChar;

    theKey.prepend( QChar( sortChar + '0' ) );

    Q3IconViewItem::setKey( theKey );
}

void KFileIVI::dropped( QDropEvent *e, const QList<Q3IconDragItem> & )
{
    KonqOperations::doDrop( item(), item()->url(), e, iconView() );
}

void KFileIVI::returnPressed()
{
    if ( static_cast<KonqIconViewWidget*>(iconView())->isDesktop() ) {
        KUrl url = m_fileitem->url();
        // When clicking on a link to e.g. $HOME from the desktop, we want to open $HOME
        // Symlink resolution must only happen on the desktop though (#63014)
        if ( m_fileitem->isLink() && url.isLocalFile() )
            url = KUrl( url, m_fileitem->linkDest() );

        (void) new KRun( url, 0L ,m_fileitem->mode(), m_fileitem->isLocalFile() );
    } else {
        m_fileitem->run();
    }
}


void KFileIVI::paintItem( QPainter *p, const QColorGroup &c )
{
    QColorGroup cg = updateColors(c);
    paintFontUpdate( p );

    //*** TEMPORARY CODE - MUST BE MADE CONFIGURABLE FIRST - Martijn
    // SET UNDERLINE ON HOVER ONLY
    /*if ( ( ( KonqIconViewWidget* ) iconView() )->m_pActiveItem == this )
    {
        QFont f( p->font() );
        f.setUnderline( true );
        p->setFont( f );
    }*/

    K3IconViewItem::paintItem( p, cg );
    paintOverlay(p);

}

void KFileIVI::paintOverlay( QPainter *p ) const
{
    if ( !d->m_overlay.isNull() ) {
        QRect rect = pixmapRect(true);
        p->drawPixmap(x() + rect.x() , y() + pixmapRect().height() - d->m_overlay.height(), d->m_overlay);
    }
}

void KFileIVI::paintFontUpdate( QPainter *p ) const
{
    if ( m_fileitem->isLink() )
    {
        QFont f( p->font() );
        f.setItalic( true );
        p->setFont( f );
    }
}

QColorGroup KFileIVI::updateColors( const QColorGroup &c ) const
{
    QColorGroup cg( c );
    cg.setColor( QPalette::Text, static_cast<KonqIconViewWidget*>(iconView())->itemColor() );
    return cg;
}

bool KFileIVI::move( int x, int y )
{
    if ( static_cast<KonqIconViewWidget*>(iconView())->isDesktop() ) {
	if ( x < 5 )
	    x = 5;
	if ( x > iconView()->viewport()->width() - ( width() + 5 ) )
	    x = iconView()->viewport()->width() - ( width() + 5 );
	if ( y < 5 )
	    y = 5;
	if ( y > iconView()->viewport()->height() - ( height() + 5 ) )
	    y = iconView()->viewport()->height() - ( height() + 5 );
    }
    return Q3IconViewItem::move( x, y );
}

bool KFileIVI::hasAnimation() const
{
    return !d->m_animatedIcon.isEmpty() && !m_bThumbnail;
}

void KFileIVI::setMouseOverAnimation( const QString& movieFileName )
{
    if ( !movieFileName.isEmpty() )
    {
        //kDebug(1203) << "K3IconViewItem::setMouseOverAnimation " << movieFileName << endl;
        d->m_animatedIcon = movieFileName;
    }
}

QString KFileIVI::mouseOverAnimation() const
{
    return d->m_animatedIcon;
}

bool KFileIVI::isAnimated() const
{
    return d->m_animated;
}

void KFileIVI::setAnimated( bool a )
{
    d->m_animated = a;
}

int KFileIVI::compare( Q3IconViewItem *i ) const
{
    KonqIconViewWidget* view = static_cast<KonqIconViewWidget*>(iconView());
    if ( view->caseInsensitiveSort() )
        return key().localeAwareCompare( i->key() );
    else
        return view->m_pSettings->caseSensitiveCompare( key(), i->key() );
}

void KFileIVI::updatePixmapSize()
{
    int size = m_size ? m_size :
        KGlobal::iconLoader()->currentSize( K3Icon::Desktop );

    KonqIconViewWidget* view = static_cast<KonqIconViewWidget*>( iconView() );

    if ( view->canPreview( item() ) ) {
        int previewSize = view->previewIconSize( size );
        setPixmapSize( QSize( previewSize, previewSize ) );
    }
    else {
        QSize pixSize = QSize( size, size );
        if ( pixSize != pixmapSize() )
            setPixmapSize( pixSize );
    }
}

QPixmap KFileIVI::Private::cachedPixmap( QIcon::Mode mode) const
{
    return cachedPixmaps.value( mode );
}

void KFileIVI::Private::addCachedPixmap( const QPixmap& pixmap, QIcon::Mode mode )
{
    if ( pixmap.isNull() )
        kWarning(1203) << k_funcinfo << "pixmap is null" << endl;
    cachedPixmaps.insert( mode, pixmap );
}

void KFileIVI::Private::setCachedPixmaps( const QPixmap& pixmap, QIcon::Mode mode )
{
    cachedPixmaps.clear();

    addCachedPixmap( pixmap, mode );
}


/* vim: set noet sw=4 ts=8 softtabstop=4: */
