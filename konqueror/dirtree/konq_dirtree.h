
#ifndef __konq_dirtree_h__
#define __konq_dirtree_h__

#include <kbrowser.h>
#include <kurl.h>

#include <qlistview.h>
#include <qdict.h>
#include <qmap.h>

class KonqDirTree;
class QTimer;
class KonqDirTree;

class KonqDirTreeEditExtension : public EditExtension
{
  Q_OBJECT
public:
  KonqDirTreeEditExtension( QObject *parent, KonqDirTree *dirTree );

  virtual void can( bool &cut, bool &copy, bool &paste, bool &move );

  virtual void cutSelection();
  virtual void copySelection();
  virtual void pasteSelection( bool move = false );
  virtual void moveSelection( const QString &destinationURL = QString::null );

private:
  KonqDirTree *m_tree;
};

class KonqDirTreeBrowserView : public BrowserView
{
  Q_OBJECT
  friend class KonqDirTree;
public:
  KonqDirTreeBrowserView( QWidget *parent = 0L, const char *name = 0L );
  virtual ~KonqDirTreeBrowserView();

  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 );

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

protected:
  virtual void resizeEvent( QResizeEvent * );

private:
  KonqDirTree *m_pTree;
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

private:
  KonqDirTree *m_tree;
  KFileItem *m_item;
  KonqDirTreeItem *m_topLevelItem;
};

class KonqDirTree : public QListView
{
  Q_OBJECT
public:
  KonqDirTree( KonqDirTreeBrowserView *parent );
  virtual ~KonqDirTree();

  void openSubFolder( KonqDirTreeItem *item, KonqDirTreeItem *topLevel );

  void addSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url );
  void removeSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url );

  // QList<KonqDirTreeItem> selectedItems();

protected:
  virtual void contentsDragEnterEvent( QDragEnterEvent *e );
  virtual void contentsDragMoveEvent( QDragMoveEvent *e );
  virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
  virtual void contentsDropEvent( QDropEvent *ev );

  virtual void contentsMousePressEvent( QMouseEvent *e );
  virtual void contentsMouseMoveEvent( QMouseEvent *e );
  virtual void contentsMouseReleaseEvent( QMouseEvent *e );

private slots:
  void slotNewItem( KFileItem *item );
  void slotDeleteItem( KFileItem *item );

  void slotDoubleClicked( QListViewItem *item );
  void slotRightButtonPressed( QListViewItem *item );
  void slotClicked( QListViewItem *item );

  void slotListingStopped();

  void slotAnimation();

  void slotAutoOpenFolder();

private:
  void init();
  void scanDir( QListViewItem *parent, const QString &path );
  void scanDir2( QListViewItem *parent, const QString &path );
  void loadTopLevelItem( QListViewItem *parent, const QString &filename );

  void groupPopupMenu( const QString &path, QListViewItem *item );

  void addGroup( const QString &path, QListViewItem *item );
  void addLink( const QString &path, QListViewItem *item );
  void removeGroup( const QString &path, QListViewItem *item );

  //  KonqDirTreeItem *findDir( const QMap<KURL, KonqDirTreeItem*> &dict, const KURL &url );

  //  void unselectAll();

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

  KonqDirTreeBrowserView *m_view;

  QMap<KURL, QListViewItem *> m_mapCurrentOpeningFolders;

  QTimer *m_animationTimer;

  int m_animationCounter;

  QPoint m_dragPos;
  bool m_bDrag;

  QListViewItem *m_dropItem;

  QPixmap m_folderPixmap;
  QPixmap m_folderOpenPixmap;

  QTimer *m_autoOpenTimer;
};

#endif
