/* This file is part of the KDE project
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
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
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

#include <favicons.h>

FavIconUpdater * FavIconUpdater::s_self = 0L;

FavIconWebGrabber::FavIconWebGrabber( KHTMLPart * part, const KURL & url )
    : m_part(part), m_url(url)
{
    kdDebug(26000) << "FavIconWebGrabber::FavIconWebGrabber starting get" << endl;
    KIO::Job * job = KIO::get(m_url, false, false);
    connect( job, SIGNAL( result( KIO::Job *)),
             this, SLOT( slotFinished(KIO::Job *)));
    connect( job, SIGNAL( mimetype( KIO::Job *, const QString &)),
             this, SLOT( slotMimetype(KIO::Job *, const QString &)));
}

void FavIconWebGrabber::slotMimetype( KIO::Job *job, const QString &_type )
{
    KIO::SimpleJob *sjob = static_cast<KIO::SimpleJob *>(job);
    // Update our URL in case of a redirection
    m_url = sjob->url();
    QString type = _type; // necessary copy if we plan to use it
    sjob->putOnHold();
    // What to do if type is not text/html ??
    kdDebug(26000) << "slotMimetype : " << type << endl;
    // Now open the URL in the part
    m_part->openURL( m_url );
}

void FavIconWebGrabber::slotFinished( KIO::Job * job )
{
    // The whole point of all this is to abort silently on error
    if (job->error())
    {
        kdDebug(26000) << job->errorString() << endl;
    }
}


void FavIconUpdater::slotCompleted()
{
    kdDebug(26000) << "FavIconUpdater::slotCompleted" << endl;
    // we either got the icon now, or we give up
    // therefore, move on to the next one
}

FavIconUpdater * FavIconUpdater::self() {
   if ( !s_self )
      s_self = new FavIconUpdater( kapp, "FavIconUpdater" );
   return s_self;
}

FavIconUpdater::FavIconUpdater( QObject *parent, const char *name )
   : KonqFavIconMgr( parent, name )
{
}

FavIconUpdater::~FavIconUpdater()
{
   s_self = 0L;
}

void FavIconUpdater::queueIcon(const KBookmark &bk) {
   // TODO - should add to a queue.
   downloadIcon(bk);
}

void FavIconUpdater::downloadIcon(const KBookmark &bk) {
   QString favicon = KonqFavIconMgr::iconForURL(bk.url().url());
   if (favicon != QString::null) {
      kdDebug(26000) << "downloadIcon() - favicon" << favicon << endl;
      bk.internalElement().setAttribute("icon",favicon);
      KEBTopLevel::self()->slotCommandExecuted();
   } else {
      KonqFavIconMgr::downloadHostIcon(bk.url());
      favicon = KonqFavIconMgr::iconForURL(bk.url().url());
      kdDebug(26000) << "favicon == " << favicon << endl;;
      if (favicon == QString::null) {
         downloadIconComplex(bk);
      }
   }
}

void FavIconUpdater::downloadIconComplex(const KBookmark &bk) {
   kdDebug(26000) << "woop." << endl;

   m_bk = bk;

   KHTMLPart *part = new KHTMLPart;

   part->widget()->resize(1,1);
   part->hide();

   part->setPluginsEnabled(false);
   part->setJScriptEnabled(false);
   part->setJavaEnabled(false);
   part->setAutoloadImages(false);

   ((QScrollView *)part->widget())->setHScrollBarMode( QScrollView::AlwaysOff );
   ((QScrollView *)part->widget())->setVScrollBarMode( QScrollView::AlwaysOff );

   m_part = part;

   connect( part, SIGNAL( canceled(const QString &) ),
            this, SLOT( slotCompleted() ) );
   connect( part, SIGNAL( completed() ),
            this, SLOT( slotCompleted() ) );

   KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject( m_part );
   if (!ext) return;

   m_browserIface = new FavIconBrowserInterface( this, "browseriface" );
   ext->setBrowserInterface( m_browserIface );

   connect( ext, SIGNAL( setIconURL( const KURL & ) ),
            this, SLOT( setIconURL( const KURL & ) ) );

   // Now start getting, to ensure mimetype and possible connection
   FavIconWebGrabber * run = new FavIconWebGrabber( part, bk.url() );
}

FavIconBrowserInterface::FavIconBrowserInterface( FavIconUpdater *view, const char *name )
   : KParts::BrowserInterface( view, name )
{
   m_view = view;
}

// callback from khtml
void FavIconUpdater::setIconURL( const KURL & iconURL )
{
   kdDebug(26000) << "setIconURL called" << endl;
   this->setIconForURL( m_bk.url(), iconURL );
   // AK - this in turn calls notifyChange
   //      not really sure why we don't just call it directly...
}

void FavIconUpdater::notifyChange( bool isHost, QString hostOrURL, QString iconName )
{
// 1. here - add (store link to bk <-> qstring url) map
// 2. add a new class for faviconmgr
// 3. on notifyChanged update the bk's with given url
// 4. make a khtml part for each url with a browserextensions part thingy...

   kdDebug(26000) << "notifyChange called" << endl;
   m_bk.internalElement().setAttribute("icon",iconName);
   /*
   kdDebug(26000) << "!!!- " << isHost << endl;;
   kdDebug() << "!!!- " << hostOrURL << "==" << m_bk.url().url() << "-> "
                        << iconName << endl;
   */

   KEBTopLevel::self()->slotCommandExecuted();
}

#include "favicons.moc"
