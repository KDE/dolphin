// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "toplevel.h"
#include "listview.h"
#include "testlink.h"
#include "bookmarkiterator.h"

#include <qtimer.h>
#include <qpainter.h>

#include <kdebug.h>
#include <klocale.h>

#include <krfcdate.h>
#include <kcharsets.h>
#include <kbookmarkmanager.h>

#include <kaction.h>

TestLinkItrHolder *TestLinkItrHolder::s_self = 0;

TestLinkItrHolder::TestLinkItrHolder() 
    : BookmarkIteratorHolder() {
    // do stuff
}

void TestLinkItrHolder::doItrListChanged() {
    KEBApp::self()->setCancelTestsEnabled(count() > 0);
}

/* -------------------------- */

TestLinkItr::TestLinkItr(QValueList<KBookmark> bks)
    : BookmarkIterator(bks) {
    m_job = 0;
}

TestLinkItr::~TestLinkItr() {
    if (m_job) {
        // kdDebug() << "JOB kill\n";
        curItem()->restoreStatus();
        m_job->disconnect();
        m_job->kill(false);
    }
}

bool TestLinkItr::isApplicable(const KBookmark &bk) const {
    return (!bk.isGroup() && !bk.isSeparator());
}

void TestLinkItr::doAction() {
    m_errSet = false;

    m_job = KIO::get(curBk().url(), true, false);
    m_job->addMetaData("errorPage", "true");

    connect(m_job, SIGNAL( result( KIO::Job *)),
            this, SLOT( slotJobResult(KIO::Job *)));
    connect(m_job, SIGNAL( data( KIO::Job *,  const QByteArray &)),
            this, SLOT( slotJobData(KIO::Job *, const QByteArray &)));

    curItem()->setTmpStatus(i18n("Checking..."));
    QString oldModDate = TestLinkItrHolder::self()->getMod(curBk().url().url());
    curItem()->setOldStatus(oldModDate);
    TestLinkItrHolder::self()->setMod(curBk().url().url(), i18n("Checking..."));
}

void TestLinkItr::slotJobData(KIO::Job *job, const QByteArray &data) {
    KIO::TransferJob *transfer = (KIO::TransferJob *)job;

    if (transfer->isErrorPage()) {
        QStringList lines = QStringList::split('\n', data);
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
            curItem()->nsPut(QString::number(KRFCDate::parseDate(modDate)));
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
            jerr.replace("\n", " ");
            curItem()->nsPut(jerr);
            chkErr = false;
        }
    }

    if (chkErr) {
        if (!modDate.isEmpty()) {
            curItem()->nsPut(QString::number(KRFCDate::parseDate(modDate)));
        } else if (!m_errSet) {
            curItem()->nsPut(QString::number(KRFCDate::parseDate("0")));
        }
    }

    curItem()->modUpdate();
    delayedEmitNextOne();
}

/* -------------------------- */

const QString TestLinkItrHolder::getMod(const QString &url) const {
    return m_modify.contains(url) 
        ? m_modify[url] 
        : QString::null;
}

const QString TestLinkItrHolder::getOldMod(const QString &url) const {
    return self()->m_oldModify.contains(url) 
        ? self()->m_oldModify[url] 
        : QString::null;
}

void TestLinkItrHolder::setMod(const QString &url, const QString &val) {
    m_modify[url] = val;
}

