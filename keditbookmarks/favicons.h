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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __favicons_h
#define __favicons_h

#include <kbookmark.h>
#include <konq_faviconmgr.h>

#include <kparts/part.h>
#include <kparts/browserinterface.h>

#include "bookmarkiterator.h"

class FavIconsItrHolder : public BookmarkIteratorHolder {
public:
   static FavIconsItrHolder* self() { 
      if (!s_self) { s_self = new FavIconsItrHolder(); }; return s_self; 
   }
protected:
   virtual void doItrListChanged();
private:
   FavIconsItrHolder();
   static FavIconsItrHolder *s_self;
};

class FavIconUpdater;

class FavIconsItr : public BookmarkIterator
{
   Q_OBJECT

public:
   FavIconsItr(QValueList<KBookmark> bks);
   ~FavIconsItr();
   virtual BookmarkIteratorHolder* holder() { return FavIconsItrHolder::self(); }

public slots:
   void slotDone(bool succeeded);

private:
   virtual void doBlah();
   virtual bool isBlahable(const KBookmark &bk);
   FavIconUpdater *m_updater;
   bool m_done;
};

class FavIconWebGrabber : public QObject
{
   Q_OBJECT
public:
   FavIconWebGrabber(KParts::ReadOnlyPart *part, const KURL &url);
   ~FavIconWebGrabber() {}

protected slots:
   void slotMimetype(KIO::Job *job, const QString &_type);
   void slotFinished(KIO::Job *job);

private:
   KParts::ReadOnlyPart *m_part;
   KURL m_url;
};

class FavIconBrowserInterface : public KParts::BrowserInterface
{
   Q_OBJECT
public:
   FavIconBrowserInterface(FavIconUpdater *view, const char *name)
      : KParts::BrowserInterface(view, name), m_view = view {
      ;
   }
private:
   FavIconUpdater *m_view;
};

class FavIconUpdater : public KonqFavIconMgr 
{
   Q_OBJECT

public:   
   FavIconUpdater(QObject *parent, const char *name);
   void downloadIcon(const KBookmark &bk);
   void downloadIconActual(const KBookmark &bk);

   virtual void notifyChange(bool isHost, QString hostOrURL, QString iconName);

protected slots:
   void setIconURL(const KURL &iconURL);
   void slotCompleted();

signals:
   void done(bool succeeded);

private:
   KParts::ReadOnlyPart *m_part;
   FavIconBrowserInterface *m_browserIface;
   KBookmark m_bk;
};

#endif

