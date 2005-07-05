// kate: space-indent on; indent-width 3; replace-tabs on;
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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __listview_h
#define __listview_h

#include <assert.h>

#include <qlistview.h>
#include <qmap.h>

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
   void setSelected ( bool s );

   virtual void setOpen(bool);

   bool isEmptyFolderPadder() const { return m_emptyFolderPadder; }
   const KBookmark bookmark() const { return m_bookmark; }

  typedef enum { GreyStyle, BoldStyle, GreyBoldStyle, DefaultStyle } PaintStyle;

   static bool KEBListViewItem::parentSelected(QListViewItem * item);

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
   void slotMoved();
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
   void updateListViewSetup(bool readOnly);
   void connectSignals();

   // selected item stuff
   void selected(KEBListViewItem * item, bool s);
   KEBListViewItem * firstSelected() const;
   QMap<KEBListViewItem *, bool> selectedItemsMap() const
   { return mSelectedItems; }

   // bookmark helpers
   QValueList<KBookmark> itemsToBookmarks(const QMap<KEBListViewItem *, bool> & items) const;

   // bookmark stuff
   QValueList<KBookmark> allBookmarks() const;
   QValueList<KBookmark> selectedBookmarksExpanded() const;

   // address stuff
   KEBListViewItem* getItemAtAddress(const QString &address) const;
   QString userAddress() const;

   // gui stuff - DESIGN - all of it???
   SelcAbilities getSelectionAbilities() const;

   void updateListView();
   void setOpen(bool open); // DESIGN -rename to setAllOpenFlag
   void setCurrent(KEBListViewItem *item, bool select);
   void renameNextCell(bool dir);

   QWidget *widget() const { return m_listView; }
   void rename(int);
   void clearSelection();

   void updateStatus(QString url);

   static ListView* self() { return s_self; }
   static void createListViews(QSplitter *parent);

   void handleMoved(KEBListView *);
   void handleDropped(KEBListView *, QDropEvent *, QListViewItem *, QListViewItem *);
   void handleContextMenu(KEBListView *, KListView *, QListViewItem *, const QPoint &);
   void handleDoubleClicked(KEBListView *, QListViewItem *, const QPoint &, int);
   void handleItemRenamed(KEBListView *, QListViewItem *, const QString &, int);

   static void startRename(int column, KEBListViewItem *item);

   static void deselectAllChildren(KEBListViewItem *item);

   ~ListView();

public slots:
   void slotBkInfoUpdateListViewItem();

private:
   void updateTree();
   void selectedBookmarksExpandedHelper(KEBListViewItem * item, QValueList<KBookmark> & bookmarks) const;
   void fillWithGroup(KEBListView *, KBookmarkGroup, KEBListViewItem * = 0);

   ListView();

   KEBListView *m_listView;

//    // Actually this is a std:set, the bool is ignored
   QMap<KEBListViewItem *, bool> mSelectedItems;

   // statics
   static ListView *s_self;
   static int s_myrenamecolumn;
   static KEBListViewItem *s_myrenameitem;
   static QStringList s_selected_addresses;
   static QString s_current_address;
};

#endif
