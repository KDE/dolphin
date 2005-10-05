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

#ifndef __konq_listviewitems_h__
#define __konq_listviewitems_h__

#include <klistview.h>
#include <qstring.h>
#include <kicontheme.h>

#include <q3valuevector.h>

// for mode_t
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

class QPainter;
class KFileItem;
class KonqBaseListViewWidget;


class KonqBaseListViewItem : public KListViewItem
{
   public:
      KonqBaseListViewItem( KonqBaseListViewWidget *_listViewWidget, 
                            KFileItem *_fileitem );
      KonqBaseListViewItem( KonqBaseListViewWidget *_treeViewWidget, 
                            KonqBaseListViewItem *_parent, KFileItem *_fileitem );
      virtual ~KonqBaseListViewItem();

      /** @return the file item held by this instance */
      KFileItem * item() { return m_fileitem; }

      void mimetypeFound();
      virtual void updateContents() = 0;
      virtual void setDisabled( bool disabled ) { m_bDisabled = disabled; }
      virtual void setActive  ( bool active   ) { m_bActive   = active;   }
      virtual int compare( Q3ListViewItem* i, int col, bool ascending ) const;
      
      int state() const 
      {
         if (m_bDisabled)
            return KIcon::DisabledState;
         if (m_bActive)
            return KIcon::ActiveState; 
         return KIcon::DefaultState;
      }

      /** For KonqMimeTypeResolver */
      QRect rect() const;
      
   protected:
      short int sortChar;
      bool m_bDisabled;
      bool m_bActive; 
      
      /** Pointer to the file item in KDirLister's list */
      KFileItem* m_fileitem;
      /** Parent tree view */
      KonqBaseListViewWidget* m_pListViewWidget;

      static const char* makeAccessString( const mode_t mode );
};

/**
 * One item in the detailed list or in the tree (not text)
 */
class KonqListViewItem : public KonqBaseListViewItem
{
   public:
      /**
       * Create an item in the tree toplevel representing a file
       * @param _parent the parent widget, the tree view
       * @param _fileitem the file item created by KDirLister
       */
      KonqListViewItem( KonqBaseListViewWidget *_parent, KFileItem *_fileitem );

      /**
       * Create an item representing a file, inside a directory
       * @param _treeview the parent tree view
       * @param _parent the parent widget, a directory item in the tree view
       * @param _fileitem the file item created by KDirLister
       */
      KonqListViewItem( KonqBaseListViewWidget *_treeview, 
                        KonqListViewItem *_parent, KFileItem *_fileitem );

      virtual ~KonqListViewItem();

      virtual void paintCell( QPainter *_painter, const QColorGroup & cg,
                              int column, int width, int alignment );
      virtual void paintFocus( QPainter * _painter, const QColorGroup & cg, const QRect & r );
      virtual void updateContents();
      virtual void setDisabled( bool disabled );
      virtual void setActive  ( bool active   );

      virtual void setPixmap( int column, const QPixmap & pm );
      virtual const QPixmap * pixmap( int column ) const;

private:
      QVector<QPixmap *> m_pixmaps;
};

#endif
