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

#ifndef __konq_listviewitems_h__
#define __konq_listviewitems_h__

#include <klistview.h>
#include <qstring.h>
#include <kicontheme.h>
#include <konq_fileitem.h>

class KonqBaseListViewWidget;
class KonqListViewDir;
class QPainter;
class KonqBaseListViewItem;
class KonqListViewItem;

class KonqBaseListViewItem : public KListViewItem
{
   public:
      KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget,KonqFileItem* _fileitem);
      KonqBaseListViewItem(KonqBaseListViewItem *_parent,KonqFileItem* _fileitem);
      virtual ~KonqBaseListViewItem() {}
      /** @return the file item held by this instance */
      KonqFileItem * item() { return m_fileitem; }
      void mimetypeFound();
      virtual void updateContents() = 0;
      virtual void setDisabled( bool disabled ) { m_bDisabled = disabled; }
      int state() const { return m_bDisabled ? KIcon::DisabledState : KIcon::DefaultState; }

      /** For KonqMimeTypeResolver */
      QRect rect() const;

   protected:
      QChar sortChar;
      bool m_bDisabled;
      /** Pointer to the file item in KonqDirLister's list */
      KonqFileItem* m_fileitem;
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
       * @param _fileitem the file item created by KonqDirLister
       */
      KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqFileItem* _fileitem );
      /**
       * Create an item representing a file, inside a directory
       * @param _treeview the parent tree view  - now unused
       * @param _parent the parent widget, a directory item in the tree view
       * @param _fileitem the file item created by KonqDirLister
       */
      KonqListViewItem( KonqBaseListViewWidget *, KonqListViewItem *_parent, KonqFileItem* _fileitem );

      virtual ~KonqListViewItem() { }

      virtual QString key( int _column, bool ) const;
      virtual void paintCell( QPainter *_painter, const QColorGroup & cg,
                              int column, int width, int alignment );
      virtual void updateContents();
      virtual void setDisabled( bool disabled );
   protected:
      /** Parent tree view */
      KonqBaseListViewWidget* m_pListViewWidget;
};

#endif
