/***********************************************************************
 *
 *  KfindTop.h
 *
 ***********************************************************************/

#ifndef KFINDTOP_H
#define KFINDTOP_H

#include <ktopwidget.h>
#include <klocale.h>

class QPopupMenu;
class KMenuBar;
class KToolBar;
class KStatusBar;
class Kfind;               
class QPushButton;
class KfindTabDialog;

class KfindTop: public KTopLevelWidget
{
  Q_OBJECT;

public:
  KfindTop(const char *);
  //    ~Kfind();
  void menuInit();
  void toolBarInit();

public slots:
  void help();
  void about();
  void enableSaveResults(bool enable);
  void enableMenuItems(bool enable);
  void prefs();
  void statusChanged(const char *);
  void enableSearchButton(bool);
  void enableStatusBar(bool enable);

signals:
  //File Menu
  void open();
  void addToArchive();
  void deleteFile();
  void renameFile();
  void properties();
  void openFolder();
  void saveResults();

  //Edit Menu
  void undo();
  void cut();
  void copy();
  void selectAll();
  void invertSelection();
  //Options Menu
  void keys();

protected:
  void resizeEvent( QResizeEvent * );

private:
  KMenuBar       *_mainMenu;
  
  QPopupMenu     *_fileMenu;
  QPopupMenu     *_editMenu;
  QPopupMenu     *_optionMenu;
  QPopupMenu     *_helpMenu;
  
  KToolBar       *_toolBar;
  KStatusBar     *_statusBar;
  Kfind          *_kfind;
  KfindTabDialog *_kfindTabDialog;

  int        openWithM;
  int        toArchM;
  int        deleteM;
  int        renameM;
  int        propsM;
  int        openFldrM;
  int        saveSearchM;
  int        quitM;
};

#endif
