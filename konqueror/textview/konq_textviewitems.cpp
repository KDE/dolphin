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

/*#include "konq_propsview.h"*/
#include "konq_textviewitems.h"
/*#include "konq_textviewwidget.h"
#include <kfileitem.h>
#include <kio/job.h>
#include <klocale.h>
#include <assert.h>
#include <stdio.h>*/

/**************************************************************
 *
 * KonqTextViewItem
 *
 **************************************************************/
/*KonqTextViewItem::KonqTextViewItem( KonqTextViewWidget *_parent, KFileItem* _fileitem)
   :QListViewItem( _parent )
   ,sortChar('1')
   ,m_fileitem( _fileitem )
   ,m_pTextView (_parent)
{
   setText( 1, m_fileitem->text());

   //setText( 3, m_fileitem->time(KIO::UDS_MODIFICATION_TIME));

   for( KIO::UDSEntry::ConstIterator it = m_fileitem->entry().begin(); it != m_fileitem->entry().end(); it++ )
   {
      if ((*it).m_uds==KIO::UDS_MODIFICATION_TIME)
      {
         QDateTime dt;
         dt.setTime_t((time_t) (*it).m_long);
         //setText(3,dt.toString());
         setText(3,m_pTextView->locale->formatDate(dt.date(),true)+" "+m_pTextView->locale->formatTime(dt.time()));
         break;
      };
   };
   long size(m_fileitem->size());
   if (size>=0)
   {
      QString tmp;
      tmp.sprintf("%9d",size);
      setText( 2, tmp );
      mode_t m=m_fileitem->mode();
      if (m_fileitem->isLink())
      {
         if (S_ISDIR(m))
         {
            sortChar='0';
            type=DIRLINK;
            tmp="~";
         }
         else
         {
            tmp="@";
            type=REGULARLINK;
         };
      }
      else if (S_ISREG(m))
      {
         if ((m_fileitem->permissions() & (S_IXUSR|S_IXGRP|S_IXOTH)) !=0 )
         {
            tmp="*";
            type=EXEC;
         }
         else
         {
            tmp="";
            type=REGULAR;
         };
      }
      else if (S_ISDIR(m))
      {
         type=DIR;
         tmp="/";
         sortChar='0';
      }
      else if (S_ISCHR(m))
      {
         type=CHARDEV;
         tmp="-";
      }
      else if (S_ISBLK(m))
      {
         type=BLOCKDEV;
         tmp="+";
      }
      else if (S_ISSOCK(m))
      {
         type=SOCKET;
         tmp="=";
      }
      else if (S_ISFIFO(m))
      {
         type=FIFO;
         tmp=">";
      }
      else
      {
         tmp="!";
         type=UNKNOWN;
      };
      setText(0,tmp);
   }
   else
   {
      setText(2,"???");
      setText(0,"?");
   };
};

QString KonqTextViewItem::key( int _column, bool asc) const
{
   if (_column==0) return key(1,asc);
   QString tmp=sortChar;
   if (!asc && (sortChar=='0')) tmp=QChar('2');
   tmp+=text(_column);
   return tmp;
};

void KonqTextViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
   if (!m_pTextView->props()->bgPixmap().isNull())
      _painter->drawTiledPixmap( 0, 0, _width, height(),m_pTextView->props()->bgPixmap(),0, 0 );
   QColorGroup cg( _cg );
   cg.setColor(QColorGroup::Text, m_pTextView->colors[type]);
   //         cg.setColor( QColorGroup::Base, Qt::color0 );
   QListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
};*/

/*KonqTextViewItem::KonqTextViewItem( KonqTextViewWidget *_parent, KFileItem* _fileitem )
:QListViewItem( _parent )
,sortChar('1')
,m_fileitem( _fileitem )
,m_pTextView (_parent)
{
   setText( 1, m_fileitem->text());
   setText( 3, m_fileitem->time(KIO::UDS_MODIFICATION_TIME));
   long size(m_fileitem->size());
   if (size>=0)
   {
      QString tmp;
      tmp.sprintf("%9d",m_fileitem->size());
      setText( 2, tmp );
      mode_t m=m_fileitem->mode();
      if (m_fileitem->isLink())
      {
         if (S_ISDIR(m))
         {
            sortChar='0';
            tmp="~";
         }
         else tmp="@";
      }
      else if (S_ISREG(m))
      {
         if (((m & (S_IXUSR|S_ISUID)) != 0)
             ||((m & (S_IXGRP|S_ISGID)) != 0)
             ||((m & (S_IXOTH|S_ISVTX)) !=0 ))
            tmp="*";
         else tmp="";
      }
      else if (S_ISDIR(m))
      {
         tmp="/";
         sortChar='0';
      }
      else if (S_ISCHR(m)) tmp="-";
      else if (S_ISBLK(m)) tmp="+";
      else if (S_ISSOCK(m)) tmp="=";
      else if (S_ISFIFO(m)) tmp=">";
      else tmp="?";
      setText(0,tmp);
   }
   else
   {
      setText(2,"???");
      setText(0,"!");
   };
};*/

/*const char* KonqTextViewItem::makeAccessString( mode_t mode) const
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
}*/

QString KonqTextViewItem::key( int _column, bool asc) const
{
   if (_column==0) return key(1,asc);
   QString tmp=sortChar;
   if (!asc && (sortChar=='0')) tmp=QChar('2');
   tmp+=text(_column);
   return tmp;
};

void KonqTextViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
   if (!m_pTextView->props()->bgPixmap().isNull())
      _painter->drawTiledPixmap( 0, 0, _width, height(),m_pTextView->props()->bgPixmap(),0, 0 );
   QColorGroup cg( _cg );
   cg.setColor(QColorGroup::Text, m_pTextView->colors[type]);
   cg.setColor(QColorGroup::HighlightedText, m_pTextView->highlight[type]);
   cg.setColor(QColorGroup::Highlight, Qt::darkGray);
   
   //         cg.setColor( QColorGroup::Base, Qt::color0 );
   QListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
};

void KonqTextViewItem::setup()
{
   widthChanged();
   int h(listView()->fontMetrics().height());
   if ( h % 2 > 0 ) h++;
   setHeight(h);
};

/*void KonqTextViewItem::setup()
{
   const QListView* lv=listView();
   lv->widthChanged( this, -1);
   int h(lv->fontMetrics().height());
   if ( h % 2 > 0 ) h++;
   setHeight(h);
};

/*void KonqTextViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment)
{
   if (!m_pTextView->props()->bgPixmap().isNull())
   {
      _painter->drawTiledPixmap( 0, 0, _width, height(),
                                 m_pTextView->props()->bgPixmap(),
                                 0, 0 ); // ?
   }
   QColorGroup cg( _cg );
   cg.setColor( QColorGroup::Base, Qt::color0 );
   QListViewItem::paintCell( _painter, _cg, _column, _width, _alignment );
}*/
