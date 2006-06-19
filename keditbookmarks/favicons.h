// -*- c-basic-offset: 4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
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

#ifndef __favicons_h
#define __favicons_h

#include <kbookmark.h>
#include <konq_faviconmgr.h>

#include "bookmarkiterator.h"

class FavIconsItrHolder : public BookmarkIteratorHolder {
public:
   static FavIconsItrHolder* self() { 
      if (!s_self) { s_self = new FavIconsItrHolder(); }; return s_self; 
   }
   void addAffectedBookmark( const QString & address );
protected:
   virtual void doItrListChanged();
private:
   FavIconsItrHolder();
   static FavIconsItrHolder *s_self;
   QString m_affectedBookmark;
};

class FavIconUpdater;

class FavIconsItr : public BookmarkIterator
{
   Q_OBJECT

public:
   FavIconsItr(QList<KBookmark> bks);
   ~FavIconsItr();
   virtual FavIconsItrHolder* holder() const { return FavIconsItrHolder::self(); }

public Q_SLOTS:
   void slotDone(bool succeeded);

protected:
   virtual void doAction();
   virtual bool isApplicable(const KBookmark &bk) const;

private:
   FavIconUpdater *m_updater;
};

#endif

