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

#ifndef __konq_textviewitem_h__
#define __konq_textviewitem_h__

#include <qlistview.h>
#include <qstring.h>
#include <kio/global.h>
#include <klocale.h>
#include "konq_listviewitems.h"
#include "konq_textviewwidget.h"

class KonqFileItem;
class QPainter;

#define KTVI_REGULAR 0
#define KTVI_REGULARLINK 1
#define KTVI_EXEC 2
#define KTVI_DIR 3
#define KTVI_DIRLINK 4
#define KTVI_BADLINK 5
#define KTVI_SOCKET 6
#define KTVI_CHARDEV 7
#define KTVI_BLOCKDEV 8
#define KTVI_FIFO 9
#define KTVI_UNKNOWN 10


class KonqTextViewItem : public KonqBaseListViewItem
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
      virtual void paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment );
      virtual void updateContents();

   protected:
      virtual void setup();

      /** Parent text view */
      KonqTextViewWidget* m_pTextView;
      int type;
};

inline KonqTextViewItem::KonqTextViewItem( KonqTextViewWidget *_parent, KonqFileItem* _fileitem)
:KonqBaseListViewItem( _parent,_fileitem )
,m_pTextView (_parent)
{
   updateContents();
   /*QString tmp;
   long int size=m_fileitem->size();
   mode_t m=m_fileitem->mode();
   if (m_fileitem->isLink())
   {
      if (S_ISDIR(m))
      {
         sortChar='0';
         type=KTVI_DIRLINK;
         tmp="~";
      }
      else if (S_ISREG(m))
      {
         tmp="@";
         type=KTVI_REGULARLINK;
      }
      else
      {
         tmp="!";
         type=KTVI_UNKNOWN;
         size=-1;
      };
   }
   else if (S_ISREG(m))
   {
      if ((m_fileitem->permissions() & (S_IXUSR|S_IXGRP|S_IXOTH)) !=0 )
      {
         tmp="*";
         type=KTVI_EXEC;
      }
      else
      {
         tmp="";
         type=KTVI_REGULAR;
      };
   }
   else if (S_ISDIR(m))
   {
      type=KTVI_DIR;
      tmp="/";
      sortChar='0';
   }
   else if (S_ISCHR(m))
   {
      type=KTVI_CHARDEV;
      tmp="-";
   }
   else if (S_ISBLK(m))
   {
      type=KTVI_BLOCKDEV;
      tmp="+";
   }
   else if (S_ISSOCK(m))
   {
      type=KTVI_SOCKET;
      tmp="=";
   }
   else if (S_ISFIFO(m))
   {
      type=KTVI_FIFO;
      tmp=">";
   }
   else
   {
      tmp="!";
      type=KTVI_UNKNOWN;
      size=-1;
   };
   setText(0,tmp);

   setText(1,m_fileitem->text());

   int nextColumn(2);
   
   if (m_pTextView->showSize())
   {
      tmp.sprintf("%9d",size);
      setText(nextColumn, tmp );
      nextColumn++;
   };

   if (m_pTextView->showTime())
   {
      for( KIO::UDSEntry::ConstIterator it = m_fileitem->entry().begin(); it != m_fileitem->entry().end(); it++ )
      {
         if ((*it).m_uds==KIO::UDS_MODIFICATION_TIME)
         {
            QDateTime dt;
            dt.setTime_t((time_t) (*it).m_long);
            //setText(3,dt.toString());
            setText(nextColumn,KGlobal::locale()->formatDate(dt.date(),true)+" "+KGlobal::locale()->formatTime(dt.time()));
            break;
         };

      };
      nextColumn++;
   };

   if (m_pTextView->showOwner())
   {
      setText(nextColumn,m_fileitem->user());
      nextColumn++;
   };
   if (m_pTextView->showGroup())
   {
      setText(nextColumn,m_fileitem->group());
      nextColumn++;
   };
   if (m_pTextView->showPermissions())
   {
      setText(nextColumn,makeAccessString(m_fileitem->permissions()));
   };*/
};

#endif
