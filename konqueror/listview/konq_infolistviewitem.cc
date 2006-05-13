/* This file is part of the KDE project
   Copyright (C) 2002 Rolf Magnus <ramagnus@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation version 2.0

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/

#include "konq_listview.h"
#include <konq_settings.h>
#include <kfilemetainfo.h>
#include <kdebug.h>
#include <klocale.h>
#include <assert.h>
#include <stdio.h>
#include <QPainter>
#include <q3header.h>
#include <kiconloader.h>
#include "konq_infolistviewitem.h"
#include "konq_infolistviewwidget.h"
#include <QDateTime>

/**************************************************************
 *
 * KonqInfoListViewItem
 *
 **************************************************************/
KonqInfoListViewItem::KonqInfoListViewItem( KonqInfoListViewWidget *_widget, KonqInfoListViewItem * _parent, KFileItem* _fileitem )
:KonqBaseListViewItem( _widget,_parent,_fileitem ), m_ILVWidget(_widget)
{
    updateContents();
}

KonqInfoListViewItem::KonqInfoListViewItem( KonqInfoListViewWidget *_listViewWidget, KFileItem* _fileitem )
:KonqBaseListViewItem(_listViewWidget,_fileitem), m_ILVWidget(_listViewWidget)
{
   updateContents();
}

void KonqInfoListViewItem::updateContents()
{
    // Set the pixmap
    setDisabled( m_bDisabled );

    // Set the text of each column
    setText(0,m_fileitem->text());

#if 0
   if (S_ISDIR(m_fileitem->mode()))
       sortChar='0';
   //now we have the first column, so let's do the rest

   for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
      ColumnInfo *tmpColumn=&static_cast<KonqBaseListViewWidget *>(listView())->columnConfigInfo()[i];
      if (tmpColumn->displayThisOne)
      {
         switch (tmpColumn->udsId)
         {
         case KIO::UDS_USER:
            setText(tmpColumn->displayInColumn,m_fileitem->user());
            break;
         case KIO::UDS_GROUP:
            setText(tmpColumn->displayInColumn,m_fileitem->group());
            break;
         case KIO::UDS_FILE_TYPE:
            setText(tmpColumn->displayInColumn,m_fileitem->mimeComment());
            break;
         case KIO::UDS_MIME_TYPE:
            setText(tmpColumn->displayInColumn,m_fileitem->mimetype());
            break;
         case KIO::UDS_URL:
            setText(tmpColumn->displayInColumn,m_fileitem->url().prettyURL());
            break;
         case KIO::UDS_LINK_DEST:
            setText(tmpColumn->displayInColumn,m_fileitem->linkDest());
            break;
         case KIO::UDS_SIZE:
            if ( static_cast<KonqBaseListViewWidget *>(listView())->m_pSettings->fileSizeInBytes() )
                setText(tmpColumn->displayInColumn,KGlobal::locale()->formatNumber( m_fileitem->size(),0)+" ");
            else
                setText(tmpColumn->displayInColumn,KIO::convertSize(m_fileitem->size())+" ");
            break;
         case KIO::UDS_ACCESS:
            setText(tmpColumn->displayInColumn,makeAccessString(m_fileitem->permissions()));
            break;
         case KIO::UDS_MODIFICATION_TIME:
         case KIO::UDS_ACCESS_TIME:
         case KIO::UDS_CREATION_TIME:
            {
               QDateTime dt;
               time_t _time = m_fileitem->time( tmpColumn->udsId );
               if ( _time != 0 )
               {
                   dt.setTime_t( _time );
                   setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDateTime(dt));
               }
            }
            break;
         default:
            break;
         };
      };
   };
#endif
}

void KonqInfoListViewItem::gotMetaInfo()
{
    KFileMetaInfo info = item()->metaInfo(false);

    if (!info.isValid()) return;

    QStringList::ConstIterator it = m_ILVWidget->columnKeys().begin();
    for (int i = 1; it != m_ILVWidget->columnKeys().end(); ++it, ++i)
    {
        KFileMetaInfoItem kfmii = info.item(*it);

        m_columnTypes.append(kfmii.type());
        m_columnValues.append(kfmii.value());

        if (!kfmii.isValid())
            continue;

        QString s = kfmii.string().simplified();
        setText(i, s.isNull() ? QString("") : s);
    }
}

