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
#include "konq_treeviewitem.h"
#include "konq_treeviewwidget.h"
#include <konq_fileitem.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <assert.h>
#include <stdio.h>


/**************************************************************
 *
 * KonqListViewDir
 *
 **************************************************************/

KonqListViewDir::KonqListViewDir( KonqTreeViewWidget *_parent, KonqFileItem* _fileitem )
:KonqListViewItem( _parent, _fileitem )
{
  _parent->addSubDir( _fileitem->url(), this );
  setExpandable (TRUE);

  m_bComplete = false;
}

KonqListViewDir::KonqListViewDir( KonqTreeViewWidget *_treeview, KonqListViewDir * _parent, KonqFileItem* _fileitem )
:KonqListViewItem(_treeview,_parent,_fileitem)
{
  _treeview->addSubDir( _fileitem->url(), this );
  setExpandable (TRUE);
  m_bComplete = false;
}

KonqListViewDir::~KonqListViewDir()
{
}

void KonqListViewDir::setOpen( bool _open )
{
  if ( _open && !m_bComplete ) // complete it before opening
    static_cast<KonqTreeViewWidget *>(listView())->openSubFolder( m_fileitem->url(), this );

  QListViewItem::setOpen( _open );
}

QString KonqListViewDir::url( int _trailing )
{
  return m_fileitem->url().url( _trailing );
}

