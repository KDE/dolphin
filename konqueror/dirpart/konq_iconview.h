/* This file is part of the KDE project
   Copyright (c) 2005 Pascal Létourneau <pascal.letourneau@kdemail.net>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_iconview_h__
#define __konq_iconview_h__

#include <QListView>

class KonqFileTip;

class KonqIconView : public QListView
{
Q_OBJECT
public:
    KonqIconView( QWidget* parent );
    ~KonqIconView();
protected:
    virtual bool viewportEvent( QEvent* ev );
    virtual void contextMenuEvent( QContextMenuEvent* ev );
    virtual void mouseReleaseEvent( QMouseEvent* ev );
    virtual void keyPressEvent( QKeyEvent* ev );
    virtual QStyleOptionViewItem viewOptions() const;
Q_SIGNALS:
    void toolTip( const QModelIndex& index );
    void contextMenu( const QPoint& pos, const QModelIndexList& indexes );
    void execute( const QModelIndex& index, Qt::MouseButton mb );
};

#endif
