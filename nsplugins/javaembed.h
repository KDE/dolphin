/****************************************************************************
    Definition of QXEmbed class

   Copyright (C) 1999-2000 Troll Tech AS

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*****************************************************************************/

#ifndef KJAVAEMBED_H
#define KJAVAEMBED_H

#include <qwidget.h>

class KJavaEmbedPrivate;
class KJavaEmbed : public QWidget
{
    Q_OBJECT

public:

    KJavaEmbed( QWidget *parent=0, const char *name=0, WFlags f = 0 );
    ~KJavaEmbed();

    void embed( WId w );
    bool embedded() { if( window != 0 ) return true; else return false; }

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    QSizePolicy sizePolicy() const;

    bool eventFilter( QObject *, QEvent * );

protected:
    bool event( QEvent * );

    void focusInEvent( QFocusEvent * );
    void focusOutEvent( QFocusEvent * );
    void resizeEvent(QResizeEvent *);

    //void showEvent( QShowEvent * );

    bool x11Event( XEvent* );

    bool focusNextPrevChild( bool next );

private:
    WId window;
    KJavaEmbedPrivate* d;
};


#endif
