/***********************************************************************
 *
 *  Kfwin.h
 *
 ***********************************************************************/

#ifndef KFWIN_H
#define KFWIN_H

#include <klistview.h>
#include <kfileitem.h>
#include <kurl.h>

class KfArchiver;
class QPixmap;
class QFileInfo;
class KPopupMenu;
class KfindWindow;

class KfFileLVI : public QListViewItem
{
 public:
  KfFileLVI(KfindWindow* lv, const KFileItem &item,const QString& matchingLine);
  ~KfFileLVI();

  QString key(int column, bool) const;

  QFileInfo *fileInfo;
  KFileItem fileitem;
};

class KfindWindow: public   KListView
{
  Q_OBJECT
public:
  KfindWindow( QWidget * parent = 0, const char * name = 0 );

  void beginSearch(const KURL& baseUrl);
  void endSearch();

  void insertItem(const KFileItem &item, const QString& matchingLine);

  QString reducedDir(const QString& fullDir);

public slots:
  void copySelection();
  void slotContextMenu(KListView *,QListViewItem *item,const QPoint&p);

private slots:
  void deleteFiles();
  void fileProperties();
  void openFolder();
  void saveResults();
  void openBinding();
  void selectionHasChanged();
  void slotExecute(QListViewItem*);
  void slotOpenWith();
  
protected:
  virtual void resizeEvent(QResizeEvent *e);

  virtual QDragObject *dragObject();

signals:
  void resultSelected(bool);

private:
  QString m_baseDir;
  KPopupMenu *m_menu;
  bool haveSelection;
  bool m_pressed;
  void resetColumns(bool init);
};

#endif
