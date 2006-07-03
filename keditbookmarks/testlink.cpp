// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "toplevel.h"
#include "testlink.h"
#include "commands.h"
#include "bookmarkiterator.h"

#include <QTimer>
#include <QPainter>

#include <kdebug.h>

#include <krfcdate.h>
#include <kcharsets.h>
#include <kbookmarkmanager.h>

#include <kaction.h>
#include <klocale.h>

TestLinkItrHolder *TestLinkItrHolder::s_self = 0;

TestLinkItrHolder::TestLinkItrHolder()
    : BookmarkIteratorHolder() {
    // do stuff
}

void TestLinkItrHolder::doItrListChanged() {
    KEBApp::self()->setCancelTestsEnabled(count() > 0);
    if(count() == 0)
    {
        kDebug()<<"Notifing managers "<<m_affectedBookmark<<endl;
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(m_affectedBookmark).toGroup());
        m_affectedBookmark.clear();
    }
}

void TestLinkItrHolder::addAffectedBookmark( const QString & address )
{
    kDebug()<<"addAffectedBookmark "<<address<<endl;
    if(m_affectedBookmark.isNull())
        m_affectedBookmark = address;
    else
        m_affectedBookmark = KBookmark::commonParent(m_affectedBookmark, address);
    kDebug()<<" m_affectedBookmark is now "<<m_affectedBookmark<<endl;
}

/* -------------------------- */

TestLinkItr::TestLinkItr(QList<KBookmark> bks)
    : BookmarkIterator(bks) {
    m_job = 0;
}

TestLinkItr::~TestLinkItr() {
    //FIXME set status
    if (m_job) {
        // kDebug() << "JOB kill\n";
        m_job->disconnect();
        m_job->doKill();
    }
}

bool TestLinkItr::isApplicable(const KBookmark &bk) const {
    return (!bk.isGroup() && !bk.isSeparator());
}

void TestLinkItr::doAction() {
    m_errSet = false;

    m_job = KIO::get(curBk().url(), true, false);
    m_job->addMetaData("errorPage", "true");
    m_job->addMetaData( QString("cookies"), QString("none") );

    connect(m_job, SIGNAL( result( KJob *)),
            this, SLOT( slotJobResult(KJob *)));
    connect(m_job, SIGNAL( data( KIO::Job *, const QByteArray &)),
            this, SLOT( slotJobData(KIO::Job *, const QByteArray &)));

    //FIXME curItem()->setTmpStatus(i18n("Checking..."));
    QString oldModDate = TestLinkItrHolder::self()->getMod(curBk().url().url());
    //FIXME curItem()->setOldStatus(oldModDate);
    TestLinkItrHolder::self()->setMod(curBk().url().url(), i18n("Checking..."));
}

void TestLinkItr::slotJobData(KIO::Job *job, const QByteArray &data) {
    KIO::TransferJob *transfer = (KIO::TransferJob *)job;

    if (transfer->isErrorPage()) {
        QString strData(data);
        QStringList lines = strData.split('\n');
        for (QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
            int open_pos = (*it).indexOf("<title>", 0, Qt::CaseInsensitive);
            if (open_pos >= 0) {
                QString leftover = (*it).mid(open_pos + 7);
                int close_pos = leftover.lastIndexOf( "</title>", -1, Qt::CaseInsensitive );
                if (close_pos >= 0) {
                    // if no end tag found then just
                    // print the first line of the <title>
                    leftover = leftover.left(close_pos);
                }
                //FIXME curItem()->nsPut(KCharsets::resolveEntities(leftover));
                m_errSet = true;
                break;
            }
        }

    } else {
        QString modDate = transfer->queryMetaData("modified");
        if (!modDate.isEmpty()) {
            //FIXME curItem()->nsPut(QString::number(KRFCDate::parseDate(modDate)));
        }
    }

    transfer->doKill();
}

