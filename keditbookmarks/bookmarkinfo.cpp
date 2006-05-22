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

#include "bookmarkinfo.h"
#include "bookmarklistview.h"
#include "commands.h"
#include "toplevel.h"
#include "treeitem.h"

#include <stdlib.h>

#include <QTimer>
#include <QClipboard>
#include <QSplitter>
#include <QLayout>
#include <QLabel>
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

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <QEvent>

// SHUFFLE all these functions around, the order is just plain stupid
void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {
    commitChanges();
    m_bk = bk;

    if (m_bk.isNull()) {
        // all read only and blank

        m_title_le->setReadOnly(true);
        m_title_le->setText(QString());

        m_url_le->setReadOnly(true);
        m_url_le->setText(QString());

        m_comment_le->setReadOnly(true);
        m_comment_le->setText(QString());

        m_visitdate_le->setReadOnly(true);
        m_visitdate_le->setText(QString());

        m_credate_le->setReadOnly(true);
        m_credate_le->setText(QString());

        m_visitcount_le->setReadOnly(true);
        m_visitcount_le->setText(QString());

        return;
    }

    // read/write fields
    m_title_le->setReadOnly( (bk.isSeparator()|| !bk.hasParent() )? true : false);
    m_title_le->setText(bk.fullText());

    m_url_le->setReadOnly(bk.isGroup() || bk.isSeparator());
    m_url_le->setText(bk.isGroup() ? QString() : bk.url().pathOrUrl());

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
    if(titlecmd)
    {
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(titlecmd->affectedBookmarks()).toGroup());
        titlecmd = 0;
    }
}

void BookmarkInfoWidget::slotTextChangedTitle(const QString &str) 
{
    Q_UNUSED(str);
    if (m_bk.isNull() || !m_title_le->isModified())
        return;

    timer->setSingleShot(true);
    timer->start(1000);

    if(titlecmd)
    {
        titlecmd->modify(str);
        titlecmd->execute();
    }
    else
    {
        titlecmd = new EditCommand(m_bk.address(), 0, str);
        titlecmd->execute();
        CmdHistory::self()->addInFlightCommand(titlecmd);
    }
}

void BookmarkInfoWidget::commitURL()
{
    if(urlcmd)
    {
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(urlcmd->affectedBookmarks()).toGroup());
        urlcmd = 0;
    }
}

void BookmarkInfoWidget::slotTextChangedURL(const QString &str) {
    Q_UNUSED(str);
    if (m_bk.isNull() || !m_url_le->isModified())
        return;
    
    timer->setSingleShot(true);
    timer->start(1000);

    if(urlcmd)
    {
        urlcmd->modify(str);
        urlcmd->execute();
    }
    else
    {
        urlcmd = new EditCommand(m_bk.address(), 1, str);
        urlcmd->execute();
        CmdHistory::self()->addInFlightCommand(urlcmd);
    }
}

void BookmarkInfoWidget::commitComment()
{
    if(commentcmd)
    {
        CurrentMgr::self()->notifyManagers( CurrentMgr::bookmarkAt( commentcmd->affectedBookmarks() ).toGroup());
        commentcmd = 0;
    }
}

void BookmarkInfoWidget::slotTextChangedComment(const QString &str) {
    Q_UNUSED(str);
    if (m_bk.isNull() || !m_comment_le->isModified())
        return;
    
    timer->setSingleShot(true);
    timer->start(1000);

    if(commentcmd)
    {
        commentcmd->modify(str);
        commentcmd->execute();
    }
    else
    {
        commentcmd = new EditCommand(m_bk.address(), 2, str);
        commentcmd->execute();
        CmdHistory::self()->addInFlightCommand(commentcmd);
    }
}

void BookmarkInfoWidget::slotUpdate()
{
    if( mBookmarkListView->getSelectionAbilities().singleSelect)
    {
        const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
        QModelIndex index = *list.constBegin();
        showBookmark( static_cast<TreeItem *>(index.internalPointer())->bookmark());
    }
    else
        showBookmark( KBookmark() );
}

BookmarkInfoWidget::BookmarkInfoWidget(BookmarkListView * lv, QWidget *parent)
    : QWidget(parent), mBookmarkListView(lv) {

    connect(mBookmarkListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            SLOT( slotUpdate()));

    connect(mBookmarkListView->model(), SIGNAL(dataChanged( const QModelIndex &, const QModelIndex &)),
            SLOT( slotUpdate()));

    timer = new QTimer(this);
    connect(timer, SIGNAL( timeout() ), SLOT( commitChanges()));


    titlecmd = 0;
    urlcmd = 0;
    commentcmd = 0;

    QGridLayout *grid = new QGridLayout(this);
    grid->setSpacing(4);

    m_title_le = new KLineEdit(this);
    grid->addWidget(m_title_le, 0, 1);
    QLabel* label = new QLabel(i18n("Name:"), this);
    label->setBuddy(m_title_le);
    grid->addWidget(label, 0, 0);

    connect(m_title_le, SIGNAL( textChanged(const QString &) ),
                        SLOT( slotTextChangedTitle(const QString &) ));
    connect(m_title_le, SIGNAL( lostFocus() ), SLOT( commitTitle() ));

    m_url_le = new KLineEdit(this);
    grid->addWidget(m_url_le, 1, 1);
    label = new QLabel(i18n("Location:"), this);
    label->setBuddy(m_url_le);
    grid->addWidget(label, 1, 0);

    connect(m_url_le, SIGNAL( textChanged(const QString &) ),
                      SLOT( slotTextChangedURL(const QString &) ));
    connect(m_url_le, SIGNAL( lostFocus() ), SLOT( commitURL() ));

    m_comment_le = new KLineEdit(this);
    grid->addWidget(m_comment_le, 2, 1);
    label = new QLabel(i18n("Comment:"), this);
    label->setBuddy(m_comment_le);
    grid->addWidget(label, 2, 0);

    connect(m_comment_le, SIGNAL( textChanged(const QString &) ),
                          SLOT( slotTextChangedComment(const QString &) ));
    connect(m_comment_le, SIGNAL( lostFocus() ), SLOT( commitComment() ));

    m_credate_le = new KLineEdit(this);
    grid->addWidget(m_credate_le, 0, 3);
    label = new QLabel(i18n("First viewed:"), this);
    label->setBuddy(m_credate_le);
    grid->addWidget(label, 0, 2);

    m_visitdate_le = new KLineEdit(this);
    grid->addWidget(m_visitdate_le, 1, 3);
    label = new QLabel(i18n("Viewed last:"), this);
    label->setBuddy(m_visitdate_le);
    grid->addWidget(label, 1, 2 );

    m_visitcount_le = new KLineEdit(this);
    grid->addWidget(m_visitcount_le, 2, 3);
    label = new QLabel(i18n("Times visited:"), this);
    label->setBuddy(m_visitcount_le);
    grid->addWidget(label, 2, 2);

    showBookmark(KBookmark());
}

#include "bookmarkinfo.moc"

