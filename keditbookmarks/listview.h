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

#ifndef __listview_h
#define __listview_h

#include <assert.h>

#include <qlistview.h>

#include <klocale.h>
#include <kbookmark.h>
#include <klistview.h>
#include <kiconloader.h>

#include "toplevel.h"

class KEBListView : public KListView
{
   Q_OBJECT
public:
   enum { NameColumn = 0, UrlColumn = 1, StatusColumn = 2, AddressColumn = 3 };
   KEBListView(QWidget *parent) : KListView(parent) {}
   virtual ~KEBListView() {}

public slots:
   virtual void rename(QListViewItem *item, int c);

protected:
   virtual bool acceptDrag(QDropEvent *e) const;
   virtual QDragObject* dragObject();
};

// DESIGN - make these seperate classes?, else move impls back into listview.cpp

class KEBListViewItem : public QListViewItem
{
private:
   void normalConstruct(const KBookmark &bk) {
#ifdef DEBUG_ADDRESSES
      setText(KEBListView::AddressColumn, bk.address());
#endif
      setPixmap(0, SmallIcon(bk.icon()));
      modUpdate();
   }

public:
   KEBListViewItem(QListView *, const KBookmark &);
   KEBListViewItem(KEBListViewItem *, QListViewItem *);
   KEBListViewItem(KEBListViewItem *, QListViewItem *, const KBookmarkGroup &);
   KEBListViewItem(KEBListViewItem *, const KBookmark &);
   KEBListViewItem(KEBListViewItem *, QListViewItem *, const KBookmark &);

   void nsPut(QString nm);

   void modUpdate();
   void setTmpStatus(QString);
   void restoreStatus();

   virtual void setOpen(bool);
   bool isEmptyFolder() const { return m_emptyFolder; }
   const KBookmark& bookmark() { return m_bookmark; }
   void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

private:
   QString nsGet();

   KBookmark m_bookmark;
   int m_paintstyle;
   bool m_emptyFolder;
   QString m_oldStatus;
};

// DESIGN - make some stuff private if possible
class ListView : public QObject
{
   Q_OBJECT
public:
   // init stuff
   void initListView();

   void updateListViewSetup(bool readOnly);
   void connectSignals();

   // item stuff
   KEBListViewItem* getFirstChild();
   QPtrList<KEBListViewItem>* itemList();

   // selected item stuff
   int numSelected();
   KEBListViewItem* selectedItem();
   QPtrList<KEBListViewItem>* selectedItems();
   KEBListViewItem* firstSelected();

   // bookmark helpers
   QValueList<KBookmark> itemsToBookmarks(QPtrList<KEBListViewItem>* items);

   // bookmark stuff
   QValueList<KBookmark> getBookmarkSelection();
   QValueList<KBookmark> allBookmarks();
   QValueList<KBookmark> selectedBookmarksExpanded();

   // address stuff
   KEBListViewItem* getItemAtAddress(const QString &address);
   QString userAddress();

   // gui stuff - DESIGN - all of it???
   void updateLastAddress();
   void updateSelectedItems(); // DESIGN - rename?
   void updateListView();
   SelcAbilities getSelectionAbilities();
   void emitSlotSelectionChanged() { emit slotSelectionChanged(); }
   void setOpen(bool open); // DESIGN -rename to setAllOpenFlag
   void fillWithGroup(KBookmarkGroup group, KEBListViewItem *parentItem = 0);
   void setCurrent(KEBListViewItem *item);

   KEBListViewItem* findOpenParent(KEBListViewItem *item);
   void openParents(KEBListViewItem *item);

   static ListView* self() { assert(s_self); return s_self; }
   static void createListView(QWidget *parent);
   QWidget *widget() const { return m_listView; }
   void rename(int);
   void clearSelection();

protected slots:
   void slotDropped(QDropEvent *, QListViewItem *, QListViewItem *);
   void slotSelectionChanged();
   void slotContextMenu(KListView *, QListViewItem *, const QPoint &);
   void slotDoubleClicked(QListViewItem *, const QPoint &, int);
   void slotItemRenamed(QListViewItem *, const QString &, int);

private:
   ListView();
   static ListView *s_self;
   void deselectParents(KEBListViewItem *item);
   QString m_last_selection_address;
   KEBListView *m_listView;
};

#endif