void TestLinkItr::slotJobResult(KJob *job) {
    m_job = 0;
    //FIXME if ( !curItem() ) return;

    KIO::TransferJob *transfer = (KIO::TransferJob *)job;
    QString modDate = transfer->queryMetaData("modified");

    bool chkErr = true;
    if (transfer->error()) {
        // can we assume that errorString will contain no entities?
        QString jerr = job->errorString();
        if (!jerr.isEmpty()) {
            jerr.replace("\n", " ");
            //FIXME curItem()->nsPut(jerr);
            chkErr = false;
        }
    }

    if (chkErr) {
        if (!modDate.isEmpty()) {
            //FIXME curItem()->nsPut(QString::number(KRFCDate::parseDate(modDate)));
        } else if (!m_errSet) {
            //FIXME curItem()->nsPut(QString::number(KRFCDate::parseDate("0")));
        }
    }

    //FIXME curItem()->modUpdate();
    holder()->addAffectedBookmark(KBookmark::parentAddress(curBk().address()));
    delayedEmitNextOne();
}

/* -------------------------- */

const QString TestLinkItrHolder::getMod(const QString &url) const {
    return m_modify.contains(url)
        ? m_modify[url]
        : QString();
}

const QString TestLinkItrHolder::getOldVisit(const QString &url) const {
    return self()->m_oldModify.contains(url)
        ? self()->m_oldModify[url]
        : QString();
}

void TestLinkItrHolder::setMod(const QString &url, const QString &val) {
    m_modify[url] = val;
}

void TestLinkItrHolder::setOldVisit(const QString &url, const QString &val) {
    m_oldModify[url] = val;
}

void TestLinkItrHolder::resetToValue(const QString &url, const QString &oldValue) {
    if (!oldValue.isEmpty()) {
        m_modify[url] = oldValue;
    } else {
        m_modify.remove(url);
    }
}

/* -------------------------- */

/*
QString TestLinkItrHolder::calcPaintStyle(const QString &url, KEBListViewItem::PaintStyle &_style,
                                          const QString &nVisit, const QString &Modify) {
    bool newModValid = false;
    int newMod = 0;
    QString newModStr;
    bool initial = false;
    bool oldError = false;

    if (!Modify.isNull() && Modify == "1") {
        oldError = true;
    }

    // get new mod time if there is one
    newModStr = self()->getMod(url);

    // if no new mod time use previous one
    if (newModStr.isNull()) {
        newModStr = Modify;
        initial = true;
    }

    if (!newModStr.isNull()) {
        newMod = newModStr.toInt(&newModValid);
    }


//    kDebug() << "TestLink " << url << " " << "booktime=" << nVisit << " urltime=" << newModStr <<
//               " Modify=" << Modify << " init=" << initial << " newMod=" << newMod << "\n";

    QString visitStr;

    if (self()->getOldVisit(url).isNull()) {
        // first time
        visitStr = nVisit;
        if (!nVisit.isEmpty())
            self()->setOldVisit(url, visitStr);
    } else {
        // may be reading a second bookmark with same url
        QString oom = nVisit;
        visitStr = self()->getOldVisit(url);
        if (oom.toInt() > visitStr.toInt()) {
            self()->setOldVisit(url, oom);
            visitStr = oom;
        }
    }

    int visit = 0;
    if (!visitStr.isNull())
        visit = visitStr.toInt(); // TODO - check validity?

    QString statusStr;
    KEBListViewItem::PaintStyle style = KEBListViewItem::DefaultStyle;

//    kDebug() << "TestLink " << "isNull=" << newModStr.isNull() << "newModValid="
//              << newModValid << "newMod > visit " << newMod << ">" << visit << "\n";

    if (!newModStr.isNull() && !newModValid) {
        // Current check has error
        statusStr = newModStr;
        if (oldError) {
            style = KEBListViewItem::BoldStyle;
        } else {
            style =  KEBListViewItem::DefaultStyle;
        }

    } else if (initial && oldError) {
        // Previous check has error
        style = KEBListViewItem::GreyStyle;
        statusStr = i18n("Error ");

    } else if (!initial && !newModStr.isNull() && (newMod == 0)) {
        // Current check has no modify time
        statusStr = i18n("Ok");

    } else if (initial && !newModStr.isNull() && (newMod == 0)) {
        // previous check has no modify time recorded
        statusStr.clear();

    } else if (!newModStr.isNull() && (newMod > visit)) {
        // if modify time greater than last visit, show bold modify time
        statusStr = CurrentMgr::makeTimeStr(newMod);
        if (initial) {
            style = KEBListViewItem::GreyBoldStyle;
        } else {
            style = KEBListViewItem::BoldStyle;
        }

    } else if (visit != 0) {
        // modify time not greater than last visit, show last visit time
        statusStr = CurrentMgr::makeTimeStr(visit);
        if (initial) {
                style = KEBListViewItem::GreyStyle;
        } else {
                style = KEBListViewItem::DefaultStyle;
        }

    } else {
        statusStr.clear();
    }

    _style = style;
    return statusStr;
}
*/
/*
static void parseInfo (KBookmark &bk, QString &nVisited) {
    nVisited =
        NodeEditCommand::getNodeText(bk, QStringList() << "info" << "metadata"
                                     << "time_visited" );

//    kDebug() << " Visited=" << nVisited << "\n";
}
*/

