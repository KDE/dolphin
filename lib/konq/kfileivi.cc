/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// XXX: This ugly hack should go away as soon as the latest release
// of Qt include a way to access QIconView::mask() and QIconView::tmpText
#define private public
#define protected public
#include <qiconview.h>
#undef private
#undef protected
#include "kfileivi.h"
#include "konq_fileitem.h"
#include "konq_drag.h"
#include "konq_iconviewwidget.h"
#include "konq_operations.h"

#include <kapp.h>
#include <kipc.h>
#include <kurldrag.h>
#include <kalphapainter.h>
#include <kimageeffect.h>
#include <kstaticdeleter.h>
#include <kiconeffect.h>

#undef Bool

static QPixmap *konqiv_buffer_pixmap = 0;
static KStaticDeleter <QPixmap> konqiv_buffer_pixmap_deleter;

static QPixmap *get_konqiv_buffer_pixmap( const QSize &s )
{
    if ( !konqiv_buffer_pixmap ) {
      konqiv_buffer_pixmap = konqiv_buffer_pixmap_deleter.setObject(
	  new QPixmap( s ));
      return konqiv_buffer_pixmap;
    }

    konqiv_buffer_pixmap->resize( s );
    return konqiv_buffer_pixmap;
}

/**
 * Private data for KFileIVI
 */
struct KFileIVI::Private
{
    QIconSet icons; // Icon states (cached to prevent re-applying icon effects
		    // every time)
    QPixmap  thumb; // Raw unprocessed thumbnail
    int	     state; // Currently displayed state of the icon
};

KFileIVI::KFileIVI( KonqIconViewWidget *iconview, KonqFileItem* fileitem, int size )
    : QIconViewItem( iconview, fileitem->text(),
		     fileitem->pixmap( size, KIcon::DefaultState ) ),
  m_size(size), m_state( KIcon::DefaultState ),
    m_bDisabled( false ), m_bThumbnail( false ), m_fileitem( fileitem )
{
    setDropEnabled( S_ISDIR( m_fileitem->mode() ) );
    d = new KFileIVI::Private;
    
    // Cache entry for the icon effects
    d->icons.reset( *pixmap(), QIconSet::Large );
    d->state = KIcon::DefaultState;
}

KFileIVI::~KFileIVI()
{
    delete d;
}

void KFileIVI::invalidateThumb( int state, bool redraw )
{
    QIconSet::Mode mode;
    switch( state )
    {
	case KIcon::DisabledState:
	    mode = QIconSet::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIconSet::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIconSet::Normal;
	    break;
    }
    d->icons = QIconSet();
    d->icons.setPixmap( KGlobal::iconLoader()->iconEffect()->
			apply( d->thumb, KIcon::Desktop, state ),
			QIconSet::Large, mode );
    d->state = state;
    
    QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large, mode ),
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
     
    QIconSet::Mode mode;
    switch( state )
    {
	case KIcon::DisabledState:
	    mode = QIconSet::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIconSet::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIconSet::Normal;
	    break;
    }
    
    // We cannot just reset() the iconset here, because setIcon can be
    // called with any state and not just normal state. So we just
    // create a dummy empty iconset as base object.
    d->icons = QIconSet();
    d->icons.setPixmap( m_fileitem->pixmap( m_size, m_state ),
			QIconSet::Large, mode );
    d->state = m_state;
    QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large, mode ),
			      recalc, redraw );
}

void KFileIVI::setDisabled( bool disabled )
{
    if ( m_bDisabled != disabled && !isThumbnail() )
    {
        m_bDisabled = disabled;
        m_state = m_bDisabled ? KIcon::DisabledState : KIcon::DefaultState;
        QIconViewItem::setPixmap( m_fileitem->pixmap( m_size, m_state ), false, true );
    }
}

