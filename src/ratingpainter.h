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

#ifndef _NEPOMUK_RATING_PAINTER_H_
#define _NEPOMUK_RATING_PAINTER_H_

class QPainter;

#include <QtCore/QRect>
#include <QtCore/QPoint>

#include <kicon.h>

#include <nepomuk/nepomuk_export.h>


namespace Nepomuk {
    /**
     * Utility class that draws a row of stars for a rating value.
     */
    class NEPOMUK_EXPORT RatingPainter
    {
    public:
        /**
         * Create a new RatingPainter.
         * Normally the static methods should be sufficient.
         */
        RatingPainter();
        ~RatingPainter();

        int maxRating() const;
        bool halfStepsEnabled() const;
        Qt::Alignment alignment() const;
        Qt::LayoutDirection direction() const;
        KIcon icon() const;
        QPixmap customPixmap() const;

        /**
         * The maximum rating. Defaults to 10.
         */
        void setMaxRating( int max );

        /**
         * If half steps are enabled (the default) then
         * one rating step corresponds to half a star.
         */
        void setHalfStepsEnabled( bool enabled );
        
        /**
         * The alignment of the stars in the drawing rect.
         * All alignment flags are supported.
         */
        void setAlignment( Qt::Alignment align );

        /**
         * LTR or RTL
         */
        void setLayoutDirection( Qt::LayoutDirection direction );

        /**
         * Set a custom icon. Defaults to "rating".
         */
        void setIcon( const KIcon& icon );

        /**
         * Set a custom pixmap.
         */
        void setCustomPixmap( const QPixmap& pixmap );

        /**
         * Draw the rating into the configured rect.
         */
        void draw( QPainter* painter, const QRect& rect, int rating, int hoverRating = -1 );

        /**
         * Calculate the rating value from mouse position pos.
         *
         * \return The rating corresponding to pos or -1 if pos is
         * outside of the configured rect.
         */
        int fromPosition( const QRect& rect, const QPoint& pos );

        /**
         * Convenience method that draws a rating into the given rect.
         *
         * LayoutDirection is read from QPainter.
         *
         * \param align can be aligned vertically and horizontally. Using Qt::AlignJustify will insert spacing
         * between the stars.
         */
        static void drawRating( QPainter* p, const QRect& rect, Qt::Alignment align, int rating, int hoverRating = -1 );

        /**
         * Get the rating that would be selected if the user clicked position pos
         * within rect if the rating has been drawn with drawRating() using the same
         * rect and align values.
         *
         * \return The new rating or -1 if pos is outside of the rating area.
         */
        static int getRatingFromPosition( const QRect& rect, Qt::Alignment align, Qt::LayoutDirection direction, const QPoint& pos );

    private:
        class Private;
        Private* const d;
    };
}

#endif