/*static void parseNsInfo(const QString &nsinfo, QString &nCreate, QString &nAccess, QString &nModify) {
    QStringList sl = nsinfo.split(' ');

    for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
        QStringList spl = (*it).split('"');

        if (spl[0] == "LAST_MODIFIED=") {
            nModify = spl[1];
        } else if (spl[0] == "ADD_DATE=") {
            nCreate = spl[1];
        } else if (spl[0] == "LAST_VISIT=") {
            nAccess = spl[1];
        }
    }
}*/

// Still use nsinfo for storing old modify time
/*static const QString updateNsInfoMod(const QString &_nsinfo, const QString &nm) {
    QString nCreate, nAccess, nModify;
    parseNsInfo(_nsinfo, nCreate, nAccess, nModify);

    bool numValid = false;
    nm.toInt(&numValid);

    QString tmp;
    tmp  =  "ADD_DATE=\"" + ((nCreate.isEmpty()) ? QString::number(time(0)) : nCreate) + "\"";
    tmp += " LAST_VISIT=\"" + ((nAccess.isEmpty()) ? QString("0") : nAccess) + "\"";
    tmp += " LAST_MODIFIED=\"" + ((numValid) ? nm : QString("1")) + "\"";

//  if (!numValid) kDebug() << tmp << "\n";
    return tmp;
}*/

// KEBListViewItem !!!!!!!!!!!
//FIXME nsPut
/*
void KEBListViewItem::nsPut(const QString &newModDate) {
    static const QString NetscapeInfoAttribute = "netscapeinfo";
    const QString info = m_bookmark.internalElement().attribute(NetscapeInfoAttribute);
    QString blah = updateNsInfoMod(info, newModDate);
    m_bookmark.internalElement().setAttribute(NetscapeInfoAttribute, blah);
    TestLinkItrHolder::self()->setMod(m_bookmark.url().url(), newModDate);
    setText(KEBListView::StatusColumn, newModDate);
}
*/

// KEBListViewItem !!!!!!!!!!!
/*
void KEBListViewItem::modUpdate() {
    QString nCreate, nAccess, oldModify;
    QString iVisit;

    QString nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
    if (!nsinfo.isEmpty()) {
        parseNsInfo(nsinfo, nCreate, nAccess, oldModify);
    }

    parseInfo(m_bookmark, iVisit);

    QString statusLine;
    statusLine = TestLinkItrHolder::calcPaintStyle(m_bookmark.url().url(), m_paintStyle, iVisit, oldModify);
    if (statusLine != "Error")
        setText(KEBListView::StatusColumn, statusLine);
}
*/
/* -------------------------- */

// KEBListViewItem !!!!!!!!!!!
/*
void KEBListViewItem::setOldStatus(const QString &oldStatus) {
    // kDebug() << "KEBListViewItem::setOldStatus" << endl;
    m_oldStatus = oldStatus;
}

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::setTmpStatus(const QString &status) {
    // kDebug() << "KEBListViewItem::setTmpStatus" << endl;
    m_paintStyle = KEBListViewItem::BoldStyle;
    setText(KEBListView::StatusColumn, status);
}

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::restoreStatus() {
    if (!m_oldStatus.isNull()) {
        // kDebug() << "KEBListViewItem::restoreStatus" << endl;
        TestLinkItrHolder::self()->resetToValue(m_bookmark.url().url(), m_oldStatus);
        modUpdate();
    }
}
*/
#include "testlink.moc"
