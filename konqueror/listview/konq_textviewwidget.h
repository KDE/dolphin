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
#ifndef __konq_textviewwidget_h__
#define __konq_textviewwidget_h__

#include "konq_listviewwidget.h"

//#include <qtimer.h>
#include <kurl.h>
#include <konq_fileitem.h>

class KonqListView;
class KonqTextViewItem;

/**
 * The text view
 */
class KonqTextViewWidget : public KonqBaseListViewWidget
{
   friend KonqTextViewItem;
   Q_OBJECT
   public:
      KonqTextViewWidget( KonqListView *parent, QWidget *parentWidget );
      ~KonqTextViewWidget();

   protected slots:
      // slots connected to the directory lister
      //virtual void slotStarted( const QString & );
      //virtual void slotCompleted();
      virtual void slotNewItems( const KFileItemList & );
   protected:
      virtual void createColumns();
      virtual bool isSingleClickArea( const QPoint& ) {return TRUE;};
      /** Common method for slotCompleted and slotCanceled */
      virtual void setComplete();

      //this timer is only for testing, can be reomved
      //QTime timer;
      QColor colors[11];
      QColor highlight[11];
};

#endif
