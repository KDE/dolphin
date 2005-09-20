/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __bookmarklistview_h
#define __bookmarklistview_h

#include <QTreeView>
#include <QVector>
#include <kbookmark.h>

struct SelcAbilities {
    bool itemSelected:1;
    bool group:1;
    bool root:1;
    bool separator:1;
    bool urlIsEmpty:1;
    bool multiSelect:1;
    bool singleSelect:1;
    bool notEmpty:1;
};

class BookmarkListView;

class BookmarkView : public QTreeView
{
    Q_OBJECT
public:
    BookmarkView( QWidget * parent = 0 );
    virtual ~BookmarkView();
    virtual void setModel(QAbstractItemModel * view);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
public slots:
    void aboutToMoveRows(const QModelIndex & oldParent, int first, int last, const QModelIndex & newParent, int position);
    void rowsMoved(const QModelIndex & oldParent, int first, int last, const QModelIndex & newParent, int position);
private:
    QPersistentModelIndex moveOldParent;
    QPersistentModelIndex moveNewParent;
};

class BookmarkFolderView : public BookmarkView
{
    Q_OBJECT
public:
    BookmarkFolderView( BookmarkListView * view, QWidget * parent = 0 );
    virtual ~BookmarkFolderView();
    virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
private:
    BookmarkListView * mview;
};

class BookmarkListView : public BookmarkView
{
    Q_OBJECT
public:
    BookmarkListView( QWidget * parent = 0 );
    virtual ~BookmarkListView();
    virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
    virtual void drawRow ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    virtual QItemSelectionModel::SelectionFlags selectionCommand ( const QModelIndex & index, const QEvent * event = 0 ) const;
    SelcAbilities getSelectionAbilities() const;
    void loadColumnSetting();
    void saveColumnSetting ();

protected:
    virtual void contextMenuEvent ( QContextMenuEvent * e );
private:
    int min(int a, int b);
    int max(int a, int b);
    QRect merge(QRect a, QRect b);
    void deselectChildren(const QModelIndex & parent);
    QRect BookmarkListView::rectForRow(QModelIndex index);
    QRect BookmarkListView::rectForRowWithChildren(QModelIndex index);
    bool parentSelected(const QModelIndex & index ) const;
    mutable SelcAbilities selctionAbilities;
    mutable bool dirtyGetSelectionAbilies;
};
#endif