void KFileIVI::setThumbnailPixmap( const QPixmap & pixmap )
{
    m_bThumbnail = true;
    d->thumb = pixmap;
    // QIconSet::reset() doesn't seem to clear the other generated pixmaps,
    // so we just create a blank QIconSet here
    d->icons = QIconSet();
    d->icons.setPixmap( KGlobal::iconLoader()->iconEffect()->
		    apply( pixmap, KIcon::Desktop, KIcon::DefaultState ),
		    QIconSet::Large, QIconSet::Normal );

    d->state = KIcon::DefaultState;
    
    // Recalc when setting this pixmap!
    QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large,
			      QIconSet::Normal ), true );
}

void KFileIVI::setEffect( int group, int state )
{
    QIconSet::Mode mode;
    switch( state )
    {
	case KIcon::DisabledState:
	    mode = QIconSet::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIconSet::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIconSet::Normal;
	    break;
    }
    // Do not update if the fingerprint is identical (prevents flicker)!
    if( KGlobal::iconLoader()->iconEffect()->fingerprint
	    ( KIcon::Desktop, d->state ) !=
	KGlobal::iconLoader()->iconEffect()->fingerprint
	    ( KIcon::Desktop, state ) )
    {
	// Effects on are not applied until they are first accessed to
	// save memory. Do this now when needed
	if( m_bThumbnail )
	{
	    if( d->icons.isGenerated( QIconSet::Large, mode ) )
		d->icons.setPixmap( KGlobal::iconLoader()->iconEffect()->
				    apply( d->thumb, KIcon::Desktop, state ),
				    QIconSet::Large, mode );
	}
	else
	{
	    if( d->icons.isGenerated( QIconSet::Large, mode ) )
		d->icons.setPixmap( m_fileitem->pixmap( m_size, state ),
				    QIconSet::Large, mode );
	}
	QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large, mode ) );
    }
    d->state = state;
}

void KFileIVI::setThumbnailName( const QString & name )
{
    m_thumbnailName = name;
}

void KFileIVI::refreshIcon( bool redraw )
{
    if ( !isThumbnail())
        QIconViewItem::setPixmap( m_fileitem->pixmap( m_size, m_state ), true, redraw );
}

bool KFileIVI::acceptDrop( const QMimeSource *mime ) const
{
    if ( mime->provides( "text/uri-list" ) ) // We're dragging URLs
    {
        if ( m_fileitem->acceptsDrops() ) // Directory, executables, ...
            return true;
        KURL::List uris;
        if ( iconView()->inherits( "KonqIconViewWidget" ) )
            // Use cache if we can
            uris = ( static_cast<KonqIconViewWidget*>(iconView()) )->dragURLs();
        else
            KURLDrag::decode( mime, uris );

        // Check if we want to drop something on itself
        // (Nothing will happen, but it's a convenient way to move icons)
        KURL::List::Iterator it = uris.begin();
        for ( ; it != uris.end() ; it++ )
        {
            if ( m_fileitem->url().cmp( *it, true /*ignore trailing slashes*/ ) )
                return true;
        }
    }
    return QIconViewItem::acceptDrop( mime );
}

void KFileIVI::setKey( const QString &key )
{
    QString theKey = key;

    QVariant sortDirProp = iconView()->property( "sortDirectoriesFirst" );

    if ( S_ISDIR( m_fileitem->mode() ) && ( !sortDirProp.isValid() || ( sortDirProp.type() == QVariant::Bool && sortDirProp.toBool() ) ) )
      theKey.prepend( iconView()->sortDirection() ? '0' : '1' );
    else
      theKey.prepend( iconView()->sortDirection() ? '1' : '0' );

    QIconViewItem::setKey( theKey );
}

void KFileIVI::dropped( QDropEvent *e, const QValueList<QIconDragItem> & )
{
  KonqOperations::doDrop( item(), item()->url(), e, iconView() );
}

void KFileIVI::returnPressed()
{
    m_fileitem->run();
}

