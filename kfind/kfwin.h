/***********************************************************************
 *
 *  Kfwin.h
 *
 ***********************************************************************/

#ifndef KFWIN_H
#define KFWIN_H

#include <qlistview.h> 

class KfArchiver;
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
  
  virtual void timerEvent(QTimerEvent *);

  void updateResults(const char * );

  void beginSearch();
  void endSearch();

  void copySelection();

  void insertItem(QString str);

public slots:
  void selectAll();
  void unselectAll();

private slots:
  void rightButtonPressed(QListViewItem *, const QPoint &, int);
  void deleteFiles();
  void fileProperties();
  void openFolder();
  void saveResults();
  void changeItem(const char*);
  void addToArchive();
  void openBinding();

protected:
  void resizeEvent(QResizeEvent *e);
  void contentsMouseReleaseEvent(QMouseEvent *e);
  void contentsMousePressEvent(QMouseEvent *e);

signals:
  void resultSelected(bool);
  void statusChanged(const char *);

private:
  bool haveSelection;
  void execAddToArchive(KfArchiver *arch, QString filename);
  void resetColumns();
  void selectionChanged(bool selectionMade);
};

#endif
