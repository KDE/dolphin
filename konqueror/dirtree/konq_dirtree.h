
#ifndef __konq_dirtree_h__
#define __konq_dirtree_h__

#include <kbrowser.h>

#include <qlistview.h>
#include <qdict.h>
#include <qmap.h>

class KonqDirTree;

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

  void addSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const QString &url );
  void removeSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const QString &url );

private slots:
  void slotNewItem( KFileItem *item );
  void slotDeleteItem( KFileItem *item );

  void slotDoubleClicked( QListViewItem *item );
  void slotRightButtonPressed( QListViewItem *item );
  void slotClicked( QListViewItem *item );

private:
  void init();
  void scanDir( QListViewItem *parent, const QString &path );
  void scanDir2( QListViewItem *parent, const QString &path );
  void loadTopLevelItem( QListViewItem *parent, const QString &filename );

  void groupPopupMenu( const QString &path, QListViewItem *item );

  void addGroup( const QString &path, QListViewItem *item );
  void addLink( const QString &path, QListViewItem *item );
  void removeGroup( const QString &path, QListViewItem *item );

  KonqDirTreeItem *findDir( const QDict<KonqDirTreeItem> &dict, const QString &url );

  QList<KonqDirTreeItem> selectedItems();

  void unselectAll();

  struct TopLevelItem
  {
    TopLevelItem( KonqDirTreeItem *item, KDirLister *lister, QDict<KonqDirTreeItem> *subDirMap ) 
    { m_item = item; m_dirLister = lister; m_mapSubDirs = subDirMap; }
    TopLevelItem() { m_item = 0; m_dirLister = 0; m_mapSubDirs = 0; }
    
    KonqDirTreeItem *m_item;
    KDirLister *m_dirLister;
    QDict<KonqDirTreeItem> *m_mapSubDirs;
  };

  TopLevelItem findTopLevelByItem( KonqDirTreeItem *item );
  TopLevelItem findTopLevelByDirLister( KDirLister *lister );

  QListViewItem *m_root;

  QValueList<TopLevelItem> m_topLevelItems;

  QList<QListViewItem> m_unselectableItems;

  QMap<QListViewItem *,QString> m_groupItems;

  KonqDirTreeBrowserView *m_view;
};

#endif
