/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_dirtree_h__
#define __konq_dirtree_h__

#include <kparts/browserextension.h>
#include <kdirnotify.h>
#include <kurl.h>
#include <kglobalsettings.h>
#include <konq_operations.h>
#include <konq_fileitem.h>

#include <klistview.h>
#include <qdict.h>
#include <qmap.h>
#include <qpixmap.h>

class KonqDirTreeBrowserExtension;
class KonqDirTree;
class KonqDrag;
class QTimer;
class KonqDirTreePart;

class KonqDirTreePart : public KParts::ReadOnlyPart
{
  Q_OBJECT
  friend class KonqDirTree;
  Q_PROPERTY( bool supportsUndo READ supportsUndo )
public:
  KonqDirTreePart( QWidget *parentWidget, QObject *parent, const char *name = 0L );
  virtual ~KonqDirTreePart();

  virtual bool openURL( const KURL & );

  virtual bool closeURL();

  virtual bool openFile() { return true; }

  KonqDirTreeBrowserExtension *extension() const { return m_extension; }

  bool supportsUndo() const { return true; }

private:
  KonqDirTree *m_pTree;
  KonqDirTreeBrowserExtension *m_extension;
};

class KonqDirTree;
class KonqDirLister;

class KonqDirTreeItem : public QListViewItem
{
public:
  KonqDirTreeItem( KonqDirTree *parent, KonqDirTreeItem *parentItem, KonqDirTreeItem *topLevelItem, KonqFileItem *item );
  KonqDirTreeItem( KonqDirTree *parent, KonqDirTreeItem *topLevelItem, KonqFileItem *item );

  void initItem( KonqDirTree *parent, KonqDirTreeItem *topLevelItem, KonqFileItem *item );

  virtual ~KonqDirTreeItem();

  virtual void setOpen( bool open );

  virtual void paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment );

  KonqFileItem *fileItem() const { return m_item; }

  void setListable( bool b );
  bool isListable() const { return m_bListable; }

  void setClickable( bool b ) { m_bClickable = b; }
  bool isClickable() const { return m_bClickable; }

  // Whether the item is a .desktop link
  void setLink( bool b ) { m_bLink = b; }
  bool isLink() const { return m_bLink; }

  // The URL to open when this link is clicked
  KURL externalURL() const;

private:
  KonqDirTree *m_tree;
  KonqFileItem *m_item;
  KonqDirTreeItem *m_topLevelItem;
  bool m_bListable:1;
  bool m_bClickable:1;
  bool m_bLink:1;
};

class KonqDirTree : public KListView, public KDirNotify
{
  Q_OBJECT
public:
  KonqDirTree( KonqDirTreePart *parent, QWidget *parentWidget );
  virtual ~KonqDirTree();

  void openSubFolder( KonqDirTreeItem *item, KonqDirTreeItem *topLevel );

  void addSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url );
  void removeSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url );

  void followURL( const KURL &url );

  // Reimplemented from KDirNotify
  void FilesAdded( const KURL & dir );
  void FilesRemoved( const KURL::List & urls );
  void FilesChanged( const KURL::List & urls );

protected:
  virtual void contentsDragEnterEvent( QDragEnterEvent *e );
  virtual void contentsDragMoveEvent( QDragMoveEvent *e );
  virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
  virtual void contentsDropEvent( QDropEvent *ev );

  virtual void contentsMousePressEvent( QMouseEvent *e );
  virtual void contentsMouseMoveEvent( QMouseEvent *e );
  virtual void contentsMouseReleaseEvent( QMouseEvent *e );

private slots:
  void slotNewItems( const KFileItemList & );
  void slotDeleteItem( KFileItem *item );

  void slotDoubleClicked( QListViewItem *item );
  void slotRightButtonPressed( QListViewItem *item );
  void slotClicked( QListViewItem *item );

  void slotListingStopped();

  void slotAnimation();

  void slotAutoOpenFolder();

  void rescanConfiguration();

private:
  void clearTree();
  void scanDir( KonqDirTreeItem *parent, const QString &path, bool isRoot = false );
  void scanDir2( KonqDirTreeItem *parent, const QString &path );
  void loadTopLevelItem( KonqDirTreeItem *parent, const QString &filename );

  void stripIcon( QString &icon );

  struct TopLevelItem
  {
    TopLevelItem( KonqDirTreeItem *item, KonqDirLister *lister, QMap<KURL, KonqDirTreeItem*> *subDirMap, KURL::List *pendingURLList )
    { m_item = item; m_dirLister = lister; m_mapSubDirs = subDirMap; m_lstPendingURLs = pendingURLList; }
    TopLevelItem() { m_item = 0; m_dirLister = 0; m_mapSubDirs = 0; }

    KonqDirTreeItem *m_item;
    KonqDirLister *m_dirLister;
    QMap<KURL,KonqDirTreeItem *> *m_mapSubDirs;
    KURL::List *m_lstPendingURLs;
  };

  TopLevelItem findTopLevelByItem( KonqDirTreeItem *item );
  TopLevelItem findTopLevelByDirLister( const KonqDirLister *lister );

  QValueList<TopLevelItem> m_topLevelItems;

  QMap<QListViewItem *,QString> m_groupItems;

  KonqDirTreePart *m_view;

  QMap<KURL, QListViewItem *> m_mapCurrentOpeningFolders;

  QTimer *m_animationTimer;

  int m_animationCounter;

  QPoint m_dragPos;
  bool m_bDrag;

  QListViewItem *m_dropItem;

  QPixmap m_folderPixmap;

  QTimer *m_autoOpenTimer;

  KURL m_selectAfterOpening;

  // The base URL for our configuration directory
  KURL m_dirtreeDir;
};

class KonqDirTreeBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KonqDirTree;
public:
  KonqDirTreeBrowserExtension( KonqDirTreePart *parent, KonqDirTree *dirTree );

  KonqDrag * konqDragObject();
protected slots:
  void copy();
  void cut();
  void paste();
  void trash() { KonqOperations::del(m_tree,
                                     KonqOperations::TRASH,
                                     selectedUrls()); }
  void del() { KonqOperations::del(m_tree,
                                   KonqOperations::DEL,
                                   selectedUrls()); }
  void shred() { KonqOperations::del(m_tree,
                                     KonqOperations::SHRED,
                                     selectedUrls()); }

  KURL::List selectedUrls();

  void slotSelectionChanged();

private:
  KonqDirTree *m_tree;
};

#endif
