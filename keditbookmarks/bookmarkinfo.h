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

#ifndef __bookmarkinfo_h
#define __bookmarkinfo_h

#include "commands.h"

#include <kbookmark.h>
#include <qwidget.h>
#include <klineedit.h>

class QTimer;

class BookmarkLineEdit : public KLineEdit {
    Q_OBJECT
public:
    BookmarkLineEdit( QWidget * );
public slots:
    virtual void cut ();
};


class BookmarkInfoWidget : public QWidget {
    Q_OBJECT
public:
    BookmarkInfoWidget(QWidget * = 0, const char * = 0);
    void showBookmark(const KBookmark &bk);
    void saveBookmark(const KBookmark &bk);
    bool connected() { return m_connected; };
    void setConnected(bool b) { m_connected = b; };

public slots:
    void slotTextChangedURL(const QString &);
    void slotTextChangedTitle(const QString &);
    void slotTextChangedComment(const QString &);

    // _The deal with all those commitChanges() calls_
    // First a short example how all the components 
    // normally fit together:
    // Note: not all details are included
    // For example: The user clicks on "New Bookmark"
    // This constructs a cmd = new CreateCommand( .. )
    // CmdHistory::self()->addCommand( cmd ) is called
    //   CmdHistory executes the command
    //   and enables the undo button
    //   and emits slotCommandExecuted
    // We catch the signal and call
    //  CurrentMgr::self()->notifyManagers( .. );

    // The bookmarkinfo widget is special, because
    // we don't want to send a notification 
    // for every change, but want to enable the undo
    // button and need to send the notification
    // if the user has stopped typing

    // So as soon as the user starts typing
    // we create a command
    // and call CmdHistory::self()->addInFlightCommand( cmd );
    // addInFlightCommand() doesn't execute the command, it just
    // adds it to the command history (To enable the undo button)
    // For every keystroke after that the command is modified
    // and we change our internal state to reflect the change
    // (Basically changing it in the same way, executing would have.)

    // At this point we have a modified state, but haven't send it
    // to the other bookmarkmanagers
    // That is done in commitChanges()
    // commitChanges() should be called everywhere, where we are
    // sure that the user has stopped typing.
    // And a few other cleanups are done in commitChanges()
    void commitChanges();
    void commitTitle();
    void commitURL();
    void commitComment();

signals:
    void updateListViewItem();
private:
    NodeEditCommand *titlecmd;
    EditCommand *urlcmd;
    NodeEditCommand *commentcmd;
    QTimer * timer;
    BookmarkLineEdit *m_title_le, *m_url_le,
        *m_comment_le;
    KLineEdit  *m_visitdate_le, *m_credate_le,
              *m_visitcount_le;
    KBookmark m_bk;
    bool m_connected;
};

#endif
