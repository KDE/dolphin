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
void KEBListViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
  QColorGroup col(cg);

  int h, s, v;

  if (column == 2) {
    if (render == 0) {
      col.background().hsv(&h,&s,&v);
      if (v >180 && v < 220) {
	col.setColor(QColorGroup::Text, Qt::darkGray);
      } else
	col.setColor(QColorGroup::Text, Qt::gray);
    } else if (render == 2) {
      QFont font=p->font();
      font.setBold( true );
      p->setFont(font);
    }
  }

  QListViewItem::paintCell( p, col, column, width, alignment );
}


void KEBListViewItem::init( const KBookmark & bk )
{
    setPixmap(0, SmallIcon( bk.icon() ) );
#ifdef DEBUG_ADDRESSES
    setText(3, bk.address());
#endif
    modUpdate();
}

// AK - TODO abstract this away, possibly split this rather large file up?

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

QString internal_nsPut(QString _nsinfo, QString nm) {

  QString nCreate, nAccess, nModify;

  internal_nsGet(_nsinfo, nCreate, nAccess, nModify);

  QString nsinfo;

  nsinfo  = "ADD_DATE=\"";
  nsinfo += (nCreate.isEmpty()) ? QString::number(time(0)) : nCreate;

  nsinfo += "\" LAST_VISIT=\"";
  nsinfo += (nAccess.isEmpty()) ? "0" : nAccess;

  nsinfo += "\" LAST_MODIFIED=\"";

  bool okNum = false;
  nm.toInt(&okNum);
  nsinfo += (okNum) ? nm : "1";

  nsinfo += "\"";

  return nsinfo;

}

void KEBListViewItem::nsPut( QString nm )
{
   QString _nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
   QString nsinfo = internal_nsPut(_nsinfo,nm);
   m_bookmark.internalElement().setAttribute("netscapeinfo",nsinfo);
   KEBTopLevel::self()->setModified(true);
   KEBTopLevel::self()->Modify[m_bookmark.url().url()] = nm;
   setText(2, nm);
}

// */ of nsinfo stuff

void KEBListViewItem::modUpdate( )
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
      if (!ok)
	nis = true;
    }

    if (top->oldModify.contains(url)) {
      if (nMod) {
	oModify = top->oldModify[url];
      } else { // may be reading a second bookmark with same url
	QString oom;
	nsGet(oom);
	int ood = oom.toInt();

	oModify = top->oldModify[url];
	int ond = oModify.toInt();

	if (ood > ond) {
	  top->oldModify[url] = oom;
	  oModify = oom;
	}
      }
    } else { // first time
      nsGet(oModify);
      top->oldModify[url] = oModify;
    }
    oM = oModify.toInt();
    if (oM == 1) {
      ois = true;
    }

    //    kdDebug() << "nMod=" << nMod << " nis=" << nis << " nM=" << nM << " oM=" << nM << "\n";
    QString sn;
    QDateTime dt;

    if (nMod && nis) { // Eror in current check
      sn = nModify;
      if (ois)
	render = 1;
      else
	render = 2;
    } else if (nMod && nM == 0) { // No modify time returned
      sn = i18n(".");
    } else if (nMod && nM >= oM) { // Info from current check
      dt.setTime_t(nM);
      if (dt.daysTo(QDateTime::currentDateTime()) > 31) {
	sn =  KGlobal::locale()->formatDate(dt.date(), false);
      } else {
	sn =  KGlobal::locale()->formatDateTime(dt, false);
      }
      if (nM > oM)
	render = 2;
      else {
	render = 1;
      }
    } else if (ois) { // Error in previous check
      sn = i18n("...Error...");
      render = 0;
    } else if (oM) { // Info from previous check
      dt.setTime_t(oM);
      if (dt.daysTo(QDateTime::currentDateTime()) > 31) {
	sn =  KGlobal::locale()->formatDate(dt.date(), false);
      } else {
	sn =  KGlobal::locale()->formatDateTime(dt, false);
      }
      render = 0;
    }
    setText(2,  sn);
  }
}

void KEBListViewItem::setTmpStatus(QString status, QString &oldStatus) {
  KEBTopLevel *top = KEBTopLevel::self();
  QString url = m_bookmark.url().url();

  render = 2;
  setText(2,status);
  if (top->Modify.contains(url)) {
    oldStatus = top->Modify[url];
  } else {
    oldStatus = "";
  }
  //  kdDebug() << "setStatus " << status << " old=" << oldStatus << "\n";
  top->Modify[url] = status;
}

void KEBListViewItem::restoreStatus( QString oldStatus)
{
  KEBTopLevel *top = KEBTopLevel::self();
  QString url = m_bookmark.url().url();

  if (! oldStatus.isEmpty()) {
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

