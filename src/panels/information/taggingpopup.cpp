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

#include "taggingpopup.h"

#include <QtCore/QEventLoop>
#include <QtCore/QPointer>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMouseEvent>

#include <KDebug>
#include <KDialog>


class Nepomuk::TaggingPopup::Private
{
public:
    Private( TaggingPopup* parent )
        : eventLoop( 0 ),
          m_parent( parent ) {
    }

    QEventLoop* eventLoop;
    QPoint popupPos;

    QRect geometryForPopupPos( const QPoint& p ) {
        QSize size = m_parent->sizeHint();

        // we want a little margin
        const int margin = KDialog::marginHint();
        size.setHeight( size.height() + margin*2 );
        size.setWidth( size.width() + margin*2 );

        QRect screen = QApplication::desktop()->screenGeometry( QApplication::desktop()->screenNumber( p ) );

        // calculate popup position
        QPoint pos( p.x() - size.width()/2, p.y() - size.height()/2 );

        // ensure we do not leave the desktop
        if ( pos.x() + size.width() > screen.right() ) {
            pos.setX( screen.right() - size.width() );
        }
        else if ( pos.x() < screen.left() ) {
            pos.setX( screen.left() );
        }

        if ( pos.y() + size.height() > screen.bottom() ) {
            pos.setY( screen.bottom() - size.height() );
        }
        else if ( pos.y() < screen.top() ) {
            pos.setY( screen.top() );
        }

        return QRect( pos, size );
    }

private:
    TaggingPopup* m_parent;
};


Nepomuk::TaggingPopup::TaggingPopup( QWidget* parent )
    : TagCloud( parent ),
      d( new Private( this ) )
{
    setFrameStyle( QFrame::StyledPanel );
    setWindowFlags( Qt::Popup );
}


Nepomuk::TaggingPopup::~TaggingPopup()
{
    delete d;
}


void Nepomuk::TaggingPopup::popup( const QPoint& p )
{
    setGeometry( d->geometryForPopupPos( p ) );
    d->popupPos = p;

    show();
}


void Nepomuk::TaggingPopup::exec( const QPoint& pos )
{
    QEventLoop eventLoop;
    d->eventLoop = &eventLoop;
    popup( pos );

    QPointer<QObject> guard = this;
    (void) eventLoop.exec();
    if ( !guard.isNull() )
        d->eventLoop = 0;
}


void Nepomuk::TaggingPopup::mousePressEvent( QMouseEvent* e )
{
    if ( !rect().contains( e->pos() ) ) {
        hide();
    }
    else {
        TagCloud::mousePressEvent( e );
    }
}


void Nepomuk::TaggingPopup::hideEvent( QHideEvent* e )
{
    Q_UNUSED( e );
    if ( d->eventLoop ) {
        d->eventLoop->exit();
    }
}


bool Nepomuk::TaggingPopup::event( QEvent* e )
{
    if ( e->type() == QEvent::LayoutRequest ) {
        if ( isVisible() ) {
            setGeometry( d->geometryForPopupPos( d->popupPos ) );
            return true;
        }
    }

    return TagCloud::event( e );
}

