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

#ifndef __testlink_h
#define __testlink_h

#include <qobject.h>

#include <kio/job.h>
#include <kbookmark.h>

#include "listview.h"
#include "bookmarkiterator.h"

class TestLinkItrHolder : public BookmarkIteratorHolder {
public:
   static TestLinkItrHolder* self() { 
      if (!s_self) { s_self = new TestLinkItrHolder(); }; return s_self; 
   }
   void resetToValue(const QString &url, const QString &val);
   const QString getMod(const QString &url) const;
   const QString getOldVisit(const QString &url) const;
   void setMod(const QString &url, const QString &val);
   void setOldVisit(const QString &url, const QString &val);
   static QString calcPaintStyle(const QString &, KEBListViewItem::PaintStyle&, 
                                 const QString &, const QString &);
protected:
   virtual void doItrListChanged();
private:
   TestLinkItrHolder();
   static TestLinkItrHolder *s_self;
   QMap<QString, QString> m_modify;
   QMap<QString, QString> m_oldModify;
};

class TestLinkItr : public BookmarkIterator
{
   Q_OBJECT

public:
   TestLinkItr(QValueList<KBookmark> bks);
   ~TestLinkItr();
   virtual BookmarkIteratorHolder* holder() const { return TestLinkItrHolder::self(); }

public slots:
   void slotJobResult(KIO::Job *job);
   void slotJobData(KIO::Job *job, const QByteArray &data);

private:
   virtual void doAction();
   virtual bool isApplicable(const KBookmark &bk) const;

   KIO::TransferJob *m_job;
   bool m_errSet;
};

#endif
