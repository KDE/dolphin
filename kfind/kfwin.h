/***********************************************************************
 *
 *  Kfwin.h
 *
 ***********************************************************************/

#ifndef KFWIN_H
#define KFWIN_H

#include <qlistbox.h> 

class KfArchiver;

class KfindWindow: public   QListBox
{
  Q_OBJECT  
public:
  KfindWindow( QWidget * parent = 0, const char * name = 0 );
  virtual ~KfindWindow();
  
  virtual void timerEvent(QTimerEvent *);

  void updateResults(const char * );

  void beginSearch();
  void appendResult(const char *);
  int numItems();

  void doneSearch();

  void clearList();

  void copySelection();

public slots:
  void selectAll();
  void unselectAll();
  void invertSelection();

private slots:
  void highlighted();
  void deleteFiles();
  void fileProperties();
  void openFolder();
  void saveResults();
  void changeItem(const char*);
  void addToArchive();
  void openBinding();

protected:

signals:
  void resultSelected(bool);
  void statusChanged(const char *);

private:
  void execAddToArchive(KfArchiver *arch,QString filename);
};

#endif
