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

#include "konq_treeviewitem.h"
#include "konq_treeviewwidget.h"
#include "konq_listview.h"
#include <konq_fileitem.h>


/**************************************************************
 *
 * KonqListViewDir
 *
 **************************************************************/

KonqListViewDir::KonqListViewDir( KonqTreeViewWidget *_parent, KonqFileItem* _fileitem )
:KonqListViewItem( _parent, _fileitem )
{
  setExpandable (TRUE);
  m_bComplete = false;
  _parent->addSubDir( this );
}

KonqListViewDir::KonqListViewDir( KonqTreeViewWidget *_treeview, KonqListViewDir * _parent, KonqFileItem* _fileitem )
:KonqListViewItem(_treeview,_parent,_fileitem)
{
  setExpandable( TRUE );
  m_bComplete = false;
  _treeview->addSubDir( this );
}

KonqListViewDir::~KonqListViewDir()
{
}

void KonqListViewDir::setOpen( bool _open )
{
  if ( _open != isOpen() )
  {
    KonqTreeViewWidget* treeView = static_cast<KonqTreeViewWidget *>(listView());

    if ( _open )
    {
      if ( !m_bComplete ) // complete it before opening
        treeView->openSubFolder( this );
      else
      {
        KFileItemList lst;
        lst.setAutoDelete( false );

        QListViewItem* it = firstChild();
        while ( it )
        {
          lst.append( static_cast<KonqListViewItem*>(it)->item() );
          it = it->nextSibling();
        }

        // add the items to the counts for the statusbar
        treeView->m_pBrowserView->newItems( lst );
      }
    }
    else
    {
      treeView->stopListingSubFolder( this );

      QListViewItem* it = firstChild();
      while ( it )
      {
        // unselect the items in the closed folder
        treeView->setSelected( it, false );
        // delete the item from the counts for the statusbar
        KonqFileItem* item = static_cast<KonqListViewItem*>(it)->item();
        treeView->m_pBrowserView->deleteItem( item );
        it = it->nextSibling();
      }
    }

    QListViewItem::setOpen( _open );
    treeView->slotOnViewport();
  }
}

QString KonqListViewDir::url( int _trailing )
{
  return m_fileitem->url().url( _trailing );
}

