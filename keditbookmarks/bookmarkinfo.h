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

#include <kbookmark.h>
#include <qwidget.h>

class KLineEdit;

class BookmarkInfoWidget : public QWidget {
public:
   BookmarkInfoWidget(QWidget * = 0, const char * = 0);
   void showBookmark(const KBookmark &bk);
private:
   KLineEdit *m_title_le, *m_url_le, *m_comment_le, *m_moddate_le, *m_credate_le;
   KBookmark m_bk;
};

#endif
