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

#include <kdebug.h>
#include <konq_dirlister.h>
#include <kglobal.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>

#include <konq_settings.h>
#include "konq_propsview.h"

#include <stdlib.h>
#include <assert.h>

KonqTextViewWidget::KonqTextViewWidget( KonqListView *parent, QWidget *parentWidget )
:KonqBaseListViewWidget(parent,parentWidget)
//,timer()
{
   kdDebug(1202) << "+KonqTextViewWidget" << endl;
   m_filenameColumn=1;

   setAllColumnsShowFocus(TRUE);
   setRootIsDecorated(false);

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
   //timer.start();
   m_showIcons=FALSE;
}

KonqTextViewWidget::~KonqTextViewWidget()
{}

void KonqTextViewWidget::createColumns()
{
   //the textview has fixed size columns
   //these both columns are always required, so add them
   if (columns()<2)
   {
      addColumn(i18n("Name"),fontMetrics().width("_a_quite_long_filename_"));
      addColumn(" ",fontMetrics().width("@")+2);
      setColumnAlignment(1,AlignRight);
      //this way the column with the name has the index 0 and
      //so the "jump to filename beginning with this character" works
      header()->moveSection(0,2);
   };
   setSorting(0,TRUE);

   //remove all but the first two columns
   for (int i=columns()-1; i>1; i--)
      removeColumn(i);

   int currentColumn(m_filenameColumn+1);
   //now add the checked columns
   for (int i=0; i<NumberOfAtoms; i++)
   {
      if ((confColumns[i].displayThisOne) && (confColumns[i].displayInColumn==currentColumn))
      {
         if (sortedByColumn==confColumns[i].desktopFileName)
            setSorting(currentColumn,ascending);
         ColumnInfo *tmpColumn=&confColumns[i];
         QCString tmpName=tmpColumn->name.utf8();
         if (tmpColumn->udsId==KIO::UDS_SIZE)
         {
            addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatNumber(888888888, 0)+"  "));
            setColumnAlignment(currentColumn,AlignRight);
         }
         else if ((tmpColumn->udsId==KIO::UDS_MODIFICATION_TIME)
                  || (tmpColumn->udsId==KIO::UDS_ACCESS_TIME)
                  || (tmpColumn->udsId==KIO::UDS_CREATION_TIME))
         {
            QDateTime dt(QDate(2000,10,10),QTime(20,20,20));
            //addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatDateTime(dt)+"--"));
            addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatDate(dt.date(),TRUE)+KGlobal::locale()->formatTime(dt.time())+"----"));
            setColumnAlignment(currentColumn,AlignCenter);
         }
         else if (tmpColumn->udsId==KIO::UDS_ACCESS)
            addColumn(i18n(tmpName),fontMetrics().width("--Permissions--"));
         else if (tmpColumn->udsId==KIO::UDS_USER)
            addColumn(i18n(tmpName),fontMetrics().width("a_long_username"));
         else if (tmpColumn->udsId==KIO::UDS_GROUP)
            addColumn(i18n(tmpName),fontMetrics().width("a_groupname"));
         else if (tmpColumn->udsId==KIO::UDS_LINK_DEST)
            addColumn(i18n(tmpName),fontMetrics().width("_a_quite_long_filename_"));
         else if (tmpColumn->udsId==KIO::UDS_FILE_TYPE)
            addColumn(i18n(tmpName),fontMetrics().width("a_comment_for_mimetype_"));
         else if (tmpColumn->udsId==KIO::UDS_MIME_TYPE)
            addColumn(i18n(tmpName),fontMetrics().width("_a_long_/_mimetype_"));
         else if (tmpColumn->udsId==KIO::UDS_URL)
            addColumn(i18n(tmpName),fontMetrics().width("_a_long_lonq_long_url_"));

         i=-1;
         currentColumn++;
      };
   };
   if (sortedByColumn=="FileName")
      setSorting(0,ascending);

};

void KonqTextViewWidget::slotNewItems( const KFileItemList & entries )
{
   for( QListIterator<KFileItem> kit (entries); kit.current(); ++kit )
      new KonqTextViewItem( this,static_cast<KonqFileItem*> (*kit));
   kdDebug(1202)<<"::slotNewItem: received: "<<entries.count()<<endl;
}

#include "konq_textviewwidget.moc"