void KFileIVI::paintItem( QPainter *p, const QColorGroup &c )
{
    QColorGroup cg( c );
    cg.setColor( QColorGroup::Text, static_cast<KonqIconViewWidget*>(iconView())->itemColor() );
    if ( m_fileitem->isLink() )
    {
        QFont f( p->font() );
        f.setItalic( TRUE );
        p->setFont( f );
    }

		//*** TEMPORARY CODE - MUST BE MADE CONFIGURABLE FIRST - Martijn
		// SET UNDERLINE ON HOVER ONLY
/*    if ( ( ( KonqIconViewWidget* ) iconView() )->m_pActiveItem == this )
    {
        QFont f( p->font() );
        f.setUnderline( TRUE );
        p->setFont( f );
    }*/
		//***

    if (!KGlobal::iconLoader()->alphaBlending(KIcon::Desktop))
    {
      // default fallback if we're not using alphablending
      QIconViewItem::paintItem( p, cg );
      return;
    }

    // Ok, we're using alpha blending.

    QPixmap unknown_icon=KIconLoader::unknown();

    if ( !iconView() )
	return;


    // First, let's get the background.

    QRect r=pixmapRect(false);
    QRect rcontents=r; 
    rcontents.moveBy(-iconView()->contentsX(), -iconView()->contentsY());
    
    QPixmap background(r.width(),r.height());
    QPainter *pbg=new QPainter(&background);
    static_cast<KonqIconViewWidget *>(iconView())->drawBackground(pbg,
	rcontents , QPoint(0,0));
    delete pbg;

    QImage bgImg(background.convertToImage());

    // This is done to handle overlapped icons. There are no overlapped
    // icons in 98% of the cases, so I don't think it's a bad solution.
    // The right solution would be using QIconView's ItemContainer,
    // or any other way to get icons that overlap with a given rectangle
    // and blend (paint over bgImg) only the ones which are below the current
    // one.

    QIconViewItem *it=iconView()->firstItem();
    while (it!=static_cast<QIconViewItem *>(this) && it )
    {
      if ( r.intersects( it->rect() ) &&
	( r.intersects(it->pixmapRect(false)) || r.intersects(it->textRect(false)) ) )
	  static_cast<KFileIVI *>(it)->paintAlphaItem(p, cg, bgImg,
	      unknown_icon, true, r.x(), r.y());
      it=it->nextItem();
    }

    it=nextItem();
    while (it)
    {
      if ( r.intersects( it->rect() ) &&
	( r.intersects(it->pixmapRect(false)) || r.intersects(it->textRect(false)) ) )
	  static_cast<KFileIVI *>(it)->paintAlphaItem(p, cg, bgImg,
	      unknown_icon, true, r.x(), r.y());
      it=it->nextItem();
    }

    // and now, let's paint this item's icon.

    paintAlphaItem(p, cg, bgImg, unknown_icon, false, r.x(), r.y());
}