int KonqInfoListViewItem::compare( Q3ListViewItem *item, int col, bool ascending ) const
{
    if ( col == 0 )
        return KonqBaseListViewItem::compare( item, 0, ascending );

    KonqInfoListViewItem *i = static_cast<KonqInfoListViewItem *>(item);

    int size1 = m_columnValues.size();
    int size2 = i->m_columnValues.size();

    if ( size1 < col || size2 < col )
        return ascending ? ( size2 - size1 ) : ( size1 - size2 );

    QVariant value1 = m_columnValues[ col-1 ];
    QVariant value2 = i->m_columnValues[ col-1 ];
    QVariant::Type type1 = m_columnTypes[ col-1 ];
    QVariant::Type type2 = i->m_columnTypes[ col-1 ];

    if ( type1 != type2 )
        return ascending ? ( type1 - type2 ) : ( type2 - type1 );

#define KONQ_CASE( x ) \
    case QVariant::x:\
        return ( value1.to##x() > value2.to##x() ) ? 1 : (  value1.to##x() == value2.to##x() ) ? 0 : -1;

    switch( type1 ) {
    KONQ_CASE( Bool )
    KONQ_CASE( Int )
    KONQ_CASE( LongLong )
    KONQ_CASE( UInt )
    KONQ_CASE( ULongLong )
    KONQ_CASE( Double )
    KONQ_CASE( Date )
    KONQ_CASE( Time )
    KONQ_CASE( DateTime )
    case QVariant::Size:
    {
        int w1 = value1.toSize().width();
        int w2 = value2.toSize().width();
        if ( w1 != w2 )
            return ( w1 > w2 ) ? 1 : -1;
        int h1 = value1.toSize().height();
        int h2 = value2.toSize().height();
        return ( h1 > h2 ) ? 1 : ( h1 == h2 ) ? 0 : -1;
    }
    default:
        break;
    }
#undef KONQ_CASE

    QString text1 = text(col);
    QString text2 = i->text(col);

    if ( text1.isEmpty() )
        return ascending ? 1 : -1;
    if ( text2.isEmpty() )
        return ascending ? -1 : 1;

    return text1.toLower().localeAwareCompare(text2.toLower());
}

void KonqInfoListViewItem::setDisabled( bool disabled )
{
    KonqBaseListViewItem::setDisabled( disabled );
    int iconSize = static_cast<KonqBaseListViewWidget *>(listView())->iconSize();
    iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( K3Icon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

void KonqInfoListViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
    QColorGroup cg( _cg );

    if ( _column == 0 )
    {
        _painter->setFont( m_pListViewWidget->itemFont() );
    }

    cg.setColor( QPalette::Text, m_pListViewWidget->itemColor() );

    K3ListView *lv = static_cast< K3ListView* >( listView() );
    const QPixmap *pm = lv->viewport()->paletteBackgroundPixmap();
    if ( _column == 0 && isSelected() && !lv->allColumnsShowFocus() )
    {
        int newWidth = width( lv->fontMetrics(), lv, _column );
        if ( newWidth > _width )
            newWidth = _width;
        if ( pm && !pm->isNull() )
        {
            cg.setBrush( QPalette::Base, QBrush( backgroundColor(_column), *pm ) );
            QPoint o = _painter->brushOrigin();
            _painter->setBrushOrigin( o.x() - lv->contentsX(), o.y() - lv->contentsY() );
            const QPalette::ColorRole crole = lv->viewport()->backgroundRole();
            _painter->fillRect( newWidth, 0, _width - newWidth, height(), cg.brush( crole ) );
            _painter->setBrushOrigin( o );
        }
        else
        {
            _painter->fillRect( newWidth, 0, _width - newWidth, height(), backgroundColor(_column) );
        }

        _width = newWidth;
    }

    K3ListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

void KonqInfoListViewItem::paintFocus( QPainter * _painter, const QColorGroup & cg, const QRect & _r )
{
    QRect r( _r );
    Q3ListView *lv = static_cast< Q3ListView * >( listView() );
    r.setWidth( width( lv->fontMetrics(), lv, 0 ) );
    if ( r.right() > lv->header()->sectionRect( 0 ).right() )
        r.setRight( lv->header()->sectionRect( 0 ).right() );
    Q3ListViewItem::paintFocus( _painter, cg, r );
}

#if 0
void KonqBaseListViewItem::mimetypeFound()
{
    // Update icon
    setDisabled( m_bDisabled );
    uint done = 0;
    KonqBaseListViewWidget * lv = static_cast<KonqBaseListViewWidget*>(listView());
    for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms && done < 2; i++)
    {
        ColumnInfo *tmpColumn=&lv->columnConfigInfo()[i];
        if (lv->columnConfigInfo()[i].udsId==KIO::UDS_FILE_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimeComment());
            done++;
        }
        if (lv->columnConfigInfo()[i].udsId==KIO::UDS_MIME_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimetype());
            done++;
        }
    }
}

#endif
