// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
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

class QSplitter;

class KEBListViewItem : public QListViewItem
{
public:
   KEBListViewItem(QListView *, const KBookmarkGroup &);
   KEBListViewItem(KEBListViewItem *, QListViewItem *);
   KEBListViewItem(KEBListViewItem *, QListViewItem *, const KBookmarkGroup &);
   KEBListViewItem(KEBListViewItem *, const KBookmark &);
   KEBListViewItem(KEBListViewItem *, QListViewItem *, const KBookmark &);

   KEBListViewItem(QListView *, const KBookmark &);
   KEBListViewItem(QListView *, QListViewItem *, const KBookmark &);

   void nsPut(const QString &nm);

   void modUpdate();

   void setOldStatus(const QString &);
   void setTmpStatus(const QString &);
   void restoreStatus();

   void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

   virtual void setOpen(bool);

   bool isEmptyFolderPadder() const { return m_emptyFolderPadder; }
   KBookmark bookmark() const { return m_bookmark; }

  typedef enum { GreyStyle, BoldStyle, GreyBoldStyle, DefaultStyle } PaintStyle;

private:
   const QString nsGet() const;
   void normalConstruct(const KBookmark &);

   KBookmark m_bookmark;
   PaintStyle m_paintStyle;
   bool m_emptyFolderPadder;
   QString m_oldStatus;
   void greyStyle(QColorGroup &);
   void boldStyle(QPainter *);
};

class KEBListView : public KListView
{
   Q_OBJECT
public:
   enum { 
      NameColumn = 0,
      UrlColumn = 1,
      CommentColumn = 2,
      StatusColumn = 3,
      AddressColumn = 4
   };
   KEBListView(QWidget *parent, bool folderList) 
      : KListView(parent), m_folderList(folderList) {}
   virtual ~KEBListView() {}

   void init();
   void makeConnections();
   void readonlyFlagInit(bool);

   void loadColumnSetting();
   void saveColumnSetting();

   void updateByURL(QString url);

   bool isFolderList() const { return m_folderList; }

   KEBListViewItem* rootItem() const;

public slots:
   virtual void rename(QListViewItem *item, int c);
   void slotSelectionChanged();
   void slotMoved();
   void slotCurrentChanged(QListViewItem*);
   void slotContextMenu(KListView *, QListViewItem *, const QPoint &);
   void slotItemRenamed(QListViewItem *, const QString &, int);
   void slotDoubleClicked(QListViewItem *, const QPoint &, int);
   void slotDropped(QDropEvent*, QListViewItem*, QListViewItem*);
   void slotColumnSizeChanged(int, int, int);

protected:
   virtual bool acceptDrag(QDropEvent *e) const;
   virtual QDragObject* dragObject();

private:
   bool m_folderList;
   bool m_widthsDirty;
};

// DESIGN - make some stuff private if possible
class ListView : public QObject
{
   Q_OBJECT
public:
   // init stuff
   void initListViews();
   void setInitialAddress(QString address);
   void updateListViewSetup(bool readOnly);
   void connectSignals();

   // selected item stuff
   int numSelected() const;
   KEBListViewItem* selectedItem() const;
   QPtrList<KEBListViewItem>* selectedItems() const;

   // bookmark helpers
   QValueList<KBookmark> itemsToBookmarks(QPtrList<KEBListViewItem>* items) const;

   // bookmark stuff
   QValueList<KBookmark> getBookmarkSelection() const;
   QValueList<KBookmark> allBookmarks() const;
   QValueList<KBookmark> selectedBookmarksExpanded() const;

   // address stuff
   KEBListViewItem* getItemAtAddress(const QString &address) const;
   QString userAddress() const;

   // gui stuff - DESIGN - all of it???
   SelcAbilities getSelectionAbilities() const;

   void updateSelectedItems(); // DESIGN - rename?
   void updateListView();
   void emitSlotSelectionChanged() { emit handleSelectionChanged(m_listView); }
   void setOpen(bool open); // DESIGN -rename to setAllOpenFlag
   void setCurrent(KEBListViewItem *item);
   void renameNextCell(bool dir);

   QWidget *widget() const { return m_listView; }
   void rename(int);
   void clearSelection();

   void updateStatus(QString url);

   static ListView* self() { return s_self; }
   static void createListViews(QSplitter *parent);

   void handleMoved(KEBListView *);
   void handleDropped(KEBListView *, QDropEvent *, QListViewItem *, QListViewItem *);
   void handleSelectionChanged(KEBListView *);
   void handleCurrentChanged(KEBListView *, QListViewItem *);
   void handleContextMenu(KEBListView *, KListView *, QListViewItem *, const QPoint &);
   void handleDoubleClicked(KEBListView *, QListViewItem *, const QPoint &, int);
   void handleItemRenamed(KEBListView *, QListViewItem *, const QString &, int);

   static void startRename(int column, KEBListViewItem *item);

   ~ListView();

public slots:
   void slotBkInfoUpdateListViewItem();

private:
   void updateTree(bool updateSplitView = true);
   void fillWithGroup(KEBListView *, KBookmarkGroup, KEBListViewItem * = 0);
 
   ListView();
   enum Which { NoChildren, SomeChildren, AllChildren };
   static Which whichChildrenSelected(KEBListViewItem *item);
   static void deselectAllButParent(KEBListViewItem *item);

   KEBListView *m_listView;
   KEBListView *m_folderListView;

   QString m_last_selection_address;
   QString m_currentSelectedRootAddress;

   bool m_splitView;

   // statics
   static ListView *s_self;
   static bool s_listview_is_dirty;
   static int s_myrenamecolumn;
   static KEBListViewItem *s_myrenameitem;
   static QStringList s_selected_addresses;
};

#endif