void KFileIVI::paintAlphaItem( QPainter *p, const QColorGroup &cg,
	QImage &background, QPixmap & unknown_icon , bool onBg, int bgx, int bgy)
{
    QPixmap *txtPixmap=0L;
    QRect txtRect=textRect(false);
    if (onBg)
    {
      txtRect.moveTopLeft(QPoint(0,0));
      QImage txtImg(txtRect.size(),32);
      KImageEffect::paint(bgx-textRect(false).x(), bgy-textRect(false).y(),
	  txtImg, background);
      txtPixmap=new QPixmap();
      txtPixmap->convertFromImage(txtImg);
      p=new QPainter(txtPixmap);
    } else {
      if (!p) return;
      p->save();
    }

    // antonio: The next lines have been copied from Qt and are
    // copyrighted by Trolltech AS (1992-2000) . They also include
    // many changes by me.

    if ( isSelected() ) {
	p->setPen( cg.highlightedText() );
    } else {
	p->setPen( cg.text() );
    }

    calcTmpText();

    if ( iconView()->itemTextPos() == QIconView::Bottom ) {
	int w = ( pixmap() ? pixmap() : &unknown_icon )->width();

	if ( isSelected() ) {
	    QPixmap *pix = pixmap() ? pixmap() : &unknown_icon;
	    if ( pix && !pix->isNull() ) {
		QPixmap *buffer = get_konqiv_buffer_pixmap( pix->size() );
		QBitmap mask = iconView()->mask( pix );

		QPainter p2( buffer );
		p2.fillRect( pix->rect(), white );
		p2.drawPixmap( 0, 0, *pix );
		p2.end();
		buffer->setMask( mask );
		p2.begin( buffer );
		p2.fillRect( pix->rect(), QBrush( cg.highlight(),
		      QBrush::Dense4Pattern) );
		p2.end();
		//QRect cr = pix->rect();
		KAlphaPainter::draw(((onBg)? 0L : p) , *buffer, background,
		    x() + ( width() - w ) / 2, y(), onBg, bgx, bgy);
	    }
	} else
	  KAlphaPainter::draw( ((onBg)? 0L : p),
	      *( pixmap() ? pixmap() : &unknown_icon), background,
	      x() + ( width() - w ) / 2, y(), onBg, bgx, bgy);


	p->save();
	if ( isSelected() ) {
	    p->fillRect( txtRect, cg.highlight() );
	    p->setPen( QPen( cg.highlightedText() ) );
	} else if ( iconView()->itemTextBackground() != NoBrush )
	    p->fillRect( txtRect, iconView()->itemTextBackground() );

	int align = AlignHCenter;
	if ( iconView()->wordWrapIconText() )
	    align |= WordBreak;
	p->drawText( txtRect, align, iconView()->wordWrapIconText() ? text() : tmpText );

	p->restore();
    } else {
	int h = ( pixmap() ? pixmap() : &unknown_icon )->height();

	if ( isSelected() ) {
	    QPixmap *pix = pixmap() ? pixmap() : &unknown_icon;
	    if ( pix && !pix->isNull() ) {
		QPixmap *buffer = get_konqiv_buffer_pixmap( pix->size() );
		QBitmap mask = iconView()->mask( pix );

		QPainter p2( buffer );
		p2.fillRect( pix->rect(), white );
		p2.drawPixmap( 0, 0, *pix );
		p2.end();
		buffer->setMask( mask );
		p2.begin( buffer );
		p2.fillRect( pix->rect(), QBrush( cg.highlight(), QBrush::Dense4Pattern) );
		p2.end();
		//QRect cr = pix->rect();
		KAlphaPainter::draw(((onBg)? 0L : p), *buffer, background,
		    x(), y() + ( height() - h ) / 2 , onBg, bgx, bgy);
	    }
	} else {
	   KAlphaPainter::draw(((onBg) ? 0L : p),
	       *( pixmap() ? pixmap() : &unknown_icon ), background,
	       x(), y() + ( height() - h ) / 2 , onBg, bgx, bgy);
	}

	p->save();
	if ( isSelected() ) {
	    p->fillRect( txtRect, cg.highlight() );
	    p->setPen( QPen( cg.highlightedText() ) );
	} else if ( iconView()->itemTextBackground() != NoBrush )
	    p->fillRect( txtRect, iconView()->itemTextBackground() );

	int align = AlignLeft;
	if ( iconView()->wordWrapIconText() )
	    align |= WordBreak;
	p->drawText( txtRect, align, iconView()->wordWrapIconText() ? text() : tmpText );

	p->restore();
    }

    if (onBg) {
      delete p;
      KImageEffect::paint(textRect(false).x()-bgx, textRect(false).y()-bgy, background,
	  txtPixmap->convertToImage());
      delete txtPixmap;
    } else
      p->restore();
}

void KFileIVI::move( int x, int y )
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
    QIconViewItem::move( x, y );
}

/* vim: set noet sw=4 ts=8 softtabstop=4: */

