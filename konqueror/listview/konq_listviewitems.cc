/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_propsview.h"
#include "konq_listviewitems.h"
#include "konq_listviewwidget.h"
#include <konqfileitem.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <assert.h>
#include <stdio.h>

/**************************************************************
 *
 * KonqListViewItem
 *
 **************************************************************/
//KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqListViewDir * _parent, KonqFileItem* _fileitem )
KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqListViewItem * _parent, KonqFileItem* _fileitem )
:KonqBaseListViewItem( _parent,_fileitem )
{
   m_pListViewWidget = _listViewWidget;
   init();
}

KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqFileItem* _fileitem )
:KonqBaseListViewItem(_listViewWidget,_fileitem)
{
   m_pListViewWidget = _listViewWidget;
   init();
}

void KonqListViewItem::init()
{
   setPixmap( 0, m_fileitem->pixmap( KIconLoader::Small, false /*no image preview*/ ) );
   // Set the text of each column
   const KIO::UDSEntry & entry = m_fileitem->entry();
   KIO::UDSEntry::ConstIterator it = entry.begin();
   for( ; it != entry.end(); it++ )
   {
      const KIO::UDSAtom & atom = (*it);
      int * pColumn = m_pListViewWidget->columnForAtom( atom.m_uds );
      if ( pColumn )
      {
         // We want to put that atom in the column *pColumn
         // Now convert the text
         QString text;
         switch (atom.m_uds)
         {
         case KIO::UDS_ACCESS:
            text = makeAccessString( (mode_t) atom.m_long );
            break;
         case KIO::UDS_FILE_TYPE :
            text = makeTypeString( atom );
            break;
         case KIO::UDS_NAME :
            text = m_fileitem->text();
            break;
         default:
            // Otherwise use the type
            switch (atom.m_uds & (KIO::UDS_TIME|KIO::UDS_STRING|KIO::UDS_LONG) )  {
            case KIO::UDS_TIME :
               text = makeTimeString( atom );
               break;
            case KIO::UDS_STRING :
               text = atom.m_str;
               break;
            case KIO::UDS_LONG :
               text = makeNumericString( atom );
               break;
            }
         }
         setText( *pColumn, text );
      }
   }
}

QString KonqListViewItem::key( int _column, bool ) const
{
  // TODO return a judicious key for dates

  /*
  static char buffer[ 12 ];
  unsigned long uds = m_fileitem->entry()[ _column ].m_uds;

  if ( uds & KIO::UDS_STRING )
    return m_fileitem->entry()[ _column ].m_str;
  else if ( uds & KIO::UDS_LONG )
  {
    sprintf( buffer, "%08d", (int)m_fileitem->entry()[ _column ].m_long );
    return buffer;
  }
  else
    assert( 0 );
    */
  return text( _column );
}

QString KonqListViewItem::makeNumericString( const KIO::UDSAtom &_atom ) const
{
  return KGlobal::locale()->formatNumber( _atom.m_long, 0);
}

QString KonqListViewItem::makeTimeString( const KIO::UDSAtom &_atom ) const
{
   QDateTime dt; dt.setTime_t((time_t) _atom.m_long);
   return KGlobal::locale()->formatDate(dt.date(), true) + " " +
      KGlobal::locale()->formatTime(dt.time());
}

QString KonqListViewItem::makeTypeString( const KIO::UDSAtom &_atom ) const
{
   mode_t mode = (mode_t) _atom.m_long;

   if ( S_ISLNK( mode ) )
      return i18n( "Link" );
   else if ( S_ISDIR( mode ) )
      return i18n( "Directory" );
   // TODO check for devices here, too
   else
      return i18n( "File" );
}

void KonqListViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
  QColorGroup cg( _cg );

  if ( _column == 0 ) {
    _painter->setFont( m_pListViewWidget->itemFont() );
    cg.setColor( QColorGroup::Text, m_pListViewWidget->itemColor() );
  }
  else
    _painter->setPen( m_pListViewWidget->color() );

  if (!m_pListViewWidget->props()->bgPixmap().isNull())
  {
    _painter->drawTiledPixmap( 0, 0, _width, height(),
                               m_pListViewWidget->props()->bgPixmap(),
                               0, 0 ); // ?
  }

  // Now prevent QListViewItem::paintCell from drawing a white background
  // I hope color0 is transparent :-))
  //cg.setColor( QColorGroup::Base, Qt::color0 );

  QListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

