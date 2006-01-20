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
#ifndef __konq_textviewwidget_h__
#define __konq_textviewwidget_h__

#include "konq_listviewwidget.h"

#include <kurl.h>

class KonqListView;
class KonqTextViewItem;

/**
 * The text view
 */
class KonqTextViewWidget : public KonqBaseListViewWidget
{
   friend class KonqTextViewItem;
   Q_OBJECT
   public:
      KonqTextViewWidget( KonqListView *parent, QWidget *parentWidget );
      ~KonqTextViewWidget();
      virtual bool isExecuteArea( const QPoint& point );

   protected Q_SLOTS:
      // slots connected to the directory lister
      virtual void setComplete();
      virtual void slotNewItems( const KFileItemList & );
   protected:
//      bool isNameColumn(const QPoint& point );
//      virtual void viewportDragMoveEvent( QDragMoveEvent *_ev );
//      virtual void viewportDropEvent( QDropEvent *ev  );
      virtual void createColumns();

      QColor colors[11];
};

#endif
