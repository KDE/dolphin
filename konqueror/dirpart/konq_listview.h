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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_listview_h__
#define __konq_listview_h__

#include <QTreeView>

class KonqFileTip;

class KonqListView : public QTreeView
{
    friend class KonqPart;
    Q_OBJECT
public:
    KonqListView( QWidget* parent );
    ~KonqListView();
    virtual void setSelectionModel( QItemSelectionModel* selectionModel );
    bool isExecutableArea( const QPoint& p );
protected:
    virtual bool viewportEvent( QEvent* ev );
    virtual void contextMenuEvent( QContextMenuEvent* ev );
    virtual void mouseReleaseEvent( QMouseEvent* ev );
    virtual void keyPressEvent( QKeyEvent* ev );
signals:
    void toolTip( const QModelIndex& index );
    void contextMenu( const QPoint& pos, const QModelIndexList& indexes );
    void execute( const QModelIndex& index, Qt::MouseButton mb );
private slots:
    void slotCurrentChanged( const QModelIndex& index, const QModelIndex& );
};

#endif
