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
  Q_OBJECT

public:
  KfindTop(const char *);
  virtual ~KfindTop();
  void menuInit();
  void toolBarInit();

public slots:
  void enableSaveResults(bool enable);
  void enableMenuItems(bool enable);
  void prefs();
  void statusChanged(const char *);
  void enableSearchButton(bool);
  void enableStatusBar(bool enable);
  void resizeOnFloating();

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

private:
  KMenuBar       *_mainMenu;
  
  QPopupMenu     *_fileMenu;
  QPopupMenu     *_editMenu;
  QPopupMenu     *_optionMenu;
  QPopupMenu     *_helpMenu;
  
  KToolBar       *_toolBar;
  KStatusBar     *_statusBar;
  Kfind          *_kfind;

  int        openWithM;
  int        toArchM;
  int        deleteM;
  int        renameM;
  int        propsM;
  int        openFldrM;
  int        saveSearchM;
  int        quitM;

  int        _width;
};

#endif
