/***********************************************************************
 *
 *  Kfwin.h
 *
 ***********************************************************************/

#ifndef KFWIN_H
#define KFWIN_H

#include <k3listview.h>
#include <kfileitem.h>
#include <kurl.h>

class KfArchiver;
class QPixmap;
class QFileInfo;
class KMenu;
class KfindWindow;

class KfFileLVI : public Q3ListViewItem
{
 public:
  KfFileLVI(KfindWindow* lv, const KFileItem &item,const QString& matchingLine);
  ~KfFileLVI();

  QString key(int column, bool) const;

  QFileInfo *fileInfo;
  KFileItem fileitem;
};

class KfindWindow: public   K3ListView
{
  Q_OBJECT
public:
  KfindWindow( QWidget * parent = 0 );

  void beginSearch(const KUrl& baseUrl);
  void endSearch();

  void insertItem(const KFileItem &item, const QString& matchingLine);

  QString reducedDir(const QString& fullDir);

public Q_SLOTS:
  void copySelection();
  void slotContextMenu(K3ListView *,Q3ListViewItem *item,const QPoint&p);

private Q_SLOTS:
  void deleteFiles();
  void fileProperties();
  void openFolder();
  void saveResults();
  void openBinding();
  void selectionHasChanged();
  void slotExecute(Q3ListViewItem*);
  void slotOpenWith();
  
protected:
  virtual void resizeEvent(QResizeEvent *e);

  virtual Q3DragObject *dragObject();

Q_SIGNALS:
  void resultSelected(bool);

private:
  QString m_baseDir;
  KMenu *m_menu;
  bool haveSelection;
  bool m_pressed;
  void resetColumns(bool init);
};

#endif
