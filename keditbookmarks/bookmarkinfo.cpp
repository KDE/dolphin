// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "bookmarkinfo.h"
#include "commands.h"

#include <stdlib.h>

#include <qclipboard.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>
#include <kdebug.h>

#include <kapplication.h>
#include <kstdaction.h>
#include <kaction.h>
#include <dcopclient.h>
#include <dcopref.h>

#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kfiledialog.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

// SHUFFLE all these functions around, the order is just plain stupid

// rename to something else
static QString blah(QString in)
{
    int secs;
    bool ok;
    secs = in.toInt(&ok);
    if (!ok)
        return QString::null;
    QDateTime td;
    td.setTime_t(secs);
    return td.toString("dd.MM.yyyy - hh:mm");
}

void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {

    m_bk = bk;

    if (m_bk.isNull()) {
        // all read only and blank

        m_title_le->setReadOnly(true);
        m_title_le->setText(QString::null);

        m_url_le->setReadOnly(true);
        m_url_le->setText(QString::null);

        m_comment_le->setReadOnly(true);
        m_comment_le->setText(QString::null);

        m_visitdate_le->setReadOnly(true);
        m_visitdate_le->setText(QString::null);

        m_credate_le->setReadOnly(true);
        m_credate_le->setText(QString::null);

        m_visitcount_le->setReadOnly(true);
        m_visitcount_le->setText(QString::null);

        return;
    }

    // read/write fields
    m_title_le->setReadOnly( (bk.isSeparator()|| !bk.hasParent() )? true : false);
    m_title_le->setText(bk.fullText());

    m_url_le->setReadOnly(bk.isGroup() || bk.isSeparator());
    m_url_le->setText(bk.isGroup() ? QString::null : bk.url().url());

    m_comment_le->setReadOnly((bk.isSeparator()|| !bk.hasParent()) ? true : false );
    m_comment_le->setText(
            NodeEditCommand::getNodeText(bk, QStringList() << "desc"));

    // readonly fields

    QString visitDate = 
        blah( NodeEditCommand::getNodeText(bk, QStringList() << "info" << "metadata"
                                                             << "time_visited" ));
    m_visitdate_le->setReadOnly(true);
    m_visitdate_le->setText(visitDate);

    QString creationDate = 
        blah( NodeEditCommand::getNodeText(bk, QStringList() << "info" << "metadata"
                                                             << "time_added" ));
    m_credate_le->setReadOnly(true);
    m_credate_le->setText(creationDate);

    // TODO - get the actual field name from the spec if it exists, else copy galeon
    m_visitcount_le->setReadOnly(true);
    m_visitcount_le->setText(
            NodeEditCommand::getNodeText(bk, QStringList() << "info" << "metadata"
                                                           << "visit_count" ));
}

void BookmarkInfoWidget::slotTextChangedTitle(const QString &str) {
    if (m_bk.isNull() || str == m_bk.fullText() )
        return;
    NodeEditCommand::setNodeText(m_bk, QStringList() << "title", str);
    emit updateListViewItem();
}

void BookmarkInfoWidget::slotTextChangedURL(const QString &str) {
    if (m_bk.isNull() || str == m_bk.url().url() )
        return;
    m_bk.internalElement().setAttribute("href", KURL(str).url(0, 106));
    emit updateListViewItem();
}

void BookmarkInfoWidget::slotTextChangedComment(const QString &str) {
    if (m_bk.isNull() || str == NodeEditCommand::getNodeText(m_bk, QStringList() << "desc") )
        return;
    NodeEditCommand::setNodeText(m_bk, QStringList() << "desc", str);
    emit updateListViewItem();
}

BookmarkInfoWidget::BookmarkInfoWidget(QWidget *parent, const char *name) 
    : QWidget(parent, name), m_connected(false) {

    QBoxLayout *vbox = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout(vbox, 3, 4, 4);

    m_title_le = new KLineEdit(this);
    grid->addWidget(m_title_le, 0, 1);
    grid->addWidget(
            new QLabel(m_title_le, i18n("Name:"), this), 
            0, 0);
    connect(m_title_le, SIGNAL( textChanged(const QString &) ), 
                        SLOT( slotTextChangedTitle(const QString &) ));

    m_url_le = new KLineEdit(this);
    grid->addWidget(m_url_le, 1, 1);
    grid->addWidget(
            new QLabel(m_url_le, i18n("Location:"), this), 
            1, 0);
    connect(m_url_le, SIGNAL( textChanged(const QString &) ), 
                      SLOT( slotTextChangedURL(const QString &) ));

    m_comment_le = new KLineEdit(this);
    grid->addWidget(m_comment_le, 2, 1);
    grid->addWidget(
            new QLabel(m_comment_le, i18n("Comment:"), this), 
            2, 0);
    connect(m_comment_le, SIGNAL( textChanged(const QString &) ), 
                          SLOT( slotTextChangedComment(const QString &) ));

    m_credate_le = new KLineEdit(this);
    grid->addWidget(m_credate_le, 0, 3);
    grid->addWidget(
            new QLabel(m_credate_le, i18n("First viewed:"), this), 
            0, 2);

    m_visitdate_le = new KLineEdit(this);
    grid->addWidget(m_visitdate_le, 1, 3);
    grid->addWidget(
            new QLabel(m_visitdate_le, i18n("Viewed last:"), this), 
            1, 2 );

    m_visitcount_le = new KLineEdit(this);
    grid->addWidget(m_visitcount_le, 2, 3);
    grid->addWidget(
            new QLabel(m_visitcount_le, i18n("Times visited:"), this), 
            2, 2);
}

#include "bookmarkinfo.moc"

