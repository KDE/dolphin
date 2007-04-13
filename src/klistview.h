/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef __KLISTVIEW_H__
#define __KLISTVIEW_H__

#include <QListView>

#include <libdolphin_export.h>

class KItemCategorizer;

class LIBDOLPHINPRIVATE_EXPORT KListView
    : public QListView
{
    Q_OBJECT

public:
    KListView(QWidget *parent = 0);

    ~KListView();

    void setModel(QAbstractItemModel *model);

    QRect visualRect(const QModelIndex &index) const;

    KItemCategorizer *itemCategorizer() const;

    void setItemCategorizer(KItemCategorizer *itemCategorizer);

    virtual QModelIndex indexAt(const QPoint &point) const;

    virtual int sizeHintForRow(int row) const;


protected:
    virtual void drawNewCategory(const QString &category,
                                 const QStyleOptionViewItem &option,
                                 QPainter *painter);

    virtual int categoryHeight(const QStyleOptionViewItem &option) const;

    virtual void paintEvent(QPaintEvent *event);

    virtual void setSelection(const QRect &rect,
                              QItemSelectionModel::SelectionFlags flags);

    virtual void timerEvent(QTimerEvent *event);


protected Q_SLOTS:
    virtual void rowsInserted(const QModelIndex &parent,
                              int start,
                              int end);

    virtual void rowsAboutToBeRemoved(const QModelIndex &parent,
                                      int start,
                                      int end);

    virtual void rowsInsertedArtifficial(const QModelIndex &parent,
                                         int start,
                                         int end);

    virtual void rowsAboutToBeRemovedArtifficial(const QModelIndex &parent,
                                                 int start,
                                                 int end);

    virtual void itemsLayoutChanged();


private:
    class Private;
    Private *d;
};

#endif // __KLISTVIEW_H__
