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

#ifndef __konq_textviewitems_h__
#define __konq_textviewitems_h__

#include <qlistview.h>
#include <qstring.h>
#include <kio/global.h>

class KMimeType;
class KonqFileItem;
class KonqTextViewDir;
class QPainter;

#include "konq_textviewwidget.h"
#include "konq_propsview.h"
#include <klocale.h>
#include <qdatetime.h>

/**
 * One item in the text
 */
#include <iostream.h>

class KonqTextViewItem : public QListViewItem
{
   public:
      /**
       * Create an item in the text toplevel representing a file
       * @param _parent the parent widget, the text view
       * @param _fileitem the file item created by KDirLister
       */
      KonqTextViewItem( KonqTextViewWidget *_parent, KonqFileItem* _fileitem);
      virtual ~KonqTextViewItem() {/*cerr<<"~KonqTextViewItem: "<<text(1)<<endl;*/ };
      virtual QString key( int _column, bool asc) const;
      /** Call this before destroying the text view (decreases reference count
       * on the view)*/
      //virtual void prepareToDie() { m_pTextView = 0L; }
      virtual void paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment );
      /** @return the file item held by this instance */
      KonqFileItem * item() { return m_fileitem; }

   protected:
      QChar sortChar;
      virtual void setup();
      const char* makeAccessString( mode_t mode ) const;

      /** Pointer to the file item in KDirLister's list */
      KonqFileItem* m_fileitem;
      /** Parent text view */
      KonqTextViewWidget* m_pTextView;
      int type;
};


inline KonqTextViewItem::KonqTextViewItem( KonqTextViewWidget *_parent, KonqFileItem* _fileitem)
:QListViewItem( _parent )
,sortChar('1')
,m_fileitem( _fileitem )
,m_pTextView (_parent)
{
   QString tmp;
   long int size=m_fileitem->size();
   mode_t m=m_fileitem->mode();
   if (m_fileitem->isLink())
   {
      if (S_ISDIR(m))
      {
         sortChar='0';
         type=DIRLINK;
         tmp="~";
      }
      else if (S_ISREG(m))
      {
         tmp="@";
         type=REGULARLINK;
      }
      else
      {
         tmp="!";
         type=UNKNOWN;
         size=-1;
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
      size=-1;
   };
   setText(0,tmp);

   setText(1,m_fileitem->text());

   int nextColumn(2);
   
   if (m_pTextView->m_showSize)
   {
      tmp.sprintf("%9d",size);
      setText(nextColumn, tmp );
      nextColumn++;
   };

   if (m_pTextView->m_showTime)
   {
      for( KIO::UDSEntry::ConstIterator it = m_fileitem->entry().begin(); it != m_fileitem->entry().end(); it++ )
      {
         if ((*it).m_uds==KIO::UDS_MODIFICATION_TIME)
         {
            QDateTime dt;
            dt.setTime_t((time_t) (*it).m_long);
            //setText(3,dt.toString());
            setText(nextColumn,m_pTextView->locale->formatDate(dt.date(),true)+" "+m_pTextView->locale->formatTime(dt.time()));
            break;
         };

      };
      nextColumn++;
   };

   if (m_pTextView->m_showOwner)
   {
      setText(nextColumn,m_fileitem->user());
      nextColumn++;
   };
//   int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
//   cerr<<" set to: "<<permissionsWidth<<endl;

   if (m_pTextView->m_showOwner)
   {
      setText(nextColumn,m_fileitem->group());
      nextColumn++;
   };
   if (m_pTextView->m_showPermissions)
   {
      setText(nextColumn,makeAccessString(m_fileitem->permissions()));
   };
};

/*inline QString KonqTextViewItem::key( int _column, bool asc) const
{
   if (_column==0) return key(1,asc);
   QString tmp=sortChar;
   if (!asc && (sortChar=='0')) tmp=QChar('2');
   tmp+=text(_column);
   return tmp;
};

inline void KonqTextViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
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

inline void KonqTextViewItem::setup()
{
   widthChanged();
   int h(listView()->fontMetrics().height());
   if ( h % 2 > 0 ) h++;
   setHeight(h);
};*/

inline const char* KonqTextViewItem::makeAccessString( mode_t mode) const
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
#endif
