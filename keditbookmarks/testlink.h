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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __testlink_h
#define __testlink_h

#include <qobject.h>
//Added by qt3to4:
#include <Q3ValueList>

#include <kio/job.h>
#include <kbookmark.h>

#include "listview.h"
#include "bookmarkiterator.h"

class TestLinkItrHolder : public BookmarkIteratorHolder {
public:
   static TestLinkItrHolder* self() { 
      if (!s_self) { s_self = new TestLinkItrHolder(); }; return s_self; 
   }
   void addAffectedBookmark( const QString & address );
   void resetToValue(const QString &url, const QString &val);
   const QString getMod(const QString &url) const;
   const QString getOldVisit(const QString &url) const;
   void setMod(const QString &url, const QString &val);
   void setOldVisit(const QString &url, const QString &val);
protected:
   virtual void doItrListChanged();
private:
   TestLinkItrHolder();
   static TestLinkItrHolder *s_self;
   QMap<QString, QString> m_modify;
   QMap<QString, QString> m_oldModify;
   QString m_affectedBookmark;
};

class TestLinkItr : public BookmarkIterator
{
   Q_OBJECT

public:
   TestLinkItr(QVector<KBookmark> bks);
   ~TestLinkItr();
   virtual TestLinkItrHolder* holder() const { return TestLinkItrHolder::self(); }

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
