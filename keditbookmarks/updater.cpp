// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "updater.h"

#include "bookmarkiterator.h"
#include "toplevel.h"

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>

#include <kio/job.h>

#include <kmimetype.h>
#include <kparts/part.h>
#include <kparts/componentfactory.h>
#include <kparts/browserextension.h>
#include <assert.h>

FavIconUpdater::FavIconUpdater(QObject *parent)
    : KonqFavIconMgr(parent) {
    m_part = 0;
    m_webGrabber = 0;
    m_browserIface = 0;
}

void FavIconUpdater::slotCompleted() {
    // kDebug() << "FavIconUpdater::slotCompleted" << endl;
    // kDebug() << "emit done(true)" << endl;
    emit done(true);
}

void FavIconUpdater::downloadIcon(const KBookmark &bk) {
    QString favicon = KMimeType::favIconForUrl(bk.url().url());
    if (!favicon.isNull()) {
        // kDebug() << "downloadIcon() - favicon" << favicon << endl;
        bk.internalElement().setAttribute("icon", favicon);
        KEBApp::self()->notifyCommandExecuted();
        // kDebug() << "emit done(true)" << endl;
        emit done(true);

    } else {
        KonqFavIconMgr::downloadHostIcon(bk.url());
        favicon = KMimeType::favIconForUrl(bk.url().url());
        // kDebug() << "favicon == " << favicon << endl;
        if (favicon.isNull()) {
            downloadIconActual(bk);
        }
    }
}

FavIconUpdater::~FavIconUpdater() {
    // kDebug() << "~FavIconUpdater" << endl;
    delete m_browserIface;
    delete m_webGrabber;
    delete m_part;
}

void FavIconUpdater::downloadIconActual(const KBookmark &bk) {
    m_bk = bk;

    if (!m_part) {
        KParts::ReadOnlyPart *part
            = KParts::ComponentFactory
            ::createPartInstanceFromQuery<KParts::ReadOnlyPart>("text/html", QString());

        part->setProperty("pluginsEnabled", QVariant(false));
        part->setProperty("javaScriptEnabled", QVariant(false));
        part->setProperty("javaEnabled", QVariant(false));
        part->setProperty("autoloadImages", QVariant(false));

        connect(part, SIGNAL( canceled(const QString &) ),
                this, SLOT( slotCompleted() ));
        connect(part, SIGNAL( completed() ),
                this, SLOT( slotCompleted() ));

        KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(part);
        assert(ext);

        m_browserIface = new FavIconBrowserInterface(this);
        ext->setBrowserInterface(m_browserIface);

        connect(ext, SIGNAL( setIconURL(const KUrl &) ),
                this, SLOT( setIconURL(const KUrl &) ));

        m_part = part;
    }

    m_webGrabber = new FavIconWebGrabber(m_part, bk.url());
}

// khtml callback
void FavIconUpdater::setIconURL(const KUrl &iconURL) {
    setIconForURL(m_bk.url(), iconURL);
}

void FavIconUpdater::notifyChange(bool isHost, QString hostOrURL, QString iconName) {
    // kDebug() << "FavIconUpdater::notifyChange()" << endl;

    Q_UNUSED(isHost);
    // kDebug() << isHost << endl;
    Q_UNUSED(hostOrURL);
    // kDebug() << hostOrURL << "==" << m_bk.url().url() << "-> " << iconName << endl;

    m_bk.internalElement().setAttribute("icon", iconName);
    KEBApp::self()->notifyCommandExecuted();
}

/* -------------------------- */

FavIconWebGrabber::FavIconWebGrabber(KParts::ReadOnlyPart *part, const KUrl &url)
    : m_part(part), m_url(url) {

    // kDebug() << "FavIconWebGrabber::FavIconWebGrabber starting KIO::get()" << endl;

    // the use of KIO rather than directly using KHTML is to allow silently abort on error

    KIO::Job *job = KIO::get(m_url, false, false);
    job->addMetaData( QString("cookies"), QString("none") );
    connect(job, SIGNAL( result( KJob *)),
            this, SLOT( slotFinished(KJob *) ));
    connect(job, SIGNAL( mimetype( KIO::Job *, const QString &) ),
            this, SLOT( slotMimetype(KIO::Job *, const QString &) ));
}

void FavIconWebGrabber::slotMimetype(KIO::Job *job, const QString & /*type*/) {
    KIO::SimpleJob *sjob = static_cast<KIO::SimpleJob *>(job);
    m_url = sjob->url(); // allow for redirection
    sjob->putOnHold();

    // QString typeLocal = typeUncopied; // local copy
    // kDebug() << "slotMimetype : " << typeLocal << endl;
    // TODO - what to do if typeLocal is not text/html ??

    m_part->openUrl(m_url);
}

void FavIconWebGrabber::slotFinished(KJob *job) {
    if (job->error()) {
        // kDebug() << "FavIconWebGrabber::slotFinished() " << job->errorString() << endl;
    }
}

#include "updater.moc"
