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

#ifndef _NEPOMUK_TAGGING_POPUP_H_
#define _NEPOMUK_TAGGING_POPUP_H_

#include "tagcloud.h"

class QMouseEvent;
class QHideEvent;

namespace Nepomuk {
    class TaggingPopup : public TagCloud
    {
    public:
        TaggingPopup( QWidget* parent = 0 );
        ~TaggingPopup();

        void popup( const QPoint& pos );
        void exec( const QPoint& pos );

	bool event( QEvent* e );

    protected:
        void mousePressEvent( QMouseEvent* e );
        void hideEvent( QHideEvent* e );

    private:
        class Private;
        Private* const d;
    };
}

#endif
