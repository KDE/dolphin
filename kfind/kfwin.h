/***********************************************************************
 *
 *  Kfwin.h
 *
 ***********************************************************************/

#ifndef KFWIN_H
#define KFWIN_H

#include <qlistview.h>

class KfArchiver;
class QPixmap;
class QFileInfo;

class KfFileLVI : public QListViewItem
{
 public:
  KfFileLVI(QListView* lv, QString file);
  ~KfFileLVI();

  QString key(int column, bool) const;

  QFileInfo *fileInfo;
};

class KfindWindow: public   QListView
{
  Q_OBJECT
public:
  KfindWindow( QWidget * parent = 0, const char * name = 0 );

  void beginSearch();
  void endSearch();

  void copySelection();

  void insertItem(QString str);

public slots:
  void selectAll();
  void unselectAll();

private slots:
  void deleteFiles();
  void fileProperties();
  void openFolder();
  void saveResults();
  void addToArchive();
  void openBinding();
  void selectionHasChanged();

protected:
  virtual void resizeEvent(QResizeEvent *e);
  virtual void contentsMousePressEvent(QMouseEvent *e);
  virtual void contentsMouseReleaseEvent(QMouseEvent *e);
  virtual void contentsMouseMoveEvent(QMouseEvent *e);

  QList<KfFileLVI> * selectedItems();

signals:
  void resultSelected(bool);

private:
  bool haveSelection;
  bool m_pressed;
  void execAddToArchive(KfArchiver *arch, QString filename);
  void resetColumns(bool init);

  QList<KfFileLVI> mySelectedItems;

};

#endif
