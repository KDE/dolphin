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
#include "konq_listview.h"
#include <konq_fileitem.h>
#include <konq_settings.h>
#include <kdebug.h>
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
KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *, KonqListViewItem * _parent, KonqFileItem* _fileitem )
:KonqBaseListViewItem( _parent,_fileitem )
{
   updateContents();
}

KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqFileItem* _fileitem )
:KonqBaseListViewItem(_listViewWidget,_fileitem)
{
   updateContents();
}

void KonqListViewItem::updateContents()
{
   // Set the pixmap
   setDisabled( m_bDisabled );

   // Set the text of each column
   setText(0,m_fileitem->text());

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
                   setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDate(dt.date(),TRUE)+" "+KGlobal::locale()->formatTime(dt.time())+" ");
                   //setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDateTime(dt));
               }
            }
            break;
         default:
            break;
         };
      };
   };
}

void KonqListViewItem::setDisabled( bool disabled )
{
    KonqBaseListViewItem::setDisabled( disabled );
    int iconSize = static_cast<KonqBaseListViewWidget *>(listView())->iconSize();
    iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( KIcon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

QString KonqListViewItem::key( int _column, bool asc ) const
{
   QString tmp=sortChar;
   if (!asc && (sortChar=='0')) tmp=QChar('2');
   //check if it is a time or size column
   for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
     ColumnInfo *cInfo=&static_cast<KonqBaseListViewWidget *>(listView())->columnConfigInfo()[i];
     if (_column==cInfo->displayInColumn)
     {
       switch (cInfo->udsId)
       {
         case KIO::UDS_MODIFICATION_TIME:
         case KIO::UDS_ACCESS_TIME:
         case KIO::UDS_CREATION_TIME:
           return tmp + QString::number( m_fileitem->time(cInfo->udsId) ).rightJustify( 20, '0' );
         case KIO::UDS_SIZE:
           return tmp + QString::number( m_fileitem->size() ).rightJustify( 20, '0' );
         default:
           break;
       }
       break;
     }
   }
   tmp += static_cast<KonqBaseListViewWidget *>(listView())->caseInsensitiveSort() ? text(_column).lower() : text(_column);
   return tmp;
}

void KonqListViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
  QColorGroup cg( _cg );

  if ( _column == 0 )
  {
     _painter->setFont( static_cast<KonqBaseListViewWidget *>(listView())->itemFont() );
  }
  //else
  //   _painter->setPen( static_cast<KonqBaseListViewWidget *>(listView())->color() );

  cg.setColor( QColorGroup::Text, static_cast<KonqBaseListViewWidget *>(listView())->itemColor() );

  KListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

const char* KonqBaseListViewItem::makeAccessString( const mode_t mode)
{
   static char buffer[ 12 ];

   char uxbit,gxbit,oxbit;

   if ( (mode & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
      uxbit = 's';
   else if ( (mode & (S_IXUSR|S_ISUID)) == S_ISUID )
      uxbit = 'S';
   else if ( (mode & (S_IXUSR|S_ISUID)) == S_IXUSR )
      uxbit = 'x';
   else
      uxbit = '-';

   if ( (mode & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
      gxbit = 's';
   else if ( (mode & (S_IXGRP|S_ISGID)) == S_ISGID )
      gxbit = 'S';
   else if ( (mode & (S_IXGRP|S_ISGID)) == S_IXGRP )
      gxbit = 'x';
   else
      gxbit = '-';

   if ( (mode & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
      oxbit = 't';
   else if ( (mode & (S_IXOTH|S_ISVTX)) == S_ISVTX )
      oxbit = 'T';
   else if ( (mode & (S_IXOTH|S_ISVTX)) == S_IXOTH )
      oxbit = 'x';
   else
      oxbit = '-';

   buffer[0] = ((( mode & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
   buffer[1] = ((( mode & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
   buffer[2] = uxbit;
   buffer[3] = ((( mode & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
   buffer[4] = ((( mode & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
   buffer[5] = gxbit;
   buffer[6] = ((( mode & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
   buffer[7] = ((( mode & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
   buffer[8] = oxbit;
   buffer[9] = 0;

   return buffer;
}


KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget,KonqFileItem* _fileitem)
:KListViewItem(_listViewWidget)
,sortChar('1')
,m_bDisabled(false)
,m_fileitem(_fileitem)
{}

KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewItem *_parent,KonqFileItem* _fileitem)
:KListViewItem(_parent)
,sortChar('1')
,m_bDisabled(false)
,m_fileitem(_fileitem)
{}

QRect KonqBaseListViewItem::rect() const
{
    QRect r = listView()->itemRect(this);
    return QRect( listView()->viewportToContents( r.topLeft() ), QSize( r.width(), r.height() ) );
}

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

