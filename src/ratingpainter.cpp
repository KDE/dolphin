/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "ratingpainter.h"

#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <kicon.h>
#include <kiconeffect.h>
#include <kdebug.h>


class Nepomuk::RatingPainter::Private
{
public:
    Private()
        : maxRating(10),
          icon( "rating" ),
          bHalfSteps(true),
          alignment(Qt::AlignCenter),
          direction(Qt::LeftToRight) {
    }

    int maxRating;
    KIcon icon;
    bool bHalfSteps;
    Qt::Alignment alignment;
    Qt::LayoutDirection direction;
    QPixmap customPixmap;
};


Nepomuk::RatingPainter::RatingPainter()
    : d(new Private())
{
}


Nepomuk::RatingPainter::~RatingPainter()
{
    delete d;
}


int Nepomuk::RatingPainter::maxRating() const
{
    return d->maxRating;
}


bool Nepomuk::RatingPainter::halfStepsEnabled() const
{
    return d->bHalfSteps;
}


Qt::Alignment Nepomuk::RatingPainter::alignment() const
{
    return d->alignment;
}


Qt::LayoutDirection Nepomuk::RatingPainter::direction() const
{
    return d->direction;
}


KIcon Nepomuk::RatingPainter::icon() const
{
    return d->icon;
}


QPixmap Nepomuk::RatingPainter::customPixmap() const
{
    return d->customPixmap;
}


void Nepomuk::RatingPainter::setMaxRating( int max )
{
    d->maxRating = max;
}


void Nepomuk::RatingPainter::setHalfStepsEnabled( bool enabled )
{
    d->bHalfSteps = enabled;
}


void Nepomuk::RatingPainter::setAlignment( Qt::Alignment align )
{
    d->alignment = align;
}


void Nepomuk::RatingPainter::setLayoutDirection( Qt::LayoutDirection direction )
{
    d->direction = direction;
}


void Nepomuk::RatingPainter::setIcon( const KIcon& icon )
{
    d->icon = icon;
}


void Nepomuk::RatingPainter::setCustomPixmap( const QPixmap& pixmap )
{
    d->customPixmap = pixmap;
}


void Nepomuk::RatingPainter::draw( QPainter* painter, const QRect& rect, int rating, int hoverRating )
{
    rating = qMin( rating, d->maxRating );
    hoverRating = qMin( hoverRating, d->maxRating );

    int numUsedStars = d->bHalfSteps ? d->maxRating/2 : d->maxRating;

    if ( hoverRating > 0 && hoverRating < rating ) {
        int tmp = hoverRating;
        hoverRating = rating;
        rating = tmp;
    }

    // get the rating pixmaps
    QPixmap ratingPix;
    if ( !d->customPixmap.isNull() ) {
        ratingPix = d->customPixmap;
    }
    else {
        KIcon ratingIcon( "rating" );
        int iconSize = qMin( rect.height(), rect.width() / numUsedStars );
        ratingPix = ratingIcon.pixmap( iconSize );
    }

    QPixmap disabledRatingPix = KIconEffect().apply( ratingPix, KIconEffect::ToGray, 1.0, QColor(), false );
    QPixmap hoverPix;

    bool half = d->bHalfSteps && rating%2;
    int numRatingStars = d->bHalfSteps ? rating/2 : rating;

    int numHoverStars = 0;
    bool halfHover = false;
    if ( hoverRating > 0 && rating != hoverRating ) {
        numHoverStars = d->bHalfSteps ? hoverRating/2 : hoverRating;
        halfHover = d->bHalfSteps && hoverRating%2;
        hoverPix = KIconEffect().apply( ratingPix, KIconEffect::ToGray, 0.5, QColor(), false );
    }

    int usedSpacing = 0;
    if ( d->alignment & Qt::AlignJustify ) {
        int w = rect.width();
        w -= numUsedStars * ratingPix.width();
        usedSpacing = w / ( numUsedStars-1 );
    }

    int i = 0;
    int x = rect.x();
    if ( d->alignment & Qt::AlignRight ) {
        x += ( rect.width() - ratingPix.width()*numUsedStars );
    }
    else if ( d->alignment & Qt::AlignCenter ) {
        x += ( rect.width() - ratingPix.width()*numUsedStars )/2;
    }

    int xInc = ratingPix.width() + usedSpacing;
    if ( d->direction == Qt::RightToLeft ) {
        x = rect.width() - ratingPix.width() - x;
        xInc = -xInc;
    }

    int y = rect.y();
    if( d->alignment & Qt::AlignVCenter ) {
        y += ( rect.height() / 2 - ratingPix.height() / 2 );
    }
    else if ( d->alignment & Qt::AlignBottom ) {
        y += ( rect.height() - ratingPix.height() );
    }
    for(; i < numRatingStars; ++i ) {
        painter->drawPixmap( x, y, ratingPix );
        x += xInc;
    }
    if( half ) {
        painter->drawPixmap( x, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? ratingPix : ( numHoverStars > 0 ? hoverPix : disabledRatingPix ),
                             0, 0, ratingPix.width()/2, ratingPix.height() );
        painter->drawPixmap( x + ratingPix.width()/2, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? ( numHoverStars > 0 ? hoverPix : disabledRatingPix ) : ratingPix,
                             ratingPix.width()/2, 0, ratingPix.width()/2, ratingPix.height() );
        x += xInc;
        ++i;
    }
    for(; i < numHoverStars; ++i ) {
        painter->drawPixmap( x, y, hoverPix );
        x += xInc;
    }
    if( halfHover ) {
        painter->drawPixmap( x, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? hoverPix : disabledRatingPix,
                             0, 0, ratingPix.width()/2, ratingPix.height() );
        painter->drawPixmap( x + ratingPix.width()/2, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? disabledRatingPix : hoverPix,
                             ratingPix.width()/2, 0, ratingPix.width()/2, ratingPix.height() );
        x += xInc;
        ++i;
    }
    for(; i < numUsedStars; ++i ) {
        painter->drawPixmap( x, y, disabledRatingPix );
        x += xInc;
    }
}


