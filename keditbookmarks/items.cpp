/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "toplevel.h"
#include "commands.h"
#include <kaction.h>
#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kbookmarkexporter.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kicondialog.h>
#include <kapplication.h>
#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>
#include <dcopclient.h>
#include <assert.h>
#include <stdlib.h>
#include <klocale.h>
#include <kiconloader.h>

#include <konq_faviconmgr.h>

void KEBListView::rename( QListViewItem *_item, int c )
{
   KEBListViewItem * item = static_cast<KEBListViewItem *>(_item);
   if ( !(item->bookmark().isGroup() && c == 1) 
         && !item->bookmark().isSeparator() 
         && ( firstChild() != item) 
   ) {
      KListView::rename( _item, c );
   }
}

bool KEBListView::acceptDrag(QDropEvent * e) const
{
   return e->source() == viewport() || KBookmarkDrag::canDecode( e );
}

QDragObject *KEBListView::dragObject()
{
   if( KEBTopLevel::self()->numSelected() == 0 ) {
      return (QDragObject*)0;
   }

   /* viewport() - not sure why klistview does it this way*/
   QValueList<KBookmark> bookmarks = KEBTopLevel::self()->getBookmarkSelection();
   KBookmarkDrag * drag = KBookmarkDrag::newDrag( bookmarks, viewport() );
   drag->setPixmap( SmallIcon( (bookmarks.size() > 1) ? ("bookmark") : (bookmarks.first().icon()) ) );
   return drag;
}

// toplevel item (there should be only one!)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmark & group )
   : QListViewItem(parent, i18n("Bookmarks") ), m_bookmark(group)
{
   setPixmap(0, SmallIcon("bookmark"));
   setExpandable(true); // Didn't know this was necessary :)
}

// bookmark (first of its group)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk )
   : QListViewItem(parent, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk)
{
   init(bk);
}

// bookmark (after another)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark & bk )
   : QListViewItem(parent, after, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk)
{
   init(bk);
}

// group
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup & gp )
   : QListViewItem(parent, after, gp.fullText()), m_bookmark(gp)
{
   init(gp);
   setExpandable(true);
}

// empty folder item
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after )
    : QListViewItem(parent, after, i18n("The empty folder, not to empty", "Empty folder") )
{
   m_emptyFolder = true;
   setPixmap(0, SmallIcon("bookmark"));
}

void KEBListViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
   QColorGroup col(cg);

   if (column == 2) {
      if (render == 0) {
         int h, s, v;
         col.background().hsv(&h,&s,&v);
         if (v > 180 && v < 220) {
            col.setColor(QColorGroup::Text, Qt::darkGray);
         } else {
            col.setColor(QColorGroup::Text, Qt::gray);
         }
      } else if (render == 2) {
         QFont font=p->font();
         font.setBold( true );
         p->setFont(font);
      }
   }

   QListViewItem::paintCell( p, col, column, width, alignment );
}

#define COL_ADDR 3
#define COL_STAT 2

void KEBListViewItem::init( const KBookmark & bk )
{
   m_emptyFolder = false;
   setPixmap(0, SmallIcon( bk.icon() ) );
#ifdef DEBUG_ADDRESSES
   setText(COL_ADDR, bk.address());
#endif
   modUpdate();
}

void internal_nsGet(QString nsinfo, QString & nCreate, QString & nAccess, QString & nModify) {
   QStringList sl = QStringList::split(' ', nsinfo);
   for ( QStringList::Iterator it = sl.begin(); it != sl.end(); ++it ) {
      QStringList spl = QStringList::split('"', *it);
      // kdDebug() << spl[0] << "+" << spl[1] << "\n";
      if (spl[0] == "LAST_MODIFIED=") {
         nModify = spl[1];
      } else if (spl[0] == "ADD_DATE=") {
         nCreate = spl[1];
      } else if (spl[0] == "LAST_VISIT=") {
         nAccess = spl[1];
      }
   }
}

void KEBListViewItem::nsGet(QString & nCreate, QString & nAccess, QString & nModify )
{
   QString nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
   internal_nsGet(nsinfo, nCreate, nAccess, nModify);
}

void KEBListViewItem::nsGet(QString & nModify )
{
   QString c, a;
   nsGet(c, a, nModify);
}

