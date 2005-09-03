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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfileivi.h"
#include "kivdirectoryoverlay.h"
#include "konq_iconviewwidget.h"
#include "konq_operations.h"
#include "konq_settings.h"

#include <qpainter.h>
//Added by qt3to4:
#include <QIcon>
#include <QPixmap>
#include <Q3ValueList>
#include <QDropEvent>

#include <kurldrag.h>
#include <kiconeffect.h>
#include <kfileitem.h>
#include <kdebug.h>

#undef Bool

/**
 * Private data for KFileIVI
 */
struct KFileIVI::Private
{
    QIcon icons; // Icon states (cached to prevent re-applying icon effects
		    // every time)
	int m_generated; // bitmask(QIcon::Mode) for which states have been generated
    QPixmap  thumb; // Raw unprocessed thumbnail
    QString m_animatedIcon; // Name of animation
    bool m_animated;        // Animation currently running ?
	KIVDirectoryOverlay* m_directoryOverlay;
	QPixmap m_overlay;
	QString m_overlayName;
};

KFileIVI::KFileIVI( KonqIconViewWidget *iconview, KFileItem* fileitem, int size )
    : KIconViewItem( iconview, fileitem->text() ),
    m_size( size ), m_state( KIcon::DefaultState ),
    m_bDisabled( false ), m_bThumbnail( false ), m_fileitem( fileitem )
{
    d = new KFileIVI::Private;
    d->m_generated = 0;

    updatePixmapSize();
    setPixmap( m_fileitem->pixmap( m_size, m_state ) );
    setDropEnabled( S_ISDIR( m_fileitem->mode() ) );

    // Cache entry for the icon effects
	d->icons = QIcon( *pixmap() );
	d->m_generated = QIcon::Normal;
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
	case KIcon::DisabledState:
	    mode = QIcon::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIcon::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIcon::Normal;
	    break;
    }
    d->icons = QIcon();
    QPixmap newThumb = KGlobal::iconLoader()->iconEffect()->
			apply( d->thumb, KIcon::Desktop, state );
    d->icons.addPixmap( newThumb, mode );
    d->m_generated = mode;

    m_state = state;

    Q3IconViewItem::setPixmap( d->icons.pixmap( newThumb.size(), mode ),
			      false, redraw );
}

void KFileIVI::setIcon( int size, int state, bool recalc, bool redraw )
{
    m_size = size;
    m_bThumbnail = false;
    if ( m_bDisabled )
      m_state = KIcon::DisabledState;
    else
      m_state = state;

    if ( d->m_overlayName.isNull() )
        d->m_overlay = QPixmap();
    else {
        int halfSize;
        if (m_size == 0) {
            halfSize = IconSize(KIcon::Desktop) / 2;
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
	case KIcon::DisabledState:
	    mode = QIcon::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIcon::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIcon::Normal;
	    break;
    }

    // We cannot just reset() the iconset here, because setIcon can be
    // called with any state and not just normal state. So we just
    // create a dummy empty iconset as base object.
    d->icons = QIcon();
    d->icons.addPixmap( pixmap, mode );
	d->m_generated |= mode;

    updatePixmapSize();
    Q3IconViewItem::setPixmap( d->icons.pixmap( pixmapSize(), mode ),
			      recalc, redraw );
}

void KFileIVI::setDisabled( bool disabled )
{
    if ( m_bDisabled != disabled )
    {
        m_bDisabled = disabled;
        bool active = ( m_state == KIcon::ActiveState );
        setEffect( m_bDisabled ? KIcon::DisabledState : 
                   ( active ? KIcon::ActiveState : KIcon::DefaultState ) );
    }
}

void KFileIVI::setThumbnailPixmap( const QPixmap & pixmap )
{
    m_bThumbnail = true;
    d->thumb = pixmap;
    // QIconSet::reset() doesn't seem to clear the other generated pixmaps,
    // so we just create a blank QIconSet here
    d->icons = QIcon();
    d->icons.addPixmap( KGlobal::iconLoader()->iconEffect()->
		    apply( pixmap, KIcon::Desktop, KIcon::DefaultState ), QIcon::Normal );
	d->m_generated = QIcon::Normal;

    m_state = KIcon::DefaultState;

    // Recalc when setting this pixmap!
    updatePixmapSize();
    Q3IconViewItem::setPixmap( d->icons.pixmap( pixmapSize(),
			      QIcon::Normal ), true );
}

void KFileIVI::setActive( bool active )
{
    if ( active )
        setEffect( KIcon::ActiveState );
    else
        setEffect( m_bDisabled ? KIcon::DisabledState : KIcon::DefaultState );
}

void KFileIVI::setEffect( int state )
{
    QIcon::Mode mode;
    switch( state )
    {
	case KIcon::DisabledState:
	    mode = QIcon::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIcon::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIcon::Normal;
	    break;
    }
    // Do not update if the fingerprint is identical (prevents flicker)!

    KIconEffect *effect = KGlobal::iconLoader()->iconEffect();

    bool haveEffect = effect->hasEffect( KIcon::Desktop, m_state ) !=
                      effect->hasEffect( KIcon::Desktop, state );

                //kdDebug(1203) << "desktop;defaultstate=" <<
                //      effect->fingerprint(KIcon::Desktop, KIcon::DefaultState) <<
                //      endl;
                //kdDebug(1203) << "desktop;activestate=" <<
                //      effect->fingerprint(KIcon::Desktop, KIcon::ActiveState) <<
                //      endl;

    if( haveEffect &&
        effect->fingerprint( KIcon::Desktop, m_state ) !=
	effect->fingerprint( KIcon::Desktop, state ) )
    {
	// Effects on are not applied until they are first accessed to
	// save memory. Do this now when needed
	if( m_bThumbnail )
	{
	    if( !(d->m_generated & mode) ) {
		d->icons.addPixmap( effect->apply( d->thumb, KIcon::Desktop, state ), mode );
		d->m_generated |= mode;
	    }
	}
	else
	{
	    if( !(d->m_generated & mode) ) {
		d->icons.addPixmap( m_fileitem->pixmap( m_size, state ), mode );
		d->m_generated |= mode;
	    }
	}
	Q3IconViewItem::setPixmap( d->icons.pixmap( QSize(m_size, m_size), mode ) );
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
        KURL::List uris = ( static_cast<KonqIconViewWidget*>(iconView()) )->dragURLs();

        // Check if we want to drop something on itself
        // (Nothing will happen, but it's a convenient way to move icons)
        KURL::List::Iterator it = uris.begin();
        for ( ; it != uris.end() ; it++ )
        {
            if ( m_fileitem->url().equals( *it, true /*ignore trailing slashes*/ ) )
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

void KFileIVI::dropped( QDropEvent *e, const Q3ValueList<Q3IconDragItem> & )
{
    KonqOperations::doDrop( item(), item()->url(), e, iconView() );
}

void KFileIVI::returnPressed()
{
    m_fileitem->run();
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
        f.setUnderline( TRUE );
        p->setFont( f );
    }*/

    KIconViewItem::paintItem( p, cg );
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
        f.setItalic( TRUE );
        p->setFont( f );
    }
}

QColorGroup KFileIVI::updateColors( const QColorGroup &c ) const
{
    QColorGroup cg( c );
    cg.setColor( QColorGroup::Text, static_cast<KonqIconViewWidget*>(iconView())->itemColor() );
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
        //kdDebug(1203) << "KIconViewItem::setMouseOverAnimation " << movieFileName << endl;
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
        KGlobal::iconLoader()->currentSize( KIcon::Desktop );

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

/* vim: set noet sw=4 ts=8 softtabstop=4: */
