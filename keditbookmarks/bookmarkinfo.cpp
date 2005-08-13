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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "bookmarkinfo.h"
#include "commands.h"
#include "toplevel.h"

#include <stdlib.h>

#include <qtimer.h>
#include <qclipboard.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <QBoxLayout>

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

BookmarkLineEdit::BookmarkLineEdit( QWidget *parent )
    : KLineEdit( parent )
{
}

void BookmarkLineEdit::cut()
{
    QString select( selectedText() );
    int pos( selectionStart() );
    QString newText(  text().remove( pos, select.length() ) );
    KLineEdit::cut();
    setEdited( true ); //KDE 4 setModified( true );
    emit textChanged( newText );
    setText( newText );
}


void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {
    commitChanges();
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
    m_url_le->setText(bk.isGroup() ? QString::null : bk.url().pathOrURL());

    m_comment_le->setReadOnly((bk.isSeparator()|| !bk.hasParent()) ? true : false );
    m_comment_le->setText(
            EditCommand::getNodeText(bk, QStringList() << "desc"));

    // readonly fields
    updateStatus();
 
}

void BookmarkInfoWidget::updateStatus()
{
   QString visitDate =
        CurrentMgr::makeTimeStr( EditCommand::getNodeText(m_bk, QStringList() << "info" << "metadata"
                                                             << "time_visited" ));
    m_visitdate_le->setReadOnly(true);
    m_visitdate_le->setText(visitDate);

    QString creationDate =
        CurrentMgr::makeTimeStr( EditCommand::getNodeText(m_bk, QStringList() << "info" << "metadata"
                                                             << "time_added" ));
    m_credate_le->setReadOnly(true);
    m_credate_le->setText(creationDate);

    // TODO - get the actual field name from the spec if it exists, else copy galeon
    m_visitcount_le->setReadOnly(true);
    m_visitcount_le->setText(
            EditCommand::getNodeText(m_bk, QStringList() << "info" << "metadata"
                                                           << "visit_count" ));
}

void BookmarkInfoWidget::commitChanges()
{
    commitTitle();
    commitURL();
    commitComment();
}

void BookmarkInfoWidget::commitTitle()
{
//     if(titlecmd)
//     {
//         emit updateListViewItem();
//         CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(titlecmd->affectedBookmarks()).toGroup());
//         titlecmd = 0;
//     }
}

void BookmarkInfoWidget::slotTextChangedTitle(const QString &str) 
{
    if (m_bk.isNull() || !m_title_le->isModified())
        return;

//     timer->start(1000, true);

//     if(titlecmd)
//     {
//         EditCommand::setNodeText(m_bk, QStringList() << "title", str);
//         //titlecmd->modify(str);
//     }
//     else
//     {
//         //titlecmd = new NodeEditCommand(m_bk.address(), str, "title");
//         //titlecmd->execute();
//         //CmdHistory::self()->addInFlightCommand(titlecmd);
//     }
}

void BookmarkInfoWidget::commitURL()
{
//     if(urlcmd)
//     {
//         emit updateListViewItem();
//         CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(urlcmd->affectedBookmarks()).toGroup());
//         urlcmd = 0;
//     }
}

void BookmarkInfoWidget::slotTextChangedURL(const QString &str) {
    if (m_bk.isNull() || !m_url_le->isModified())
        return;

//     timer->start(1000, true);

//     if(urlcmd)
//     {
//         KURL u = KURL::fromPathOrURL(str);
//         m_bk.internalElement().setAttribute("href", u.url(0, 106));
//         //urlcmd->modify("href", u.url(0, 106));
//     }
//     else
//     {
//         KURL u = KURL::fromPathOrURL(str);
//         //urlcmd = new EditCommand(m_bk.address(), EditCommand::Edition("href", u.url(0, 106)), i18n("URL"));
//         //urlcmd->execute();
//         //CmdHistory::self()->addInFlightCommand(urlcmd);
//     }
}

void BookmarkInfoWidget::commitComment()
{
//     if(commentcmd)
//     {
//         emit updateListViewItem();
//         CurrentMgr::self()->notifyManagers( CurrentMgr::bookmarkAt( commentcmd->affectedBookmarks() ).toGroup());
//         commentcmd = 0;
//     }
}

void BookmarkInfoWidget::slotTextChangedComment(const QString &str) {
    if (m_bk.isNull() || !m_comment_le->isModified())
        return;

//     timer->start(1000, true);

//     if(commentcmd)
//     {
//         EditCommand::setNodeText(m_bk, QStringList() << "desc", str);
//         //commentcmd->modify(str);
//     }
//     else
//     {
//         //commentcmd = new NodeEditCommand(m_bk.address(), str, "desc");
//         //commentcmd->execute();
//         //CmdHistory::self()->addInFlightCommand(commentcmd);
//     }
}

BookmarkInfoWidget::BookmarkInfoWidget(QWidget *parent, const char *name)
    : QWidget(parent, name), m_connected(false) {

    timer = new QTimer(this);
    connect(timer, SIGNAL( timeout() ), SLOT( commitChanges()));

//     titlecmd = 0;
//     urlcmd = 0;
//     commentcmd = 0;

    QBoxLayout *vbox = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout(vbox, 3, 4, 4);

    m_title_le = new BookmarkLineEdit(this);
    grid->addWidget(m_title_le, 0, 1);
    grid->addWidget(
            new QLabel(m_title_le, i18n("Name:"), this),
            0, 0);

    connect(m_title_le, SIGNAL( textChanged(const QString &) ),
                        SLOT( slotTextChangedTitle(const QString &) ));
    connect(m_title_le, SIGNAL( lostFocus() ), SLOT( commitTitle() ));

    m_url_le = new BookmarkLineEdit(this);
    grid->addWidget(m_url_le, 1, 1);
    grid->addWidget(
            new QLabel(m_url_le, i18n("Location:"), this),
            1, 0);

    connect(m_url_le, SIGNAL( textChanged(const QString &) ),
                      SLOT( slotTextChangedURL(const QString &) ));
    connect(m_url_le, SIGNAL( lostFocus() ), SLOT( commitURL() ));

    m_comment_le = new BookmarkLineEdit(this);
    grid->addWidget(m_comment_le, 2, 1);
    grid->addWidget(
            new QLabel(m_comment_le, i18n("Comment:"), this),
            2, 0);
    connect(m_comment_le, SIGNAL( textChanged(const QString &) ),
                          SLOT( slotTextChangedComment(const QString &) ));
    connect(m_comment_le, SIGNAL( lostFocus() ), SLOT( commitComment() ));

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