QString internal_nsPut(QString _nsinfo, QString nm) 
{
   QString nCreate, nAccess, nModify;
   internal_nsGet(_nsinfo, nCreate, nAccess, nModify);

   bool numOk = false;
   nm.toInt(&numOk);

   QString nsinfo;
   nsinfo  =  "ADD_DATE=\"" + ((nCreate.isEmpty()) ? QString::number(time(0)) : nCreate) + "\"";
   nsinfo += " LAST_VISIT=\"" + ((nAccess.isEmpty()) ? "0" : nAccess) + "\"";
   nsinfo += " LAST_MODIFIED=\"" + ((numOk) ? nm : "1") + "\"";

   return nsinfo;
}

void KEBListViewItem::nsPut(QString nm)
{
   QString _nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
   QString nsinfo = internal_nsPut(_nsinfo,nm);
   m_bookmark.internalElement().setAttribute("netscapeinfo",nsinfo);
   KEBTopLevel::self()->setModified(true);
   KEBTopLevel::self()->Modify[m_bookmark.url().url()] = nm;
   setText(COL_STAT, nm);
}

QString mkTimeStr(int b)
{
   QDateTime dt;
   dt.setTime_t(b);
   if (dt.daysTo(QDateTime::currentDateTime()) > 31) {
      return KGlobal::locale()->formatDate(dt.date(), false);
   } else {
      return KGlobal::locale()->formatDateTime(dt, false);
   }
}

void KEBListViewItem::modUpdate()
{
   QString url = m_bookmark.url().url();
   KEBTopLevel *top = KEBTopLevel::self();

   if (top) {
      QString nModify, oModify;
      bool ois = false, nis = false, nMod = false;
      int nM = 0, oM = 0;

      // get new mod date if there is one
      if ( top->Modify.contains(url)) {
         nModify = top->Modify[url];
         nMod = true;
         bool ok = false;
         nM = nModify.toInt(&ok);
         if (!ok) {
            nis = true;
         }
      }

      if (top->oldModify.contains(url)) {
         if (nMod) {
            oModify = top->oldModify[url];

         } else { 
            // may be reading a second bookmark with same url

            QString oom;
            nsGet(oom);

            oModify = top->oldModify[url];

            if (oom.toInt() > oModify.toInt()) {
               top->oldModify[url] = oom;
               oModify = oom;
            }
         }

      } else { 
         // first time
         nsGet(oModify);
         top->oldModify[url] = oModify;
      }

      oM = oModify.toInt();
      if (oM == 1) {
         ois = true;
      }

      // kdDebug() << "nMod=" << nMod << " nis=" << nis << " nM=" << nM << " oM=" << nM << "\n";
      QString sn;

      if (nMod && nis) { 
         // error in current check
         sn = nModify;
         render = ois ? 1 : 2;

      } else if (nMod && nM == 0) { 
         // no modify time returned
         sn = i18n("Ok");

      } else if (nMod && nM >= oM) { 
         // info from current check
         sn = mkTimeStr(nM);
         render = (nM > oM) ? 2 : 1;

      } else if (ois) { 
         // error in previous check
         sn = i18n("Error");
         render = 0;

      } else if (oM) { 
         // info from previous check
         sn = mkTimeStr(oM);
         render = 0;
      }
      setText(COL_STAT, sn);
   }
}

void KEBListViewItem::setTmpStatus(QString status, QString &oldStatus) {
   KEBTopLevel *top = KEBTopLevel::self();
   QString url = m_bookmark.url().url();

   render = 2;
   setText(COL_STAT,status);
   oldStatus = top->Modify.contains(url) ? top->Modify[url] : "";
   top->Modify[url] = status;
   //  kdDebug() << "setStatus " << status << " old=" << oldStatus << "\n";
}

void KEBListViewItem::restoreStatus(QString oldStatus)
{
   KEBTopLevel *top = KEBTopLevel::self();
   QString url = m_bookmark.url().url();

   if (!oldStatus.isEmpty()) {
      top->Modify[url] = oldStatus;
   } else {
      top->Modify.remove(url);
   }
   modUpdate();
}

void KEBListViewItem::setOpen( bool open )
{
   m_bookmark.internalElement().setAttribute( "folded", open ? "no" : "yes" );
   QListViewItem::setOpen( open );
}

