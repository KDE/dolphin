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

#include <qlistview.h>
#include <qstring.h>
#include <kio/global.h>
#include <klocale.h>
#include "konq_listviewwidget.h"

class KonqBaseListViewWidget;
class KMimeType;
class KonqFileItem;
class KonqListViewDir;
class QPainter;
class KonqBaseListViewItem;
class KonqListViewItem;

class KonqBaseListViewItem : public QListViewItem
{
   public:
      KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget,KonqFileItem* _fileitem);
      KonqBaseListViewItem(KonqBaseListViewItem *_parent,KonqFileItem* _fileitem);
      virtual ~KonqBaseListViewItem() {}
      /** @return the file item held by this instance */
      KonqFileItem * item() {return m_fileitem;}
      /** Call this before destroying the tree view (decreases reference count
       * on the view) */
      virtual void prepareToDie() {}
   protected:
      /** Pointer to the file item in KDirLister's list */
      KonqFileItem* m_fileitem;
      const char* makeAccessString( const mode_t mode ) const;
};

/**
 * One item in the tree
 */
class KonqListViewItem : public KonqBaseListViewItem
{
   public:
      /**
       * Create an item in the tree toplevel representing a file
       * @param _parent the parent widget, the tree view
       * @param _fileitem the file item created by KDirLister
       */
      KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqFileItem* _fileitem );
      /**
       * Create an item representing a file, inside a directory
       * @param _treeview the parent tree view
       * @param _parent the parent widget, a directory item in the tree view
       * @param _fileitem the file item created by KDirLister
       */
      KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqListViewItem *_parent, KonqFileItem* _fileitem );
      //KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqListViewDir *_parent, KonqFileItem* _fileitem );
      virtual ~KonqListViewItem() { }

      virtual void prepareToDie() { m_pListViewWidget = 0L; }
      virtual QString key( int _column, bool ) const;
      virtual void paintCell( QPainter *_painter, const QColorGroup & cg,
                              int column, int width, int alignment );
   protected:
      void init();

      QString makeNumericString( const KIO::UDSAtom &_atom ) const;
      QString makeTimeString( const KIO::UDSAtom &_atom ) const;
      QString makeTypeString( const KIO::UDSAtom &_atom ) const;

      /** Parent tree view */
      KonqBaseListViewWidget* m_pListViewWidget;
};

inline KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget,KonqFileItem* _fileitem)
:QListViewItem(_listViewWidget),m_fileitem(_fileitem)
{}

inline KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewItem *_parent,KonqFileItem* _fileitem)
:QListViewItem(_parent),m_fileitem(_fileitem)
{}

inline const char* KonqBaseListViewItem::makeAccessString( mode_t mode) const
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
