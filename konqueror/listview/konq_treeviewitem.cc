/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2001, 2002, 2004 Michael Brade <brade@kde.org>

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

#include "konq_treeviewwidget.h"
#include "konq_listview.h"


KonqListViewDir::KonqListViewDir( KonqTreeViewWidget *_parent, KFileItem* _fileitem )
  : KonqListViewItem( _parent, _fileitem )
{
  setExpandable( true );
  m_bComplete = false;
}

KonqListViewDir::KonqListViewDir( KonqTreeViewWidget *_treeview, KonqListViewDir *_parent, KFileItem *_fileitem )
  : KonqListViewItem( _treeview, _parent, _fileitem )
{
  setExpandable( true );
  m_bComplete = false;
}

void KonqListViewDir::setOpen( bool _open )
{
  open( _open, false );
}

void KonqListViewDir::open( bool _open, bool _reload )
{
  if ( _open != isOpen() || _reload )
  {
    KonqTreeViewWidget *treeView = static_cast<KonqTreeViewWidget *>(m_pListViewWidget);

    if ( _open )
    {
      if ( !m_bComplete || _reload ) // complete it before opening
        treeView->openSubFolder( this, _reload );
      else
      {
        // we need to add the items to the counts for the statusbar
        KFileItemList lst;

        Q3ListViewItem* it = firstChild();
        while ( it )
        {
          lst.append( static_cast<KonqListViewItem*>(it)->item() );
          it = it->nextSibling();
        }

        // tell the view about the new items to count
        treeView->m_pBrowserView->newItems( lst );
      }
    }
    else
    {
      treeView->stopListingSubFolder( this );

      Q3ListViewItem* it = firstChild();
      while ( it )
      {
        // unselect the items in the closed folder
        treeView->setSelected( it, false );
        // delete the item from the counts for the statusbar
        KFileItem* item = static_cast<KonqListViewItem*>(it)->item();
        treeView->m_pBrowserView->deleteItem( item );
        it = it->nextSibling();
      }
    }

    Q3ListViewItem::setOpen( _open );
    treeView->slotOnViewport();
  }
}

QString KonqListViewDir::url( int _trailing )
{
  return item()->url().url( _trailing );
}