int Nepomuk::RatingPainter::fromPosition( const QRect& rect, const QPoint& pos )
{
    int numUsedStars = d->bHalfSteps ? d->maxRating/2 : d->maxRating;
    QPixmap ratingPix;
    if ( !d->customPixmap.isNull() ) {
        ratingPix = d->customPixmap;
    }
    else {
        KIcon ratingIcon( "rating" );
        int iconSize = qMin( rect.height(), rect.width() / numUsedStars );
        ratingPix = ratingIcon.pixmap( iconSize );
    }

    QRect usedRect( rect );
    if ( d->alignment & Qt::AlignRight ) {
        usedRect.setLeft( rect.right() - numUsedStars * ratingPix.width() );
    }
    else if ( d->alignment & Qt::AlignHCenter ) {
        int x = ( rect.width() - numUsedStars * ratingPix.width() )/2;
        usedRect.setLeft( rect.left() + x );
        usedRect.setRight( rect.right() - x );
    }
    else { // d->alignment & Qt::AlignLeft
        usedRect.setRight( rect.left() + numUsedStars * ratingPix.width() - 1 );
    }

    if ( d->alignment & Qt::AlignBottom ) {
        usedRect.setTop( rect.bottom() - ratingPix.height() + 1 );
    }
    else if ( d->alignment & Qt::AlignVCenter ) {
        int x = ( rect.height() - ratingPix.height() )/2;
        usedRect.setTop( rect.top() + x );
        usedRect.setBottom( rect.bottom() - x );
    }
    else { // d->alignment & Qt::AlignTop
        usedRect.setBottom( rect.top() + ratingPix.height() - 1 );
    }

    if ( usedRect.contains( pos ) ) {
        int x = 0;
        if ( d->direction == Qt::RightToLeft ) {
            x = usedRect.right() - pos.x();
        }
        else {
            x = pos.x() - usedRect.left();
        }

        double one = ( double )usedRect.width() / ( double )d->maxRating;

        kDebug() << "rating:" << ( int )( ( double )x/one + 0.5 );

        return ( int )( ( double )x/one + 0.5 );
    }
    else {
        return -1;
    }
}


void Nepomuk::RatingPainter::drawRating( QPainter* painter, const QRect& rect, Qt::Alignment align, int rating, int hoverRating )
{
    RatingPainter rp;
    rp.setAlignment( align );
    rp.setLayoutDirection( painter->layoutDirection() );
    rp.draw( painter, rect, rating, hoverRating );
}


int Nepomuk::RatingPainter::getRatingFromPosition( const QRect& rect, Qt::Alignment align, Qt::LayoutDirection direction, const QPoint& pos )
{
    RatingPainter rp;
    rp.setAlignment( align );
    rp.setLayoutDirection( direction );
    return rp.fromPosition( rect, pos );
}
