/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

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

#include <qregexp.h>
#include <qtimer.h>
#include <qpainter.h>

#include <kdebug.h>
#include <klocale.h>

#include <krfcdate.h>
#include <kcharsets.h>
#include <kbookmarkmanager.h>

#include "toplevel.h"
#include "listview.h"
#include "testlink.h"
#include "bookmarkiterator.h"

#include <kaction.h>

TestLinkItrHolder *TestLinkItrHolder::s_self = 0;

TestLinkItrHolder::TestLinkItrHolder() 
   : BookmarkIteratorHolder() {
   // do stuff
}

void TestLinkItrHolder::doItrListChanged() {
   KEBTopLevel::self()->setCancelTestsEnabled(m_itrs.count() > 0);
}

TestLinkItr::TestLinkItr(QValueList<KBookmark> bks)
   : BookmarkIterator(bks) {

   m_job = 0;
}

// TODO - use m_done!!!

TestLinkItr::~TestLinkItr() {
   if (m_job) {
      kdDebug() << "JOB kill\n";
      curItem()->restoreStatus();
      m_job->disconnect();
      m_job->kill(false);
   }
}

bool TestLinkItr::isBlahable(const KBookmark &bk) {
   return (!bk.isGroup() && !bk.isSeparator());
}

void TestLinkItr::doBlah() {
   m_job = KIO::get(m_book.url(), true, false);
   connect(m_job, SIGNAL( result( KIO::Job *)),
           this, SLOT( slotJobResult(KIO::Job *)));
   connect(m_job, SIGNAL( data( KIO::Job *,  const QByteArray &)),
           this, SLOT( slotJobData(KIO::Job *, const QByteArray &)));
   m_job->addMetaData("errorPage", "true");
   curItem()->setTmpStatus(i18n("Checking..."));
}

void TestLinkItr::setItemMod(KEBListViewItem *item, QString modDate) {
   time_t modt = KRFCDate::parseDate(modDate);
   QString ms;
   ms.setNum(modt);
   item->nsPut(ms);
}

void TestLinkItr::slotJobData(KIO::Job *job, const QByteArray &data) {
   KIO::TransferJob *transfer = (KIO::TransferJob *)job;

   m_errSet = false;
   QString arrStr = data;

   if (transfer->isErrorPage()) {
      QStringList lines = QStringList::split('\n',arrStr);
      for (QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
         int open_pos = (*it).find("<title>", 0, false);
         if (open_pos >= 0) {
            QString leftover = (*it).mid(open_pos + 7);
            int close_pos = leftover.findRev("</title>", -1, false);
            if (close_pos >= 0) {
               // if no end tag found then just 
               // print the first line of the <title>
               leftover = leftover.left(close_pos);
            }
            curItem()->nsPut(KCharsets::resolveEntities(leftover));
            m_errSet = true;
            break;
         }
      }

   } else {
      QString modDate = transfer->queryMetaData("modified");
      if (!modDate.isEmpty()) {
         setItemMod(curItem(), modDate);
      }
   }

   transfer->kill(false);
}

void TestLinkItr::slotJobResult(KIO::Job *job) {
   m_job = 0;

   KIO::TransferJob *transfer = (KIO::TransferJob *)job;
   QString modDate = transfer->queryMetaData("modified");

   bool chkErr = true;
   if (transfer->error()) {
      // can we assume that errorString will contain no entities?
      QString jerr = job->errorString();
      if (!jerr.isEmpty()) {
         jerr.replace(QRegExp("\n"), " ");
         curItem()->nsPut(jerr);
         chkErr = false;
      }
   }
   
   if (chkErr) {
      if (!modDate.isEmpty()) {
         setItemMod(curItem(), modDate);
      } else if (!m_errSet) {
         setItemMod(curItem(), "0");
      }
   }

   curItem()->modUpdate();
   delayedEmitNextOne();
}

void TestLinkItr::paintCellHelper(QPainter *p, QColorGroup &cg, int style) {
   if (style == 0) {
      int h, s, v;
      cg.background().hsv(&h,&s,&v);
      cg.setColor(
         QColorGroup::Text, 
         (v > 180 && v < 220) ? (Qt::darkGray) : (Qt::gray));

   } else if (style == 2) {
      QFont font = p->font();
      font.setBold(true);
      p->setFont(font);
   }
}

static QString mkTimeStr(int b) {
   QDateTime dt;
   dt.setTime_t(b);
   return (dt.daysTo(QDateTime::currentDateTime()) > 31)
        ? KGlobal::locale()->formatDate(dt.date(), false)
        : KGlobal::locale()->formatDateTime(dt, false);
}

void TestLinkItrHolder::blah2(QString url, QString val) {
   if (!val.isEmpty()) {
      self()->m_modify[url] = val;
   } else {
      self()->m_modify.remove(url);
   }
}

QString TestLinkItrHolder::getMod(QString url) {
   return self()->m_modify.contains(url) 
        ? self()->m_modify[url] 
        : QString::null;
}

