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

void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {
   m_title_le->setText(bk.text());
   m_url_le->setText(bk.url().url());
   m_comment_le->setText("a comment");
   m_moddate_le->setText("the modification date");
   m_credate_le->setText("the creation date");
}

BookmarkInfoWidget::BookmarkInfoWidget(
   QWidget * parent, const char * name
) : QWidget (parent, name) {
   QBoxLayout *vbox = new QVBoxLayout(this);
   QGridLayout *grid = new QGridLayout(vbox, 3, 4);

   m_title_le = new KLineEdit(this);
   m_title_le->setReadOnly(true);
   grid->addWidget(m_title_le, 0, 1);
   grid->addWidget(
            new QLabel(m_title_le, i18n("Name:"), this), 
            0, 0);

   m_url_le = new KLineEdit(this);
   m_url_le->setReadOnly(true);
   grid->addWidget(m_url_le, 1, 1);
   grid->addWidget(
            new QLabel(m_url_le, i18n("Location:"), this), 
            1, 0);

   m_comment_le = new KLineEdit(this);
   m_comment_le->setReadOnly(true);
   grid->addWidget(m_comment_le, 2, 1);
   grid->addWidget(
            new QLabel(m_comment_le, i18n("Comment:"), this), 
            2, 0);

   m_moddate_le = new KLineEdit(this);
   m_moddate_le->setReadOnly(true);
   grid->addWidget(m_moddate_le, 0, 3);
   grid->addWidget(
            new QLabel(m_moddate_le, i18n("First viewed:"), this), 
            0, 2 );

   m_credate_le = new KLineEdit(this);
   m_credate_le->setReadOnly(true);
   grid->addWidget(m_credate_le, 1, 3);
   grid->addWidget(
            new QLabel(m_credate_le, i18n("Viewed last:"), this), 
            1, 2);
}

