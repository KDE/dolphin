
#ifndef __konq_dirtree_h__
#define __konq_dirtree_h__

#include <kparts/browserextension.h>
#include <kurl.h>
#include <kuserpaths.h>
#include <konqoperations.h>

#include <klistview.h>
#include <qdict.h>
#include <qmap.h>

class KonqDirTree;
class QTimer;
class KonqDirTreePart;

class KonqDirTreeBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KonqDirTree;
public:
  KonqDirTreeBrowserExtension( KonqDirTreePart *parent, KonqDirTree *dirTree );

protected slots:
  void copy();
  void cut();
  void pastecut() { pasteSelection( true ); }
  void pastecopy() { pasteSelection( false ); }
  void trash() { KonqOperations::del(KonqOperations::TRASH,
                                     selectedUrls()); }
  void del() { KonqOperations::del(KonqOperations::DEL,
                                   selectedUrls()); }
  void shred() { KonqOperations::del(KonqOperations::SHRED,
                                     selectedUrls()); }

  KURL::List selectedUrls();

  void slotSelectionChanged();
  void slotResult( KIO::Job * );
private:
  void pasteSelection( bool move );

  KonqDirTree *m_tree;
};

class KonqDirTreePart : public KParts::ReadOnlyPart
{
  Q_OBJECT
  friend class KonqDirTree;
public:
  KonqDirTreePart( QWidget *parentWidget, QObject *parent, const char *name = 0L );
  virtual ~KonqDirTreePart();

  virtual bool openURL( const KURL & );

  virtual bool closeURL();

  virtual bool openFile() { return true; }

  KonqDirTreeBrowserExtension *extension() const { return m_extension; }

protected:
  virtual bool event( QEvent *e );

private:
  KonqDirTree *m_pTree;
  KonqDirTreeBrowserExtension *m_extension;
};

class KonqDirTree;
class KDirLister;

class KonqDirTreeItem : public QListViewItem
{
public:
  KonqDirTreeItem( KonqDirTree *parent, QListViewItem *parentItem, KonqDirTreeItem *topLevelItem, KFileItem *item );

  virtual ~KonqDirTreeItem();

  virtual void setOpen( bool open );

  KFileItem *fileItem() const { return m_item; }

  void setListable( bool b );

private:
  KonqDirTree *m_tree;
  KFileItem *m_item;
  KonqDirTreeItem *m_topLevelItem;
  bool m_bListable;
};

class KonqDirTree : public KListView
{
  Q_OBJECT
public:
  KonqDirTree( KonqDirTreePart *parent, QWidget *parentWidget );
  virtual ~KonqDirTree();

  void openSubFolder( KonqDirTreeItem *item, KonqDirTreeItem *topLevel );

  void addSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url );
  void removeSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url );

  void followURL( const KURL &url );

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

private:
  void init();
  void scanDir( QListViewItem *parent, const QString &path, bool isRoot = false );
  void scanDir2( QListViewItem *parent, const QString &path );
  void loadTopLevelItem( QListViewItem *parent, const QString &filename );

  void stripIcon( QString &icon );

  struct TopLevelItem
  {
    TopLevelItem( KonqDirTreeItem *item, KDirLister *lister, QMap<KURL, KonqDirTreeItem*> *subDirMap, KURL::List *pendingURLList )
    { m_item = item; m_dirLister = lister; m_mapSubDirs = subDirMap; m_lstPendingURLs = pendingURLList; }
    TopLevelItem() { m_item = 0; m_dirLister = 0; m_mapSubDirs = 0; }

    KonqDirTreeItem *m_item;
    KDirLister *m_dirLister;
    QMap<KURL,KonqDirTreeItem *> *m_mapSubDirs;
    KURL::List *m_lstPendingURLs;
  };

  TopLevelItem findTopLevelByItem( KonqDirTreeItem *item );
  TopLevelItem findTopLevelByDirLister( KDirLister *lister );

  QListViewItem *m_root;

  QValueList<TopLevelItem> m_topLevelItems;

  QList<QListViewItem> m_unselectableItems;

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

  QListViewItem *m_lastItem;
};

#endif
