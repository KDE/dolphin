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
#include <kdirlister.h>
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

   colors[REGULAR]=Qt::black;
   colors[EXEC]=QColor(0,170,0);
   colors[REGULARLINK]=Qt::black;
   colors[DIR]=Qt::black;
   colors[DIRLINK]=Qt::black;
   colors[BADLINK]=Qt::red;
   colors[SOCKET]=Qt::magenta;
   colors[FIFO]=Qt::magenta;
   colors[UNKNOWN]=Qt::red;
   colors[CHARDEV]=Qt::blue;
   colors[BLOCKDEV]=Qt::blue;

   highlight[REGULAR]=Qt::white;
   highlight[EXEC]=colors[EXEC].light(200);
   highlight[REGULARLINK]=Qt::white;
   highlight[DIR]=Qt::white;
   highlight[DIRLINK]=Qt::white;
   highlight[BADLINK]=colors[BADLINK].light();
   highlight[SOCKET]=colors[SOCKET].light();
   highlight[FIFO]=colors[FIFO].light();
   highlight[UNKNOWN]=colors[UNKNOWN].light();
   highlight[CHARDEV]=colors[CHARDEV].light(180);
   highlight[BLOCKDEV]=colors[BLOCKDEV].light(180);
   m_filenameColumn=1;

   timer.start();
}

KonqTextViewWidget::~KonqTextViewWidget()
{}

void KonqTextViewWidget::keyPressEvent( QKeyEvent *_ev )
{
   //cerr<<"keyPressEvent"<<endl;
   // We are only interested in the insert key here
   KonqTextViewItem* item = (KonqTextViewItem*)currentItem();
   //insert without modifiers toggles the selection of the current item and moves to the next
   if ((_ev->key()==Key_Insert) && (_ev->state()==0))
   {
      if (item==0) return;
      item->setSelected(!item->isSelected());
      QListViewItem *nextItem=item->itemBelow();
      if (nextItem!=0)
      {
         setCurrentItem(nextItem);
         ensureItemVisible(nextItem);
      }
      else item->repaint();
      emit selectionChanged();
      updateSelectedFilesInfo();
   }
   else
   {
      //cerr<<"QListView->keyPressEvent"<<endl;
      KonqBaseListViewWidget::keyPressEvent( _ev );
   };
};

bool KonqTextViewWidget::openURL( const KURL &url )
{
   if ( !m_dirLister )
   {
      // Create the directory lister
      m_dirLister = new KDirLister(true);

      QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ),
                        this, SLOT( slotStarted( const QString & ) ) );
      QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
      QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
      QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
      QObject::connect( m_dirLister, SIGNAL( newItems( const KonqFileItemList & ) ),
                        this, SLOT( slotNewItems( const KonqFileItemList & ) ) );
      QObject::connect( m_dirLister, SIGNAL( deleteItem( KonqFileItem * ) ),
                        this, SLOT( slotDeleteItem( KonqFileItem * ) ) );
   }

   m_bTopLevelComplete = false;

   m_url=url;

   if ( m_pProps->enterDir( url ) )
   {
      // nothing to do yet
   }

   // Start the directory lister !
   m_dirLister->openURL( url, m_pProps->m_bShowDot, false /* new url */ );

   //  setCaptionFromURL( m_sURL );
   return true;
}

void KonqTextViewWidget::slotStarted( const QString & /*url*/ )
{
   if ( !m_bTopLevelComplete )
      emitStarted(0);
   setUpdatesEnabled(FALSE);
   timer.restart();
   if (m_settingsChanged)
   {
      m_settingsChanged=FALSE;
      for (int i=columns()-1; i>1; i--)
         removeColumn(i);

      if (m_showSize)
      {
         addColumn(i18n("Size"),fontMetrics().width("000000000"));
         setColumnAlignment(2,AlignRight);
      };
      if (m_showTime)
      {
         QDateTime dt(QDate(2000,10,10),QTime(20,20,20));
         addColumn(i18n("Modified"),fontMetrics().width(KGlobal::locale()->formatDate(dt.date(),true)+" "+KGlobal::locale()->formatTime(dt.time())+"---"));
      };
      if (m_showOwner)
         addColumn(i18n("Owner"),fontMetrics().width("a_long_username"));

      //   int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
      //   cerr<<" set to: "<<permissionsWidth<<endl;

      if (m_showGroup)
         addColumn(i18n("Group"),fontMetrics().width("a_groupname"));
      if (m_showPermissions)
         addColumn(i18n("Permissions"),fontMetrics().width("--Permissions--"));
      //   int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
      //   cerr<<" set to: "<<permissionsWidth<<endl;
   };
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

void KonqTextViewWidget::slotNewItems( const KonqFileItemList & entries )
{
   for( QListIterator<KonqFileItem> kit (entries); kit.current(); ++kit )
      new KonqTextViewItem( this, (*kit));
}

#include "konq_textviewwidget.moc"
