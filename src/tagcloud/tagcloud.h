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

#ifndef _NEPOMUK_TAG_CLOUD_H_
#define _NEPOMUK_TAG_CLOUD_H_

#include <QtGui/QFrame>
#include <QtCore/QList>

#include <nepomuk/tag.h>
#include <nepomuk/nepomuk_export.h>

#include <Soprano/Statement>

class QResizeEvent;
class QPaintEvent;
class QMouseEvent;
class QEvent;

namespace Nepomuk {
    class NEPOMUK_EXPORT TagCloud : public QFrame
    {
        Q_OBJECT

    public:
        TagCloud( QWidget* parent = 0 );
        ~TagCloud();

        enum Sorting {
            SortAlpabetically,
            SortByWeight,
            SortRandom
        };

        int heightForWidth( int w ) const;
        QSize sizeHint() const;
        QSize minimumSizeHint() const;

        bool zoomEnabled() const;

    public Q_SLOTS:
        /**
         * Set the maximum used font size. The default is 0
         * which means to calculate proper values from the KDE
         * defaults.
         */
        void setMaxFontSize( int size );

        /**
         * Set the minimum used font size. The default is 0
         * which means to calculate proper values from the KDE
         * defaults.
         */
        void setMinFontSize( int size );

        /**
         * Set the maximum number of displayed tags. The default is 0
         * which means to display all tags.
         *
         * NOT IMPLEMENTED YET
         */
        void setMaxNumberDisplayedTags( int n );

        /**
         * Allow selection of tags, i.e. enabling and disabling of tags.
         */
        void setSelectionEnabled( bool enabled );

        void setNewTagButtonEnabled( bool enabled );
        void setContextMenuEnabled( bool enabled );
        void setAlignment( Qt::Alignment alignment );

        void setZoomEnabled( bool zoom );

        /**
         * Default: SortAlpabetically
         */
        void setSorting( Sorting );

        /**
         * Will reset tags set via showTags()
         */
        void showAllTags();

        /**
         * Set the tags to be shown in the tag cloud.
         * If the new tag button is enabled (setEnableNewTagButton())
         * new tags will automatically be added to the list of shown tags.
         */
        void showTags( const QList<Tag>& tags );

        void showResourceTags( const Resource& resource );

        /**
         * Select or deselect a tag. This does only make sense
         * if selection is enabled and \p tag is actually
         * displayed.
         *
         * \sa setSelectionEnabled
         */
        void setTagSelected( const Tag& tag, bool selected );

        void setCustomNewTagAction( QAction* action );

    Q_SIGNALS:
        void tagClicked( const Nepomuk::Tag& tag );
        void tagAdded( const Nepomuk::Tag& tag );
        void tagToggled( const Nepomuk::Tag& tag, bool enabled );

    protected:
        void resizeEvent( QResizeEvent* e );
        void paintEvent( QPaintEvent* e );
        void mousePressEvent( QMouseEvent* );
        void mouseMoveEvent( QMouseEvent* );
        void leaveEvent( QEvent* );

    private Q_SLOTS:
        void slotStatementAdded( const Soprano::Statement& s );
        void slotStatementRemoved( const Soprano::Statement& s );

    private:
        class Private;
        Private* const d;
    };
}

#endif
