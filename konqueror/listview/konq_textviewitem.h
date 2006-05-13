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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_textviewitem_h__
#define __konq_textviewitem_h__

#include <QListView>
#include <QString>
#include <kio/global.h>
#include <klocale.h>
#include "konq_listviewitems.h"
#include "konq_textviewwidget.h"

class KFileItem;
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
      KonqTextViewItem( KonqTextViewWidget *_parent, KFileItem* _fileitem );
      virtual ~KonqTextViewItem() {/*cerr<<"~KonqTextViewItem: "<<text(1)<<endl;*/ };
      virtual int compare( Q3ListViewItem* i, int col, bool ascending ) const;
//      virtual QString key( int _column, bool asc) const;
      /** Call this before destroying the text view (decreases reference count
       * on the view)*/
      virtual void paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment );
//      virtual void paintFocus( QPainter *_painter, const QColorGroup & _cg, const QRect & r );
      virtual void updateContents();

   protected:
      virtual void setup();
      int type;
};

inline KonqTextViewItem::KonqTextViewItem( KonqTextViewWidget *_parent, KFileItem* _fileitem )
:KonqBaseListViewItem( _parent,_fileitem )
{
   updateContents();
}

#endif
