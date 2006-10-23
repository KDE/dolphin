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

#ifndef __konq_model_h__
#define __konq_model_h__

#include <QAbstractTableModel>

#include <kfileitem.h>

#include <QHash>
#include <QFont>


// ####### NOT COMPILED ANYMORE, REPLACED WITH KDIRMODEL

class KonqModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    KonqModel( QObject* );
    ~KonqModel();
    void clearFileItems();
    void appendFileItems( const KFileItemList& items );
    KFileItem* fileItem( const QModelIndex& index ) const;
    void setItemFont( const QFont& font );
    void setItemColor( const QColor& color );
    void addPreview( const KFileItem* item, const QPixmap& pixmap );

    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const { return 5; }
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual bool setData( const QModelIndex& index, const QVariant& value, int role=Qt::EditRole );
    virtual bool insertRows( int row, int count, const QModelIndex& parent = QModelIndex() );
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder );
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData* mimeData ( const QModelIndexList& indexes ) const;
private:
    QList<KFileItem*> m_dirList;
    QList<KFileItem*> m_fileList;
    QFont m_font;
    QColor m_color;
    QHash<const KFileItem*,QIcon> m_previews;
    QVariant data( KFileItem* item, int column ) const;
};

#endif
