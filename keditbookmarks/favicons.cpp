/* This file is part of the KDE project
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

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>

#include <kio/job.h>

#include <kparts/part.h>
#include <kparts/componentfactory.h>
#include <kparts/browserextension.h>

#include "favicons.h"
#include "listview.h"
#include "toplevel.h"
#include "bookmarkiterator.h"

FavIconsItrHolder *FavIconsItrHolder::s_self = 0;

void FavIconsItrHolder::doItrListChanged() {
   KEBTopLevel::self()->setCancelFavIconUpdatesEnabled(m_itrs.count() > 0);
}

FavIconsItrHolder::FavIconsItrHolder() 
   : BookmarkIteratorHolder() {
   // do stuff
}

FavIconsItr::FavIconsItr(QValueList<KBookmark> bks)
   : BookmarkIterator(bks) {

   m_updater = 0;
   m_done = false;
}

FavIconsItr::~FavIconsItr() {
   delete m_updater;
   if (!m_done) {
      curItem()->restoreStatus();
   }
}

void FavIconsItr::slotDone(bool succeeded) {
   kdDebug(26000) << "FavIconsItr::slotDone()" << endl;
   m_done = true;
   curItem()->setTmpStatus(succeeded ? i18n("OK") : i18n("No favicon found"));
   delayedEmitNextOne();
}

bool FavIconsItr::isBlahable(const KBookmark &bk) {
   return (!bk.isGroup() && !bk.isSeparator());
}

void FavIconsItr::doBlah() {
   kdDebug(26000) << "FavIconsItr::doBlah()" << endl;
   m_done = false;
   curItem()->setTmpStatus(i18n("Updating favicon..."));
   if (!m_updater) {
      m_updater = new FavIconUpdater(kapp, "FavIconUpdater");
      connect(m_updater, SIGNAL( done(bool) ),
              this,      SLOT( slotDone(bool) ) );
   }
   m_updater->downloadIcon(m_book);
   // TODO - a single shot timeout?
}

/* ---------------------------------------------------------------------------------- */

// NB the whole point of all this (KIO rather than KHTML directly???) is to abort silently on error

FavIconWebGrabber::FavIconWebGrabber(KParts::ReadOnlyPart *part, const KURL &url)
  : m_part(part), m_url(url) {

   kdDebug(26000) << "FavIconWebGrabber::FavIconWebGrabber starting get" << endl;
   KIO::Job *job = KIO::get(m_url, false, false);
   connect(job, SIGNAL( result( KIO::Job *)),
           this, SLOT( slotFinished(KIO::Job *) ));
   connect(job, SIGNAL( mimetype( KIO::Job *, const QString &) ),
           this, SLOT( slotMimetype(KIO::Job *, const QString &) ));
}

void FavIconWebGrabber::slotMimetype(KIO::Job *job, const QString &typeUncopied) {
   // update our URL in case of a redirection
   KIO::SimpleJob *sjob = static_cast<KIO::SimpleJob *>(job);
   m_url = sjob->url();
   sjob->putOnHold();

   QString type = typeUncopied; // necessary copy if we plan to use it
   kdDebug(26000) << "slotMimetype : " << type << endl;
   // FIXME - what to do if type is not text/html ??

   m_part->openURL(m_url);
}

void FavIconWebGrabber::slotFinished(KIO::Job *job) {
   if (job->error()) {
      kdDebug(26000) << job->errorString() << endl;
   }
}

FavIconUpdater::FavIconUpdater(QObject *parent, const char *name)
   : KonqFavIconMgr(parent, name) {
   ;
}

void FavIconUpdater::slotCompleted() {
   kdDebug(26000) << "FavIconUpdater::slotCompleted" << endl;
   kdDebug(26000) << "emit done(true)" << endl;
   emit done(true);
}

void FavIconUpdater::downloadIcon(const KBookmark &bk) {
   QString favicon = KonqFavIconMgr::iconForURL(bk.url().url());
   if (favicon != QString::null) {
      kdDebug(26000) << "downloadIcon() - favicon" << favicon << endl;
      bk.internalElement().setAttribute("icon", favicon);
      kdDebug(26000) << "favicon - emitSlotCommandExecuted()" << favicon << endl;
      KEBTopLevel::self()->emitSlotCommandExecuted();
      kdDebug(26000) << "emit done(true)" << endl;
      emit done(true);

   } else {
      KonqFavIconMgr::downloadHostIcon(bk.url());
      favicon = KonqFavIconMgr::iconForURL(bk.url().url());
      kdDebug(26000) << "favicon == " << favicon << endl;;
      if (favicon == QString::null) {
         downloadIconActual(bk);
      }
   }
}

void FavIconUpdater::downloadIconActual(const KBookmark &bk) {
   kdDebug(26000) << "woop. umm.. whatever that means" << endl;

   m_bk = bk;

   KParts::ReadOnlyPart *part 
      = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>
           ("text/html" /* mimetype */, QString::null /* constraint */);
           // parentWidget, widgetName, parent, name);

   part->setProperty("pluginsEnabled", QVariant(false, 1));
   part->setProperty("javaScriptEnabled", QVariant(false, 1));
   part->setProperty("javaEnabled", QVariant(false, 1));
   part->setProperty("autoloadImages", QVariant(false, 1));

   /*
   part->widget()->resize(1,1);
   part->hide();

   ((QScrollView *)part->widget())->setHScrollBarMode(QScrollView::AlwaysOff);
   ((QScrollView *)part->widget())->setVScrollBarMode(QScrollView::AlwaysOff);
   */

   m_part = part;

   connect(part, SIGNAL( canceled(const QString &) ),
           this, SLOT( slotCompleted() ));
   connect(part, SIGNAL( completed() ),
           this, SLOT( slotCompleted() ));

   KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(m_part);
   if (!ext) {
      return;
   }

   m_browserIface = new FavIconBrowserInterface(this, "browseriface");
   ext->setBrowserInterface(m_browserIface);

   connect(ext, SIGNAL( setIconURL(const KURL &) ),
           this, SLOT( setIconURL(const KURL &) ));

   // why "new" ?
   new FavIconWebGrabber(part, bk.url());
   // TODO return
}

FavIconBrowserInterface::FavIconBrowserInterface(FavIconUpdater *view, const char *name)
   : KParts::BrowserInterface(view, name) {

   m_view = view;
}

// khtml callback
void FavIconUpdater::setIconURL(const KURL &iconURL) {
   setIconForURL(m_bk.url(), iconURL);
}

void FavIconUpdater::notifyChange(bool isHost, QString hostOrURL, QString iconName) {
   kdDebug(26000) << "notifyChange called" << endl;

   m_bk.internalElement().setAttribute("icon", iconName);

   kdDebug(26000) << "!!!- " << isHost << endl;;
   kdDebug(26000) << "!!!- " << hostOrURL << "==" << m_bk.url().url() << "-> " << iconName << endl;

   KEBTopLevel::self()->emitSlotCommandExecuted();
}

#include "favicons.moc"
