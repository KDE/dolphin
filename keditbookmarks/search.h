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

#ifndef __search_h
#define __search_h

#include <qobject.h>
#include <kbookmark.h>
#include <klineedit.h>

class KBookmarkTextMap;

class MagicKLineEdit : public KLineEdit {
public:
   MagicKLineEdit(const QString &text, QWidget *parent, const char *name = 0);
   virtual void focusOutEvent(QFocusEvent *ev);
   virtual void mousePressEvent(QMouseEvent *ev);
   virtual void focusInEvent(QFocusEvent *ev);
   void setGrayedText(const QString &text) { m_grayedText = text; }
   QString grayedText() const { return m_grayedText; }
private:
   QString m_grayedText;
};

class Searcher : public QObject {
   Q_OBJECT
protected slots:
   void slotSearchNext();
   void slotSearchTextChanged(const QString & text);
public:
   static Searcher* self() { 
      if (!s_self) { s_self = new Searcher(); }; return s_self; 
   }
private:
   Searcher() { m_bktextmap = 0; }
   static Searcher *s_self;
   KBookmarkTextMap *m_bktextmap;
};

#endif
