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

#include "konq_listview.h"
#include "konq_textviewwidget.h"
#include "konq_textviewitem.h"

#include <qdragobject.h>
#include <qheader.h>

#include <kcursor.h>
#include <kdebug.h>
#include <konqdirlister.h>
#include <kglobal.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <konqoperations.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>

#include <konqsettings.h>
#include "konq_propsview.h"

#include <stdlib.h>
#include <assert.h>

KonqTextViewWidget::KonqTextViewWidget( KonqListView *parent, QWidget *parentWidget )
:KonqBaseListViewWidget(parent,parentWidget)
,timer()
{
   kdDebug(1202) << "+KonqTextViewWidget" << endl;

   addColumn(" ",fontMetrics().width("@")+2);
   addColumn(i18n("Name"),fontMetrics().width("_a_quite_long_filename_"));
   setColumnAlignment(0,AlignRight);

   addColumn(i18n("Size"),fontMetrics().width("000000000"));
   //int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
   //cerr<<" set to: "<<permissionsWidth<<endl;

   addColumn(i18n("Owner"),fontMetrics().width("a_long_username"));
   addColumn(i18n("Group"),fontMetrics().width("a_groupname"));

   setColumnAlignment(2,AlignRight);
   setAllColumnsShowFocus(TRUE);
   setRootIsDecorated(false);
   setSorting(1);

   colors[KTVI_REGULAR]=Qt::black;
   colors[KTVI_EXEC]=QColor(0,170,0);
   colors[KTVI_REGULARLINK]=Qt::black;
   colors[KTVI_DIR]=Qt::black;
   colors[KTVI_DIRLINK]=Qt::black;
   colors[KTVI_BADLINK]=Qt::red;
   colors[KTVI_SOCKET]=Qt::magenta;
   colors[KTVI_FIFO]=Qt::magenta;
   colors[KTVI_UNKNOWN]=Qt::red;
   colors[KTVI_CHARDEV]=Qt::blue;
   colors[KTVI_BLOCKDEV]=Qt::blue;

   highlight[KTVI_REGULAR]=Qt::white;
   highlight[KTVI_EXEC]=colors[KTVI_EXEC].light(200);
   highlight[KTVI_REGULARLINK]=Qt::white;
   highlight[KTVI_DIR]=Qt::white;
   highlight[KTVI_DIRLINK]=Qt::white;
   highlight[KTVI_BADLINK]=colors[KTVI_BADLINK].light();
   highlight[KTVI_SOCKET]=colors[KTVI_SOCKET].light();
   highlight[KTVI_FIFO]=colors[KTVI_FIFO].light();
   highlight[KTVI_UNKNOWN]=colors[KTVI_UNKNOWN].light();
   highlight[KTVI_CHARDEV]=colors[KTVI_CHARDEV].light(180);
   highlight[KTVI_BLOCKDEV]=colors[KTVI_BLOCKDEV].light(180);
   m_filenameColumn=1;
   timer.start();
}

KonqTextViewWidget::~KonqTextViewWidget()
{}

void KonqTextViewWidget::createColumns()
{
   //the textview has fixed size columns
   //these both columns are always required, so add them
   if (columns()<2)
   {
      addColumn(" ",fontMetrics().width("@")+2);
      addColumn(i18n("Name"),fontMetrics().width("_a_quite_long_filename_"));
      setColumnAlignment(0,AlignRight);
   };

   //remove all but the first two columns
   for (int i=columns()-1; i>1; i--)
      removeColumn(i);

   int currentColumn(2);
   //now add the checked columns
   for (int i=0; i<confColumns.count(); i++)
   {
      if (confColumns.at(i)->displayThisOne)
      {
         ColumnInfo *tmpColumn=confColumns.at(i);
         QString tmpName=tmpColumn->name;
         if (tmpColumn->udsId==KIO::UDS_SIZE)
         {
            addColumn(i18n(tmpName),fontMetrics().width("000000000"));
            setColumnAlignment(currentColumn,AlignRight);
         }
         else if ((tmpColumn->udsId==KIO::UDS_MODIFICATION_TIME)
                  || (tmpColumn->udsId==KIO::UDS_ACCESS_TIME)
                  || (tmpColumn->udsId==KIO::UDS_CREATION_TIME))
         {
            QDateTime dt(QDate(2000,10,10),QTime(20,20,20));
            addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatDate(dt.date(),true)+" "+KGlobal::locale()->formatTime(dt.time())+"---"));
         }
         else if (tmpColumn->udsId==KIO::UDS_ACCESS)
            addColumn(i18n(tmpName),fontMetrics().width("--Permissions--"));
         else if (tmpColumn->udsId==KIO::UDS_USER)
            addColumn(i18n(tmpName),fontMetrics().width("a_long_username"));
         else if (tmpColumn->udsId==KIO::UDS_GROUP)
            addColumn(i18n(tmpName),fontMetrics().width("a_groupname"));
         else if (tmpColumn->udsId==KIO::UDS_LINK_DEST)
            addColumn(i18n(tmpName),fontMetrics().width("_a_quite_long_filename_"));
            
         confColumns.at(i)->displayInColumn=currentColumn;
         currentColumn++;
      };
   };
};

void KonqTextViewWidget::slotStarted( const QString & /*url*/ )
{
   if ( !m_bTopLevelComplete )
      emitStarted(m_dirLister->job());
   setUpdatesEnabled(FALSE);
   timer.restart();
}

void KonqTextViewWidget::setComplete()
{
   m_bTopLevelComplete = true;

   setContentsPos( m_pBrowserView->extension()->urlArgs().xOffset, m_pBrowserView->extension()->urlArgs().yOffset );
   setUpdatesEnabled(TRUE);
   repaintContents(0,0,width(),height());
   emit selectionChanged();
   //setContentsPos( m_iXOffset, m_iYOffset );
//   show();
}

void KonqTextViewWidget::slotCompleted()
{
   setComplete();
   emitCompleted();
   //cerr<<"needed "<<timer.elapsed()<<" msecs"<<endl;
}

void KonqTextViewWidget::slotNewItems( const KFileItemList & entries )
{
   for( QListIterator<KFileItem> kit (entries); kit.current(); ++kit )
      new KonqTextViewItem( this,static_cast<KonqFileItem*> (*kit));
   kdDebug(1202)<<"::slotNewItem: received: "<<entries.count()<<endl;
}

#include "konq_textviewwidget.moc"
