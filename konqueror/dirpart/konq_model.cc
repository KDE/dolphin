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

// ####### NOT COMPILED ANYMORE, REPLACED WITH KDIRMODEL #######

#include "konq_model.h"

#include <QMimeData>
#include <QIcon>

#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>

KonqModel::KonqModel( QObject* parent ) : QAbstractTableModel( parent )
{
}

KonqModel::~KonqModel()
{
}

void KonqModel::clearFileItems()
{
    m_dirList.clear();
    m_fileList.clear();
}

void KonqModel::appendFileItems( const KFileItemList& items )
{
    KFileItemList::const_iterator kit = items.begin();
    const KFileItemList::const_iterator kend = items.end();
    for (; kit != kend; ++kit )
        if ( (*kit)->isDir() )
            m_dirList.append( *kit );
        else
            m_fileList.append( *kit );
}

KFileItem* KonqModel::fileItem( const QModelIndex& index ) const
{
    if ( !index.isValid() )
        return 0;
    int row = index.row();
    if ( row < m_dirList.count() )
        return m_dirList.at( row );
    else
        return m_fileList.at( row - m_dirList.count() );
}

void KonqModel::setItemFont( const QFont& font )
{
    m_font = font;
}

void KonqModel::setItemColor( const QColor& color )
{
    m_color = color;
}

void KonqModel::addPreview( const KFileItem* item, const QPixmap& pixmap )
{
    m_previews[ item ] = QIcon( pixmap );
}

QVariant KonqModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
         switch ( section ) {
            case 0:
                return QString( "Name" );
            case 1:
                return QString( "Size" );
            case 2:
                return QString( "File Type" );
            case 3:
                return QString( "User" );
            case 4:
                return QString( "Group" );
         }
    }
    else if ( role == Qt::TextAlignmentRole )
        if ( section == 1 )
            return int( Qt::AlignRight|Qt::AlignVCenter );


    return QVariant();
}

int KonqModel::rowCount( const QModelIndex& ) const
{
    return m_dirList.count() + m_fileList.count();
}

QVariant KonqModel::data( const QModelIndex& index, int role ) const
{
    if ( index.column() == 0 ) {
        if ( role == Qt::DecorationRole )
            if ( m_previews.contains( fileItem( index ) ) )
                return m_previews[ fileItem( index ) ];
            else
                return QIcon( fileItem( index )->pixmap( KGlobal::iconLoader()->currentSize( K3Icon::Desktop ) ) );
        else if ( role == Qt::FontRole )
            return m_font;
        else if ( role == Qt::TextColorRole )
            return m_color;
    }

    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        return data( fileItem( index ), index.column() );
    }
    else if ( role == Qt::TextAlignmentRole && index.column() == 1 )
        return int( Qt::AlignRight|Qt::AlignVCenter );

    return QVariant();
}

bool KonqModel::insertRows( int row, int count, const QModelIndex& )
{
    beginInsertRows( QModelIndex(), row, row + count - 1 );
    endInsertRows();
    return true;
}

bool KonqModel::setData( const QModelIndex& index, const QVariant& value, int )
{
    return true;
}

#define LESS_THAN( col )\
template <bool asc> static bool col##LessThan( const KFileItem* item1, const KFileItem* item2 )\
{\
    return lessThan( const_cast<KFileItem*>(item1)->col(),\
                     const_cast<KFileItem*>(item2)->col() ) ? asc : !asc;\
}

template<typename T> bool lessThan( const T& t1, const T& t2 )
{
    return t1 < t2;
}

template<> bool lessThan( const QString& s1, const QString& s2 )
{
    return s1.localeAwareCompare( s2 ) < 0;
}

LESS_THAN( text )
LESS_THAN( size )
LESS_THAN( mimeComment )
LESS_THAN( user )
LESS_THAN( group )

typedef bool (*LessThan)( const KFileItem*, const KFileItem* );

#define MAKE( col )\
    { &col##LessThan<true>, &col##LessThan<false> }

static LessThan columns[ 5 ][ 2 ] =
    { MAKE( text ), MAKE( size ), MAKE( mimeComment ), MAKE( user ), MAKE( group ) };

void KonqModel::sort( int column, Qt::SortOrder order )
{
    LessThan lessThan = columns[ column ][ order == Qt::AscendingOrder ? 0 : 1 ];
    qSort( m_dirList.begin(), m_dirList.end(), lessThan );
    qSort( m_fileList.begin(), m_fileList.end(), lessThan );

    reset();
}

QVariant KonqModel::data( KFileItem* item, int column ) const
{
    switch ( column ) {
        default:
        case 0:
            return item->text();
        case 1:
            return item->size();
        case 2:
            return item->mimeComment();
        case 3:
            return item->user();
        case 4:
            return item->group();
    }
}

Qt::ItemFlags KonqModel::flags( const QModelIndex& index ) const
{
    if ( !index.isValid() || index.column() != 0 )
        return 0;
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
    if ( fileItem( index )->isDir() )
        flags |= Qt::ItemIsDropEnabled;
    return flags;
}

QStringList KonqModel::mimeTypes() const
{
    return QStringList() << QLatin1String( "text/uri-list" )
                         << QLatin1String( "application/x-kde-cutselection" )
                         << QLatin1String( "text/plain" )
                         << QLatin1String( "application/x-kde-urilist" );
}

QMimeData* KonqModel::mimeData( const QModelIndexList& indexes ) const
{
    KUrl::List urls;
    foreach ( QModelIndex index, indexes ) {
        urls << fileItem( index )->url();
    }
    QMimeData *data = new QMimeData();
    urls.populateMimeData( data );
    return data;
}

// ####### NOT COMPILED ANYMORE, REPLACED WITH KDIRMODEL #######

#include "konq_model.moc"