QString TestLinkItrHolder::getOldMod(QString url) {
   return self()->m_oldModify.contains(url) 
        ? self()->m_oldModify[url] 
        : QString::null;
}

void TestLinkItrHolder::setMod(QString url, QString val) {
   self()->m_modify[url] = val;
}

void TestLinkItrHolder::setOldMod(QString url, QString val) {
   self()->m_oldModify[url] = val;
}

QString TestLinkItrHolder::calcPaintStyle(QString url, int &paintstyle, QString nsinfo) {
   bool newModValid = false;
   int newMod = 0;

   // get new mod date if there is one
   QString newModStr = getMod(url);
   if (!newModStr.isNull()) {
      newMod = newModStr.toInt(&newModValid);
   }

   QString oldModStr;

   if (getOldMod(url).isNull()) {
      // first time
      oldModStr = nsinfo;
      setOldMod(url, oldModStr);

   } else if (!newModStr.isNull()) {
      // umm... nsGet not called here, missing optimisation?
      oldModStr = getOldMod(url);

   } else { 
      // may be reading a second bookmark with same url
      QString oom = nsinfo;
      oldModStr = getOldMod(url);
      if (oom.toInt() > oldModStr.toInt()) {
         setOldMod(url, oom);
         oldModStr = oom;
      }
   }

   int oldMod = oldModStr.toInt(); // TODO - check validity?
   QString statusStr;

   if (!newModStr.isNull() && !newModValid) { 
      // error in current check
      statusStr = newModStr;
      paintstyle = (oldMod == 1) ? 1 : 2;

   } else if (!newModStr.isNull() && (newMod == 0)) { 
      // no modify time returned
      statusStr = i18n("Ok");
      paintstyle = 0;

   } else if (!newModStr.isNull() && (newMod >= oldMod)) { 
      // info from current check
      statusStr = mkTimeStr(newMod);
      paintstyle = (newMod == oldMod) ? 1 : 2;

   } else if (oldMod == 1) { 
      // error in previous check
      statusStr = i18n("Error");
      paintstyle = 0;

   } else if (oldMod != 0) { 
      // info from previous check
      statusStr = mkTimeStr(oldMod);
      paintstyle = 0;
   }

   return statusStr;
}

static void parseNsInfo(QString nsinfo, QString &nCreate, QString &nAccess, QString &nModify) {
   QStringList sl = QStringList::split(' ', nsinfo);

   for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
      QStringList spl = QStringList::split('"', (*it));

      if (spl[0] == "LAST_MODIFIED=") {
         nModify = spl[1];
      } else if (spl[0] == "ADD_DATE=") {
         nCreate = spl[1];
      } else if (spl[0] == "LAST_VISIT=") {
         nAccess = spl[1];
      }
   }
}

QString KEBListViewItem::nsGet() {
   QString nCreate, nAccess, nModify;
   QString nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
   parseNsInfo(nsinfo, nCreate, nAccess, nModify);
   return nModify;
}

static QString updateNsInfoMod(QString _nsinfo, QString nm) {
   QString nCreate, nAccess, nModify;
   parseNsInfo(_nsinfo, nCreate, nAccess, nModify);

   bool numValid = false;
   nm.toInt(&numValid);

   QString tmp;
   tmp  =  "ADD_DATE=\"" + ((nCreate.isEmpty()) ? QString::number(time(0)) : nCreate) + "\"";
   tmp += " LAST_VISIT=\"" + ((nAccess.isEmpty()) ? "0" : nAccess) + "\"";
   tmp += " LAST_MODIFIED=\"" + ((numValid) ? nm : "1") + "\"";

   return tmp;
}

void KEBListViewItem::nsPut(QString nm) {
   QString tmp = updateNsInfoMod(m_bookmark.internalElement().attribute("netscapeinfo"), nm);
   m_bookmark.internalElement().setAttribute("netscapeinfo", tmp);

   KEBTopLevel::self()->setModifiedFlag(true);
   setText(KEBListView::StatusColumn, nm);
   TestLinkItrHolder::setMod(m_bookmark.url().url(), nm);
}

void KEBListViewItem::modUpdate() {
   QString statusLine;
   statusLine = TestLinkItrHolder::calcPaintStyle(m_bookmark.url().url(), m_paintstyle, nsGet());
   setText(KEBListView::StatusColumn, statusLine);
}

void KEBListViewItem::setTmpStatus(QString status) {
   QString url = m_bookmark.url().url();
   m_paintstyle = 2;
   setText(KEBListView::StatusColumn,status);
   m_oldStatus = TestLinkItrHolder::getMod(url);
   TestLinkItrHolder::setMod(url, status);
}

void KEBListViewItem::restoreStatus() {
   TestLinkItrHolder::blah2(m_bookmark.url().url(), m_oldStatus);
   modUpdate();
}

#include "testlink.moc"
