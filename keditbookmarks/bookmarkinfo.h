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

#ifndef __bookmarkinfo_h
#define __bookmarkinfo_h

#include <kbookmark.h>
#include <qwidget.h>

class BookmarkListView;
class EditCommand;

class KLineEdit;

class QTimer;

class BookmarkInfoWidget : public QWidget {
    Q_OBJECT
public:
    BookmarkInfoWidget(BookmarkListView * lv, QWidget * = 0);
    
    KBookmark bookmark() { return m_bk; }
    void updateStatus(); //FIXME where was this called?

public Q_SLOTS:
    void slotTextChangedURL(const QString &);
    void slotTextChangedTitle(const QString &);
    void slotTextChangedComment(const QString &);

    void slotUpdate();

    void commitChanges();
    void commitTitle();
    void commitURL();
    void commitComment();

private:
    void showBookmark(const KBookmark &bk);
    EditCommand * titlecmd, * urlcmd, * commentcmd;
    QTimer * timer;
    KLineEdit *m_title_le, *m_url_le,
        *m_comment_le;
    KLineEdit  *m_visitdate_le, *m_credate_le,
              *m_visitcount_le;
    KBookmark m_bk;
    BookmarkListView * mBookmarkListView;
};

#endif