void TestLinkItrHolder::setOldMod(const QString &url, const QString &val) {
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

static QString mkTimeStr(int b) {
    QDateTime dt;
    dt.setTime_t(b);
    return (dt.daysTo(QDateTime::currentDateTime()) > 31)
        ? KGlobal::locale()->formatDate(dt.date(), false)
        : KGlobal::locale()->formatDateTime(dt, false);
}

QString TestLinkItrHolder::calcPaintStyle(const QString &url, KEBListViewItem::PaintStyle &_style, const QString &nsinfo) {
    bool newModValid = false;
    int newMod = 0;

    // get new mod date if there is one
    QString newModStr = self()->getMod(url);
    if (!newModStr.isNull()) {
        newMod = newModStr.toInt(&newModValid);
    }

    QString oldModStr;

    if (self()->getOldMod(url).isNull()) {
        // first time
        oldModStr = nsinfo;
        if (!nsinfo.isEmpty())
            self()->setOldMod(url, oldModStr);

    } else if (!newModStr.isNull()) {
        oldModStr = self()->getOldMod(url);

    } else { 
        // may be reading a second bookmark with same url
        QString oom = nsinfo;
        oldModStr = self()->getOldMod(url);
        if (oom.toInt() > oldModStr.toInt()) {
            self()->setOldMod(url, oom);
            oldModStr = oom;
        }
    }

    int oldMod = 0;
    if (!oldModStr.isNull())
        oldMod = oldModStr.toInt(); // TODO - check validity?

    QString statusStr;
    KEBListViewItem::PaintStyle style = KEBListViewItem::DefaultStyle;

    if (!newModStr.isNull() && !newModValid) { 
        // error in current check
        statusStr = newModStr;
        style = (oldMod == 1) 
            ? KEBListViewItem::DefaultStyle : KEBListViewItem::BoldStyle;

    } else if (!newModStr.isNull() && (newMod == 0)) { 
        // no modify time returned
        statusStr = i18n("Ok");

    } else if (!newModStr.isNull() && (newMod >= oldMod)) { 
        // info from current check
        statusStr = mkTimeStr(newMod);
        style = (newMod == oldMod) 
            ? KEBListViewItem::DefaultStyle : KEBListViewItem::BoldStyle;

    } else if (oldMod == 1) { 
        // error in previous check
        statusStr = i18n("Error");

    } else if (oldMod != 0) { 
        // info from previous check
        statusStr = mkTimeStr(oldMod);

    } else {
        statusStr = QString::null;
    }

    _style = style;
    return statusStr;
}

static void parseNsInfo(const QString &nsinfo, QString &nCreate, QString &nAccess, QString &nModify) {
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

static const QString updateNsInfoMod(const QString &_nsinfo, const QString &nm) {
    QString nCreate, nAccess, nModify;
    parseNsInfo(_nsinfo, nCreate, nAccess, nModify);

    bool numValid = false;
    nm.toInt(&numValid);

    QString tmp;
    tmp  =  "ADD_DATE=\"" + ((nCreate.isEmpty()) ? QString::number(time(0)) : nCreate) + "\"";
    tmp += " LAST_VISIT=\"" + ((nAccess.isEmpty()) ? QString("0") : nAccess) + "\"";
    tmp += " LAST_MODIFIED=\"" + ((numValid) ? nm : QString("1")) + "\"";

    return tmp;
}

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::nsPut(const QString &newModDate) {
    static const QString NetscapeInfoAttribute = "netscapeinfo";
    const QString info = m_bookmark.internalElement().attribute(NetscapeInfoAttribute);
    QString blah = updateNsInfoMod(info, newModDate);
    m_bookmark.internalElement().setAttribute(NetscapeInfoAttribute, blah);
    TestLinkItrHolder::self()->setMod(m_bookmark.url().url(), newModDate);
    setText(KEBListView::StatusColumn, newModDate);
    KEBApp::self()->setModifiedFlag(true);
}

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::modUpdate() {
    QString nCreate, nAccess, nModify;
    QString nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
    if (!nsinfo.isEmpty())
        parseNsInfo(nsinfo, nCreate, nAccess, nModify);
    QString statusLine;
    statusLine = TestLinkItrHolder::calcPaintStyle(m_bookmark.url().url(), m_paintStyle, nModify);
    if (statusLine != "Error")
        setText(KEBListView::StatusColumn, statusLine);
}

/* -------------------------- */

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::setOldStatus(const QString &oldStatus) {
    // kdDebug() << "KEBListViewItem::setOldStatus" << endl;
    m_oldStatus = oldStatus;
}

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::setTmpStatus(const QString &status) {
    // kdDebug() << "KEBListViewItem::setTmpStatus" << endl;
    m_paintStyle = KEBListViewItem::BoldStyle;
    setText(KEBListView::StatusColumn, status);
}

// KEBListViewItem !!!!!!!!!!!
void KEBListViewItem::restoreStatus() {
    if (!m_oldStatus.isNull()) {
        // kdDebug() << "KEBListViewItem::restoreStatus" << endl;
        TestLinkItrHolder::self()->resetToValue(m_bookmark.url().url(), m_oldStatus);
        modUpdate();
    }
}

#include "testlink.moc"
