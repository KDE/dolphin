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
#ifndef __konq_treeviewwidget_h__
#define __konq_treeviewwidget_h__

#include "konq_listviewwidget.h"
#include "konq_treeviewitem.h"
#include <qdict.h>
#include <kurl.h>
#include <konq_fileitem.h>
#include <klistview.h>

class KonqListView;

class KonqTreeViewWidget : public KonqBaseListViewWidget
{
   friend class KonqListViewDir;
   Q_OBJECT
   public:
      KonqTreeViewWidget( KonqListView *parent, QWidget *parentWidget);
      virtual ~KonqTreeViewWidget();

   protected slots:
      // from QListView
      virtual void slotReturnPressed( QListViewItem *_item );

      // slots connected to the directory lister
      virtual void slotClear();
      virtual void slotNewItems( const KFileItemList & );

   protected:
      KonqListViewDir * findDir( const QString &_url );
      void openSubFolder(const KURL &_url, KonqListViewDir* _dir);
      virtual void addSubDir( const KURL & _url, KonqListViewDir* _dir );
      virtual void removeSubDir( const KURL & _url );
      /** Common method for slotCompleted and slotCanceled */
      virtual void setComplete();

      /** If 0L, we are listing the toplevel.
       * Otherwise, m_pWorkingDir points to the directory item we are listing,
       * and all files found will be created under this directory item.
       */
      KonqListViewDir* m_pWorkingDir;
      // Cache, for findDir
      KonqListViewDir* m_lasttvd;

      bool m_bSubFolderComplete;

      QDict<KonqListViewDir> m_mapSubDirs;
};

#endif
